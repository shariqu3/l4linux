# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the L4 architecture.
#

# BITS is used as extension for files which are available in a 32 bit
# and a 64 bit version to simplify shared Makefiles.
# e.g.: obj-y += foo_$(BITS).o
BITS := 32
export BITS

ifeq ($(LINSRCARCH),arm)
# arm selection
L4API		:= f
L4X_SRCARCH	:= arm

CHECKFLAGS	:= -D__arm__

L4_MK_ARCH	:= arm
OBJCOPYARCH	:= elf32-littlearm
endif

ifeq ($(LINSRCARCH),arm64)
BITS		:= 64
L4API		:= f
L4X_SRCARCH	:= arm64
CHECKFLAGS      += -D__AARCH64EL__
CHECKFLAGS      += -D__aarch64__
L4_MK_ARCH      := arm64
OBJCOPYARCH     := elf64-littleaarch64
endif

ifeq ($(LINSRCARCH),x86)
# x86 selection
L4API		:= f
L4X_SRCARCH	:= x86

ifneq ($(CONFIG_X86_32),)
L4_MK_ARCH	:= x86
OBJCOPYARCH	:= elf32-i386
else
L4_MK_ARCH	:= amd64
OBJCOPYARCH	:= elf64-x86-64
endif

endif

PLATFORMNAME-$(CONFIG_L4_PLATFORM_GENERIC)  = generic
PLATFORMNAME-$(CONFIG_L4_PLATFORM_REALVIEW) = realview
PLATFORMNAME                                = $(PLATFORMNAME-y)

# select defconfig based on actual architecture
ifeq ($(ARCH),x86)
  ifeq ($(shell uname -m | sed -e 's/i.86/i386/'),i386)
        KBUILD_DEFCONFIG := i386_defconfig
  else
        KBUILD_DEFCONFIG := x86_64_defconfig
  endif
else
        KBUILD_DEFCONFIG := $(ARCH)_defconfig
endif

ifeq ($(L4X_SRCARCH),x86)
ifdef CONFIG_CC_IS_GCC
RETPOLINE_CFLAGS	:= -mindirect-branch=thunk-extern -mindirect-branch-register
RETPOLINE_VDSO_CFLAGS	:= -mindirect-branch=thunk-inline -mindirect-branch-register
endif
ifdef CONFIG_CC_IS_CLANG
RETPOLINE_CFLAGS	:= -mretpoline-external-thunk
RETPOLINE_VDSO_CFLAGS	:= -mretpoline
endif
RETPOLINE_CFLAGS	+= $(call cc-option,-mindirect-branch-cs-prefix)

ifdef CONFIG_MITIGATION_RETHUNK
RETHUNK_CFLAGS		:= -mfunction-return=thunk-extern
RETHUNK_RUSTFLAGS	:= -Zfunction-return=thunk-extern
RETPOLINE_CFLAGS	+= $(RETHUNK_CFLAGS)
RETPOLINE_RUSTFLAGS	+= $(RETHUNK_RUSTFLAGS)
endif

export RETHUNK_CFLAGS
export RETHUNK_RUSTFLAGS
export RETPOLINE_CFLAGS
export RETPOLINE_RUSTFLAGS
export RETPOLINE_VDSO_CFLAGS

# For gcc stack alignment is specified with -mpreferred-stack-boundary,
# clang has the option -mstack-alignment for that purpose.
# XXX The 16-byte-alignment for AMD64 is required to be able to execute SSE
# instructions in external L4Re libraries. Executing such instructions is
# forbidden in the Linux kernel but for now we accept it because the library
# code is usually called early. See ticket #CD-1027.
ifdef CONFIG_CC_IS_GCC
      cc_stack_align4 := -mpreferred-stack-boundary=2
      cc_stack_align8 := -mpreferred-stack-boundary=4
endif
ifdef CONFIG_CC_IS_CLANG
      cc_stack_align4 := -mstack-alignment=4
      cc_stack_align8 := -mstack-alignment=16
endif
endif # L4X_SRCARCH==x86

VMLINUZ_SUFFIX = $(patsubst "%",%,$(CONFIG_L4_VMLINUZ_SUFFIX))

export L4X_SRCARCH L4_MK_ARCH L4_MK_API OBJCOPYARCH VMLINUZ_SUFFIX

#HAS_BIARCH	:= $(call cc-option-yn, -m32)
#ifeq ($(HAS_BIARCH),y)
#AS		:= $(AS) --32
#LD		:= $(LD) -m elf_i386
#CC		:= $(CC) -m32
#endif

BASEENV		:= l4env

L4_MK_API	:= l4$(L4API)

# overwrite UTS_MACHINE to get proper architecture in user land
UTS_MACHINE	:= $(L4X_SRCARCH)

KBUILD_CFLAGS   += -pipe -DARCH_$(L4_MK_ARCH)

ifeq ($(L4X_SRCARCH),x86)

ifneq ($(CONFIG_X86),)
#
# Prevent GCC from generating any FP code by mistake.
#
# This must happen before we try the -mpreferred-stack-boundary, see:
#
#    https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53383
#
# It would be nicer to use L4_CFLAGS from l4defs-nofpu.mk.inc here but these
# flags enable more warnings.
KBUILD_CFLAGS += -mno-sse -mno-mmx -mno-sse2 -mno-sse3 -mno-sse4 -mno-sse4a \
                 -mno-3dnow -mno-avx -mno-avx2
KBUILD_RUSTFLAGS += --target=$(objtree)/scripts/target.json
KBUILD_RUSTFLAGS += -Ctarget-feature=-sse,-sse2,-sse3,-ssse3,-sse4.1,-sse4.2,-avx,-avx2

# The target.json file is not available when invoking rustc-option, so use the
# built-in target when checking whether flags are supported instead.
KBUILD_RUSTFLAGS_OPTION_CHKS += --target=x86_64-unknown-none

#
# CFLAGS for compiling floating point code inside the kernel.
#
CC_FLAGS_FPU := -msse -msse2
ifdef CONFIG_CC_IS_GCC
CC_FLAGS_FPU += -mhard-float
endif

