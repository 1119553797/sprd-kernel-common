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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <asm/io.h>
#include <linux/file.h>
#include <mach/dma.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <mach/hardware.h>
#include <mach/board.h>
#include <linux/regulator/consumer.h>
#include <mach/regulator.h>

#if defined (CONFIG_ARCH_SC8825)
#include <mach/i2c-sprd.h>
#elif defined (CONFIG_ARCH_SC8810)
#include <mach/i2c-sc8810.h>
#elif defined (CONFIG_ARCH_SC8830)
#include <mach/i2c-sprd.h>
#endif

#include "sensor_drv_k.h"

/* FIXME: Move to camera device platform data later */
/*#if defined(CONFIG_ARCH_SC8825)*/

#if defined(CONFIG_ARCH_SC8830)

#define REGU_NAME_CAMAVDD	"vddcama"
#define REGU_NAME_CAMVIO	"vddcamio"
#define REGU_NAME_CAMDVDD	"vddcamd"
#define REGU_NAME_CAMMOT	"vddcammot"
#define SENSOR_CLK              "clk_sensor"

#else

#define REGU_NAME_CAMAVDD	"vddcama"
#define REGU_NAME_CAMVIO	"vddcamio"
#define REGU_NAME_CAMDVDD	"vddcamcore"
#define REGU_NAME_CAMMOT	"vddcammot"
#define SENSOR_CLK              "ccir_mclk"

#endif

/*#endif*/

#define DEBUG_SENSOR_DRV
#ifdef DEBUG_SENSOR_DRV
#define SENSOR_PRINT   pr_debug
#else
#define SENSOR_PRINT(...)
#endif
#define SENSOR_PRINT_ERR   printk
#define SENSOR_PRINT_HIGH  printk

#define SENSOR_K_SUCCESS 		0
#define SENSOR_K_FAIL 			(-1)
#define SENSOR_K_FALSE 			0
#define SENSOR_K_TRUE 			1

#define _pard(a) 				__raw_readl(a)

#define REG_RD(a)                                      __raw_readl(a)
#define REG_WR(a,v)                                    __raw_writel(v,a)
#define REG_AWR(a,v)                                   	__raw_writel((__raw_readl(a) & v), a)
#define REG_OWR(a,v)                                   __raw_writel((__raw_readl(a) | v), a)
#define REG_XWR(a,v)                                   __raw_writel((__raw_readl(a) ^ v), a)
#define REG_MWR(a,m,v)                                 \
	do {                                           \
		uint32_t _tmp = __raw_readl(a);        \
		_tmp &= ~(m);                          \
		__raw_writel(_tmp | ((m) & (v)), (a)); \
	}while(0)

#define BIT_0                                          0x01
#define BIT_1                                          0x02
#define BIT_2                                          0x04
#define BIT_3                                          0x08
#define BIT_4                                          0x10
#define BIT_5                                          0x20
#define BIT_6                                          0x40
#define BIT_7                                          0x80
#define BIT_8                                          0x0100
#define BIT_9                                          0x0200
#define BIT_10                                         0x0400
#define BIT_11                                         0x0800
#define BIT_12                                         0x1000
#define BIT_13                                         0x2000
#define BIT_14                                         0x4000
#define BIT_15                                         0x8000
#define BIT_16                                         0x010000
#define BIT_17                                         0x020000
#define BIT_18                                         0x040000
#define BIT_19                                         0x080000
#define BIT_20                                         0x100000
#define BIT_21                                         0x200000
#define BIT_22                                         0x400000
#define BIT_23                                         0x800000
#define BIT_24                                         0x01000000
#define BIT_25                                         0x02000000
#define BIT_26                                         0x04000000
#define BIT_27                                         0x08000000
#define BIT_28                                         0x10000000
#define BIT_29                                         0x20000000
#define BIT_30                                         0x40000000
#define BIT_31                                         0x80000000


//#define LOCAL 				static
#define LOCAL

#define PNULL  				((void *)0)

#define NUMBER_MAX                         0x7FFFFFF

#define SENSOR_MINOR 		MISC_DYNAMIC_MINOR
#define init_MUTEX(sem)    		sema_init(sem, 1)
#define SLEEP_MS(ms)			msleep(ms)

#define SENSOR_I2C_OP_TRY_NUM   		4
#define SENSOR_CMD_BITS_8   			1
#define SENSOR_CMD_BITS_16	   		2
#define SENSOR_I2C_VAL_8BIT			0x00
#define SENSOR_I2C_VAL_16BIT			0x01
#define SENSOR_I2C_REG_8BIT			(0x00 << 1)
#define SENSOR_I2C_REG_16BIT			(0x01 << 1)
#define SENSOR_I2C_CUSTOM 			(0x01 << 2)
#define SENSOR_LOW_EIGHT_BIT     		0xff

#define SENSOR_WRITE_DELAY			0xffff

typedef enum {
	SENSOR_MAIN = 0,
	SENSOR_SUB,
	SENSOR_ATV = 5,
	SENSOR_ID_MAX
} SENSOR_ID_E;

typedef struct sensor_mem_tag {
    void    *buf_ptr;
    size_t  size;
} SENSOR_MEM_T;

LOCAL struct mutex sensor_lock;
LOCAL wait_queue_head_t wait_queue_sensor;
struct semaphore g_sem_sensor;

LOCAL uint32_t g_sensor_id 			= SENSOR_ID_MAX;

LOCAL uint32_t s_sensor_mclk 		= 0;
static struct clk *s_ccir_clk 			= NULL;
LOCAL struct clk *s_ccir_enable_clk 	= NULL;

LOCAL struct i2c_client *this_client = NULL;

LOCAL SENSOR_MEM_T s_sensor_mem = {0};


LOCAL const struct i2c_device_id sensor_device_id[] = {
	{SENSOR_MAIN_I2C_NAME, 0},
	{SENSOR_SUB_I2C_NAME, 1},
	{}
};


LOCAL unsigned short sensor_main_force[] =
    { 2, SENSOR_MAIN_I2C_ADDR, I2C_CLIENT_END, I2C_CLIENT_END };
LOCAL const unsigned short *const sensor_main_forces[] =
    { sensor_main_force, NULL };
LOCAL unsigned short sensor_main_default_addr_list[] =
    { SENSOR_MAIN_I2C_ADDR, SENSOR_SUB_I2C_ADDR, I2C_CLIENT_END };
LOCAL unsigned short sensor_sub_force[] =
    { 2, SENSOR_SUB_I2C_ADDR, I2C_CLIENT_END, I2C_CLIENT_END };
LOCAL const unsigned short *const sensor_sub_forces[] =
    { sensor_sub_force, NULL };
LOCAL unsigned short sensor_sub_default_addr_list[] =
    { SENSOR_SUB_I2C_ADDR, I2C_CLIENT_END };


#define SENSOR_MCLK_SRC_NUM   4
#define SENSOR_MCLK_DIV_MAX     4
#define ABS(a) ((a) > 0 ? (a) : -(a))

typedef struct SN_MCLK {
	int clock;
	char *src_name;
} SN_MCLK;

LOCAL const SN_MCLK sensor_mclk_tab[SENSOR_MCLK_SRC_NUM] = {
	{96, "clk_96m"},
	{77, "clk_76m800k"},
	{48, "clk_48m"},
	{26, "ext_26m"}
};

LOCAL void* _Sensor_K_kmalloc(size_t size, unsigned flags)
{
    if(PNULL == s_sensor_mem.buf_ptr) {
        s_sensor_mem.buf_ptr = kmalloc(size, flags);
        if(PNULL != s_sensor_mem.buf_ptr) {
            s_sensor_mem.size = size;
        }

        return s_sensor_mem.buf_ptr;
    }else if(size <= s_sensor_mem.size) {
        return s_sensor_mem.buf_ptr;
    }else {
        //realloc memory
        kfree(s_sensor_mem.buf_ptr);
        s_sensor_mem.buf_ptr = PNULL;
        s_sensor_mem.size = 0;

        s_sensor_mem.buf_ptr = kmalloc(size, flags);
        if(PNULL != s_sensor_mem.buf_ptr) {
            s_sensor_mem.size = size;
        }

        return s_sensor_mem.buf_ptr;
    }
}

LOCAL void* _Sensor_K_kzalloc(size_t size, unsigned flags)
{
    void *ptr = _Sensor_K_kmalloc(size, flags);
    if(PNULL != ptr) {
        memset(ptr, 0, size);
    }

    return ptr;
}

LOCAL void _Sensor_K_kfree(void *p)
{
    /* memory will not be free */
    return;
}

