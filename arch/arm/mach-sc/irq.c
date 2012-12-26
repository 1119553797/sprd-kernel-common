/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 * Author: steve.zhan <steve.zhan@spreadtrum.com>
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/io.h>

#include <asm/hardware/gic.h>

#include <mach/hardware.h>
#include <mach/irqs.h>

/* general interrupt registers */
#define	INTC0_REG(off)		(SPRD_INTC0_BASE + (off))
#define	INTCV0_IRQ_MSKSTS	INTC0_REG(0x0000)
#define	INTCV0_IRQ_RAW		INTC0_REG(0x0004)
#define	INTCV0_IRQ_EN		INTC0_REG(0x0008)
#define	INTCV0_IRQ_DIS		INTC0_REG(0x000C)
#define	INTCV0_IRQ_SOFT		INTC0_REG(0x0010)

#ifdef SPRD_INTC1_BASE
#define	INTC1_REG(off)		(SPRD_INTC1_BASE + (off))
#else
#define	INTC1_REG(off)		(SPRD_INTC0_BASE + 0x1000 + (off))
#endif

#define	INTCV1_IRQ_MSKSTS	INTC1_REG(0x0000)
#define	INTCV1_IRQ_RAW		INTC1_REG(0x0004)
#define	INTCV1_IRQ_EN		INTC1_REG(0x0008)
#define	INTCV1_IRQ_DIS		INTC1_REG(0x000C)
#define	INTCV1_IRQ_SOFT		INTC1_REG(0x0010)

#define INTC1_IRQ_NUM_MIN	(32)
#define INTC_NUM_MAX		(61)

static void sci_irq_eoi(struct irq_data *data)
{
	/* nothing to do... */
}

#ifdef CONFIG_PM
static int sci_set_wake(struct irq_data *d, unsigned int on)
{
	return 0;
}

#else
#define sci_set_wake	NULL
#endif

static void sci_irq_mask(struct irq_data *data)
{
	unsigned int irq = SCI_GET_INTC_IRQ(data->irq);
	if (irq <= INTC_NUM_MAX) {
		if (irq >= INTC1_IRQ_NUM_MIN) {
			__raw_writel(1 << (irq - INTC1_IRQ_NUM_MIN),
				     INTCV1_IRQ_DIS);
		} else {
			__raw_writel(1 << irq, INTCV0_IRQ_DIS);
		}
	}
}

static void sci_irq_unmask(struct irq_data *data)
{
	unsigned int irq = SCI_GET_INTC_IRQ(data->irq);
	if (irq <= INTC_NUM_MAX) {
		if (irq >= INTC1_IRQ_NUM_MIN) {
			__raw_writel(1 << (irq - INTC1_IRQ_NUM_MIN),
				     INTCV1_IRQ_EN);
		} else {
			__raw_writel(1 << irq, INTCV0_IRQ_EN);
		}
	}
}

void __init sci_init_irq(void)
{
#ifdef CONFIG_NKERNEL
	extern void nk_ddi_init(void);
	nk_ddi_init();
#endif
	gic_init(0, 29, (void __iomem *)CORE_GIC_DIS_VA,
		 (void __iomem *)CORE_GIC_CPU_VA);
	gic_arch_extn.irq_eoi = sci_irq_eoi;
	gic_arch_extn.irq_mask = sci_irq_mask;
	gic_arch_extn.irq_unmask = sci_irq_unmask;
	gic_arch_extn.irq_set_wake = sci_set_wake;
	ana_init_irq();
}