ifeq ($(CONFIG_X86_KERNEL_IBT),y)
#
# Kernel IBT has S_CET.NOTRACK_EN=0, as such the compilers must not generate
# NOTRACK prefixes. Current generation compilers unconditionally employ NOTRACK
# for jump-tables, as such, disable jump-tables for now.
#
# (jump-tables are implicitly disabled by RETPOLINE)
#
#   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=104816
#
KBUILD_CFLAGS += $(call cc-option,-fcf-protection=branch -fno-jump-tables)
KBUILD_RUSTFLAGS += -Zcf-protection=branch $(if $(call rustc-min-version,109300),-Cjump-tables=n,-Zno-jump-tables)
else
KBUILD_CFLAGS += $(call cc-option,-fcf-protection=none)
endif
endif # X86

ifeq ($(CONFIG_X86_32),y)
        BITS := 32
        UTS_MACHINE := i386
        CHECKFLAGS += -D__i386__

        KBUILD_AFLAGS += -m32
        KBUILD_CFLAGS += -m32

        #KBUILD_CFLAGS += -msoft-float -mregparm=3 -freg-struct-return
	KBUILD_CFLAGS += -mregparm=3

        # Never want PIC in a 32-bit kernel, prevent breakage with GCC built
        # with nonstandard options
        KBUILD_CFLAGS += -fno-pic

        # Align the stack to the register width instead of using the default
        # alignment of 16 bytes. This reduces stack usage and the number of
        # alignment instructions.
        KBUILD_CFLAGS += $(cc_stack_align4)

        # CPU-specific tuning. Anything which can be shared with UML should go here.
        include $(srctree)/arch/x86/Makefile_32.cpu
        KBUILD_CFLAGS += $(cflags-y)

        percpu_seg := fs
else
        BITS := 64
        UTS_MACHINE := x86_64
        CHECKFLAGS += -D__x86_64__

        KBUILD_AFLAGS += -m64
        KBUILD_CFLAGS += -m64

        # Align jump targets to 1 byte, not the default 16 bytes:
        KBUILD_CFLAGS += $(call cc-option,-falign-jumps=1)

        # Pack loops tightly as well:
        KBUILD_CFLAGS += $(call cc-option,-falign-loops=1)

        # Don't autogenerate traditional x87 instructions
        KBUILD_CFLAGS += -mno-80387
        KBUILD_CFLAGS += -mno-fp-ret-in-387

        # By default gcc and clang use a stack alignment of 16 bytes for x86.
        # However the standard kernel entry on x86-64 leaves the stack on an
        # 8-byte boundary. If the compiler isn't informed about the actual
        # alignment it will generate extra alignment instructions for the
        # default alignment which keep the stack *mis*aligned.
        # Furthermore an alignment to the register width reduces stack usage
        # and the number of alignment instructions.
        KBUILD_CFLAGS += $(cc_stack_align8)

	# Use -mskip-rax-setup if supported.
	KBUILD_CFLAGS += -mskip-rax-setup

ifdef CONFIG_X86_NATIVE_CPU
        KBUILD_CFLAGS += -march=native
        KBUILD_RUSTFLAGS += -Ctarget-cpu=native
else
        KBUILD_CFLAGS += -march=x86-64 -mtune=generic
        KBUILD_RUSTFLAGS += -Ctarget-cpu=x86-64 -Ztune-cpu=generic
endif

        KBUILD_CFLAGS += -mno-red-zone
        KBUILD_CFLAGS += -mcmodel=kernel
        KBUILD_RUSTFLAGS += -Cno-redzone=y
        KBUILD_RUSTFLAGS += -Ccode-model=kernel

        percpu_seg := gs
endif

ifeq ($(CONFIG_STACKPROTECTOR),y)
    ifeq ($(CONFIG_SMP),y)
	KBUILD_CFLAGS += -mstack-protector-guard-reg=$(percpu_seg)
	KBUILD_CFLAGS += -mstack-protector-guard-symbol=__ref_stack_chk_guard
    else
	KBUILD_CFLAGS += -mstack-protector-guard=global
    endif
endif

ifneq ($(CONFIG_X86),)
#
# If the function graph tracer is used with mcount instead of fentry,
# '-maccumulate-outgoing-args' is needed to prevent a GCC bug
# (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=42109)
#
ifdef CONFIG_FUNCTION_GRAPH_TRACER
  ifndef CONFIG_HAVE_FENTRY
	ACCUMULATE_OUTGOING_ARGS := 1
  endif
endif

ifeq ($(ACCUMULATE_OUTGOING_ARGS), 1)
	# This compiler flag is not supported by Clang:
	KBUILD_CFLAGS += $(call cc-option,-maccumulate-outgoing-args,)
endif
endif # X86

# Workaround for a gcc prelease that unfortunately was shipped in a suse release
KBUILD_CFLAGS += -Wno-sign-compare
#
KBUILD_CFLAGS += -fno-asynchronous-unwind-tables

# Avoid indirect branches in kernel to deal with Spectre
ifdef CONFIG_MITIGATION_RETPOLINE
  KBUILD_CFLAGS += $(RETPOLINE_CFLAGS)
  KBUILD_RUSTFLAGS += $(RETPOLINE_RUSTFLAGS)
  # Additionally, avoid generating expensive indirect jumps which
  # are subject to retpolines for small number of switch cases.
  # LLVM turns off jump table generation by default when under
  # retpoline builds, however, gcc does not for x86. This has
  # only been fixed starting from gcc stable version 8.4.0 and
  # onwards, but not for older ones. See gcc bug #86952.
  ifndef CONFIG_CC_IS_CLANG
    KBUILD_CFLAGS += -fno-jump-tables
  endif
endif

ifdef CONFIG_MITIGATION_SLS
  KBUILD_CFLAGS += -mharden-sls=all
endif

ifdef CONFIG_CALL_PADDING
PADDING_CFLAGS := -fpatchable-function-entry=$(CONFIG_FUNCTION_PADDING_BYTES),$(CONFIG_FUNCTION_PADDING_BYTES)
KBUILD_CFLAGS += $(PADDING_CFLAGS)
export PADDING_CFLAGS

PADDING_RUSTFLAGS := -Zpatchable-function-entry=$(CONFIG_FUNCTION_PADDING_BYTES),$(CONFIG_FUNCTION_PADDING_BYTES)
KBUILD_RUSTFLAGS += $(PADDING_RUSTFLAGS)
export PADDING_RUSTFLAGS
endif

KBUILD_LDFLAGS += -m elf_$(UTS_MACHINE)

#
# The 64-bit kernel must be aligned to 2MB.  Pass -z max-page-size=0x200000 to
# the linker to force 2MB page size regardless of the default page size used
# by the linker.
#
ifdef CONFIG_X86_64
LDFLAGS_vmlinux += -z max-page-size=0x200000
endif
endif

