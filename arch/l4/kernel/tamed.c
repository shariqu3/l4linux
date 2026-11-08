/*
 * Interrupt disable/enable implemented with a queue
 *
 * For deadlock reasons this file must _not_ use any Linux functionality!
 */

#include <linux/mm.h>
#include <linux/module.h>
#include <linux/stddef.h>

#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>
#include <l4/re/env.h>
#include <l4/util/atomic.h>
#include <l4/util/kprintf.h>
#include <l4/util/util.h>
#include <l4/log/log.h>

#include <asm/l4lxapi/thread.h>
#include <asm/generic/stack_id.h>
#include <asm/generic/setup.h>
#include <asm/generic/tamed.h>
#include <asm/generic/smp.h>
#include <asm/generic/vcpu.h>
#include <asm/generic/cap_alloc.h>
#include <asm/api/macros.h>

#include <l4/sys/kdebug.h>

#ifdef CONFIG_L4_VCPU
#include <asm/server/server.h>
#include <asm/l4x/exception.h>
#include <l4/vcpu/vcpu.h>
#else

#define L4X_TAMED_LABEL

#define MAX_WQ_ENTRIES	64

typedef struct _sem_wq {
	l4_cap_idx_t    thread;
	l4_cap_idx_t    reply_cap;  /* explicit reply cap of this waiter */
	unsigned        prio;
	struct _sem_wq  *next;
} sem_wq_t;

typedef struct _cli_sem {
	volatile long counter;
	sem_wq_t      *queue;
} cli_sem_t;

typedef struct _cli_lock {
	cli_sem_t              sem;
	volatile l4_cap_idx_t  owner;
} cli_lock_t;

#ifdef CONFIG_SMP

// number of max available pCPUs
#define NR_TAMERS 32

static int cpu_to_nr[NR_CPUS];

#define TAMED_DEFINE(t, v)  __typeof__(t) v[NR_TAMERS]
#define tamed_per_nr(v, nr) v[nr]

#else

#define NR_TAMERS 1

#define TAMED_DEFINE(t, v)  __typeof__(t) v
#define tamed_per_nr(v, nr) (*((void)(nr), &v))

#endif

static TAMED_DEFINE(sem_wq_t [MAX_WQ_ENTRIES], wq_entries);
static TAMED_DEFINE(int, wq_len);  /* track wait queue length here */
static TAMED_DEFINE(int, next_entry);
static TAMED_DEFINE(l4_cap_idx_t, cli_sem_thread_id);
static TAMED_DEFINE(l4lx_thread_t, cli_sem_thread_th);
static TAMED_DEFINE(cli_lock_t, cli_lock);
static TAMED_DEFINE(unsigned char [L4LX_THREAD_STACK_SIZE], stack_mem);

static inline int get_tamer_nr(int vcpu)
{
#ifdef CONFIG_SMP
	return cpu_to_nr[vcpu];
#else
	return 0;
#endif
}

/* One explicit reply capability per queued waiter plus one for the current
 * request. Using explicit reply caps allows the tamer to defer replies to
 * queued threads without overwriting the reply cap of another caller. */
#define TAMED_REPLY_CAPS	(MAX_WQ_ENTRIES + 1)

static TAMED_DEFINE(unsigned [TAMED_REPLY_CAPS], tamed_reply_cap_used);
static l4_umword_t tamed_reply_cap_base;

static inline l4_cap_idx_t tamed_reply_cap_alloc(int nr)
{
	unsigned *used = tamed_per_nr(tamed_reply_cap_used, nr);
	int i;

	for (i = 0; i < TAMED_REPLY_CAPS; i++)
		if (!used[i]) {
			used[i] = 1;
			return (l4_cap_idx_t)(tamed_reply_cap_base
			                      + (unsigned)nr * TAMED_REPLY_CAPS + i)
			       << L4_CAP_SHIFT;
		}

	return L4_INVALID_CAP;
}

static inline void tamed_reply_cap_free(int nr, l4_cap_idx_t cap)
{
	unsigned i;

	if (l4_is_invalid_cap(cap))
		return;

	i = (cap >> L4_CAP_SHIFT) - (tamed_reply_cap_base
	                             + (unsigned)nr * TAMED_REPLY_CAPS);
	if (i < TAMED_REPLY_CAPS)
		tamed_per_nr(tamed_reply_cap_used, nr)[i] = 0;
}

#include <asm/l4x/tamed.h>

#ifdef CONFIG_L4_DEBUG_TAMED_COUNT_INTERRUPT_DISABLE
unsigned cli_sum;
unsigned cli_taken;
#endif