#ifdef CONFIG_ARCH_SC8830
LOCAL uint16_t _adi_read_reg(uint32_t addr)
{
	uint32_t adi_rd_data;

	REG_WR(SPRD_ADI_BASE + 0x24, addr);
	do{
		adi_rd_data = REG_RD(SPRD_ADI_BASE + 0x28);
	}while(adi_rd_data & 0x80000000);

	return (uint16_t)(adi_rd_data&0xffff);
	
}
LOCAL void _sensor_set_ldo(void)
{
	uint32_t val;

	printk("aiden: _sensor_set_ldo start \n");
	val = _adi_read_reg(SPRD_ADI_BASE+0x8828);
	val &= ~0x3f;
	val |= 0x10;
	REG_WR(SPRD_ADI_BASE + 0x8828, val);

	val = _adi_read_reg(SPRD_ADI_BASE+0x881c);
	val &= ~(0xe0);
	REG_WR(SPRD_ADI_BASE + 0x881c, val);

	printk("aiden: _sensor_set_ldo end 1 \n");
	printk("aiden: _sensor_set_ldo end 1 \n");
}
#endif

LOCAL uint32_t Sensor_K_GetCurId(void)
{
	return g_sensor_id;
}
LOCAL int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int res = 0;
	SENSOR_PRINT_HIGH(KERN_INFO "SENSOR:sensor_probe E.\n");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		SENSOR_PRINT_HIGH(KERN_INFO "SENSOR: %s: functionality check failed\n",
		       __FUNCTION__);
		res = -ENODEV;
		goto out;
	}
	this_client = client;
/*	if (SENSOR_MAIN_I2C_ADDR != (this_client->addr & (~0xFF))) {
		this_client->addr =
		    (this_client->addr & (~0xFF)) |
		    (sensor_main_force[1] & 0xFF);
	}*/
	SENSOR_PRINT_HIGH(KERN_INFO "sensor_probe,this_client->addr =0x%x\n",
	       this_client->addr);
	return 0;
out:
	return res;
}

LOCAL int sensor_remove(struct i2c_client *client)
{
	return 0;
}

LOCAL int sensor_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	SENSOR_PRINT_HIGH("SENSOR_DRV: detect!");
	strcpy(info->type, client->name);
	return 0;
}

#if defined(CONFIG_ARCH_SC8830)
LOCAL int _Sensor_K_PowerDown(BOOLEAN power_level)
{
	return 0;
}

LOCAL void _sensor_regulator_disable(uint32_t *power_on_count, struct regulator * ptr_cam_regulator)
{
}
LOCAL int _sensor_regulator_enable(uint32_t *power_on_count, struct regulator * ptr_cam_regulator)
{
	return 0;
}
LOCAL int _Sensor_K_SetVoltage_CAMMOT(uint32_t cammot_val)
{
	return 0;
}
LOCAL int _Sensor_K_SetVoltage_AVDD(uint32_t avdd_val)
{
	return 0;
}
LOCAL int _Sensor_K_SetVoltage_DVDD(uint32_t dvdd_val)
{
	return 0;
}
LOCAL int _Sensor_K_SetVoltage_IOVDD(uint32_t iodd_val)
{
	return 0;
}

#else

LOCAL int _Sensor_K_PowerDown(BOOLEAN power_level)
{
	SENSOR_PRINT("SENSOR: _Sensor_K_PowerDown -> main: power_down %d\n",
		     power_level);
/*
	SENSOR_PRINT("SENSOR: _Sensor_K_PowerDown PIN_CTL_CCIRPD1-> 0x8C000344 0x%x\n",
	     _pard(PIN_CTL_CCIRPD1));
	SENSOR_PRINT("SENSOR: _Sensor_K_PowerDown PIN_CTL_CCIRPD0-> 0x8C000348 0x%x\n",
	     _pard(PIN_CTL_CCIRPD0));
*/
	switch (Sensor_K_GetCurId()) {
	case SENSOR_MAIN:
		{
			gpio_request(GPIO_MAIN_SENSOR_PWN, "main camera");
			if (0 == power_level) {
				gpio_direction_output(GPIO_MAIN_SENSOR_PWN, 0);

			} else {
				gpio_direction_output(GPIO_MAIN_SENSOR_PWN, 1);
			}
			gpio_free(GPIO_MAIN_SENSOR_PWN);
			break;
		}
	case SENSOR_SUB:
		{
			gpio_request(GPIO_SUB_SENSOR_PWN, "sub camera");
			if (0 == power_level) {
				gpio_direction_output(GPIO_SUB_SENSOR_PWN, 0);
			} else {
				gpio_direction_output(GPIO_SUB_SENSOR_PWN, 1);
			}
			gpio_free(GPIO_SUB_SENSOR_PWN);
			break;
		}
	default:
		break;
	}
	return SENSOR_K_SUCCESS;
}


LOCAL uint32_t iopower_on_count = 0;
LOCAL uint32_t avddpower_on_count = 0;
LOCAL uint32_t dvddpower_on_count = 0;
LOCAL uint32_t motpower_on_count = 0;

LOCAL struct regulator *s_camvio_regulator = NULL;
LOCAL struct regulator *s_camavdd_regulator = NULL;
LOCAL struct regulator *s_camdvdd_regulator = NULL;
LOCAL struct regulator *s_cammot_regulator = NULL;

LOCAL void _sensor_regulator_disable(uint32_t *power_on_count, struct regulator * ptr_cam_regulator)
{
	SENSOR_PRINT("_sensor_regulator_disable start: cnt=0x%x, io=%x, av=%x, dv=%x, mo=%x \n", *power_on_count,
		iopower_on_count, avddpower_on_count, dvddpower_on_count, motpower_on_count);
	while(*power_on_count > 0){
		regulator_disable(ptr_cam_regulator);
		(*power_on_count)--;
	}
	SENSOR_PRINT("_sensor_regulator_disable done: cnt=0x%x, io=%x, av=%x, dv=%x, mo=%x \n", *power_on_count,
		iopower_on_count, avddpower_on_count, dvddpower_on_count, motpower_on_count);

}

LOCAL int _sensor_regulator_enable(uint32_t *power_on_count, struct regulator * ptr_cam_regulator)
{
	int err;

	err = regulator_enable(ptr_cam_regulator);
	(*power_on_count)++;

	SENSOR_PRINT("_sensor_regulator_enable done: cnt=0x%x, io=%x, av=%x, dv=%x, mo=%x \n", *power_on_count,
		iopower_on_count, avddpower_on_count, dvddpower_on_count, motpower_on_count);

	return err;
}

LOCAL int _Sensor_K_SetVoltage_CAMMOT(uint32_t cammot_val)
{
	int err = 0;
	uint32_t volt_value = 0;

	SENSOR_PRINT("SENSOR:_Sensor_K_SetVoltage_CAMMOT, cammot_val=%d  \n",cammot_val);

	if (NULL == s_cammot_regulator) {
		s_cammot_regulator = regulator_get(NULL, REGU_NAME_CAMMOT);
		if (IS_ERR(s_cammot_regulator)) {
			SENSOR_PRINT_ERR("SENSOR:could not get cammot.\n");
			return SENSOR_K_FAIL;
		}
	}

	switch (cammot_val) {
	case SENSOR_VDD_2800MV:
		err =
		    regulator_set_voltage(s_cammot_regulator,
					  SENSOER_VDD_2800MV,
					  SENSOER_VDD_2800MV);
		volt_value = SENSOER_VDD_2800MV;
		if (err)
			SENSOR_PRINT_ERR("SENSOR:could not set cammot to 2800mv.\n");
		break;
	case SENSOR_VDD_3000MV:
		err =
		    regulator_set_voltage(s_cammot_regulator,
					  SENSOER_VDD_3000MV,
					  SENSOER_VDD_3000MV);
		volt_value = SENSOER_VDD_3000MV;
		if (err)
			SENSOR_PRINT_ERR("SENSOR:could not set cammot to 3800mv.\n");
		break;
	case SENSOR_VDD_2500MV:
		err =
		    regulator_set_voltage(s_cammot_regulator,
					  SENSOER_VDD_2500MV,
					  SENSOER_VDD_2500MV);
		volt_value = SENSOER_VDD_2500MV;
		if (err)
			SENSOR_PRINT_ERR("SENSOR:could not set cammot to 1800mv.\n");
		break;
	case SENSOR_VDD_1800MV:
		err =
		    regulator_set_voltage(s_cammot_regulator,
					  SENSOER_VDD_1800MV,
					  SENSOER_VDD_1800MV);
		volt_value = SENSOER_VDD_1800MV;
		if (err)
			SENSOR_PRINT_ERR("SENSOR:could not set cammot to 1200mv.\n");
		break;
	case SENSOR_VDD_CLOSED:
	case SENSOR_VDD_UNUSED:
	default:
		volt_value = 0;
		break;
	}
	if (err) {
		SENSOR_PRINT_ERR("SENSOR:set cammot error!.\n");
		return SENSOR_K_FAIL;
	}
	if (0 != volt_value) {
		/* err = regulator_enable(s_cammot_regulator); */
		err = _sensor_regulator_enable(&motpower_on_count,   s_cammot_regulator);
		if (err) {
			regulator_put(s_cammot_regulator);
			s_cammot_regulator = NULL;
			SENSOR_PRINT_ERR("SENSOR:could not enable cammot.\n");
			return SENSOR_K_FAIL;
		}
	} else {
		/* regulator_disable(s_cammot_regulator); */
		_sensor_regulator_disable(&motpower_on_count,   s_cammot_regulator);
		regulator_put(s_cammot_regulator);
		s_cammot_regulator = NULL;
		SENSOR_PRINT("SENSOR:disable cammot.\n");
	}

	return SENSOR_K_SUCCESS;
}