ifeq ($(L4X_SRCARCH),arm)
ifeq ($(CONFIG_CPU_ENDIAN_BE8),y)
LDFLAGS_vmlinux		+= --be8
KBUILD_LDFLAGS_MODULE	+= --be8
endif

ifeq ($(CONFIG_CPU_BIG_ENDIAN),y)
KBUILD_CPPFLAGS	+= -mbig-endian
CHECKFLAGS	+= -D__ARMEB__
KBUILD_LDFLAGS	+= -EB
else
KBUILD_CPPFLAGS	+= -mlittle-endian
CHECKFLAGS	+= -D__ARMEL__
KBUILD_LDFLAGS	+= -EL
endif

#
# The Scalar Replacement of Aggregates (SRA) optimization pass in GCC 4.9 and
# later may result in code being generated that handles signed short and signed
# char struct members incorrectly. So disable it.
# (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65932)
#
KBUILD_CFLAGS	+= $(call cc-option,-fno-ipa-sra)

# This selects which instruction set is used.
arch-$(CONFIG_CPU_32v7M)	:=-march=armv7-m
arch-$(CONFIG_CPU_32v7)		:=-march=armv7-a
arch-$(CONFIG_CPU_32v6)		:=-march=armv6
# Only override the compiler option if ARMv6. The ARMv6K extensions are
# always available in ARMv7
ifeq ($(CONFIG_CPU_32v6),y)
arch-$(CONFIG_CPU_32v6K)	:=-march=armv6k
endif
arch-$(CONFIG_CPU_32v5)		:=-march=armv5te
arch-$(CONFIG_CPU_32v4T)	:=-march=armv4t
arch-$(CONFIG_CPU_32v4)		:=-march=armv4
arch-$(CONFIG_CPU_32v3)		:=-march=armv3m

# Note that GCC does not numerically define an architecture version
# macro, but instead defines a whole series of macros which makes
# testing for a specific architecture or later rather impossible.
cpp-$(CONFIG_CPU_32v7M)		:=-D__LINUX_ARM_ARCH__=7
cpp-$(CONFIG_CPU_32v7)		:=-D__LINUX_ARM_ARCH__=7
cpp-$(CONFIG_CPU_32v6)		:=-D__LINUX_ARM_ARCH__=6
# Only override the compiler option if ARMv6. The ARMv6K extensions are
# always available in ARMv7
ifeq ($(CONFIG_CPU_32v6),y)
cpp-$(CONFIG_CPU_32v6K)		:=-D__LINUX_ARM_ARCH__=6
endif
cpp-$(CONFIG_CPU_32v5)		:=-D__LINUX_ARM_ARCH__=5
cpp-$(CONFIG_CPU_32v4T)		:=-D__LINUX_ARM_ARCH__=4
cpp-$(CONFIG_CPU_32v4)		:=-D__LINUX_ARM_ARCH__=4
cpp-$(CONFIG_CPU_32v3)		:=-D__LINUX_ARM_ARCH__=3

# This selects how we optimise for the processor.
tune-$(CONFIG_CPU_ARM7TDMI)	:=-mtune=arm7tdmi
tune-$(CONFIG_CPU_ARM720T)	:=-mtune=arm7tdmi
tune-$(CONFIG_CPU_ARM740T)	:=-mtune=arm7tdmi
tune-$(CONFIG_CPU_ARM9TDMI)	:=-mtune=arm9tdmi
tune-$(CONFIG_CPU_ARM940T)	:=-mtune=arm9tdmi
tune-$(CONFIG_CPU_ARM946E)	:=-mtune=arm9e
tune-$(CONFIG_CPU_ARM920T)	:=-mtune=arm9tdmi
tune-$(CONFIG_CPU_ARM922T)	:=-mtune=arm9tdmi
tune-$(CONFIG_CPU_ARM925T)	:=-mtune=arm9tdmi
tune-$(CONFIG_CPU_ARM926T)	:=-mtune=arm9tdmi
tune-$(CONFIG_CPU_FA526)	:=-mtune=arm9tdmi
tune-$(CONFIG_CPU_SA110)	:=-mtune=strongarm110
tune-$(CONFIG_CPU_SA1100)	:=-mtune=strongarm1100
tune-$(CONFIG_CPU_XSCALE)	:=-mtune=xscale
tune-$(CONFIG_CPU_XSC3)		:=-mtune=xscale
tune-$(CONFIG_CPU_FEROCEON)	:=-mtune=xscale
tune-$(CONFIG_CPU_V6)		:=-mtune=arm1136j-s
tune-$(CONFIG_CPU_V6K)		:=-mtune=arm1136j-s

ifeq ($(CONFIG_AEABI),y)
CFLAGS_ABI	:=-mabi=aapcs-linux -mfpu=vfp
else
CFLAGS_ABI	:=$(call cc-option,-mapcs-32,-mabi=apcs-gnu) $(call cc-option,-mno-thumb-interwork,)
endif

ifeq ($(CONFIG_ARM_UNWIND),y)
CFLAGS_ABI	+=-funwind-tables
endif

ifeq ($(CONFIG_CC_IS_CLANG),y)
CFLAGS_ABI	+= -meabi gnu
endif

ifeq ($(CONFIG_CURRENT_POINTER_IN_TPIDRURO),y)
KBUILD_CFLAGS	+= -mtp=cp15
endif

# Accept old syntax despite ".syntax unified"
AFLAGS_NOWARN	:=$(call as-option,-Wa$(comma)-mno-warn-deprecated,-Wa$(comma)-W)

# The GCC option -ffreestanding is required in order to compile code containing
# ARM/NEON intrinsics in a non C99-compliant environment (such as the kernel)
CC_FLAGS_FPU	:= -ffreestanding
# Enable <arm_neon.h>
CC_FLAGS_FPU	+= -isystem $(shell $(CC) -print-file-name=include)
CC_FLAGS_FPU	+= -march=armv7-a -mfloat-abi=softfp -mfpu=neon

ifeq ($(CONFIG_THUMB2_KERNEL),y)
CFLAGS_ISA	:=-Wa,-mimplicit-it=always $(AFLAGS_NOWARN)
AFLAGS_ISA	:=$(CFLAGS_ISA) -Wa$(comma)-mthumb
CFLAGS_ISA	+=-mthumb
else
CFLAGS_ISA	:=$(call cc-option,-marm,) $(AFLAGS_NOWARN)
AFLAGS_ISA	:=$(CFLAGS_ISA)
endif