void l4x_tamed_set_mapping(int vcpu, int nr)
{
#ifdef CONFIG_SMP
	cpu_to_nr[vcpu] = nr;
#endif
	LOG_printf("vcpu %d gets tamer %d\n", vcpu, nr);
}

static inline sem_wq_t* __alloc_entry(int nr)
{
	int i = tamed_per_nr(next_entry, nr);

	/* find unused wait queue entry */
	do {
		if (l4_is_invalid_cap(tamed_per_nr(wq_entries, nr)[i].thread)) {
			tamed_per_nr(next_entry, nr) = (i + 1) % MAX_WQ_ENTRIES;
			return tamed_per_nr(wq_entries, nr) + i;
		}
		i = (i + 1) % MAX_WQ_ENTRIES;
	} while (i != tamed_per_nr(next_entry, nr));

	/* this cannot happen since we have only about 20 threads */
	l4_kd_enter("no wait queue entry");
	return NULL;
}

static inline void __free_entry(sem_wq_t *wq)
{
	wq->thread = L4_INVALID_CAP;
	wq->reply_cap = L4_INVALID_CAP;
}

/* don't worry about priorities */
static inline void __enqueue_thread(l4_cap_idx_t t, unsigned prio,
                                    l4_cap_idx_t reply_cap, int nr)
{
	sem_wq_t *wq;

	/* insert thread into wait queue */
	wq = __alloc_entry(nr);
	wq->thread = t;
	wq->reply_cap = reply_cap;
	wq->prio = prio;
	wq->next = NULL;

	if (tamed_per_nr(cli_lock, nr).sem.queue == NULL)
		tamed_per_nr(cli_lock, nr).sem.queue = wq;
	else {
		wq->next = tamed_per_nr(cli_lock, nr).sem.queue;
		tamed_per_nr(cli_lock, nr).sem.queue = wq;
	}
	tamed_per_nr(wq_len, nr)++;
}

static inline sem_wq_t* __prio_highest(int nr)
{
	sem_wq_t *wq_prio_highest = tamed_per_nr(cli_lock, nr).sem.queue;
	sem_wq_t *wp = wq_prio_highest->next;

	while (wp) {
		if (wp->prio > wq_prio_highest->prio)
			wq_prio_highest = wp;
		wp = wp->next;
	}

	return wq_prio_highest;
}

static inline void __wakeup_thread_without_switchto(sem_wq_t *wq, int nr)
{
	l4_umword_t error;

	error = l4_ipc_error(l4_ipc_send(wq->thread, l4_utcb(),
	                                 l4_msgtag(0, 0, 0, L4_MSGTAG_SCHEDULE),
	                                 L4_IPC_SEND_TIMEOUT_0),
	                     l4_utcb());
	if (error)
		LOG_printf("cli thread: wakeup to " PRINTF_L4TASK_FORM
		           " failed (0x%02lx)\n",
		           PRINTF_L4TASK_ARG(wq->thread), error);

	tamed_reply_cap_free(nr, wq->reply_cap);
}

/** The main semaphore thread. We need this thread to ensure atomicity.
 * We assume that this thread is not preempted by any other thread.
 */