LOCAL int _Sensor_K_SetVoltage_AVDD(uint32_t avdd_val)
{
	int err = 0;
	uint32_t volt_value = 0;

	SENSOR_PRINT("SENSOR:_Sensor_K_SetVoltage_AVDD, avdd_val=%d  \n",avdd_val);

	if (NULL == s_camavdd_regulator) {
		s_camavdd_regulator = regulator_get(NULL, REGU_NAME_CAMAVDD);
		if (IS_ERR(s_camavdd_regulator)) {
			SENSOR_PRINT_ERR("SENSOR:could not get camavdd.\n");
			return SENSOR_K_FAIL;
		}
	}
	switch (avdd_val) {
	case SENSOR_VDD_2800MV:
		err =
		    regulator_set_voltage(s_camavdd_regulator,
					  SENSOER_VDD_2800MV,
					  SENSOER_VDD_2800MV);
		volt_value = SENSOER_VDD_2800MV;
		if (err)
			SENSOR_PRINT_ERR("SENSOR:could not set camavdd to 2800mv.\n");
		break;
	case SENSOR_VDD_3000MV:
		err =
		    regulator_set_voltage(s_camavdd_regulator,
					  SENSOER_VDD_3000MV,
					  SENSOER_VDD_3000MV);
		volt_value = SENSOER_VDD_3000MV;
		if (err)
			SENSOR_PRINT_ERR("SENSOR:could not set camavdd to 3800mv.\n");
		break;
	case SENSOR_VDD_2500MV:
		err =
		    regulator_set_voltage(s_camavdd_regulator,
					  SENSOER_VDD_2500MV,
					  SENSOER_VDD_2500MV);
		volt_value = SENSOER_VDD_2500MV;
		if (err)
			SENSOR_PRINT_ERR("SENSOR:could not set camavdd to 1800mv.\n");
		break;
	case SENSOR_VDD_1800MV:
		err =
		    regulator_set_voltage(s_camavdd_regulator,
					  SENSOER_VDD_1800MV,
					  SENSOER_VDD_1800MV);
		volt_value = SENSOER_VDD_1800MV;
		if (err)
			SENSOR_PRINT_ERR("SENSOR:could not set camavdd to 1200mv.\n");
		break;
	case SENSOR_VDD_CLOSED:
	case SENSOR_VDD_UNUSED:
	default:
		volt_value = 0;
		break;
	}
	if (err) {
		SENSOR_PRINT_ERR("SENSOR:set camavdd error!.\n");
		return SENSOR_K_FAIL;
	}
	if (0 != volt_value) {
		/* err = regulator_enable(s_camavdd_regulator);*/
		err = _sensor_regulator_enable(&avddpower_on_count,  s_camavdd_regulator);
		if (err) {
			regulator_put(s_camavdd_regulator);
			s_camavdd_regulator = NULL;
			SENSOR_PRINT_ERR("SENSOR:could not enable camavdd.\n");
			return SENSOR_K_FAIL;
		}
	} else {
		/* regulator_disable(s_camavdd_regulator); */
		_sensor_regulator_disable(&avddpower_on_count,  s_camavdd_regulator);
		regulator_put(s_camavdd_regulator);
		s_camavdd_regulator = NULL;
		SENSOR_PRINT("SENSOR:disable camavdd.\n");
	}

	return SENSOR_K_SUCCESS;
}

LOCAL int _Sensor_K_SetVoltage_DVDD(uint32_t dvdd_val)
{
	int err = 0;
	uint32_t volt_value = 0;

	SENSOR_PRINT("SENSOR:_Sensor_K_SetVoltage_DVDD, dvdd_val=%d  \n",dvdd_val);

	if (NULL == s_camdvdd_regulator) {
		s_camdvdd_regulator = regulator_get(NULL, REGU_NAME_CAMDVDD);
		if (IS_ERR(s_camdvdd_regulator)) {
			SENSOR_PRINT_ERR("SENSOR:could not get camdvdd.\n");
			return SENSOR_K_FAIL;
		}
	}
	switch (dvdd_val) {
	case SENSOR_VDD_2800MV:
		err =
		    regulator_set_voltage(s_camdvdd_regulator,
					  SENSOER_VDD_2800MV,
					  SENSOER_VDD_2800MV);
		volt_value = SENSOER_VDD_2800MV;
		if (err)
			SENSOR_PRINT_ERR("SENSOR:could not set camdvdd to 2800mv.\n");
		break;
	case SENSOR_VDD_1800MV:
		err =
		    regulator_set_voltage(s_camdvdd_regulator,
					  SENSOER_VDD_1800MV,
					  SENSOER_VDD_1800MV);
		volt_value = SENSOER_VDD_1800MV;
		if (err)
			SENSOR_PRINT_ERR("SENSOR:could not set camdvdd to 1800mv.\n");
		break;
	case SENSOR_VDD_1500MV:
		err =
		    regulator_set_voltage(s_camdvdd_regulator,
					  SENSOER_VDD_1500MV,
					  SENSOER_VDD_1500MV);
		volt_value = SENSOER_VDD_1500MV;
		if (err)
			SENSOR_PRINT_ERR("SENSOR:could not set camdvdd to 1500mv.\n");
		break;
	case SENSOR_VDD_1300MV:
		err =
		    regulator_set_voltage(s_camdvdd_regulator,
					  SENSOER_VDD_1300MV,
					  SENSOER_VDD_1300MV);
		volt_value = SENSOER_VDD_1300MV;
		if (err)
			SENSOR_PRINT_ERR("SENSOR:could not set camdvdd to 1300mv.\n");
		break;
	case SENSOR_VDD_CLOSED:
	case SENSOR_VDD_UNUSED:
	default:
		volt_value = 0;
		break;
	}
	if (err) {
		SENSOR_PRINT_ERR("SENSOR:set camdvdd error,err=%d!.\n",err);
		return SENSOR_K_FAIL;
	}
	if (0 != volt_value) {
		/* err = regulator_enable(s_camdvdd_regulator); */
		err = _sensor_regulator_enable(&dvddpower_on_count,  s_camdvdd_regulator);
		if (err) {
			regulator_put(s_camdvdd_regulator);
			s_camdvdd_regulator = NULL;
			SENSOR_PRINT_ERR("SENSOR:could not enable camdvdd.\n");
			return SENSOR_K_FAIL;
		}
	} else {
		/* regulator_disable(s_camdvdd_regulator); */
		_sensor_regulator_disable(&dvddpower_on_count,  s_camdvdd_regulator);
		regulator_put(s_camdvdd_regulator);
		s_camdvdd_regulator = NULL;
		SENSOR_PRINT("SENSOR:disable camdvdd.\n");
	}

	return SENSOR_K_SUCCESS;
}