# Need -Uarm for gcc < 3.x
KBUILD_CPPFLAGS	+=$(cpp-y)
KBUILD_CFLAGS	+=$(CFLAGS_ABI) $(CFLAGS_ISA) $(arch-y) $(tune-y) $(call cc-option,-mshort-load-bytes,$(call cc-option,-malignment-traps,)) -msoft-float -Uarm
KBUILD_AFLAGS	+=$(CFLAGS_ABI) $(AFLAGS_ISA) -Wa,$(arch-y) $(tune-y) -include $(srctree)/arch/arm/include/asm/unified.h -msoft-float
KBUILD_RUSTFLAGS += --target=arm-unknown-linux-gnueabi

CHECKFLAGS	+= -D__arm__

# Text offset. This list is sorted numerically by address in order to
# provide a means to avoid/resolve conflicts in multi-arch kernels.
# Note: the 32kB below this value is reserved for use by the kernel
# during boot, and this offset is critical to the functioning of
# kexec-tools.
textofs-y	:= 0x00008000
# RTD1195 has Boot ROM at start of address space
textofs-$(CONFIG_ARCH_REALTEK)  := 0x00108000
# SA1111 DMA bug: we don't want the kernel to live in precious DMA-able memory
ifeq ($(CONFIG_ARCH_SA1100),y)
textofs-$(CONFIG_SA1111) := 0x00208000
endif
textofs-$(CONFIG_ARCH_QCOM_RESERVE_SMEM) := 0x00208000
textofs-$(CONFIG_ARCH_MESON) := 0x00208000
textofs-$(CONFIG_ARCH_AXXIA) := 0x00308000
textofs-$(CONFIG_L4) := 0x0200000

# Machine directory name.  This list is sorted alphanumerically
# by CONFIG_* macro name.
machine-$(CONFIG_ARCH_ACTIONS)		+= actions
machine-$(CONFIG_ARCH_ALPINE)		+= alpine
machine-$(CONFIG_ARCH_ARTPEC)		+= artpec
machine-$(CONFIG_ARCH_ASPEED)           += aspeed
machine-$(CONFIG_ARCH_AT91)		+= at91
machine-$(CONFIG_ARCH_AXXIA)		+= axxia
machine-$(CONFIG_ARCH_BCM)		+= bcm
machine-$(CONFIG_ARCH_BERLIN)		+= berlin
machine-$(CONFIG_ARCH_CLPS711X)		+= clps711x
machine-$(CONFIG_ARCH_DAVINCI)		+= davinci
machine-$(CONFIG_ARCH_DIGICOLOR)	+= digicolor
machine-$(CONFIG_ARCH_DOVE)		+= dove
machine-$(CONFIG_ARCH_EXYNOS)		+= exynos
machine-$(CONFIG_ARCH_FOOTBRIDGE)	+= footbridge
machine-$(CONFIG_ARCH_GEMINI)		+= gemini
machine-$(CONFIG_ARCH_HIGHBANK)		+= highbank
machine-$(CONFIG_ARCH_HISI)		+= hisi
machine-$(CONFIG_ARCH_HPE)		+= hpe
machine-$(CONFIG_ARCH_IXP4XX)		+= ixp4xx
machine-$(CONFIG_ARCH_KEYSTONE)		+= keystone
machine-$(CONFIG_ARCH_LPC18XX)		+= lpc18xx
machine-$(CONFIG_ARCH_LPC32XX)		+= lpc32xx
machine-$(CONFIG_ARCH_MESON)		+= meson
machine-$(CONFIG_ARCH_MMP)		+= mmp
machine-$(CONFIG_ARCH_MV78XX0)		+= mv78xx0
machine-$(CONFIG_ARCH_MVEBU)		+= mvebu
machine-$(CONFIG_ARCH_MXC)		+= imx
machine-$(CONFIG_ARCH_MEDIATEK)		+= mediatek
machine-$(CONFIG_ARCH_MILBEAUT)		+= milbeaut
machine-$(CONFIG_ARCH_MXS)		+= mxs
machine-$(CONFIG_ARCH_MSTARV7)		+= mstar
machine-$(CONFIG_ARCH_NOMADIK)		+= nomadik
machine-$(CONFIG_ARCH_NPCM)		+= npcm
machine-$(CONFIG_ARCH_OMAP1)		+= omap1
machine-$(CONFIG_ARCH_OMAP2PLUS)	+= omap2
machine-$(CONFIG_ARCH_ORION5X)		+= orion5x
machine-$(CONFIG_ARCH_PXA)		+= pxa
machine-$(CONFIG_ARCH_QCOM)		+= qcom
machine-$(CONFIG_ARCH_REALTEK)		+= realtek
machine-$(CONFIG_ARCH_ROCKCHIP)		+= rockchip
machine-$(CONFIG_ARCH_RPC)		+= rpc
machine-$(CONFIG_PLAT_SAMSUNG)		+= s3c
machine-$(CONFIG_ARCH_S5PV210)		+= s5pv210
machine-$(CONFIG_ARCH_SA1100)		+= sa1100
machine-$(CONFIG_ARCH_RENESAS)	 	+= shmobile
machine-$(CONFIG_ARCH_INTEL_SOCFPGA)	+= socfpga
machine-$(CONFIG_ARCH_STI)		+= sti
machine-$(CONFIG_ARCH_STM32)		+= stm32
machine-$(CONFIG_ARCH_SUNXI)		+= sunxi
machine-$(CONFIG_ARCH_TEGRA)		+= tegra
machine-$(CONFIG_ARCH_U8500)		+= ux500
machine-$(CONFIG_ARCH_VT8500)		+= vt8500
machine-$(CONFIG_ARCH_ZYNQ)		+= zynq
machine-$(CONFIG_PLAT_VERSATILE)	+= versatile
machine-$(CONFIG_PLAT_SPEAR)		+= spear

# legacy platforms provide their own mach/*.h headers globally,
# these three are mutually exclusive
machdirs-$(CONFIG_ARCH_FOOTBRIDGE)	+= arch/arm/mach-footbridge
machdirs-$(CONFIG_ARCH_RPC)		+= arch/arm/mach-rpc
machdirs-$(CONFIG_ARCH_SA1100)		+= arch/arm/mach-sa1100
KBUILD_CPPFLAGS += $(patsubst %,-I$(srctree)/%/include,$(machdirs-y))

