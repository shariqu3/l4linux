
#include <linux/types.h>

#include <asm/x86_init.h>
#include <asm/apic.h>

#include <asm/generic/smp.h>
#include <asm/generic/timer.h>

#include <asm/l4x/platform.h>
#include <asm/l4x/irq.h>
#include <asm/l4x/init.h>


static void __init l4x_mpparse_parse_smp_cfg(void)
{
#ifdef CONFIG_SMP
	smp_found_config = 1;
	for (int i = 0; i < l4x_nr_cpus; ++i)
		topology_register_apic(i, CPU_ACPIID_INVALID, true);
#endif
}

void __init l4x_platform_init_x86_init(void)
{
	x86_init.resources.memory_setup      = l4x_x86_memory_setup;

	x86_init.irqs.intr_init              = l4x_x86_init_irq;

	x86_init.timers.wallclock_init       = x86_init_noop;
	x86_init.timers.timer_init           = l4x_timer_init;

	x86_init.mpparse.find_mptable        = x86_init_noop;
	x86_init.mpparse.early_parse_smp_cfg = x86_init_noop;

	x86_init.mpparse.parse_smp_cfg       = l4x_mpparse_parse_smp_cfg;
}