static L4_CV void cli_sem_thread(void *data)
{
	l4_umword_t dw0;
	int i;
	int nr = *(int *)data;
	l4_umword_t operation, prio;
	l4_umword_t src;
	l4_msgtag_t ret;
	l4_utcb_t *utcb = l4_utcb();
	l4_cap_idx_t reply_cap;
	int queued;

	/* setup wait queue entry allocation */
	for (i = 0; i < MAX_WQ_ENTRIES; i++)
		__free_entry(tamed_per_nr(wq_entries, nr) + i);

	/* semaphore thread loop */
	reply_cap = tamed_reply_cap_alloc(nr);

	for (;;) {
		/* wait for request, the caller's reply capability is stored in
		 * our own explicit slot, so that deferred replies do not
		 * overwrite each other */
		ret = l4_ipc_wait_ext(reply_cap, utcb, &src, L4_IPC_NEVER);
		if (unlikely(l4_ipc_error(ret, utcb))) {
			LOG_printf("cli thread: IPC error 0x%02lx\n",
			           l4_ipc_error(ret, utcb));
			continue;
		} else if (l4_msgtag_label(ret) < 0) {
			LOG_printf("tamer: protocol failure\n");
			l4_kd_enter("PROT_FAIL");
		}

		operation = (ret.raw >> 16) & 3;
		prio      = (ret.raw >> 20) & 0xff;

		if (0)
			l4_kprintf("tamer%d-REQ %s prio=%lx src=%lx sem=%lx q=%p\n",
			           nr, operation == 1 ? "DOWN" : "UP",
			           prio, src,
			           tamed_per_nr(cli_lock, nr).sem.counter,
				   tamed_per_nr(cli_lock, nr).sem.queue);

		dw0 = 0;    /* return 0 in dw0 per default */
		queued = 0;
		switch (operation) {
			case 1: // down
				/* CLI (block thread, enqueue to semaphores wait queue) */
				/* Insert fancy explanation for this if: fixme MLP*/
				if (tamed_per_nr(wq_len, nr) == -1 * tamed_per_nr(cli_lock, nr).sem.counter) {
					dw0 = 1;
					break;
				} else {
					__enqueue_thread(src, prio, reply_cap, nr);
					queued = 1;
				}
				break;

			case 2: // up
				/* STI */
				if (tamed_per_nr(cli_lock, nr).sem.queue) {
					/* wakeup all waiting threads and reply to the
					 * thread with the highest priority */
					sem_wq_t *wp, *wq_prio_highest;

					__enqueue_thread(src, prio, reply_cap, nr);
					queued = 1;
					wq_prio_highest = __prio_highest(nr);
					while ((wp = tamed_per_nr(cli_lock, nr).sem.queue)) {
						/* remove thread from wait queue */
						tamed_per_nr(cli_lock, nr).sem.queue = wp->next;
						if (wp == wq_prio_highest)
							continue;
						l4util_atomic_inc(&tamed_per_nr(cli_lock, nr).sem.counter);
						tamed_per_nr(wq_len, nr)--;
						/* never switch to woken up thread since we have
						 * the higher priority (per definition) */
						__wakeup_thread_without_switchto(wp, nr);
						__free_entry(wp);
					}

					l4_ipc_send(wq_prio_highest->thread, utcb,
					            l4_msgtag(0, 0, 0, 0),
					            L4_IPC_SEND_TIMEOUT_0);
					tamed_reply_cap_free(nr, wq_prio_highest->reply_cap);
					tamed_per_nr(wq_len, nr)--;
					__free_entry(wq_prio_highest);
				}
				break;

			default:
				LOG_printf("cli thread: invalid request\n");
		}

		if (!queued) {
			l4_ipc_send(src, utcb, l4_msgtag(dw0, 0, 0, 0),
			            L4_IPC_SEND_TIMEOUT_0);
			tamed_reply_cap_free(nr, reply_cap);
		}

		if (0)
			l4_kprintf("tamer%d-ANS to=%lx sem=%lx q=%p dw0=%ld\n",
			           nr, src,
			           tamed_per_nr(cli_lock, nr).sem.counter,
			           tamed_per_nr(cli_lock, nr).sem.queue, dw0);

		reply_cap = tamed_reply_cap_alloc(nr);
	}
}
#endif /* vcpu */

void l4x_global_cli(void)
{
#ifdef CONFIG_L4_VCPU
	l4vcpu_irq_disable(l4x_vcpu_state_current());
	smp_mb();
#else
	l4_cap_idx_t me = l4x_cap_current_utcb(l4x_utcb_current());
	int nr = get_tamer_nr(smp_processor_id());

	if (unlikely((me >> L4_CAP_SHIFT) > 9000))
		l4_kd_enter("Unset id on stack (c)?");

#if 0
	if (tamed_per_nr(cli_lock, nr).sem.counter <= 0)
		l4_kprintf("CLI: me=%lx  f=%p,%p\n", me, __builtin_return_address(0), __builtin_return_address(1));
#endif

#ifdef CONFIG_L4_DEBUG_TAMED_COUNT_INTERRUPT_DISABLE
	cli_sum++;
#endif
	if (l4_capability_equal(me, tamed_per_nr(cli_lock, nr).owner))
		/* we already are the owner of the lock */
		return;

	/* try to get the lock */
	l4x_tamed_sem_down();

	/* we have the lock */
	tamed_per_nr(cli_lock, nr).owner = me;
#endif
}
EXPORT_SYMBOL(l4x_global_cli);

#ifdef CONFIG_L4_VCPU
static void do_vcpu_irq(l4_vcpu_state_t *v)
{
	struct pt_regs regs;

	l4x_init_kernel_regs(&regs);
	trace_hardirqs_off();
	l4x_vcpu_handle_irq(v, &regs);
	trace_hardirqs_on();
}
#endif

