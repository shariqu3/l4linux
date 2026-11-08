/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_UACCESS_32_H
#define _ASM_X86_UACCESS_32_H

/*
 * User space memory access functions
 */
#include <linux/string.h>
#include <asm/asm.h>
#include <asm/page.h>

#ifndef CONFIG_L4
unsigned long __must_check __copy_user_ll
		(void *to, const void *from, unsigned long n);
unsigned long __must_check __copy_from_user_ll_nocache_nozero
		(void *to, const void __user *from, unsigned long n);
#endif /* L4 */

static __always_inline unsigned long __must_check
raw_copy_to_user(void __user *to, const void *from, unsigned long n)
{
#ifdef CONFIG_L4
	return l4x_copy_to_user(to, from, n);
#else /* L4 */
	return __copy_user_ll((__force void *)to, from, n);
#endif /* L4 */
}

static __always_inline unsigned long
raw_copy_from_user(void *to, const void __user *from, unsigned long n)
{
#ifdef CONFIG_L4
	return l4x_copy_from_user(to, from, n);
#else /* L4 */
	return __copy_user_ll(to, (__force const void *)from, n);
#endif /* L4 */
}

unsigned long __must_check copy_from_user_inatomic_nontemporal(void *, const void __user *, unsigned long n);
unsigned long __must_check clear_user(void __user *mem, unsigned long len);
unsigned long __must_check __clear_user(void __user *mem, unsigned long len);

#endif /* _ASM_X86_UACCESS_32_H */