LOCAL int _Sensor_K_SetVoltage_IOVDD(uint32_t iodd_val)
{
	int err = 0;
	uint32_t volt_value = 0;

	SENSOR_PRINT("SENSOR:_Sensor_K_SetVoltage_IOVDD, iodd_val=%d  \n",iodd_val);

	if(NULL == s_camvio_regulator) {
		s_camvio_regulator = regulator_get(NULL, REGU_NAME_CAMVIO);
		if (IS_ERR(s_camvio_regulator)) {
			SENSOR_PRINT_ERR("SENSOR:could not get camvio.\n");
			return SENSOR_K_FAIL;
		}
	}
	switch (iodd_val) {
	case SENSOR_VDD_2800MV:
		err =
		    regulator_set_voltage(s_camvio_regulator,
					  SENSOER_VDD_2800MV,
					  SENSOER_VDD_2800MV);
		volt_value = SENSOER_VDD_2800MV;
		if (err)
			SENSOR_PRINT_ERR("SENSOR:could not set camvio to 2800mv.\n");
		break;
	case SENSOR_VDD_3800MV:
		err =
		    regulator_set_voltage(s_camvio_regulator,
					  SENSOER_VDD_3800MV,
					  SENSOER_VDD_3800MV);
		volt_value = SENSOER_VDD_3800MV;
		if (err)
			SENSOR_PRINT_ERR("SENSOR:could not set camvio to 3800mv.\n");
		break;
	case SENSOR_VDD_1800MV:
		err =
		    regulator_set_voltage(s_camvio_regulator,
					  SENSOER_VDD_1800MV,
					  SENSOER_VDD_1800MV);
		volt_value = SENSOER_VDD_1800MV;
		if (err)
			SENSOR_PRINT_ERR("SENSOR:could not set camvio to 1800mv.\n");
		break;
	case SENSOR_VDD_1200MV:
		err =
		    regulator_set_voltage(s_camvio_regulator,
					  SENSOER_VDD_1200MV,
					  SENSOER_VDD_1200MV);
		volt_value = SENSOER_VDD_1200MV;
		if (err)
			SENSOR_PRINT_ERR("SENSOR:could not set camvio to 1200mv.\n");
		break;
	case SENSOR_VDD_CLOSED:
	case SENSOR_VDD_UNUSED:
	default:
		volt_value = 0;
		break;
	}
	if (err) {
		SENSOR_PRINT_ERR("SENSOR:set camvio error!.\n");
		return SENSOR_K_FAIL;
	}
	if (0 != volt_value) {
		/* err = regulator_enable(s_camvio_regulator); */
		err = _sensor_regulator_enable(&iopower_on_count,    s_camvio_regulator);
		if (err) {
			regulator_put(s_camvio_regulator);
			s_camvio_regulator = NULL;
			SENSOR_PRINT_ERR("SENSOR:could not enable camvio.\n");
			return SENSOR_K_FAIL;
		}
	} else {
		/* regulator_disable(s_camvio_regulator); */
		_sensor_regulator_disable(&iopower_on_count,    s_camvio_regulator);
		regulator_put(s_camvio_regulator);
		s_camvio_regulator = NULL;
		SENSOR_PRINT("SENSOR:disable camvio.\n");
	}

	return SENSOR_K_SUCCESS;
}
#endif

LOCAL int select_sensor_mclk(uint8_t clk_set, char **clk_src_name,
			     uint8_t * clk_div)
{
	uint8_t i, j, mark_src = 0, mark_div = 0, mark_src_tmp = 0;
	int clk_tmp, src_delta, src_delta_min = NUMBER_MAX;
	int div_delta_min = NUMBER_MAX;

	SENSOR_PRINT_HIGH("SENSOR mclk %d.\n", clk_set);
	if (clk_set > 96 || !clk_src_name || !clk_div) {
		return SENSOR_K_FAIL;
	}
	for (i = 0; i < SENSOR_MCLK_DIV_MAX; i++) {
		clk_tmp = (int)(clk_set * (i + 1));
		src_delta_min = NUMBER_MAX;
		for (j = 0; j < SENSOR_MCLK_SRC_NUM; j++) {
			src_delta = ABS(sensor_mclk_tab[j].clock - clk_tmp);
			if (src_delta < src_delta_min) {
				src_delta_min = src_delta;
				mark_src_tmp = j;
			}
		}
		if (src_delta_min < div_delta_min) {
			div_delta_min = src_delta_min;
			mark_src = mark_src_tmp;
			mark_div = i;
		}
	}
	SENSOR_PRINT_HIGH("src %d, div=%d .\n", mark_src,
	       mark_div);

	*clk_src_name = sensor_mclk_tab[mark_src].src_name;
	*clk_div = mark_div + 1;

	return SENSOR_K_SUCCESS;
}

#if defined(CONFIG_ARCH_SC8825)
LOCAL int _Sensor_K_SetMCLK(uint32_t mclk)
{
	struct clk *clk_parent = NULL;
	int ret;
	char *clk_src_name = NULL;
	uint8_t clk_div;

	SENSOR_PRINT
	    ("SENSOR: _Sensor_K_SetMCLK -> s_sensor_mclk = %d MHz, clk = %d MHz\n",
	     s_sensor_mclk, mclk);

	if ((0 != mclk) && (s_sensor_mclk != mclk)) {
		if (s_ccir_clk) {
			clk_disable(s_ccir_clk);
			SENSOR_PRINT("###sensor s_ccir_clk clk_disable ok.\n");
		} else {
			s_ccir_clk = clk_get(NULL, SENSOR_CLK);
			if (IS_ERR(s_ccir_clk)) {
				SENSOR_PRINT_ERR
				    ("###: Failed: Can't get clock [ccir_mclk]!\n");
				SENSOR_PRINT_ERR("###: s_sensor_clk = %p.\n",
						 s_ccir_clk);
			} else {
				SENSOR_PRINT
				    ("###sensor s_ccir_clk clk_get ok.\n");
			}
		}
		if (mclk > SENSOR_MAX_MCLK) {
			mclk = SENSOR_MAX_MCLK;
		}
		if (SENSOR_K_SUCCESS !=
		    select_sensor_mclk((uint8_t) mclk, &clk_src_name,
				       &clk_div)) {
			SENSOR_PRINT_HIGH
			    ("SENSOR:Sensor_SetMCLK select clock source fail.\n");
			return -EINVAL;
		}

		clk_parent = clk_get(NULL, clk_src_name);
		if (!clk_parent) {
			SENSOR_PRINT_ERR
			    ("###:clock: failed to get clock [%s] by clk_get()!\n", clk_src_name);
			return -EINVAL;
		}

		ret = clk_set_parent(s_ccir_clk, clk_parent);
		if (ret) {
			SENSOR_PRINT_ERR
			    ("###:clock: clk_set_parent() failed!parent \n");
			return -EINVAL;
		}

		ret = clk_set_rate(s_ccir_clk, (mclk * SENOR_CLK_M_VALUE));
		if (ret) {
			SENSOR_PRINT_ERR
			    ("###:clock: clk_set_rate failed!\n");
			return -EINVAL;
		}
		ret = clk_enable(s_ccir_clk);
		if (ret) {
			SENSOR_PRINT_ERR("###:clock: clk_enable() failed!\n");
		} else {
			SENSOR_PRINT("###sensor s_ccir_clk clk_enable ok.\n");
		}

		if (NULL == s_ccir_enable_clk) {
			s_ccir_enable_clk = clk_get(NULL, "clk_ccir");
			if (IS_ERR(s_ccir_enable_clk)) {
				SENSOR_PRINT_ERR
				    ("###: Failed: Can't get clock [clk_ccir]!\n");
				SENSOR_PRINT_ERR
				    ("###: s_ccir_enable_clk = %p.\n",
				     s_ccir_enable_clk);
				return -EINVAL;
			} else {
				SENSOR_PRINT
				    ("###sensor s_ccir_enable_clk clk_get ok.\n");
			}
			ret = clk_enable(s_ccir_enable_clk);
			if (ret) {
				SENSOR_PRINT_ERR
				    ("###:clock: clk_enable() failed!\n");
			} else {
				SENSOR_PRINT
				    ("###sensor s_ccir_enable_clk clk_enable ok.\n");
			}
		}

		s_sensor_mclk = mclk;
		SENSOR_PRINT
		    ("SENSOR: Sensor_SetMCLK -> s_sensor_mclk = %d Hz.\n",
		     s_sensor_mclk);
	} else if (0 == mclk) {
		if (s_ccir_clk) {
			clk_disable(s_ccir_clk);
			SENSOR_PRINT("###sensor s_ccir_clk clk_disable ok.\n");
			clk_put(s_ccir_clk);
			SENSOR_PRINT("###sensor s_ccir_clk clk_put ok.\n");
			s_ccir_clk = NULL;
		}

		if (s_ccir_enable_clk) {
			clk_disable(s_ccir_enable_clk);
			SENSOR_PRINT
			    ("###sensor s_ccir_enable_clk clk_disable ok.\n");
			clk_put(s_ccir_enable_clk);
			SENSOR_PRINT
			    ("###sensor s_ccir_enable_clk clk_put ok.\n");
			s_ccir_enable_clk = NULL;
		}
		s_sensor_mclk = 0;
		SENSOR_PRINT("SENSOR: Sensor_SetMCLK -> Disable MCLK !!!");
	} else {
		SENSOR_PRINT("SENSOR: Sensor_SetMCLK -> Do nothing !! ");
	}
	SENSOR_PRINT("SENSOR: Sensor_SetMCLK X\n");

	return 0;
}
#else
LOCAL int _Sensor_K_SetMCLK(uint32_t mclk)
{
	printk("aiden: no _Sensor_K_SetMCLK,  mclk = %d =0x%x \n", mclk, mclk);
	return 0;
}
#endif
LOCAL int _Sensor_K_Reset(uint32_t level, uint32_t width)
{
	int err;

	SENSOR_PRINT_HIGH("Sensor rst, level %d width %d.\n", level, width);

	err = gpio_request(GPIO_SENSOR_RESET, "ccirrst");
	if (err) {
		SENSOR_PRINT_HIGH("Sensor_Reset failed requesting err=%d\n", err);
		return SENSOR_K_FAIL;
	}
	gpio_direction_output(GPIO_SENSOR_RESET, level);
	gpio_set_value(GPIO_SENSOR_RESET, level);
	SLEEP_MS(width);
	gpio_set_value(GPIO_SENSOR_RESET, !level);
	mdelay(1);
	gpio_free(GPIO_SENSOR_RESET);

	return SENSOR_K_SUCCESS;
}

