/*
  *     Deep sleep testing mode.
  *
  *     Download small piece of code into DSP and make it sleep for ever.
  *
  */


#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/initrd.h>
#include <linux/android_pmem.h>

#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/flash.h>

#include <mach/board.h>
#include <mach/hardware.h>

#include <asm/io.h>
#include <asm/delay.h>

#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <mach/gpio.h>
#include <mach/adi_hal_internal.h>
#include <mach/regs_ana.h>
#include <mach/regs_cpc.h>

#include <linux/clk.h>
#include <mach/clock_common.h>
#include <mach/clock_sc8800g.h>
#include <mach/mfp.h>

int sprd_3rdparty_gpio_wifi_power;
int sprd_3rdparty_gpio_wifi_reset;
int sprd_3rdparty_gpio_wifi_pwd;
int sprd_3rdparty_gpio_wifi_wake;
int sprd_3rdparty_gpio_wifi_irq;
int sprd_3rdparty_gpio_bt_power;
int sprd_3rdparty_gpio_bt_reset;
int sprd_3rdparty_gpio_bt_rts;
int sprd_3rdparty_gpio_tp_rst;
int sprd_3rdparty_gpio_tp_irq;
int sprd_3rdparty_gpio_cmmb_power;
int sprd_3rdparty_gpio_cmmb_reset;
int sprd_3rdparty_gpio_cmmb_irq;


static struct gpio_initdata __initdata gpio_func_cfg[] = {	
	{&sprd_3rdparty_gpio_wifi_power,	160	|GPIO_DEFAUT_LOW	|GPIO_DIRECTION_OUTPUT	|GPIO_LOGIC_TRUE},
	{&sprd_3rdparty_gpio_wifi_reset,	140	|GPIO_DEFAUT_HIGH	|GPIO_DIRECTION_OUTPUT	|GPIO_LOGIC_TRUE},
	{&sprd_3rdparty_gpio_wifi_pwd,		99	|GPIO_DEFAUT_HIGH	|GPIO_DIRECTION_OUTPUT	|GPIO_LOGIC_TRUE},
	{&sprd_3rdparty_gpio_wifi_wake,		139	|GPIO_DEFAUT_HIGH	|GPIO_DIRECTION_OUTPUT	|GPIO_LOGIC_TRUE},
	{&sprd_3rdparty_gpio_wifi_irq,		141	|GPIO_DEFAUT_LOW	|GPIO_DIRECTION_OUTPUT	|GPIO_LOGIC_TRUE},
	{&sprd_3rdparty_gpio_bt_power,		-1	|GPIO_DEFAUT_LOW	|GPIO_DIRECTION_OUTPUT	|GPIO_LOGIC_TRUE},
	{&sprd_3rdparty_gpio_bt_reset,		90	|GPIO_DEFAUT_HIGH	|GPIO_DIRECTION_OUTPUT	|GPIO_LOGIC_TRUE},
	{&sprd_3rdparty_gpio_bt_rts,		42	|GPIO_DEFAUT_LOW	|GPIO_DIRECTION_OUTPUT	|GPIO_LOGIC_TRUE},
	{&sprd_3rdparty_gpio_tp_rst,		59	|GPIO_DEFAUT_LOW	|GPIO_DIRECTION_OUTPUT	|GPIO_LOGIC_TRUE},
	{&sprd_3rdparty_gpio_tp_irq,		60	|GPIO_DEFAUT_LOW	|GPIO_DIRECTION_INPUT	|GPIO_LOGIC_TRUE},
	{&sprd_3rdparty_gpio_cmmb_power,	135	|GPIO_DEFAUT_LOW	|GPIO_DIRECTION_OUTPUT	|GPIO_LOGIC_TRUE},
	{&sprd_3rdparty_gpio_cmmb_reset,	94	|GPIO_DEFAUT_LOW	|GPIO_DIRECTION_OUTPUT	|GPIO_LOGIC_TRUE},
	{&sprd_3rdparty_gpio_tp_irq,		93	|GPIO_DEFAUT_LOW	|GPIO_DIRECTION_INPUT	|GPIO_LOGIC_TRUE},
};

EXPORT_SYMBOL_GPL(sprd_3rdparty_gpio_wifi_power);
EXPORT_SYMBOL_GPL(sprd_3rdparty_gpio_wifi_reset);
EXPORT_SYMBOL_GPL(sprd_3rdparty_gpio_wifi_pwd);
EXPORT_SYMBOL_GPL(sprd_3rdparty_gpio_wifi_wake);
EXPORT_SYMBOL_GPL(sprd_3rdparty_gpio_wifi_irq);
EXPORT_SYMBOL_GPL(sprd_3rdparty_gpio_bt_power);
EXPORT_SYMBOL_GPL(sprd_3rdparty_gpio_bt_reset);
EXPORT_SYMBOL_GPL(sprd_3rdparty_gpio_bt_rts);
EXPORT_SYMBOL_GPL(sprd_3rdparty_gpio_tp_rst);
EXPORT_SYMBOL_GPL(sprd_3rdparty_gpio_tp_irq);
EXPORT_SYMBOL_GPL(sprd_3rdparty_gpio_cmmb_power);
EXPORT_SYMBOL_GPL(sprd_3rdparty_gpio_cmmb_reset);
EXPORT_SYMBOL_GPL(sprd_3rdparty_gpio_cmmb_irq);

void __init get_gpio_cfg(struct gpio_initdata** desc, int* size)
{
    *desc = gpio_func_cfg;
    *size = ARRAY_SIZE(gpio_func_cfg);
}

