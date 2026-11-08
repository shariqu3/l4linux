#ifndef __ASM_L4__L4X_X86__SIGNAL_H__
#define __ASM_L4__L4X_X86__SIGNAL_H__

#include <asm/ptrace.h>
#include <linux/entry-common.h>

static inline int l4x_do_signal(struct pt_regs *regs, int syscall)
{
	arch_do_signal_or_restart(regs);
	return 0;
}

#endif /* ! __ASM_L4__L4X_X86__SIGNAL_H__ */