LOCAL int _Sensor_K_I2CInit(uint32_t sensor_id)
{
	g_sensor_id =  sensor_id;

	return SENSOR_K_SUCCESS;
}

LOCAL int _Sensor_K_I2CDeInit(uint32_t sensor_id)
{
	g_sensor_id =  SENSOR_ID_MAX;

	SENSOR_PRINT_HIGH("-I2C %d OK.\n", sensor_id);

	return SENSOR_K_SUCCESS;
}

LOCAL int _Sensor_K_SetResetLevel(uint32_t plus_level)
{
	int err = 0xff;
	err = gpio_request(GPIO_SENSOR_RESET, "ccirrst");
	if (err) {
		SENSOR_PRINT_HIGH("_Sensor_K_Reset failed requesting err=%d\n", err);
		return SENSOR_K_FAIL;
	}
	gpio_direction_output(GPIO_SENSOR_RESET, plus_level);
	gpio_set_value(GPIO_SENSOR_RESET, plus_level);
	SLEEP_MS(100);
	gpio_free(GPIO_SENSOR_RESET);

	return SENSOR_K_SUCCESS;
}

LOCAL int _Sensor_K_ReadReg(SENSOR_REG_BITS_T_PTR pReg)
{
	uint8_t cmd[2] = { 0 };
	uint16_t w_cmd_num = 0;
	uint16_t r_cmd_num = 0;
	uint8_t buf_r[2] = { 0 };
	int32_t ret = SENSOR_K_SUCCESS;
	struct i2c_msg msg_r[2];
	uint16_t reg_addr;
	int i;

	reg_addr = pReg->reg_addr;

	if (SENSOR_I2C_REG_16BIT ==(pReg->reg_bits & SENSOR_I2C_REG_16BIT)) {
		cmd[w_cmd_num++] = (uint8_t) ((reg_addr >> 8) & SENSOR_LOW_EIGHT_BIT);
		cmd[w_cmd_num++] = (uint8_t) (reg_addr & SENSOR_LOW_EIGHT_BIT);
	} else {
		cmd[w_cmd_num++] = (uint8_t) reg_addr;
	}

	if (SENSOR_I2C_VAL_16BIT == (pReg->reg_bits & SENSOR_I2C_VAL_16BIT)) {
		r_cmd_num = SENSOR_CMD_BITS_16;
	} else {
		r_cmd_num = SENSOR_CMD_BITS_8;
	}

	for (i = 0; i < SENSOR_I2C_OP_TRY_NUM; i++) {
		msg_r[0].addr = this_client->addr;
		msg_r[0].flags = 0;
		msg_r[0].buf = cmd;
		msg_r[0].len = w_cmd_num;
		msg_r[1].addr = this_client->addr;
		msg_r[1].flags = I2C_M_RD;
		msg_r[1].buf = buf_r;
		msg_r[1].len = r_cmd_num;
		ret = i2c_transfer(this_client->adapter, msg_r, 2);
		if (ret != 2) {
			SENSOR_PRINT_ERR("SENSOR:read reg fail, ret %d, addr 0x%x \n",
			     				ret, this_client->addr);
			SLEEP_MS(20);
			ret = SENSOR_K_FAIL;
		} else {
			pReg->reg_value = (r_cmd_num == 1) ? (uint16_t) buf_r[0] : (uint16_t) ((buf_r[0] << 8) + buf_r[1]);
			//SENSOR_PRINT_HIGH("_Sensor_K_ReadReg: i2cAddr=%x, addr=%x, value=%x, bit=%d \n",
			//		this_client->addr, pReg->reg_addr, pReg->reg_value, pReg->reg_bits);
			ret = SENSOR_K_SUCCESS;
			break;
		}
	}

	return ret;
}

LOCAL int _Sensor_K_WriteReg(SENSOR_REG_BITS_T_PTR pReg)
{
	uint8_t cmd[4] = { 0 };
	uint32_t index = 0;
	uint32_t cmd_num = 0;
	struct i2c_msg msg_w;
	int32_t ret = SENSOR_K_SUCCESS;
	uint16_t subaddr;
	uint16_t data;
	int i;

	subaddr = pReg->reg_addr;
	data = pReg->reg_value;

	if (SENSOR_I2C_REG_16BIT ==(pReg->reg_bits & SENSOR_I2C_REG_16BIT)) {
		cmd[cmd_num++] = (uint8_t) ((subaddr >> 8) & SENSOR_LOW_EIGHT_BIT);
		index++;
		cmd[cmd_num++] =  (uint8_t) (subaddr & SENSOR_LOW_EIGHT_BIT);
		index++;
	} else {
		cmd[cmd_num++] = (uint8_t) subaddr;
		index++;
	}

	if (SENSOR_I2C_VAL_16BIT == (pReg->reg_bits & SENSOR_I2C_VAL_16BIT)) {
		cmd[cmd_num++] = (uint8_t) ((data >> 8) & SENSOR_LOW_EIGHT_BIT);
		index++;
		cmd[cmd_num++] = (uint8_t) (data & SENSOR_LOW_EIGHT_BIT);
		index++;
	} else {
		cmd[cmd_num++] = (uint8_t) data;
		index++;
	}

	if (SENSOR_WRITE_DELAY != subaddr) {
		for (i = 0; i < SENSOR_I2C_OP_TRY_NUM; i++) {
			msg_w.addr = this_client->addr;
			msg_w.flags = 0;
			msg_w.buf = cmd;
			msg_w.len = index;
			ret = i2c_transfer(this_client->adapter, &msg_w, 1);
			if (ret != 1) {
				SENSOR_PRINT_HIGH("_Sensor_K_WriteReg failed:i2cAddr=%x, addr=%x, value=%x, bit=%d \n",
						this_client->addr, pReg->reg_addr, pReg->reg_value, pReg->reg_bits);
				ret = SENSOR_K_FAIL;
				continue;
			} else {
				//SENSOR_PRINT_HIGH("SENSOR: IIC write reg OK! i2cAddr=%x, 0x%04x, val:0x%04x \n",
				//		this_client->addr, subaddr, data);
				ret = SENSOR_K_SUCCESS;
				break;
			}
		}
	} else {
		SLEEP_MS(data);
		/*SENSOR_PRINT("SENSOR: IIC write Delay %d ms", data); */
	}
	if(subaddr == 0x3008){
		if(0x02 == data){
			printk("aiden: sensor stream on 1 \n");
		}else if(0x42 == data){
			printk("aiden: sensor stream off 1 \n");
		}
	}

	return ret;
}