#ifdef CONFIG_L4_VCPU
static void l4x_srv_setup_recv_wrap(l4_utcb_t *utcb)
{
#ifdef CONFIG_L4_SERVER
	l4x_srv_setup_recv(utcb);
#endif
}
#endif

void l4x_global_sti(void)
{
#ifdef CONFIG_L4_VCPU
	l4vcpu_irq_enable(l4x_vcpu_state_current(), l4x_utcb_current(),
	                  do_vcpu_irq, l4x_srv_setup_recv_wrap);
#else
	l4_cap_idx_t me = l4x_cap_current();
	int nr = get_tamer_nr(raw_smp_processor_id());

	if (unlikely((me >> L4_CAP_SHIFT) > 9000))
		l4_kd_enter("Unset id on stack (s)?");

#if 0
	if (tamed_per_nr(cli_lock, nr).sem.counter < 0)
		l4_kprintf("STI: me=%lx  f=%p,%p\n", me, __builtin_return_address(0), __builtin_return_address(1));
#endif

	if (l4_is_invalid_cap(tamed_per_nr(cli_lock, nr).owner))
		return;

	tamed_per_nr(cli_lock, nr).owner = L4_INVALID_CAP;
	l4x_tamed_sem_up();
#endif
}
EXPORT_SYMBOL(l4x_global_sti);

#ifndef CONFIG_L4_VCPU
void l4x_global_halt(void)
{
	L4XV_FN_v(l4_sleep_forever());

}
EXPORT_SYMBOL(l4x_global_halt);
#else
void l4x_global_halt(void)
{
	l4vcpu_wait_for_event(l4x_vcpu_state_current(), l4x_utcb_current(),
	                      do_vcpu_irq, l4x_srv_setup_recv_wrap);
#ifdef CONFIG_X86
	// on x86, interrupts are enabled after hlt
	local_irq_enable();
#endif
}
EXPORT_SYMBOL(l4x_global_halt);

#ifdef CONFIG_PM

struct l4x_pm_event_data {
	unsigned valid;
	l4_vcpu_ipc_regs_t vcpu;
	l4_msg_regs_t utcb_msg_regs;
};

static struct l4x_pm_event_data pm_event;

void l4x_global_wait_save(void)
{
	l4_vcpu_ipc_regs_t *vcpu = &l4x_vcpu_state_current()->i;
	l4_utcb_t *utcb = l4x_utcb_current();

	BUG_ON(!irqs_disabled());

	l4x_srv_setup_recv_wrap(utcb);
	vcpu->tag = l4_ipc_wait(utcb, &vcpu->label, L4_IPC_NEVER);

	memcpy(&pm_event.vcpu, vcpu, sizeof(*vcpu));
	memcpy(&pm_event.utcb_msg_regs, l4_utcb_mr_u(utcb),
	       sizeof(pm_event.utcb_msg_regs));
	pm_event.valid = 1;
}

void l4x_global_saved_event_inject(void)
{
	l4_vcpu_state_t *vcpu = l4x_vcpu_state_current();
	l4_vcpu_ipc_regs_t *vcpu_ipc = &vcpu->i;
	l4_utcb_t *utcb = l4x_utcb_current();
	unsigned long flags;

	if (!pm_event.valid)
		return;

	pr_info("Resume event replay: Wakeup source label=%lx\n",
	        pm_event.vcpu.label);

	local_irq_save(flags);

	memcpy(vcpu_ipc, &pm_event.vcpu, sizeof(*vcpu_ipc));
	/* Might be called from different CPU than saved, so leave TCR
	 * intact */
	memcpy(l4_utcb_mr_u(utcb), &pm_event.utcb_msg_regs,
	       sizeof(l4_utcb_mr_u(utcb)->mr));
	pm_event.valid = 0;

	do_vcpu_irq(vcpu);

	local_irq_restore(flags);
}
#endif
#endif

unsigned long l4x_global_save_flags(void)
{
#ifdef CONFIG_L4_VCPU
	return l4x_vcpu_state_current()->state;
#else
	return l4_capability_equal(tamed_per_nr(cli_lock,
	                                        get_tamer_nr(raw_smp_processor_id())).owner,
	                           l4x_cap_current()) ? L4_IRQ_DISABLED : L4_IRQ_ENABLED;
#endif
}
EXPORT_SYMBOL(l4x_global_save_flags);

