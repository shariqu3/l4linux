# SPDX-License-Identifier: GPL-2.0

b := ../../../x86/kvm

ccflags-y += -I $(srctree)/arch/x86/kvm
ccflags-$(CONFIG_KVM_WERROR) += -Werror

KVM := ../../../../virt/kvm
include $(srctree)/virt/kvm/Makefile.kvm

kvm-y			+= x86.o $(b)/emulate.o $(b)/irq.o $(b)/lapic.o $(b)/cpuid.o $(b)/pmu.o $(b)/mtrr.o \
			   $(b)/debugfs.o mmu/mmu.o $(b)/mmu/page_track.o $(b)/mmu/spte.o

kvm-$(CONFIG_X86_64) += $(b)/mmu/tdp_iter.o $(b)/mmu/tdp_mmu.o
kvm-$(CONFIG_KVM_IOAPIC) += $(b)/i8259.o $(b)/i8254.o $(b)/ioapic.o
kvm-$(CONFIG_KVM_HYPERV) += $(b)/hyperv.o
kvm-$(CONFIG_KVM_XEN)	+= $(b)/xen.o
kvm-$(CONFIG_KVM_SMM)	+= $(b)/smm.o

kvm-intel-y		+= vmx/vmx.o vmx/vmenter.o $(b)/vmx/pmu_intel.o $(b)/vmx/vmcs12.o \
			   vmx/nested.o $(b)/vmx/posted_intr.o $(b)/vmx/main.o

kvm-intel-$(CONFIG_X86_SGX_KVM)	+= $(b)/vmx/sgx.o
kvm-intel-$(CONFIG_KVM_HYPERV)	+= $(b)/vmx/hyperv.o $(b)/vmx/hyperv_evmcs.o
kvm-intel-$(CONFIG_KVM_INTEL_TDX)	+= $(b)/vmx/tdx.o

kvm-amd-y		+= svm/svm.o svm/vmenter.o $(b)/svm/pmu.o $(b)/svm/nested.o svm/avic.o

kvm-amd-$(CONFIG_KVM_AMD_SEV)	+= $(b)/svm/sev.o
kvm-amd-$(CONFIG_KVM_HYPERV)	+= $(b)/svm/hyperv.o

kvm-y			+= kvm-l4.o

ifdef CONFIG_HYPERV
kvm-y			+= kvm_onhyperv.o
kvm-intel-y		+= vmx/vmx_onhyperv.o vmx/hyperv_evmcs.o
kvm-amd-y		+= svm/svm_onhyperv.o
endif

obj-$(CONFIG_KVM_X86)	+= kvm.o
obj-$(CONFIG_KVM_INTEL)	+= kvm-intel.o
obj-$(CONFIG_KVM_AMD)	+= kvm-amd.o

AFLAGS_svm/vmenter.o    := -iquote $(obj)
$(obj)/svm/vmenter.o: $(obj)/kvm-asm-offsets.h

AFLAGS_vmx/vmenter.o    := -iquote $(obj)
$(obj)/vmx/vmenter.o: $(obj)/kvm-asm-offsets.h

$(obj)/kvm-asm-offsets.h: $(obj)/kvm-asm-offsets.s FORCE
	$(call filechk,offsets,__KVM_ASM_OFFSETS_H__)

targets += kvm-asm-offsets.s
clean-files += kvm-asm-offsets.h


# Fail the build if there is unexpected EXPORT_SYMBOL_GPL (or EXPORT_SYMBOL)
# usage.  All KVM-internal exports should use EXPORT_SYMBOL_FOR_KVM_INTERNAL.
# Only a handful of exports intended for other modules (VFIO, KVMGT) should
# use EXPORT_SYMBOL_GPL, and EXPORT_SYMBOL should never be used.
ifdef CONFIG_KVM_X86
# Search recursively for whole words and print line numbers.  Filter out the
# allowed set of exports, i.e. those that are intended for external usage.
exports_grep_trailer := --include='*.[ch]' -nrw $(srctree)/virt/kvm $(srctree)/arch/x86/kvm | \
			grep -v -e kvm_page_track_register_notifier \
				-e kvm_page_track_unregister_notifier \
				-e kvm_write_track_add_gfn \
				-e kvm_write_track_remove_gfn \
				-e kvm_get_kvm \
				-e kvm_get_kvm_safe \
				-e kvm_put_kvm

# Force grep to emit a goofy group separator that can in turn be replaced with
# the above newline macro (newlines in Make are a nightmare).  Note, grep only
# prints the group separator when N lines of context are requested via -C,
# a.k.a. --NUM.  Simply request zero lines.  Print the separator only after
# filtering out expected exports to avoid extra newlines in the error message.
define get_kvm_exports
$(shell grep "$(1)" -C0 $(exports_grep_trailer) | grep "$(1)" -C0 --group-separator="!SEP!")
endef

define check_kvm_exports
nr_kvm_exports := $(shell grep "$(1)" $(exports_grep_trailer) | wc -l)

ifneq (0,$$(nr_kvm_exports))
$$(error ERROR ***\
$$(newline)found $$(nr_kvm_exports) unwanted occurrences of $(1):\
$$(newline)  $(subst !SEP!,$$(newline) ,$(call get_kvm_exports,$(1)))\
$$(newline)in directories:\
$$(newline)  $(srctree)/arch/x86/kvm\
$$(newline)  $(srctree)/virt/kvm\
$$(newline)Use EXPORT_SYMBOL_FOR_KVM_INTERNAL, not $(1))
endif # nr_kvm_exports != 0
undefine nr_kvm_exports
endef # check_kvm_exports

$(eval $(call check_kvm_exports,EXPORT_SYMBOL_GPL))
$(eval $(call check_kvm_exports,EXPORT_SYMBOL))

undefine check_kvm_exports
undefine get_kvm_exports
undefine exports_grep_trailer
endif # CONFIG_KVM_X86
