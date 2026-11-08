# SPDX-License-Identifier: GPL-2.0
obj-y				:= dma-mapping.o extable.o fault.o init.o \
				   cache.o copypage.o flush.o \
				   ioremap.o mmap.o pgd.o mem_encrypt.o mmu.o \
				   context.o proc.o pageattr.o fixmap.o
obj-$(CONFIG_ARM64_CONTPTE)	+= contpte.o
obj-$(CONFIG_HUGETLB_PAGE)	+= hugetlbpage.o
obj-$(CONFIG_PTDUMP)		+= ptdump.o
obj-$(CONFIG_PTDUMP_DEBUGFS)	+= ptdump_debugfs.o
obj-$(CONFIG_TRANS_TABLE)	+= trans_pgd.o
obj-$(CONFIG_TRANS_TABLE)	+= trans_pgd-asm.o
obj-$(CONFIG_DEBUG_VIRTUAL)	+= physaddr.o
obj-$(CONFIG_ARM64_MTE)		+= mteswap.o
obj-$(CONFIG_ARM64_GCS)		+= gcs.o
KASAN_SANITIZE_physaddr.o	+= n

obj-$(CONFIG_KASAN)		+= kasan_init.o
KASAN_SANITIZE_kasan_init.o	:= n

include $(srctree)/arch/l4/Makefile.lib

obj-y				+= ../initrd.o

$(call L4X_FILEREF, ../../../arm64/mm, \
       dma-mapping extable cache copypage flush gcs mmap pgd proc pageattr \
       trans_pgd trans_pgd-asm mteswap ioremap physaddr contpte mem_encrypt)