void l4x_global_restore_flags(unsigned long flags)
{
#ifdef CONFIG_L4_VCPU
	l4vcpu_irq_restore(l4x_vcpu_state_current(), flags,
	                   l4x_utcb_current(), do_vcpu_irq,
	                   l4x_srv_setup_recv_wrap);
#else
	switch (flags) {
		case L4_IRQ_ENABLED:
			l4x_global_sti();
			break;
		case L4_IRQ_DISABLED:
			l4x_global_cli();
			break;
		default:
			l4_kd_enter("restore_flags wrong val");
	}
#endif
}
EXPORT_SYMBOL(l4x_global_restore_flags);

#ifndef CONFIG_L4_VCPU
/** create one semaphore thread */
void l4x_tamed_start(unsigned vcpu)
{
	char s[9]; // up to 999 CPUs
	int nr = l4x_cpu_physmap_get_id(vcpu);

	if (!l4_is_invalid_cap(tamed_per_nr(cli_sem_thread_id, nr)))
		return;

	/* the priority has to be higher than the prio of the omega0 server
	 * to ensure that the cli thread is not preempted! */
	/* Provide our own stack so that we do not need to use locking
	 * functions get one from l4lxlib */

#ifdef CONFIG_SMP
	if (nr >= NR_TAMERS)
		l4_kd_enter("l4x_tamed_init: Invalid argument");
#endif

	snprintf(s, sizeof(s), "tamer%d", nr);
	s[sizeof(s) - 1] = 0;

	/* Reset values as they get copied after first usage */
	tamed_per_nr(cli_lock,   nr).owner       = L4_INVALID_CAP;
	tamed_per_nr(cli_lock,   nr).sem.counter = 1;
	tamed_per_nr(cli_lock,   nr).sem.queue   = NULL;
	tamed_per_nr(wq_len,     nr)             = 0;
	tamed_per_nr(next_entry, nr)             = 0;
	memset(tamed_per_nr(tamed_reply_cap_used, nr), 0,
	       sizeof(tamed_per_nr(tamed_reply_cap_used, nr)));

	if (l4lx_thread_create(&tamed_per_nr(cli_sem_thread_th, nr),
	                       cli_sem_thread, vcpu,
	                       tamed_per_nr(stack_mem, nr) + sizeof(tamed_per_nr(stack_mem, 0)),
	                       &nr, sizeof(nr), l4x_cap_alloc_noctx(),
	                       CONFIG_L4_PRIO_TAMER, 0, 0, 1, s, NULL))
		LOG_printf("Failed to create tamer thread\n");

	tamed_per_nr(cli_sem_thread_id, nr) =
		l4lx_thread_get_cap(tamed_per_nr(cli_sem_thread_th, nr));

	LOG_printf("Tamer%d is " PRINTF_L4TASK_FORM "\n",
	           nr,
	           PRINTF_L4TASK_ARG(tamed_per_nr(cli_sem_thread_id, nr)));

	if (!nr)
		LOG_printf("Using tamed mode.\n");
}

void l4x_tamed_init(void)
{
	int i;
	for (i = 0; i < NR_TAMERS; i++)
		tamed_per_nr(cli_sem_thread_id, i) = L4_INVALID_CAP;

	/* Reserve explicit reply capabilities for all tamers. */
	tamed_reply_cap_base = l4re_env()->first_free_reply_cap;
	l4re_env()->first_free_reply_cap += NR_TAMERS * TAMED_REPLY_CAPS;
}

void l4x_tamed_shutdown(unsigned vcpu)
{
	int found = 0, i, nr = l4x_cpu_physmap_get_id(vcpu);

	// look for others CPUs that use the same tamer
	for (i = 0; i < NR_TAMERS; i++)
		if (l4x_cpu_physmap_get_id(i) == nr && i != nr)
			found = 1;

	if (!found) {
		// none found, shutdown thread
		l4lx_thread_shutdown(tamed_per_nr(cli_sem_thread_th, nr),
				     1, NULL, 1);
		tamed_per_nr(cli_sem_thread_id, nr) = L4_INVALID_CAP;
		LOG_printf("Tamer%d was destroyed\n", nr);
	}
}

int l4x_tamed_print_cli_stats(char *buffer)
{
#ifdef CONFIG_L4_DEBUG_TAMED_COUNT_INTERRUPT_DISABLE
	return sprintf(buffer, "cli() called     : %u\n"
	                       "cli() contention : %u\n",
	                       cli_sum, cli_taken);
#else
	return sprintf(buffer, "<CONFIG_L4_CLI_DEBUG not enabled in config>\n");
#endif
}
#endif
