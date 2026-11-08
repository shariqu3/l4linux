#ifndef __ASM_L4__L4X_ARM__FPU_H__
#define __ASM_L4__L4X_ARM__FPU_H__

struct l4x_arch_cpu_fpu_state {
	unsigned long fpexc, fpinst, fpinst2;
};

unsigned l4x_fmrx(unsigned long reg);
void l4x_fmxr(unsigned long reg, unsigned long val);

struct pt_regs *l4x_fp_get_user_regs(void);

#endif /* ! __ASM_L4__L4X_ARM__FPU_H__ */
