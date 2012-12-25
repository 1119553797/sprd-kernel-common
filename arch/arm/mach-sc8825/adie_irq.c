/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
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
#include <linux/module.h>
#include <linux/init.h>
#include <linux/irqflags.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <asm/delay.h>

#include <mach/hardware.h>
#include <mach/globalregs.h>
#include <mach/adi.h>
#include <mach/irqs.h>

#ifdef CONFIG_NKERNEL
#include <nk/nkern.h>
#define CONFIG_NKERNEL_NO_SHARED_IRQ
#endif

#ifndef CONFIG_NKERNEL

/* Analog Die interrupt registers */
#define ANA_CTL_INT_BASE		( SPRD_MISC_BASE + 0x380 )

/* registers definitions for controller ANA_CTL_INT */
#define ANA_REG_INT_MASK_STATUS         (ANA_CTL_INT_BASE + 0x0000)
#define ANA_REG_INT_RAW_STATUS          (ANA_CTL_INT_BASE + 0x0004)
#define ANA_REG_INT_EN                  (ANA_CTL_INT_BASE + 0x0008)
#define ANA_REG_INT_MASK_STATUS_SYNC    (ANA_CTL_INT_BASE + 0x000c)

/* bits definitions for register REG_INT_MASK_STATUS */
#define BIT_ANA_CHGRWDG_INT             ( BIT(6) )
#define BIT_ANA_EIC_INT                 ( BIT(5) )
#define BIT_ANA_TPC_INT                 ( BIT(4) )
#define BIT_ANA_WDG_INT                 ( BIT(3) )
#define BIT_ANA_RTC_INT                 ( BIT(2) )
#define BIT_ANA_GPIO_INT                ( BIT(1) )
#define BIT_ANA_ADC_INT                 ( BIT(0) )

/* vars definitions for controller ANA_CTL_INT */
#define MASK_ANA_INT                    ( 0x7F )

void sprd_ack_ana_irq(struct irq_data *data)
{
	/* nothing to do... */
}

static void sprd_mask_ana_irq(struct irq_data *data)
{
	int offset = data->irq - IRQ_ANA_INT_START;
	pr_debug("%s %d\n", __FUNCTION__, data->irq);
	sci_adi_write(ANA_REG_INT_EN, 0, BIT(offset) & MASK_ANA_INT);
}

static void sprd_unmask_ana_irq(struct irq_data *data)
{
	int offset = data->irq - IRQ_ANA_INT_START;
	pr_debug("%s %d\n", __FUNCTION__, data->irq);
	sci_adi_write(ANA_REG_INT_EN, BIT(offset) & MASK_ANA_INT, 0);
}

static struct irq_chip sprd_muxed_ana_chip = {
	.name = "irq-ANA",
	.irq_ack = sprd_ack_ana_irq,
	.irq_mask = sprd_mask_ana_irq,
	.irq_unmask = sprd_unmask_ana_irq,
};

static void sprd_muxed_ana_handler(unsigned int irq, struct irq_desc *desc)
{
	uint32_t irq_ana, status;
	int i;

	status = sci_adi_read(ANA_REG_INT_MASK_STATUS) & MASK_ANA_INT;
	pr_debug("%s %d 0x%08x\n", __FUNCTION__, irq, status);
	while (status) {
		i = __ffs(status);
		status &= ~(1 << i);
		irq_ana = IRQ_ANA_INT_START + i;
		pr_debug("%s generic_handle_irq %d\n", __FUNCTION__, irq_ana);
		generic_handle_irq(irq_ana);
	}
}

void __init ana_init_irq(void)
{
	int n;

	irq_set_chained_handler(IRQ_ANA_INT, sprd_muxed_ana_handler);
	for (n = IRQ_ANA_INT_START; n < IRQ_ANA_INT_START + NR_ANA_IRQS; n++) {
		irq_set_chip_and_handler(n, &sprd_muxed_ana_chip,
					 handle_level_irq);
		set_irq_flags(n, IRQF_VALID);
	}
}

#else /* CONFIG_NKERNEL */

extern NkDevXPic *nkxpic;	/* virtual XPIC device */
extern NkOsId nkxpic_owner;	/* owner of the virtual XPIC device */
extern NkOsMask nkosmask;	/* my OS mask */

extern void __nk_xirq_startup(struct irq_data *d);
extern void __nk_xirq_shutdown(struct irq_data *d);

static unsigned int nk_startup_irq(struct irq_data *data)
{
	__nk_xirq_startup(data);
#ifdef CONFIG_NKERNEL_NO_SHARED_IRQ
	nkxpic->irq[data->irq].os_enabled = nkosmask;
#else
	nkxpic->irq[data->irq].os_enabled |= nkosmask;
#endif
	nkops.nk_xirq_trigger(nkxpic->xirq, nkxpic_owner);

	return 0;
}

static void nk_shutdown_irq(struct irq_data *data)
{
	__nk_xirq_shutdown(data);
#ifdef CONFIG_NKERNEL_NO_SHARED_IRQ
	nkxpic->irq[data->irq].os_enabled = 0;
#else
	nkxpic->irq[irq].os_enabled &= ~nkosmask;
#endif
	nkops.nk_xirq_trigger(nkxpic->xirq, nkxpic_owner);
}

static void nk_sprd_ack_ana_irq(struct irq_data *data)
{
	/* nothing to do... */
}

static void nk_sprd_mask_ana_irq(struct irq_data *data)
{
	/* nothing to do... */
}

static void nk_sprd_unmask_ana_irq(struct irq_data *data)
{
	nkops.nk_xirq_trigger(data->irq, nkxpic_owner);
}

static struct irq_chip nk_sprd_muxed_ana_chip = {
	.name = "irq-ANA",
	.irq_ack = nk_sprd_ack_ana_irq,
	.irq_mask = nk_sprd_mask_ana_irq,
	.irq_unmask = nk_sprd_unmask_ana_irq,
	.irq_startup = nk_startup_irq,
	.irq_shutdown = nk_shutdown_irq,
};

void __init ana_init_irq(void)
{
	int n;

	for (n = IRQ_ANA_ADC_INT; n < IRQ_ANA_ADC_INT + NR_ANA_IRQS; ++n) {
		irq_set_chip_and_handler(n, &nk_sprd_muxed_ana_chip,
					 handle_level_irq);
		set_irq_flags(n, IRQF_VALID);
	}
}

#endif /* CONFIG_NKERNEL */
