#ifndef __ASM_L4__GENERIC__CAP_ALLOC_H__
#define __ASM_L4__GENERIC__CAP_ALLOC_H__

#include <l4/sys/types.h>
#include <l4/re/c/util/cap_alloc.h>
#include <linux/spinlock.h>

extern raw_spinlock_t l4x_cap_lock;

static inline l4_cap_idx_t l4x_cap_alloc(void)
{
	l4_cap_idx_t c;
	unsigned long flags;
	raw_spin_lock_irqsave(&l4x_cap_lock, flags);
	c = l4re_util_cap_alloc();
	raw_spin_unlock_irqrestore(&l4x_cap_lock, flags);
	return c;
}

static inline void l4x_cap_free(l4_cap_idx_t c)
{
	unsigned long flags;
	raw_spin_lock_irqsave(&l4x_cap_lock, flags);
	l4re_util_cap_free(c);
	raw_spin_unlock_irqrestore(&l4x_cap_lock, flags);
}

static inline l4_cap_idx_t l4x_cap_alloc_noctx(void)
{
	l4_cap_idx_t c;
	arch_spin_lock(&l4x_cap_lock.raw_lock);
	c = l4re_util_cap_alloc();
	arch_spin_unlock(&l4x_cap_lock.raw_lock);
	return c;
}

static inline void l4x_cap_free_noctx(l4_cap_idx_t c)
{
	arch_spin_lock(&l4x_cap_lock.raw_lock);
	l4re_util_cap_free(c);
	arch_spin_unlock(&l4x_cap_lock.raw_lock);
}


#endif /* ! __ASM_L4__GENERIC__CAP_ALLOC_H__ */