LOCAL int _Sensor_K_SetFlash(uint32_t flash_mode)
{
	switch (flash_mode) {
	case 1:		/*flash on */
	case 2:		/*for torch */
		/*low light */
		gpio_request(138, "gpio138");
		gpio_direction_output(138, 1);
		gpio_set_value(138, 1);
		gpio_request(137, "gpio137");
		gpio_direction_output(137, 0);
		gpio_set_value(137, 0);
		break;
	case 0x11:
		/*high light */
		gpio_request(138, "gpio138");
		gpio_direction_output(138, 1);
		gpio_set_value(138, 1);

		gpio_request(137, "gpio137");
		gpio_direction_output(137, 1);
		gpio_set_value(137, 1);
		break;
	case 0x10:		/*close flash */
	case 0x0:
		/*close the light */
		gpio_request(138, "gpio138");
		gpio_direction_output(138, 0);
		gpio_set_value(138, 0);
		gpio_request(137, "gpio137");
		gpio_direction_output(137, 0);
		gpio_set_value(137, 0);
		break;
	default:
		SENSOR_PRINT_HIGH("_Sensor_K_SetFlash unknow mode:flash_mode=%d \n", flash_mode);
		break;
	}

	SENSOR_PRINT("_Sensor_K_SetFlash: flash_mode=%d  \n", flash_mode);
	
	return SENSOR_K_SUCCESS;
}
int hi351_init_write(SENSOR_REG_T_PTR p_reg_table, uint32_t init_table_size);

LOCAL int _Sensor_K_WriteRegTab(SENSOR_REG_TAB_PTR pRegTab)
{
	char *pBuff = PNULL;
	uint32_t cnt = pRegTab->reg_count;
	int ret = SENSOR_K_SUCCESS;
	uint32_t size;
	SENSOR_REG_T_PTR sensor_reg_ptr;
	SENSOR_REG_BITS_T reg_bit;
	uint32_t i;
	int rettmp;
	struct timeval time1, time2;

	do_gettimeofday(&time1);
	
	size = cnt*sizeof(SENSOR_REG_T);
	pBuff = _Sensor_K_kmalloc(size, GFP_KERNEL);
	if(PNULL == pBuff){
		ret = SENSOR_K_FAIL;
		SENSOR_PRINT_ERR("_Sensor_K_WriteRegTab ERROR:kmalloc is fail, cnt=%d, size = %d \n", cnt, size);
		goto _Sensor_K_WriteRegTab_return;
	}else{
		SENSOR_PRINT("_Sensor_K_WriteRegTab: kmalloc success, cnt=%d, size = %d \n",cnt, size); 
	}

	if (copy_from_user(pBuff, pRegTab->sensor_reg_tab_ptr, size)){
		ret = SENSOR_K_FAIL;
		SENSOR_PRINT_ERR("sensor_k_write ERROR:copy_from_user fail, size = %d \n", size);
		goto _Sensor_K_WriteRegTab_return;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR)pBuff;
	
	if(0 == pRegTab->burst_mode){
		for(i=0; i<cnt; i++){
			reg_bit.reg_addr  = sensor_reg_ptr[i].reg_addr;
			reg_bit.reg_value = sensor_reg_ptr[i].reg_value;
			reg_bit.reg_bits  = pRegTab->reg_bits;
			
			rettmp = _Sensor_K_WriteReg(&reg_bit);
			if(SENSOR_K_FAIL == rettmp)
				ret = SENSOR_K_FAIL;
		}
	}else if(7 == pRegTab->burst_mode){
		printk("CAM %s, Line %d, burst_mode=%d, cnt=%d, start \n", __FUNCTION__, __LINE__, pRegTab->burst_mode, cnt);
		ret = hi351_init_write(pRegTab->sensor_reg_tab_ptr, pRegTab->reg_count);
		printk("CAM %s, Line %d, burst_mode=%d, cnt=%d end\n", __FUNCTION__, __LINE__, pRegTab->burst_mode, cnt);
	}


_Sensor_K_WriteRegTab_return:
	if(PNULL != pBuff)
		_Sensor_K_kfree(pBuff);

	do_gettimeofday(&time2);
	
	SENSOR_PRINT("_Sensor_K_WriteRegTab: done, ret = %d, cnt=%d, time=%d us \n", ret, cnt,
		(uint32_t)((time2.tv_sec - time1.tv_sec)*1000000+(time2.tv_usec - time1.tv_usec)));
	
	return ret;
}

LOCAL int _Sensor_K_SetI2CClock(uint32_t clock)
{
#if defined (CONFIG_ARCH_SC8825)
	sprd_i2c_ctl_chg_clk(SENSOR_I2C_ID, clock);
#elif defined (CONFIG_ARCH_SC8825)
	sc8810_i2c_set_clk(SENSOR_I2C_ID, clock);
#elif defined (CONFIG_ARCH_SC8830)
	//sprd_i2c_ctl_chg_clk(SENSOR_I2C_ID, clock);
#endif

	SENSOR_PRINT("_Sensor_K_SetI2CClock: set i2c clock to %d  \n", clock);

	return SENSOR_K_SUCCESS;
}

LOCAL int _Sensor_K_WriteI2C(SENSOR_I2C_T_PTR pI2cTab)
{
	char *pBuff = PNULL;
	struct i2c_msg msg_w;
	uint32_t cnt = pI2cTab->i2c_count;
	int ret = SENSOR_K_FAIL;

	pBuff = _Sensor_K_kmalloc(cnt, GFP_KERNEL);
	if(PNULL == pBuff){
		SENSOR_PRINT_ERR("_Sensor_K_WriteI2C ERROR:kmalloc is fail, size = %d \n", cnt);
		goto sensor_k_writei2c_return;
	}
	else{
		SENSOR_PRINT("_Sensor_K_WriteI2C: kmalloc success, size = %d \n", cnt);
	}

	if (copy_from_user(pBuff, pI2cTab->i2c_data, cnt)){
		SENSOR_PRINT_ERR("_Sensor_K_WriteI2C ERROR:copy_from_user fail, size = %d \n", cnt);
		goto sensor_k_writei2c_return;
	}

	msg_w.addr = pI2cTab->slave_addr;
	msg_w.flags = 0;
	msg_w.buf = pBuff;
	msg_w.len = cnt;

	ret = i2c_transfer(this_client->adapter, &msg_w, 1);
	if (ret != 1) {
		SENSOR_PRINT_ERR("SENSOR: write sensor reg fail, ret : %d, I2C slave addr: 0x%x, \n",
		ret, msg_w.addr);
	}else{
		ret = SENSOR_K_SUCCESS;
	}

sensor_k_writei2c_return:
	if(PNULL != pBuff)
		_Sensor_K_kfree(pBuff);

	SENSOR_PRINT("sensor_k_write: done, ret = %d \n", ret);

	return ret;
}

int sensor_k_open(struct inode *node, struct file *file)
{
	return 0;
}

int sensor_k_release(struct inode *node, struct file *file)
{
	return 0;
}

LOCAL ssize_t sensor_k_read(struct file *filp, char __user *ubuf, size_t cnt, loff_t *gpos)
{
	return 0;
}

LOCAL ssize_t sensor_k_write(struct file *filp, const char __user *ubuf, size_t cnt, loff_t *gpos)
{
	char buf[64];
	char *pBuff = PNULL;
	struct i2c_msg msg_w;
	int ret = SENSOR_K_FAIL;
	int need_alloc = 1;

	SENSOR_PRINT("sensor_k_write: cnt=%d, buf=%d \n", cnt, sizeof(buf));

	if (cnt < sizeof(buf)){
		pBuff = buf;
		need_alloc = 0;
	}else{
		pBuff = _Sensor_K_kmalloc(cnt, GFP_KERNEL);
		if(PNULL == pBuff){
			SENSOR_PRINT_ERR("sensor_k_write ERROR:kmalloc is fail, size = %d \n", cnt);
			goto sensor_k_write_return;
		}
		else{
			SENSOR_PRINT("sensor_k_write: kmalloc success, size = %d \n", cnt);
		}
	}

	if (copy_from_user(pBuff, ubuf, cnt)){
		SENSOR_PRINT_ERR("sensor_k_write ERROR:copy_from_user fail, size = %d \n", cnt);
		goto sensor_k_write_return;
	}
    printk("this_client->addr=0x%x.\n",this_client->addr);
	msg_w.addr = this_client->addr;
	msg_w.flags = 0;
	msg_w.buf = pBuff;
	msg_w.len = cnt;

	ret = i2c_transfer(this_client->adapter, &msg_w, 1);
	if (ret != 1) {
		SENSOR_PRINT_ERR("SENSOR: write reg fail, ret : %d, I2C w addr: 0x%x, \n",
		     						ret, this_client->addr);
	}else{
		ret = SENSOR_K_SUCCESS;
	}

sensor_k_write_return:
	if((PNULL != pBuff) && need_alloc)
		_Sensor_K_kfree(pBuff);

	SENSOR_PRINT("sensor_k_write: done, ret = %d \n", ret);

	return ret;
}

