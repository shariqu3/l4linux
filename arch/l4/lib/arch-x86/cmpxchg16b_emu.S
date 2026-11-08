/* SPDX-License-Identifier: GPL-2.0-only */
#include <linux/linkage.h>
#include <asm/percpu.h>
#include <asm/processor-flags.h>

.text

/*
 * Emulate 'cmpxchg16b %gs:(%rsi)'
 *
 * Inputs:
 * %rsi : memory location to compare
 * %rax : low 64 bits of old value
 * %rdx : high 64 bits of old value
 * %rbx : low 64 bits of new value
 * %rcx : high 64 bits of new value
 *
 * Notably this is not LOCK prefixed and is not safe against NMIs
 */
SYM_FUNC_START(this_cpu_cmpxchg16b_emu)

	pushfq

#ifdef CONFIG_L4
	push	%r12
	push	%r11
	push	%r10
	push	%r9
	push	%r8
	push	%rdi

	push	%rsi
	push	%rax
	push	%rdx
	push	%rcx
	call	l4x_global_save_flags
	mov	%rax, %r12
	call	l4x_global_cli
	pop	%rcx
	pop	%rdx
	pop	%rax
	pop	%rsi
#else
	cli
#endif

	/* if (*ptr == old) */
	cmpq	__percpu (%rsi), %rax
	jne	.Lnot_same
	cmpq	__percpu 8(%rsi), %rdx
	jne	.Lnot_same

	/* *ptr = new */
	movq	%rbx, __percpu (%rsi)
	movq	%rcx, __percpu 8(%rsi)

#ifdef CONFIG_L4
	push	%rax
	push	%rdx
	mov	%r12, %rdi
	call	l4x_global_restore_flags
	pop	%rdx
	pop	%rax

	pop	%rdi
	pop	%r8
	pop	%r9
	pop	%r10
	pop	%r11
	pop	%r12
#endif

	/* set ZF in EFLAGS to indicate success */
	orl	$X86_EFLAGS_ZF, (%rsp)

	popfq
	RET

.Lnot_same:
	/* *ptr != old */

	/* old = *ptr */
	movq	__percpu (%rsi), %rax
	movq	__percpu 8(%rsi), %rdx

#ifdef CONFIG_L4
	push	%rax
	push	%rdx
	mov	%r12, %rdi
	call	l4x_global_restore_flags
	pop	%rdx
	pop	%rax

	pop	%rdi
	pop	%r8
	pop	%r9
	pop	%r10
	pop	%r11
	pop	%r12
#endif

	/* clear ZF in EFLAGS to indicate failure */
	andl	$(~X86_EFLAGS_ZF), (%rsp)

	popfq
	RET

SYM_FUNC_END(this_cpu_cmpxchg16b_emu)
