# SPDX-License-Identifier: GPL-2.0
dtstree           = $(src)
dtb-$(CONFIG_OF) += $(patsubst $(dtstree)/%.dts,%.dtb, $(wildcard $(dtstree)/*.dts $(dtstree)/*/*.dts))
