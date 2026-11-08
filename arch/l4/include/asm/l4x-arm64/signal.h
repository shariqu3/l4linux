#ifndef __ASM_L4__L4X_ARM__SIGNAL_H__
#define __ASM_L4__L4X_ARM__SIGNAL_H__

#include <linux/linkage.h>
#include <linux/irq-entry-common.h>
#include <asm/ptrace.h>

void do_signal(struct pt_regs *regs);

static inline void l4x_do_signal(struct pt_regs *regs, int syscall)
{
	arch_do_signal_or_restart(regs);
}

#endif /* ! __ASM_L4__L4X_ARM__SIGNAL_H__ */