# The byte offset of the kernel image in RAM from the start of RAM.
TEXT_OFFSET := $(textofs-y)

export	TEXT_OFFSET GZFLAGS MMUEXT

# If we have a machine-specific directory, then include it in the build.
core-y				+= $(patsubst %,arch/arm/mach-%/,$(machine-y))
# For cleaning
core-				+= $(patsubst %,arch/arm/mach-%/,$(machine-))

core-$(CONFIG_PLAT_ORION)	+= arch/arm/plat-orion/

ifeq ($(CONFIG_STACKPROTECTOR_PER_TASK),y)
prepare: stack_protector_prepare
ifeq ($(CONFIG_CC_HAVE_STACKPROTECTOR_TLS),y)
stack_protector_prepare: prepare0
	$(eval KBUILD_CFLAGS += \
		-mstack-protector-guard=tls \
		-mstack-protector-guard-offset=$(shell	\
			awk '{if ($$2 == "TSK_STACK_CANARY") print $$3;}'\
				$(objtree)/include/generated/asm-offsets.h))
else
stack_protector_prepare: prepare0
	$(eval SSP_PLUGIN_CFLAGS := \
		-fplugin-arg-arm_ssp_per_task_plugin-offset=$(shell	\
			awk '{if ($$2 == "TSK_STACK_CANARY") print $$3;}'\
				$(objtree)/include/generated/asm-offsets.h))
	$(eval KBUILD_CFLAGS += $(SSP_PLUGIN_CFLAGS))
	$(eval GCC_PLUGINS_CFLAGS += $(SSP_PLUGIN_CFLAGS))
endif
endif
endif

ifneq ($(CONFIG_L4_ARCH_ARM64),)

cc_has_k_constraint := $(call try-run,echo				\
	'int main(void) {						\
		asm volatile("and w0, w0, %w0" :: "K" (4294967295));	\
		return 0;						\
	}' | $(CC) -S -x c -o "$$TMP" -,,-DCONFIG_CC_HAS_K_CONSTRAINT=1)

ifeq ($(CONFIG_BROKEN_GAS_INST),y)
$(warning Detected assembler with broken .inst; disassembly will be unreliable)
endif

# The GCC option -ffreestanding is required in order to compile code containing
# ARM/NEON intrinsics in a non C99-compliant environment (such as the kernel)
CC_FLAGS_FPU	:= -ffreestanding
# Enable <arm_neon.h>
CC_FLAGS_FPU	+= -isystem $(shell $(CC) -print-file-name=include)
CC_FLAGS_NO_FPU	:= -mgeneral-regs-only

KBUILD_CFLAGS	+= $(CC_FLAGS_NO_FPU) \
		   $(compat_vdso) $(cc_has_k_constraint)
KBUILD_CFLAGS	+= $(call cc-disable-warning, psabi)
KBUILD_AFLAGS	+= $(compat_vdso)

ifeq ($(call rustc-min-version, 108500),y)
KBUILD_RUSTFLAGS += --target=aarch64-unknown-none-softfloat
else
KBUILD_RUSTFLAGS += --target=aarch64-unknown-none -Ctarget-feature="-neon"
endif

KBUILD_CFLAGS	+= $(call cc-option,-mabi=lp64)
KBUILD_AFLAGS	+= $(call cc-option,-mabi=lp64)

# Avoid generating .eh_frame* sections.
ifneq ($(CONFIG_UNWIND_TABLES),y)
KBUILD_CFLAGS	+= -fno-asynchronous-unwind-tables -fno-unwind-tables
KBUILD_AFLAGS	+= -fno-asynchronous-unwind-tables -fno-unwind-tables
KBUILD_RUSTFLAGS += -Cforce-unwind-tables=n
else
KBUILD_CFLAGS	+= -fasynchronous-unwind-tables
KBUILD_AFLAGS	+= -fasynchronous-unwind-tables
KBUILD_RUSTFLAGS += -Cforce-unwind-tables=y -Zuse-sync-unwind=n
# Work around rustc bug on compilers without
# https://github.com/rust-lang/rust/pull/156973.
KBUILD_RUSTFLAGS += $(if $(call rustc-min-version,109800),,-Zllvm_module_flag=uwtable:u32:2:max)
endif

ifeq ($(CONFIG_STACKPROTECTOR_PER_TASK),y)
prepare: stack_protector_prepare
stack_protector_prepare: prepare0
	$(eval KBUILD_CFLAGS += -mstack-protector-guard=sysreg		  \
				-mstack-protector-guard-reg=sp_el0	  \
				-mstack-protector-guard-offset=$(shell	  \
			awk '{if ($$2 == "TSK_STACK_CANARY") print $$3;}' \
				$(objtree)/include/generated/asm-offsets.h))
endif

ifeq ($(CONFIG_ARM64_BTI_KERNEL),y)
  KBUILD_CFLAGS += -mbranch-protection=pac-ret+bti
  KBUILD_RUSTFLAGS += -Zbranch-protection=bti,pac-ret
else ifeq ($(CONFIG_ARM64_PTR_AUTH_KERNEL),y)
  KBUILD_RUSTFLAGS += -Zbranch-protection=pac-ret
  ifeq ($(CONFIG_CC_HAS_BRANCH_PROT_PAC_RET),y)
    KBUILD_CFLAGS += -mbranch-protection=pac-ret
  else
    KBUILD_CFLAGS += -msign-return-address=non-leaf
  endif
else
  KBUILD_CFLAGS += $(call cc-option,-mbranch-protection=none)
endif

# Tell the assembler to support instructions from the latest target
# architecture.
#
# For non-integrated assemblers we'll pass this on the command line, and for
# integrated assemblers we'll define ARM64_ASM_ARCH and ARM64_ASM_PREAMBLE for
# inline usage.
#
# We cannot pass the same arch flag to the compiler as this would allow it to
# freely generate instructions which are not supported by earlier architecture
# versions, which would prevent a single kernel image from working on earlier
# hardware.
ifeq ($(CONFIG_AS_HAS_ARMV8_5), y)
  asm-arch := armv8.5-a
else
  asm-arch := armv8.4-a
endif

ifdef asm-arch
KBUILD_CFLAGS	+= -Wa,-march=$(asm-arch) \
		   -DARM64_ASM_ARCH='"$(asm-arch)"'
endif

