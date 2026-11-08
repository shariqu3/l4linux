
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/dma-direct.h>

#include <linux/dma-mapping.h>

#include <asm/api/api.h>

#ifdef CONFIG_L4_FB_DRIVER
 #if defined(CONFIG_SERIO_I8042) || defined(CONFIG_SERIO_LIBPS2) \
     || defined(CONFIG_VGA_CONSOLE)
  #warning WARNING: L4FB enabled and also CONFIG_SERIO_I8042 or CONFIG_SERIO_LIBPS2 or CONFIG_VGA_CONSOLE
  #warning WARNING: This is usually not wanted.
 #endif

 #ifndef CONFIG_INPUT_EVDEV
  #warning WARNING: L4FB enabled but not CONFIG_INPUT_EVDEV, you probably want to enable this option.
 #endif
 #ifndef CONFIG_INPUT_MOUSEDEV
  #warning WARNING: L4FB enabled but not CONFIG_INPUT_MOUSEDEV, you probably want to enable this option.
 #endif
#endif

// this may be a bit harsh but well...
#if defined(CONFIG_X86) && !defined(CONFIG_L4_FB_DRIVER) && \
    !defined(CONFIG_VGA_CONSOLE) && \
    !defined(CONFIG_FRAMEBUFFER_CONSOLE) && \
    !defined(CONFIG_L4_SERIAL_CONSOLE)
 #error L4_FB_DRIVER nor VGA_CONSOLE nor FRAMEBUFFER_CONSOLE nor L4_SERIAL_CONSOLE enabled, choose one.
#endif


static void my_dev_free(struct device *d)
{
	kfree(d);
}

static int __init l4x_check(void)
{
	struct device *dev;
	dma_addr_t dmahandle;
	void *mem;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	BUG_ON(!dev);

	device_initialize(dev);
	dev->release           = my_dev_free;
	dev->coherent_dma_mask = 0xffffffffULL;
	dev->dma_mask          = &dev->coherent_dma_mask;

#ifdef CONFIG_ARM64
	printk("Note, on ARM64 DMA mapping are not yet enabled.\n");
	(void)dmahandle;
	(void)mem;
#else
	mem = dma_alloc_coherent(dev, 30, &dmahandle, GFP_KERNEL);
#if defined(CONFIG_X86_64)
	if (mem)
#endif
	{
		dma_addr_t l4x_phys_addr = l4x_virt_to_phys(mem);
		if (dmahandle != l4x_phys_addr) {
			printk("BUG: dmahandle=%llx != l4x_virt_to_phys=%llx (mem = %llx)\n",
			       (unsigned long long)dmahandle,
			       (unsigned long long)l4x_phys_addr,
			       (unsigned long long)(unsigned long)mem);
			BUG();
		}
		dma_free_coherent(dev, 30, mem, dmahandle);
	}
#endif

	put_device(dev);

	printk("l4x: Checks passed.\n");

	return 0;
}

module_init(l4x_check);
