# SPDX-License-Identifier: GPL-2.0

b := ../../../arm64/lib

lib-y		:= delay.o \
		   copy_page.o            \
		   clear_page.o csum.o insn.o memchr.o memcpy.o		\
		   memset.o memcmp.o strcmp.o strncmp.o strlen.o	\
		   strnlen.o strchr.o strrchr.o tishift.o

lib-$(CONFIG_ARCH_HAS_UACCESS_FLUSHCACHE) += uaccess_flushcache.o

obj-$(CONFIG_FUNCTION_ERROR_INJECTION) += error-inject.o

obj-$(CONFIG_ARM64_MTE) += mte.o

obj-$(CONFIG_KASAN_SW_TAGS) += kasan_sw_tags.o

include $(srctree)/arch/l4/Makefile.lib

lib-y   += ../pte.o ../memcpy_user.o ../uaccess.o
obj-y   += ../proc.o
$(info must also not be compiled when !CONFIG_DEBUG_FS)
obj-$(CONFIG_L4_DEBUG) += ../debugfs.o

$(call L4X_FILEREF, $(b), error-inject mte)

lib-y := $(call L4X_PREFIX_IF, $(b), $(lib-y), \
                clear_page copy_page csum delay insn memchr memcmp \
		memcpy memset strchr strcmp strlen \
		strncmp strnlen strrchr tishift)