ifeq ($(CONFIG_SHADOW_CALL_STACK), y)
KBUILD_CFLAGS	+= -ffixed-x18
KBUILD_RUSTFLAGS += -Zfixed-x18
endif

ifeq ($(CONFIG_CPU_BIG_ENDIAN), y)
KBUILD_CPPFLAGS	+= -mbig-endian
CHECKFLAGS	+= -D__AARCH64EB__
# Prefer the baremetal ELF build target, but not all toolchains include
# it so fall back to the standard linux version if needed.
KBUILD_LDFLAGS	+= -EB $(call ld-option, -maarch64elfb, -maarch64linuxb -z norelro)
UTS_MACHINE	:= aarch64_be
else
KBUILD_CPPFLAGS	+= -mlittle-endian
CHECKFLAGS	+= -D__AARCH64EL__
# Same as above, prefer ELF but fall back to linux target if needed.
KBUILD_LDFLAGS	+= -EL $(call ld-option, -maarch64elf, -maarch64linux -z norelro)
UTS_MACHINE	:= aarch64
endif

ifeq ($(CONFIG_LD_IS_LLD), y)
KBUILD_LDFLAGS	+= -z norelro
endif

CHECKFLAGS	+= -D__aarch64__

ifeq ($(CONFIG_DYNAMIC_FTRACE_WITH_CALL_OPS),y)
  KBUILD_CPPFLAGS += -DCC_USING_PATCHABLE_FUNCTION_ENTRY
  CC_FLAGS_FTRACE := -fpatchable-function-entry=4,2
else ifeq ($(CONFIG_DYNAMIC_FTRACE_WITH_ARGS),y)
  KBUILD_CPPFLAGS += -DCC_USING_PATCHABLE_FUNCTION_ENTRY
  CC_FLAGS_FTRACE := -fpatchable-function-entry=2
endif

ifeq ($(CONFIG_KASAN_SW_TAGS), y)
KASAN_SHADOW_SCALE_SHIFT := 4
else ifeq ($(CONFIG_KASAN_GENERIC), y)
KASAN_SHADOW_SCALE_SHIFT := 3
endif

KBUILD_CFLAGS += -DKASAN_SHADOW_SCALE_SHIFT=$(KASAN_SHADOW_SCALE_SHIFT)
KBUILD_CPPFLAGS += -DKASAN_SHADOW_SCALE_SHIFT=$(KASAN_SHADOW_SCALE_SHIFT)
KBUILD_AFLAGS += -DKASAN_SHADOW_SCALE_SHIFT=$(KASAN_SHADOW_SCALE_SHIFT)

endif # CONFIG_L4_ARCH_ARM64

# -----------------------------------------------

L4OBJ := $(patsubst "%",%,$(CONFIG_L4_OBJ_TREE))
L4DIR := $(shell readlink $(L4OBJ)/source)
export L4OBJ
export L4DIR

ifneq ($(config-targets),1)
# Extra Libs
L4MODS-$(CONFIG_L4_VCPU)         += libvcpu
L4MODS-$(CONFIG_L4_USE_L4SHMC)   += shmc
L4MODS-$(CONFIG_L4_BLK_DRV)      += libfdx
L4MODS-$(CONFIG_L4_EXTERNAL_RTC) += rtc
L4MODS-$(CONFIG_L4_NET_ANKH_DRV) += ankh
L4MODS-$(CONFIG_L4_TCG_STPM)     += stpm-client
endif

L4INC		= -I$(L4OBJ)/include/$(L4_MK_ARCH) \
		  -I$(L4OBJ)/include

head-y := arch/l4/kernel/head.o

libs-y				+= arch/l4/lib/arch-$(L4X_SRCARCH)/	\
				   arch/l4/l4lxlib/generic/		\
				   arch/l4/l4lxlib/$(BASEENV)/
drivers-$(CONFIG_L4_SERVER)	+= arch/l4/server/

drivers-y			+= arch/l4/drivers/
drivers-$(CONFIG_PCI)		+= arch/l4/pci/

ifeq ($(L4X_SRCARCH),x86)
drivers-$(CONFIG_PM)		+= arch/l4/power/arch-x86/
drivers-$(CONFIG_VIDEO)	+= arch/x86/video/
core-$(CONFIG_KVM)		+= arch/l4/kvm/arch-x86/
endif

ifeq ($(L4X_SRCARCH),arm)
core-y				+= arch/l4/common/arch-arm/
core-y				+= arch/l4/arm-mach-l4/
core-y				+= arch/arm/probes/
endif

L4_DEFS_MK_INC			= $(L4OBJ)/l4defs-nofpu.mk.inc

-include $(L4_DEFS_MK_INC)

L4_REQUIRED_MODS		:= libc_be_minimal_log_io \
                                   libc_minimal libc_minimal_l4re \
				   libsupc++_minimal libumalloc log l4re_c-util \
				   compiler-rt \
                                   $(if $(CONFIG_L4_LINK_LIBIO_DIRECT),libio-direct,libio) \
				   $(L4MODS-y)

L4_EXT_CFLAGS			:= $(call L4_BID_PKG_CONFIG_CALL,$(L4OBJ),--cflags --define-variable=libc_variant=libc,$(L4_REQUIRED_MODS))
ifneq ($(call L4_BID_PKG_CONFIG_FAILED,$(L4_EXT_CFLAGS)),)
$(info  Getting required compilation flags failed.)
$(info  L4OBJ: $(L4OBJ))
$(info  L4_REQUIRED_MODS: $(L4_REQUIRED_MODS))
$(error Aborting.)
endif

libs-y += $(L4OBJ)/lib/$(L4_SYSTEM)/$(L4_VARIANT)/l4f/libl4sys-direct.a \
          $(L4OBJ)/lib/$(L4_SYSTEM)/$(L4_VARIANT)/l4f/libgcc.a

export L4INC L4_REQUIRED_MODS

LDFLAGS_vmlinux  =  \
  --defsym __l4sys_invoke_direct=$(L4_BID_KIP_ADDR)+$(L4_BID_KIP_OFFS_SYS_INVOKE)

# mix-in some more include paths
LINUXINCLUDE := $(subst -I$(srctree)/arch/l4/include, \
                        -I$(srctree)/arch/l4/include \
			  -I$(objtree)/arch/l4/include/asm/l4-arch \
                          -I$(srctree)/arch/$(LINSRCARCH)/include, \
			$(LINUXINCLUDE))
                        