#if 1	//wujinyou
#define I2C_WRITE_BURST_LENGTH    512

int hi351_init_write(SENSOR_REG_T_PTR p_reg_table, uint32_t init_table_size)
{
	uint32_t              rtn = 0;
	int ret = 0;
	uint32_t              i = 0;
	uint32_t              written_num = 0;
	uint16_t              wr_reg = 0;
	uint16_t              wr_val = 0;
	uint32_t              wr_num_once = 0;
	//uint32_t              init_table_size = (sizeof(HI351_common)/sizeof(HI351_common[0]));	//NUMBER_OF_ARRAY(HI351_common);
	//SENSOR_REG_T_PTR    p_reg_table = HI351_common;
	uint8_t               *p_reg_val_tmp = 0;
	struct i2c_msg msg_w;
	struct i2c_client *i2c_client = this_client;	//Sensor_GetI2CClien();
	printk("++++SENSOR: HI351_InitExt\n");
	if(0 == i2c_client)
	{
		printk("SENSOR: HI351_InitExt:error,i2c_client is NULL!.\n");
		return -1;
	}
	p_reg_val_tmp = (uint8_t*)_Sensor_K_kzalloc(init_table_size*sizeof(uint16_t) + 16, GFP_KERNEL);

	if(PNULL == p_reg_val_tmp){
		SENSOR_PRINT_ERR("hi351_init_write ERROR:kmalloc is fail, size = %d \n", init_table_size*sizeof(uint16_t) + 16);
		return -1;
	}
	else{
		SENSOR_PRINT_HIGH("hi351_init_write: kmalloc success, size = %d \n", init_table_size*sizeof(uint16_t) + 16);
	}


	while(written_num < init_table_size)
	{
		wr_num_once = 2;

		wr_reg = p_reg_table[written_num].reg_addr;
		wr_val = p_reg_table[written_num].reg_value;
		if(SENSOR_WRITE_DELAY == wr_reg)
		{
			if(wr_val >= 10)
			{
				msleep(wr_val);
			}
			else
			{
				mdelay(wr_val);
			}
		}
		else
		{
			p_reg_val_tmp[0] = (uint8_t)(wr_reg);
		//	SENSOR_PRINT("SENSOR: HI351_InitExt, val 0x%x.\n",p_reg_val_tmp[0]);
			p_reg_val_tmp[1] = (uint8_t)(wr_val);
		//	SENSOR_PRINT("SENSOR: HI351_InitExt, val 0x%x.\n",p_reg_val_tmp[1]);

			if ((0x0e == wr_reg) && (0x01 == wr_val))
			{
				for(i = written_num + 1; i< init_table_size; i++)
				{
					if((0x0e == wr_reg) && (0x00 == wr_val))
					{
						break;
					}
					else
					{
						wr_val = p_reg_table[i].reg_value;
						p_reg_val_tmp[wr_num_once+1] = (uint8_t)(wr_val);
						wr_num_once ++;
					}
				/*SENSOR_PRINT("SENSOR: HI351_InitExt senderror, val {0x%x 0x%x} {0x%x 0x%x} {0x%x 0x%x} {0x%x 0x%x} {0x%x 0x%x} {0x%x 0x%x}.\n",
				        p_reg_val_tmp[0],p_reg_val_tmp[1],p_reg_val_tmp[2],p_reg_val_tmp[3],
				        p_reg_val_tmp[4],p_reg_val_tmp[5],p_reg_val_tmp[6],p_reg_val_tmp[7],
				        p_reg_val_tmp[8],p_reg_val_tmp[9],p_reg_val_tmp[10],p_reg_val_tmp[11]);  */
				}
				//printk("aiden: write length=%d \n", wr_num_once);
			}
			msg_w.addr = i2c_client->addr;
			msg_w.flags = 0;
			msg_w.buf = p_reg_val_tmp;
			msg_w.len = (uint32_t)(wr_num_once);
			ret = i2c_transfer(i2c_client->adapter, &msg_w, 1);
			if(ret!=1)
			{
				SENSOR_PRINT("SENSOR: HI351_InitExt senderror, val {0x%x 0x%x} {0x%x 0x%x} {0x%x 0x%x} {0x%x 0x%x} {0x%x 0x%x} {0x%x 0x%x}.\n",
				        p_reg_val_tmp[0],p_reg_val_tmp[1],p_reg_val_tmp[2],p_reg_val_tmp[3],
				        p_reg_val_tmp[4],p_reg_val_tmp[5],p_reg_val_tmp[6],p_reg_val_tmp[7],
				        p_reg_val_tmp[8],p_reg_val_tmp[9],p_reg_val_tmp[10],p_reg_val_tmp[11]);
				SENSOR_PRINT("SENSOR: HI351_InitExt, i2c write once error\n");
				rtn = 1;
				break;
			}
			else
			{
#if 0
				SENSOR_PRINT("SENSOR: HI351_InitExt, i2c write once from %d {0x%x 0x%x}, total %d registers {0x%x 0x%x}\n",
				      written_num,cmd[0],cmd[1],wr_num_once,p_reg_val_tmp[0],p_reg_val_tmp[1]);
				if(wr_num_once > 1)
				{
					SENSOR_PRINT("SENSOR: HI351_InitExt, val {0x%x 0x%x} {0x%x 0x%x} {0x%x 0x%x} {0x%x 0x%x} {0x%x 0x%x} {0x%x 0x%x}.\n",
				          p_reg_val_tmp[0],p_reg_val_tmp[1],p_reg_val_tmp[2],p_reg_val_tmp[3],
				          p_reg_val_tmp[4],p_reg_val_tmp[5],p_reg_val_tmp[6],p_reg_val_tmp[7],
				          p_reg_val_tmp[8],p_reg_val_tmp[9],p_reg_val_tmp[10],p_reg_val_tmp[11]);

				}
#endif
			}
		}
		written_num += wr_num_once-1;
	}
    SENSOR_PRINT("SENSOR: HI351_InitExt, success\n");
    _Sensor_K_kfree(p_reg_val_tmp);
    return rtn;
}
#endif


