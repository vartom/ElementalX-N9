/*
 * Copyright (C) 2013 NVIDIA Corporation
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/export.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <mach/tegra_usb_pad_ctrl.h>

#include "iomap.h"

static DEFINE_SPINLOCK(utmip_pad_lock);
static int utmip_pad_count;
static struct clk *utmi_pad_clk;

int utmi_phy_pad_enable(void)
{
	unsigned long val, flags;
	void __iomem *pad_base =  IO_ADDRESS(TEGRA_USB_BASE);
	void __iomem *clk_base = IO_ADDRESS(TEGRA_CLK_RESET_BASE);

	if (!utmi_pad_clk)
		utmi_pad_clk = clk_get_sys("utmip-pad", NULL);

	val = readl(clk_base + UTMIPLL_HW_PWRDN_CFG0);
	val &= ~UTMIPLL_HW_PWRDN_CFG0_IDDQ_OVERRIDE;
	writel(val, clk_base + UTMIPLL_HW_PWRDN_CFG0);

	clk_enable(utmi_pad_clk);

	spin_lock_irqsave(&utmip_pad_lock, flags);
	utmip_pad_count++;

	val = readl(pad_base + UTMIP_BIAS_CFG0);
	val &= ~(UTMIP_OTGPD | UTMIP_BIASPD);
	val |= UTMIP_HSSQUELCH_LEVEL(0x2) | UTMIP_HSDISCON_LEVEL(0x1) |
		UTMIP_HSDISCON_LEVEL_MSB;
	writel(val, pad_base + UTMIP_BIAS_CFG0);

	spin_unlock_irqrestore(&utmip_pad_lock, flags);

	clk_disable(utmi_pad_clk);

	return 0;
}
EXPORT_SYMBOL_GPL(utmi_phy_pad_enable);

int utmi_phy_pad_disable(void)
{
	unsigned long val, flags;
	void __iomem *pad_base =  IO_ADDRESS(TEGRA_USB_BASE);
	void __iomem *clk_base = IO_ADDRESS(TEGRA_CLK_RESET_BASE);

	if (!utmi_pad_clk)
		utmi_pad_clk = clk_get_sys("utmip-pad", NULL);

	clk_enable(utmi_pad_clk);
	spin_lock_irqsave(&utmip_pad_lock, flags);

	if (!utmip_pad_count) {
		pr_err("%s: utmip pad already powered off\n", __func__);
		goto out;
	}
	if (--utmip_pad_count == 0) {
		val = readl(pad_base + UTMIP_BIAS_CFG0);
		val |= UTMIP_OTGPD | UTMIP_BIASPD;
		val &= ~(UTMIP_HSSQUELCH_LEVEL(~0) | UTMIP_HSDISCON_LEVEL(~0) |
			UTMIP_HSDISCON_LEVEL_MSB);
		writel(val, pad_base + UTMIP_BIAS_CFG0);

		val = readl(clk_base + UTMIPLL_HW_PWRDN_CFG0);
		val |= UTMIPLL_HW_PWRDN_CFG0_IDDQ_OVERRIDE;
		writel(val, clk_base + UTMIPLL_HW_PWRDN_CFG0);
	}
out:
	spin_unlock_irqrestore(&utmip_pad_lock, flags);
	clk_disable(utmi_pad_clk);

	return 0;
}
EXPORT_SYMBOL_GPL(utmi_phy_pad_disable);