LINUXINCLUDE := $(subst -I$(srctree)/arch/$(SRCARCH)/include/uapi, \
                        -I$(srctree)/arch/$(LINSRCARCH)/include/uapi, \
                        $(LINUXINCLUDE))

KBUILD_CPPFLAGS += $(if $(CONFIG_L4_ARCH_ARM),-I$(srctree)/arch/l4/arm-mach-l4/include) \
                   -I$(objtree)/arch/l4/include/asm/orig-arch \
                   -I$(objtree)/arch/l4/include \
                   $(L4INC) $(L4_EXT_CFLAGS) \
                   $(CPPFLAGS_SYSENTER-y) \
                   $(L4BID_CPPFLAGS_SYSCALLS-y)

# for DICE
KBUILD_CPPFLAGS	+= -DL4API_$(L4_MK_API)

KBUILD_CPPFLAGS += -DL4SYS_USE_UTCB_WRAP=1

# avoid inclusion of static_assert from l4/sys/compiler.h
# and since L4Linux is C only...
KBUILD_CFLAGS += -D__GXX_EXPERIMENTAL_CXX0X__

# L4Linux uses TEXT_OFFSET in memory.h, thus have it defined everywhere
ifneq ($(CONFIG_L4_ARCH_ARM),)
KBUILD_CPPFLAGS += -DTEXT_OFFSET=0x3000000
endif

all: lImage

boot := arch/l4/boot

lImage: check_for_l4defs vmlinux
	$(Q)$(MAKE) $(build)=$(boot) $(boot)/$@

check_for_l4defs:
	$(Q)if [ ! -e $(L4_DEFS_MK_INC) ]; then \
	  echo "No $(L4_DEFS_MK_INC) available."; \
	  echo "Did you run 'make' in $(L4OBJ) to a successful end?"; \
	  exit 1; \
	fi

PHONY += check_for_l4defs

mtags:
	(cd $(srctree) && $(MAKE) tags > /dev/null 2>&1; \
	sort tags > .tags.sorting && mv .tags.sorting tags )

arch/l4/include/asm/l4-arch/asm:
	@echo '  LN      asm-l4/l4-arch/asm symlink'
	$(Q)mkdir -p $(@D)
	$(Q)ln -s $(if $(building_out_of_srctree),../../../../../source/arch/l4/include/asm,..)/arch-$(L4X_SRCARCH) $@

arch/l4/include/asm/arch:
	@echo '  LN      asm-l4/arch-$(L4X_SRCARCH)/arch -> asm/arch symlink'
	$(Q)mkdir -p $(@D)
	$(Q)ln -s $(if $(building_out_of_srctree),../../../../source/arch/l4/include/asm/)arch-$(L4X_SRCARCH)/arch $@

arch/l4/include/asm/api:
	@echo '  LN      asm-l4/api-$(BASEENV) -> asm/api symlink'
	$(Q)mkdir -p $(@D)
	$(Q)ln -s $(if $(building_out_of_srctree),../../../../source/arch/l4/include/asm/)api-$(BASEENV) $@

arch/l4/include/asm/l4x:
	@echo '  LN      asm-l4/l4x-$(L4X_SRCARCH) -> asm/l4x symlink'
	$(Q)mkdir -p $(@D)
	$(Q)ln -s $(if $(building_out_of_srctree),../../../../source/arch/l4/include/asm/)l4x-$(L4X_SRCARCH) $@

arch/l4/include/asm/l4lxapi/impl:
	@echo '  LN      asm-l4/l4lxapi/impl symlink'
	$(Q)mkdir -p $(@D)
	$(Q)ln -s $(if $(building_out_of_srctree),../../../../../source/arch/l4,../../..)/l4lxlib/l4env $@

arch/l4/include/asm/orig-arch/asm-$(LINSRCARCH):
	@echo '  LN      asm-l4/orig-arch/asm-$(LINSRCARCH) symlink'
	$(Q)mkdir -p $(@D)
	$(Q)ln -s $(if $(building_out_of_srctree),../../../../../source/arch,../../../..)/$(LINSRCARCH)/include/asm $@

symlink_files      = arch/l4/include/asm/api \
                     arch/l4/include/asm/l4lxapi/impl \
		     arch/l4/include/asm/l4x \
		     arch/l4/include/asm/l4-arch/asm

symlink_dirs       = arch/l4/include/asm/orig-arch/asm-$(LINSRCARCH)

MRPROPER_FILES	  += $(symlink_files) \
		     $(wildcard vmlinuz*) $(wildcard l4linux*) ldrImage source
MRPROPER_DIRS     += $(symlink_dirs)

archprepare: checkbin print_instructions check_linux_config $(symlink_files) $(symlink_dirs) \
             archprepare_l4x

archprepare_l4x:
ifeq ($(L4X_SRCARCH),arm)
	$(Q)$(MAKE) $(build)=arch/arm/tools kapi
	$(Q)cp $(srctree)/arch/arm/tools/mach-types .tmp.mach-types.list
	$(Q)echo "l4	MACH_L4	L4	9999" >> .tmp.mach-types.list
	$(Q)$(AWK) -f $(srctree)/arch/arm/tools/gen-mach-types .tmp.mach-types.list > .tmp.mach-types.h || { rm -f $@; /bin/false; }
	$(Q)cmp -s .tmp.mach-types.h arch/l4/include/generated/asm/mach-types.h || mv .tmp.mach-types.h arch/l4/include/generated/asm/mach-types.h
	$(Q)rm -f .tmp.mach-types.list .tmp.mach-types.h
endif
ifeq ($(L4X_SRCARCH),arm64)
	$(Q)$(MAKE) $(build)=arch/arm64/tools kapi
endif # ARM64
	$(Q)mkdir -p arch/$(L4X_SRCARCH)/lib

checkbin:
ifdef CONFIG_MITIGATION_RETPOLINE
ifeq ($(RETPOLINE_CFLAGS),)
	@echo "You are building kernel with non-retpoline compiler." >&2
	@echo "Please update your compiler." >&2
	@false
endif
endif

ifdef CONFIG_UNWINDER_ORC
orc_hash_h := arch/$(SRCARCH)/include/generated/asm/orc_hash.h
orc_hash_sh := $(srctree)/scripts/orc_hash.sh
targets += $(orc_hash_h)
quiet_cmd_orc_hash = GEN     $@
      cmd_orc_hash = mkdir -p $(dir $@); \
		     $(CONFIG_SHELL) $(orc_hash_sh) < $< > $@