LOCAL long sensor_k_ioctl(struct file *file, unsigned int cmd,
			  unsigned long arg)
{
	int ret = 0;

	mutex_lock(&sensor_lock);

	switch (cmd) {
	case SENSOR_IO_PD:
		{
			BOOLEAN power_level;
			ret = copy_from_user(&power_level, (BOOLEAN *) arg, sizeof(BOOLEAN));

			if(0 == ret)
				ret = _Sensor_K_PowerDown(power_level);
		}
		break;
	case SENSOR_IO_SET_CAMMOT:
		{
			uint32_t vdd_val;
			ret = copy_from_user(&vdd_val, (uint32_t *) arg, sizeof(uint32_t));
			if(0 == ret)
				ret = _Sensor_K_SetVoltage_CAMMOT(vdd_val);
		}
		break;

	case SENSOR_IO_SET_AVDD:
		{
			uint32_t vdd_val;
			ret = copy_from_user(&vdd_val, (uint32_t *) arg, sizeof(uint32_t));
			if(0 == ret)
				ret = _Sensor_K_SetVoltage_AVDD(vdd_val);
		}
		break;

	case SENSOR_IO_SET_DVDD:
		{
			uint32_t vdd_val;
			ret = copy_from_user(&vdd_val, (uint32_t *) arg, sizeof(uint32_t));
			if(0 == ret)
				ret = _Sensor_K_SetVoltage_DVDD(vdd_val);
		}
		break;

	case SENSOR_IO_SET_IOVDD:
		{
			uint32_t vdd_val;
			ret = copy_from_user(&vdd_val, (uint32_t *) arg, sizeof(uint32_t));
			if(0 == ret)
				ret = _Sensor_K_SetVoltage_IOVDD(vdd_val);
		}
		break;

	case SENSOR_IO_SET_MCLK:
		{
			uint32_t mclk;
			ret = copy_from_user(&mclk, (uint32_t *) arg, sizeof(uint32_t));
			if(0 == ret)
				ret = _Sensor_K_SetMCLK(mclk);
		}
		break;


	case SENSOR_IO_RST:
		{
			uint32_t rst_val[2];
			ret = copy_from_user(rst_val, (uint32_t *) arg, 2*sizeof(uint32_t));
			if(0 == ret)
				ret = _Sensor_K_Reset(rst_val[0], rst_val[1]);
		}
		break;

	case SENSOR_IO_I2C_INIT:
		{
			uint32_t sensor_id;
			ret = copy_from_user(&sensor_id, (uint32_t *) arg, sizeof(uint32_t));
			if(0 == ret)
				ret = _Sensor_K_I2CInit(sensor_id);
		}
		break;

	case SENSOR_IO_I2C_DEINIT:
		{
			uint32_t sensor_id;
			ret = copy_from_user(&sensor_id, (uint32_t *) arg, sizeof(uint32_t));
			if(0 == ret)
				ret = _Sensor_K_I2CDeInit(sensor_id);
		}
		break;

	case SENSOR_IO_SET_ID:
		{
			ret = copy_from_user(&g_sensor_id, (uint32_t *) arg, sizeof(uint32_t));
		}
		break;
	

	case SENSOR_IO_RST_LEVEL:
		{
			uint32_t level;
			ret = copy_from_user(&level, (uint32_t *) arg, sizeof(uint32_t));
			if(0 == ret)
				ret = _Sensor_K_SetResetLevel(level);
		}
		break;

	case SENSOR_IO_I2C_ADDR:
		{
			uint16_t i2c_addr;
			ret = copy_from_user(&i2c_addr, (uint16_t *) arg, sizeof(uint16_t));
			if(0 == ret){
				this_client->addr = (this_client->addr & (~0xFF)) |i2c_addr;
				printk("SENSOR_IO_I2C_ADDR: addr = %x, %x \n", i2c_addr, this_client->addr);
			}
		}
		break;

	case SENSOR_IO_I2C_READ:
		{
			SENSOR_REG_BITS_T reg;
			ret = copy_from_user(&reg, (SENSOR_REG_BITS_T *) arg, sizeof(SENSOR_REG_BITS_T));

			if(0 == ret){
				ret = _Sensor_K_ReadReg(&reg);
				if(SENSOR_K_FAIL != ret){
					ret = copy_to_user((SENSOR_REG_BITS_T *)arg, &reg, sizeof(SENSOR_REG_BITS_T));
				}
			}

		}
		break;

	case SENSOR_IO_I2C_WRITE:
		{
			SENSOR_REG_BITS_T reg;
			ret = copy_from_user(&reg, (SENSOR_REG_BITS_T *) arg, sizeof(SENSOR_REG_BITS_T));

			if(0 == ret){
				ret = _Sensor_K_WriteReg(&reg);
			}

		}
		break;

	case SENSOR_IO_SET_FLASH:
		{
			uint32_t flash_mode;
			ret = copy_from_user(&flash_mode, (uint32_t *) arg, sizeof(uint32_t));
			if(0 == ret)
				ret = _Sensor_K_SetFlash(flash_mode);
		}
		break;

	case SENSOR_IO_I2C_WRITE_REGS:
		{
			SENSOR_REG_TAB_T regTab;
			ret = copy_from_user(&regTab, (SENSOR_REG_TAB_T *) arg, sizeof(SENSOR_REG_TAB_T));
			if(0 == ret)
				ret = _Sensor_K_WriteRegTab(&regTab);
		}
		break;

	case SENSOR_IO_SET_I2CCLOCK:
		{
			uint32_t clock;
			ret = copy_from_user(&clock, (uint32_t *) arg, sizeof(uint32_t));
			if(0 == ret){
				_Sensor_K_SetI2CClock(clock);
			}
		}
		break;
	case SENSOR_IO_I2C_WRITE_EXT:
		{
			SENSOR_I2C_T i2cTab;
			ret = copy_from_user(&i2cTab, (SENSOR_I2C_T *) arg, sizeof(SENSOR_I2C_T));
			if(0 == ret)
				ret = _Sensor_K_WriteI2C(&i2cTab);	

		}
		break;

	default:
		SENSOR_PRINT("sensor_k_ioctl: invalid command %x  \n", cmd);
		break;

	}


	mutex_unlock(&sensor_lock);

	return (long)ret;
}


LOCAL struct file_operations sensor_fops = {
	.owner = THIS_MODULE,
	.open = sensor_k_open,
	.read = sensor_k_read,
	.write = sensor_k_write,
	.unlocked_ioctl = sensor_k_ioctl,
	.release = sensor_k_release,
};

LOCAL struct miscdevice sensor_dev = {
	.minor = SENSOR_MINOR,
	.name = "sprd_sensor",
	.fops = &sensor_fops,
};

LOCAL struct i2c_driver sensor_i2c_driver;
int sensor_k_probe(struct platform_device *pdev)
{
	int ret;

	printk(KERN_ALERT "sensor_k_probe called\n");

	ret = misc_register(&sensor_dev);
	if (ret) {
		printk(KERN_ERR "cannot register miscdev on minor=%d (%d)\n",
		       SENSOR_MINOR, ret);
		return ret;
	}
	init_waitqueue_head(&wait_queue_sensor);
	memset(&sensor_i2c_driver, 0, sizeof(struct i2c_driver));
	sensor_i2c_driver.driver.owner = THIS_MODULE;
	sensor_i2c_driver.probe  = sensor_probe;
	sensor_i2c_driver.remove = sensor_remove;
	sensor_i2c_driver.detect = sensor_detect;
	sensor_i2c_driver.driver.name = SENSOR_MAIN_I2C_NAME;
	sensor_i2c_driver.id_table = sensor_device_id;
	sensor_i2c_driver.address_list = &sensor_main_default_addr_list[0];

	ret = i2c_add_driver(&sensor_i2c_driver);
	if (ret) {
		SENSOR_PRINT_ERR("+I2C err %d.\n", ret);
		return SENSOR_K_FAIL;
	} else {
		SENSOR_PRINT_HIGH("+I2C OK \n");
	}
	printk(KERN_ALERT " sensor_k_probe Success\n");
	return 0;
}

LOCAL int sensor_k_remove(struct platform_device *dev)
{
	printk(KERN_INFO "sensor_k_remove called !\n");
	misc_deregister(&sensor_dev);
	printk(KERN_INFO "sensor_k_remove Success !\n");
	return 0;
}

LOCAL struct platform_driver sensor_dev_driver = {
	.probe = sensor_k_probe,
	.remove =sensor_k_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "sprd_sensor",
		   },
};

int __init sensor_k_init(void)
{
	printk(KERN_INFO "sensor_k_init called !\n");
	if (platform_driver_register(&sensor_dev_driver) != 0) {
		printk("platform device register Failed \n");
		return SENSOR_K_FAIL;
	}
#ifdef CONFIG_ARCH_SC8830
	{
		// aiden fpga
		uint32_t bit_value;

		printk("aiden: sensor_k_init: start enable module and set clock  \n");
		// 0x60d0_000
		bit_value = BIT_6;
		REG_MWR(SPRD_MMAHB_BASE, bit_value, bit_value);  // CKG enable
		REG_OWR(SPRD_MMAHB_BASE, 3); // aiden fpga

		bit_value = BIT_1;
		REG_MWR(SPRD_MMAHB_BASE+0x4, bit_value, bit_value); // reset
		REG_MWR(SPRD_MMAHB_BASE+0x4, bit_value, 0x0);

		bit_value = BIT_2 | BIT_3 | BIT_7 | BIT_8;
		REG_MWR(SPRD_MMAHB_BASE+0x8, bit_value, bit_value); // ckg_cfg

		_sensor_set_ldo();
		bit_value = BIT_4 | BIT_5;
		REG_MWR(SPRD_PIN_BASE + 0x368, bit_value, 0);  // ccir mclk pin

		REG_MWR(SPRD_MMCKG_BASE + 0x24, 0xfff, 0x101);  // sensor clock

		bit_value = BIT_18;
		REG_MWR(SPRD_APBREG_BASE, bit_value, bit_value);  // ccir clock enable
		
		REG_MWR(SPRD_DCAM_BASE+0x144, 0xFF, 0x0f);
		REG_MWR(SPRD_DCAM_BASE+0x144, 0xFF, 0xF0);

		//REG_MWR(SPRD_MMCKG_BASE + 0x20, 0xfff, 0x3);  	// MM AHB clock
		printk("aiden: sensor_k_init: end \n");
	}
#endif
	init_MUTEX(&g_sem_sensor);
	mutex_init(&sensor_lock);
	return 0;
}

void sensor_k_exit(void)
{
	printk(KERN_INFO "sensor_k_exit called !\n");
	platform_driver_unregister(&sensor_dev_driver);
}

module_init(sensor_k_init);
module_exit(sensor_k_exit);

MODULE_DESCRIPTION("Sensor Driver");
MODULE_LICENSE("GPL");
