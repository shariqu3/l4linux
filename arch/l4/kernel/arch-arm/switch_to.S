#include <linux/linkage.h>

#include <asm/asm-offsets.h>
#include <asm/assembler.h>
#include <asm/thread_notify.h>
#include <asm/unwind.h>

.macro switch_tls, base, tp, tpuser, tmp1, tmp2
.endm

/* r0: prev task, r1: prev ti  r2: next ti
 */
ENTRY(__switch_to)
 UNWIND(.fnstart	)
 UNWIND(.cantunwind	)
	add	ip, r1, #TI_CPU_SAVE
 ARM(	stmia	ip!, {r4 - sl, fp, sp, lr} )	@ Store most regs on stack
 THUMB(	stmia	ip!, {r4 - sl, fp}	   )	@ Store most regs on stack
 THUMB(	str	sp, [ip], #4		   )
 THUMB(	str	lr, [ip], #4		   )
	ldr	r4, [r2, #TI_TP_VALUE]
	ldr	r5, [r2, #TI_TP_VALUE + 4]
#ifdef CONFIG_CPU_USE_DOMAINS
	mrc	p15, 0, r6, c3, c0, 0		@ Get domain register
	str	r6, [r1, #TI_CPU_DOMAIN]	@ Save old domain register
	ldr	r6, [r2, #TI_CPU_DOMAIN]
#endif
	switch_tls r1, r4, r5, r3, r7
#if defined(CONFIG_STACKPROTECTOR) && !defined(CONFIG_SMP) && \
    !defined(CONFIG_STACKPROTECTOR_PER_TASK)
	ldr	r8, =__stack_chk_guard
	.if (TSK_STACK_CANARY > IMM12_MASK)
	add	r9, r2, #TSK_STACK_CANARY & ~IMM12_MASK
	ldr	r9, [r9, #TSK_STACK_CANARY & IMM12_MASK]
	.else
	ldr	r9, [r2, #TSK_STACK_CANARY & IMM12_MASK]
	.endif
#endif
	mov	r7, r2				@ Preserve 'next'
#ifdef CONFIG_CPU_USE_DOMAINS
	mcr	p15, 0, r6, c3, c0, 0		@ Set domain register
#endif
	mov	r5, r0
	add	r4, r2, #TI_CPU_SAVE
	ldr	r0, =thread_notify_head
	mov	r1, #THREAD_NOTIFY_SWITCH
	bl	atomic_notifier_call_chain
#if defined(CONFIG_STACKPROTECTOR) && !defined(CONFIG_SMP) && \
    !defined(CONFIG_STACKPROTECTOR_PER_TASK)
	str	r9, [r8]
#endif
	mov	r0, r5
#if !defined(CONFIG_THUMB2_KERNEL) && !defined(CONFIG_VMAP_STACK)
	set_current r7, r8
	ldmia	r4, {r4 - sl, fp, sp, pc}	@ Load all regs saved previously
#else
	mov	r1, r7
	ldmia	r4, {r4 - sl, fp, ip, lr}	@ Load all regs saved previously
#ifdef CONFIG_VMAP_STACK
	@
	@ Do a dummy read from the new stack while running from the old one so
	@ that we can rely on do_translation_fault() to fix up any stale PMD
	@ entries covering the vmalloc region.
	@
	ldr	r2, [ip]
#endif

	@ When CONFIG_THREAD_INFO_IN_TASK=n, the update of SP itself is what
	@ effectuates the task switch, as that is what causes the observable
	@ values of current and current_thread_info to change. When
	@ CONFIG_THREAD_INFO_IN_TASK=y, setting current (and therefore
	@ current_thread_info) is done explicitly, and the update of SP just
	@ switches us to another stack, with few other side effects. In order
	@ to prevent this distinction from causing any inconsistencies, let's
	@ keep the 'set_current' call as close as we can to the update of SP.
	set_current r1, r2
	mov	sp, ip
	ret	lr
#endif
 UNWIND(.fnend		)
ENDPROC(__switch_to)