$(orc_hash_h): $(srctree)/arch/x86/include/asm/orc_types.h $(orc_hash_sh) FORCE
	$(call if_changed,orc_hash)
archprepare: $(orc_hash_h)
endif

archclean:
	$(Q)rm -rf $(objtree)/arch/i386
	$(Q)rm -rf $(objtree)/arch/x86_64

ifeq ($(L4X_SRCARCH),x86)

archscripts: scripts_basic
	$(Q)$(MAKE) $(build)=arch/x86/tools relocs vdso2c

###
# Syscall table generation

archheaders:
	$(Q)$(MAKE) $(build)=arch/x86/entry/syscalls all

###
# <asm/cpufeaturemasks.h> header generation

cpufeaturemasks.hdr := arch/l4/include/generated/asm/cpufeaturemasks.h
cpufeaturemasks.awk := $(srctree)/arch/x86/tools/cpufeaturemasks.awk
cpufeatures_hdr := $(srctree)/arch/x86/include/asm/cpufeatures.h
targets += $(cpufeaturemasks.hdr)
      filechk_gen_featuremasks = $(AWK) -f $(cpufeaturemasks.awk) $(cpufeatures_hdr) $(KCONFIG_CONFIG)

$(cpufeaturemasks.hdr): $(cpufeaturemasks.awk) $(cpufeatures_hdr) $(KCONFIG_CONFIG) FORCE
	$(shell mkdir -p $(dir $@))
	$(call filechk,gen_featuremasks)
archprepare: $(cpufeaturemasks.hdr)

endif #x86

ifeq ($(L4X_SRCARCH),arm)
archheaders:
	$(Q)$(MAKE) $(build)=arch/arm/tools uapi

vdso-install-$(CONFIG_VDSO) += arch/arm/vdso/vdso.so.dbg

endif

chkl4conf:
	@if [ -d $(L4OBJ)/pkg/l4re-core/l4sys ]; then                        \
		echo "Configuration looks Ok.";                              \
	else                                                                 \
		echo \"$(L4OBJ)\" does not look like an L4 build directory!; \
	fi

## Check a few options that should not be enabled
# regparm: gcc may call memset from our libs, so obey calling convention
#          this should probably be fixed

# M486 M586 M586TSC M586MMX: cmpxchg64 emulation uses cli

DIS_OPTS-x86    := CPU_SUP_INTEL CPU_SUP_AMD CPU_SUP_HYGON \
                   CPU_SUP_CENTAUR CPU_SUP_ZHAOXIN
DIS_OPTS-x86-32 := HPET_TIMER X86_X32 $(DIS_OPTS-x86)
DIS_OPTS-x86-64 := PAGE_TABLE_ISOLATION $(DIS_OPTS-x86)
DIS_OPTS-arm-32 := PERF_EVENTS ARM_PATCH_PHYS_VIRT HIGHMEM \
                   HARDEN_BRANCH_PREDICTOR
DIS_OPTS-arm-64 := ARM64_PTR_AUTH
DIS_OPTS := HIGHMEM4G HIGHMEM64G PCI_GOANY PCI_GOBIOS \
            X86_MCE MTRR PARAVIRT KEXEC MCA SOFTWARE_SUSPEND APM \
	    VM86 X86_DS X86_PTRACE_BTS HT_IRQ DMI \
	    TRANSPARENT_HUGEPAGE COMPACTION X86_PAE ACPI X86_SMAP \
	    M486 M586 M586TSC M586MMX PNP \
	    X86_16BIT MICROCODE RSEQ \
	    $(DIS_OPTS-$(L4X_SRCARCH)-$(BITS))
ENA_OPTS := $(ENA_OPTS-$(L4X_SRCARCH))

print_instructions:
	@if [ ! -e vmlinuz$(VMLINUZ_SUFFIX) ]; then \
	  echo ===============================================; \
	  echo "  For quick build instructions, please visit:"; \
	  echo "    http://wiki.tudos.org/Quickstart"; \
	  echo "    http://l4linux.org/build.shtml"; \
	  echo ===============================================; \
	fi

check_linux_config:
	@$(foreach i,$(addprefix CONFIG_,$(DIS_OPTS)),     \
	  if [ "$($(i))" = "y" ]; then                     \
	    echo; echo "ERROR: $(i) must not be enabled."; \
	    failed=1;                                      \
	  fi;)                                             \
	  if [ -n "$$failed" ]; then                       \
	    echo; exit 1;                                  \
	  fi
	@$(foreach i,$(addprefix CONFIG_,$(ENA_OPTS)),     \
	  if [ "$($(i))" != "y" ]; then                    \
	    echo; echo "ERROR: $(i) must be enabled.";     \
	    failed=1;                                      \
	  fi;)                                             \
	  if [ -n "$$failed" ]; then                       \
	    echo; exit 1;                                  \
	  fi
	@if [ "$(CONFIG_X86_32)" = "y" -a "$(CONFIG_PAGE_OFFSET)" != "0" ]; then \
	  echo "CONFIG_PAGE_OFFSET must be 0."; \
	  exit 1; \
	fi

PHONY += chkl4conf check_linux_config

ifneq ($(CONFIG_L4_ARCH_ARM64),)

ifeq ($(KBUILD_EXTMOD),)
# We need to generate vdso-offsets.h before compiling certain files in kernel/.
# In order to do that, we should use the archprepare target, but we can't since
# asm-offsets.h is included in some files used to generate vdso-offsets.h, and
# asm-offsets.h is built in prepare0, for which archprepare is a dependency.
# Therefore we need to generate the header after prepare0 has been made, hence
# this hack.
prepare: vdso_prepare
vdso_prepare: prepare0
	$(Q)$(MAKE) $(build)=arch/arm64/kernel/vdso \
	include/generated/vdso-offsets.h arch/arm64/kernel/vdso/vdso.so
ifdef CONFIG_COMPAT_VDSO
	$(Q)$(MAKE) $(build)=arch/arm64/kernel/vdso32 \
	arch/arm64/kernel/vdso32/vdso.so
endif
endif

vdso-install-y				+= arch/arm64/kernel/vdso/vdso.so.dbg
vdso-install-$(CONFIG_COMPAT_VDSO)	+= arch/arm64/kernel/vdso32/vdso32.so.dbg
endif # ARM64


define archhelp
  echo  '* vmlinuz	 - Compressed kernel image'
  echo  '  chkl4conf     - Check L4 specific configuration'
endef
