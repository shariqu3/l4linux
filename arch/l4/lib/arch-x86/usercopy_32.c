#include <asm/uaccess.h>
#include <asm/uaccess_32.h>

unsigned long
copy_from_user_inatomic_nontemporal(void *to, const void __user *from,
                                    unsigned long n)
{
	if (!access_ok(from, n))
		return n;
	return raw_copy_from_user(to, from, n);
}
