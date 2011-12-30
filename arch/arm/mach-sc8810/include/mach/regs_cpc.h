/*
 * arch/arm/mach-sc8800s/include/mach/regs_cpc.h
 *
 * Chip Pin Control registers Definitions
 *
 * Copyright (C) 2010 Spreadtrum International Ltd.
 *
 * 2010-03-05: yingchun li <yingchun.li@spreadtrum.com>
 *            initial version
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef _SC8800H_REG_CPC_H_
#define _SC8800H_REG_CPC_H_
#include <mach/hardware.h>

#define PIN_CTL_BASE            		(SPRD_CPC_BASE)

#define PIN_CTL_REG					(SPRD_CPC_BASE + 0x0000)

#define PIN_MOD_PWMA				(SPRD_CPC_BASE + 0x03e0)

#define ANA_CPC_BASE			(SPRD_MISC_BASE + 0x180)
#define ANA_PIN_CTL_BASE		(ANA_CPC_BASE	+ 0x00)

#define PINMAP_REG_BASE     PIN_CTL_BASE
#define ANA_PINMAP_REG_BASE ANA_PIN_CTL_BASE
#define REG_ADDR(_x_)       (PINMAP_REG_BASE + _x_)
#define ANA_REG_ADDR(_x_)   (ANA_PINMAP_REG_BASE + _x_)

/*----------Digital Die Pin Control Register----------*/

//0x04 -- 0x88 internal use pin reg
#define PIN_SIMCLK0_REG                       REG_ADDR(0x008C)
#define PIN_SIMDA0_REG                        REG_ADDR(0x0090)
#define PIN_SIMRST0_REG                       REG_ADDR(0x0094)
#define PIN_SIMCLK1_REG                       REG_ADDR(0x0098)
#define PIN_SIMDA1_REG                        REG_ADDR(0x009C)
#define PIN_SIMRST1_REG                       REG_ADDR(0x00A0)
#define PIN_SD0_CLK_REG                       REG_ADDR(0x00A4)
#define PIN_SD_CMD_REG                        REG_ADDR(0x00A8)
#define PIN_SD_D0_REG                         REG_ADDR(0x00AC)
#define PIN_SD_D1_REG                         REG_ADDR(0x00B0)
#define PIN_SD_D2_REG                         REG_ADDR(0x00B4)
#define PIN_SD_D3_REG                         REG_ADDR(0x00B8)
#define PIN_SD1_CLK_REG                       REG_ADDR(0x00BC)
#define PIN_KEYOUT0_REG                       REG_ADDR(0x00C0)
#define PIN_KEYOUT1_REG                       REG_ADDR(0x00C4)
#define PIN_KEYOUT2_REG                       REG_ADDR(0x00C8)
#define PIN_KEYOUT3_REG                       REG_ADDR(0x00CC)
#define PIN_KEYOUT4_REG                       REG_ADDR(0x00D0)
#define PIN_KEYOUT5_REG                       REG_ADDR(0x00D4)
#define PIN_KEYOUT6_REG                       REG_ADDR(0x00D8)
#define PIN_KEYOUT7_REG                       REG_ADDR(0x00DC)
#define PIN_KEYIN0_REG                        REG_ADDR(0x00E0)
#define PIN_KEYIN1_REG                        REG_ADDR(0x00E4)
#define PIN_KEYIN2_REG                        REG_ADDR(0x00E8)
#define PIN_KEYIN3_REG                        REG_ADDR(0x00EC)
#define PIN_KEYIN4_REG                        REG_ADDR(0x00F0)
#define PIN_KEYIN5_REG                        REG_ADDR(0x00F4)
#define PIN_KEYIN6_REG                        REG_ADDR(0x00F8)
#define PIN_KEYIN7_REG                        REG_ADDR(0x00FC)
#define PIN_SPI_DI_REG                        REG_ADDR(0x0100)
#define PIN_SPI_CLK_REG                       REG_ADDR(0x0104)
#define PIN_SPI_DO_REG                        REG_ADDR(0x0108)
#define PIN_SPI_CSN0_REG                      REG_ADDR(0x010C)
#define PIN_SPI_CSN1_REG                      REG_ADDR(0x0110)
#define PIN_MTDO_REG                          REG_ADDR(0x0114)
#define PIN_MTDI_REG                          REG_ADDR(0x0118)
#define PIN_MTCK_REG                          REG_ADDR(0x011C)
#define PIN_MTMS_REG                          REG_ADDR(0x0120)
#define PIN_MTRST_N_REG                       REG_ADDR(0x0124)
#define PIN_U0TXD_REG                         REG_ADDR(0x0128)
#define PIN_U0RXD_REG                         REG_ADDR(0x012C)
#define PIN_U0CTS_REG                         REG_ADDR(0x0130)
#define PIN_U0RTS_REG                         REG_ADDR(0x0134)
#define PIN_U1TXD_REG                         REG_ADDR(0x0138)
#define PIN_U1RXD_REG                         REG_ADDR(0x013C)
#define PIN_NFWPN_REG                         REG_ADDR(0x0140)
#define PIN_NFRB_REG                          REG_ADDR(0x0144)
#define PIN_NFCLE_REG                         REG_ADDR(0x0148)
#define PIN_NFALE_REG                         REG_ADDR(0x014C)
#define PIN_NFCEN_REG                         REG_ADDR(0x0150)
#define PIN_NFWEN_REG                         REG_ADDR(0x0154)
#define PIN_NFREN_REG                         REG_ADDR(0x0158)
#define PIN_NFD0_REG                          REG_ADDR(0x015C)
#define PIN_NFD1_REG                          REG_ADDR(0x0160)
#define PIN_NFD2_REG                          REG_ADDR(0x0164)
#define PIN_NFD3_REG                          REG_ADDR(0x0168)
#define PIN_NFD4_REG                          REG_ADDR(0x016C)
#define PIN_NFD5_REG                          REG_ADDR(0x0170)
#define PIN_NFD6_REG                          REG_ADDR(0x0174)
#define PIN_NFD7_REG                          REG_ADDR(0x0178)
#define PIN_NFD8_REG                          REG_ADDR(0x017C)
#define PIN_NFD9_REG                          REG_ADDR(0x0180)
#define PIN_NFD10_REG                         REG_ADDR(0x0184)
#define PIN_NFD11_REG                         REG_ADDR(0x0188)
#define PIN_NFD12_REG                         REG_ADDR(0x018C)
#define PIN_NFD13_REG                         REG_ADDR(0x0190)
#define PIN_NFD14_REG                         REG_ADDR(0x0194)
#define PIN_NFD15_REG                         REG_ADDR(0x0198)
#define PIN_EMRST_N_REG                       REG_ADDR(0x019C)
#define PIN_EMA0_REG                          REG_ADDR(0x01A0)
#define PIN_EMA1_REG                          REG_ADDR(0x01A4)
#define PIN_EMA2_REG                          REG_ADDR(0x01A8)
#define PIN_EMA3_REG                          REG_ADDR(0x01AC)
#define PIN_EMA4_REG                          REG_ADDR(0x01B0)
#define PIN_EMA5_REG                          REG_ADDR(0x01B4)
#define PIN_EMA6_REG                          REG_ADDR(0x01B8)
#define PIN_EMA7_REG                          REG_ADDR(0x01BC)
#define PIN_EMA8_REG                          REG_ADDR(0x01C0)
#define PIN_EMA9_REG                          REG_ADDR(0x01C4)
#define PIN_EMA10_REG                         REG_ADDR(0x01C8)
#define PIN_EMA11_REG                         REG_ADDR(0x01CC)
#define PIN_EMA12_REG                         REG_ADDR(0x01D0)
#define PIN_EMA13_REG                         REG_ADDR(0x01D4)
#define PIN_EMCKE1_REG                        REG_ADDR(0x01D8)
#define PIN_EMD0_REG                          REG_ADDR(0x01DC)
#define PIN_EMD1_REG                          REG_ADDR(0x01E0)
#define PIN_EMD2_REG                          REG_ADDR(0x01E4)
#define PIN_EMD3_REG                          REG_ADDR(0x01E8)
#define PIN_EMD4_REG                          REG_ADDR(0x01EC)
#define PIN_EMD5_REG                          REG_ADDR(0x01F0)
#define PIN_EMD6_REG                          REG_ADDR(0x01F4)
#define PIN_EMD7_REG                          REG_ADDR(0x01F8)
#define PIN_EMDQM0_REG                        REG_ADDR(0x01FC)
#define PIN_EMDQS0_REG                        REG_ADDR(0x0200)
#define PIN_EMD8_REG                          REG_ADDR(0x0204)
#define PIN_EMD9_REG                          REG_ADDR(0x0208)
#define PIN_EMD10_REG                         REG_ADDR(0x020C)
#define PIN_EMD11_REG                         REG_ADDR(0x0210)
#define PIN_EMD12_REG                         REG_ADDR(0x0214)
#define PIN_EMD13_REG                         REG_ADDR(0x0218)
#define PIN_EMD14_REG                         REG_ADDR(0x021C)
#define PIN_EMD15_REG                         REG_ADDR(0x0220)
#define PIN_EMDQM1_REG                        REG_ADDR(0x0224)
#define PIN_EMDQS1_REG                        REG_ADDR(0x0228)
#define PIN_EMD16_REG                         REG_ADDR(0x022C)
#define PIN_EMD17_REG                         REG_ADDR(0x0230)
#define PIN_EMD18_REG                         REG_ADDR(0x0234)
#define PIN_EMD19_REG                         REG_ADDR(0x0238)
#define PIN_EMD20_REG                         REG_ADDR(0x023C)
#define PIN_EMD21_REG                         REG_ADDR(0x0240)
#define PIN_EMD22_REG                         REG_ADDR(0x0244)
#define PIN_EMD23_REG                         REG_ADDR(0x0248)
#define PIN_EMDQM2_REG                        REG_ADDR(0x024C)
#define PIN_EMDQS2_REG                        REG_ADDR(0x0250)
#define PIN_EMD24_REG                         REG_ADDR(0x0254)
#define PIN_EMD25_REG                         REG_ADDR(0x0258)
#define PIN_EMD26_REG                         REG_ADDR(0x025C)
#define PIN_EMD27_REG                         REG_ADDR(0x0260)
#define PIN_EMD28_REG                         REG_ADDR(0x0264)
#define PIN_EMD29_REG                         REG_ADDR(0x0268)
#define PIN_EMD30_REG                         REG_ADDR(0x026C)
#define PIN_EMD31_REG                         REG_ADDR(0x0270)
#define PIN_EMDQM3_REG                        REG_ADDR(0x0274)
#define PIN_EMDQS3_REG                        REG_ADDR(0x0278)
#define PIN_CLKDPMEM_REG                      REG_ADDR(0x027C)
#define PIN_CLKDMMEM_REG                      REG_ADDR(0x0280)
#define PIN_EMRAS_N_REG                       REG_ADDR(0x0284)
#define PIN_EMCAS_N_REG                       REG_ADDR(0x0288)
#define PIN_EMWE_N_REG                        REG_ADDR(0x028C)
#define PIN_EMCS_N0_REG                       REG_ADDR(0x0290)
#define PIN_EMCS_N1_REG                       REG_ADDR(0x0294)
#define PIN_EMGPRE_LOOP_REG                   REG_ADDR(0x0298)
#define PIN_EMGPST_LOOP_REG                   REG_ADDR(0x029C)
#define PIN_EMBA0_REG                         REG_ADDR(0x02A0)
#define PIN_EMBA1_REG                         REG_ADDR(0x02A4)
#define PIN_EMCKE0_REG                        REG_ADDR(0x02A8)
#define PIN_LCD_CSN1_REG                      REG_ADDR(0x02AC)
#define PIN_LCD_RSTN_REG                      REG_ADDR(0x02B0)
#define PIN_LCD_CD_REG                        REG_ADDR(0x02B4)
#define PIN_LCD_D0_REG                        REG_ADDR(0x02B8)
#define PIN_LCD_D1_REG                        REG_ADDR(0x02BC)
#define PIN_LCD_D2_REG                        REG_ADDR(0x02C0)
#define PIN_LCD_D3_REG                        REG_ADDR(0x02C4)
#define PIN_LCD_D4_REG                        REG_ADDR(0x02C8)
#define PIN_LCD_D5_REG                        REG_ADDR(0x02CC)
#define PIN_LCD_D6_REG                        REG_ADDR(0x02D0)
#define PIN_LCD_D7_REG                        REG_ADDR(0x02D4)
#define PIN_LCD_D8_REG                        REG_ADDR(0x02D8)
#define PIN_LCD_WRN_REG                       REG_ADDR(0x02DC)
#define PIN_LCD_RDN_REG                       REG_ADDR(0x02E0)
#define PIN_LCD_CSN0_REG                      REG_ADDR(0x02E4)
#define PIN_LCD_D9_REG                        REG_ADDR(0x02E8)
#define PIN_LCD_D10_REG                       REG_ADDR(0x02EC)
#define PIN_LCD_D11_REG                       REG_ADDR(0x02F0)
#define PIN_LCD_D12_REG                       REG_ADDR(0x02F4)
#define PIN_LCD_D13_REG                       REG_ADDR(0x02F8)
#define PIN_LCD_D14_REG                       REG_ADDR(0x02FC)
#define PIN_LCD_D15_REG                       REG_ADDR(0x0300)
#define PIN_LCD_D16_REG                       REG_ADDR(0x0304)
#define PIN_LCD_D17_REG                       REG_ADDR(0x0308)
#define PIN_LCD_FMARK_REG                     REG_ADDR(0x030C)
#define PIN_CCIRMCLK_REG                      REG_ADDR(0x0310)
#define PIN_CCIRCK_REG                        REG_ADDR(0x0314)
#define PIN_CCIRHS_REG                        REG_ADDR(0x0318)
#define PIN_CCIRVS_REG                        REG_ADDR(0x031C)
#define PIN_CCIRD0_REG                        REG_ADDR(0x0320)
#define PIN_CCIRD1_REG                        REG_ADDR(0x0324)
#define PIN_CCIRD2_REG                        REG_ADDR(0x0328)
#define PIN_CCIRD3_REG                        REG_ADDR(0x032C)
#define PIN_CCIRD4_REG                        REG_ADDR(0x0330)
#define PIN_CCIRD5_REG                        REG_ADDR(0x0334)
#define PIN_CCIRD6_REG                        REG_ADDR(0x0338)
#define PIN_CCIRD7_REG                        REG_ADDR(0x033C)
#define PIN_CCIRRST_REG                       REG_ADDR(0x0340)
#define PIN_CCIRPD1_REG                       REG_ADDR(0x0344)
#define PIN_CCIRPD0_REG                       REG_ADDR(0x0348)
#define PIN_SCL_REG                           REG_ADDR(0x034C)
#define PIN_SDA_REG                           REG_ADDR(0x0350)
#define PIN_CLK_AUX0_REG                      REG_ADDR(0x0354)
#define PIN_IISDI_REG                         REG_ADDR(0x0358)
#define PIN_IISDO_REG                         REG_ADDR(0x035C)
#define PIN_IISCLK_REG                        REG_ADDR(0x0360)
#define PIN_IISLRCK_REG                       REG_ADDR(0x0364)
#define PIN_IISMCK_REG                        REG_ADDR(0x0368)
#define PIN_RFSDA0_REG                        REG_ADDR(0x036C)
#define PIN_RFSCK0_REG                        REG_ADDR(0x0370)
#define PIN_RFSEN0_REG                        REG_ADDR(0x0374)
#define PIN_RFCTL0_REG                        REG_ADDR(0x0378)
#define PIN_RFCTL1_REG                        REG_ADDR(0x037C)
#define PIN_RFCTL2_REG                        REG_ADDR(0x0380)
#define PIN_RFCTL3_REG                        REG_ADDR(0x0384)
#define PIN_RFCTL4_REG                        REG_ADDR(0x0388)
#define PIN_RFCTL5_REG                        REG_ADDR(0x038C)
#define PIN_RFCTL6_REG                        REG_ADDR(0x0390)
#define PIN_RFCTL7_REG                        REG_ADDR(0x0394)
#define PIN_RFCTL8_REG                        REG_ADDR(0x0398)
#define PIN_RFCTL9_REG                        REG_ADDR(0x039C)
#define PIN_RFCTL10_REG                       REG_ADDR(0x03A0)
#define PIN_RFCTL11_REG                       REG_ADDR(0x03A4)
#define PIN_RFCTL12_REG                       REG_ADDR(0x03A8)
#define PIN_RFCTL13_REG                       REG_ADDR(0x03AC)
#define PIN_RFCTL14_REG                       REG_ADDR(0x03B0)
#define PIN_RFCTL15_REG                       REG_ADDR(0x03B4)
#define PIN_XTL_EN_REG                        REG_ADDR(0x03B8)
#define PIN_PTEST_REG                         REG_ADDR(0x03BC)
#define PIN_GPIO135_REG                       REG_ADDR(0x03C0)
#define PIN_GPIO136_REG                       REG_ADDR(0x03C4)
#define PIN_GPIO137_REG                       REG_ADDR(0x03C8)
#define PIN_GPIO138_REG                       REG_ADDR(0x03CC)
#define PIN_GPIO139_REG                       REG_ADDR(0x03D0)
#define PIN_GPIO140_REG                       REG_ADDR(0x03D4)
#define PIN_GPIO141_REG                       REG_ADDR(0x03D8)
#define PIN_GPIO142_REG                       REG_ADDR(0x03DC)
#define PIN_GPIO143_REG                       REG_ADDR(0x03E0)
#define PIN_GPIO144_REG                       REG_ADDR(0x03E4)
#define PIN_SD2_CLK_REG                       REG_ADDR(0x03E8)
#define PIN_SD2_CMD_REG                       REG_ADDR(0x03EC)
#define PIN_SD2_D0_REG                        REG_ADDR(0x03F0)
#define PIN_SD2_D1_REG                        REG_ADDR(0x03F4)
#define PIN_SD2_D2_REG                        REG_ADDR(0x03F8)
#define PIN_SD2_D3_REG                        REG_ADDR(0x03FC)
#define PIN_U2TXD_REG                         REG_ADDR(0x0400)
#define PIN_U2RXD_REG                         REG_ADDR(0x0404)
#define PIN_NFCEN1_REG                        REG_ADDR(0x0408)
#define PIN_SCL0_REG                          REG_ADDR(0x040C)
#define PIN_SDA0_REG                          REG_ADDR(0x0410)
#define PIN_SCL2_REG                          REG_ADDR(0x0414)
#define PIN_SDA2_REG                          REG_ADDR(0x0418)
#define PIN_SCL3_REG                          REG_ADDR(0x041C)
#define PIN_SDA3_REG                          REG_ADDR(0x0420)
#define PIN_LCD_D18_REG                       REG_ADDR(0x0424)
#define PIN_LCD_D19_REG                       REG_ADDR(0x0428)
#define PIN_LCD_D20_REG                       REG_ADDR(0x042C)
#define PIN_LCD_D21_REG                       REG_ADDR(0x0430)
#define PIN_LCD_D22_REG                       REG_ADDR(0x0434)
#define PIN_LCD_D23_REG                       REG_ADDR(0x0438)
#define PIN_TRACECLK_REG                      REG_ADDR(0x043C)
#define PIN_TRACECTRL_REG                     REG_ADDR(0x0440)
#define PIN_TRACEDAT0_REG                     REG_ADDR(0x0444)
#define PIN_TRACEDAT1_REG                     REG_ADDR(0x0448)
#define PIN_TRACEDAT2_REG                     REG_ADDR(0x044C)
#define PIN_TRACEDAT3_REG                     REG_ADDR(0x0450)
#define PIN_TRACEDAT4_REG                     REG_ADDR(0x0454)
#define PIN_TRACEDAT5_REG                     REG_ADDR(0x0458)
#define PIN_TRACEDAT6_REG                     REG_ADDR(0x045C)
#define PIN_TRACEDAT7_REG                     REG_ADDR(0x0460)
#define PIN_SIMCLK2_REG                       REG_ADDR(0x0464)
#define PIN_SIMDA2_REG                        REG_ADDR(0x0468)
#define PIN_SIMRST2_REG                       REG_ADDR(0x046C)
#define PIN_SIMCLK3_REG                       REG_ADDR(0x0470)
#define PIN_SIMDA3_REG                        REG_ADDR(0x0474)
#define PIN_SIMRST3_REG                       REG_ADDR(0x0478)

/*----------Analog Die Pin Control Register----------*/

#define ANA_PIN_TESTRSTN_REG		ANA_REG_ADDR(0x008C)
#define ANA_PIN_PBINT_REG			ANA_REG_ADDR(0x0094)
#define ANA_PIN_TP_XL_REG			ANA_REG_ADDR(0x0098)
#define ANA_PIN_TP_XR_REG			ANA_REG_ADDR(0x009C)
#define ANA_PIN_TP_YU_REG			ANA_REG_ADDR(0x00A0)
#define ANA_PIN_TP_YD_REG			ANA_REG_ADDR(0x00A4)

#define PIN_Z_EN                0x00            // High-Z in sleep mode
#define PIN_O_EN                BIT_0           // Output enable in sleep mode
#define PIN_I_EN                BIT_1           // Input enable in sleep mode
#define PIN_SPD_EN              BIT_2           // Pull down enable for sleep mode
#define PIN_SPU_EN              BIT_3           // Pull up enable for sleep mode
#define PIN_SPX_EN              0x00            // Don't pull down or up
#define PIN_FUNC_DEF            (0x00<<4)       //Function select,BIT4-5
#define PIN_FUNC_1              (0x01<<4)
#define PIN_FUNC_2              (0x02<<4)
#define PIN_FUNC_3              (0x03<<4)
#define PIN_FPD_EN          BIT_6           // Weak pull down for function mode
#define PIN_FPU_EN          BIT_7           // Weak pull up for function mode
#define PIN_FPX_EN          0x00            // Don't pull down or up
#define PIN_DS_0                (0x00<<8)           // Driver strength level 0 BIT8-9
#define PIN_DS_1                (0x01<<8)           // Driver strength level 1 BIT8-9
#define PIN_DS_2                (0x02<<8)           // Driver strength level 2 BIT8-9
#define PIN_DS_3                (0x03<<8)           // Driver strength level 3 BIT8-9
#define ANA_PIN_Z_EN                0x00            // High-Z in sleep mode
#define ANA_PIN_O_EN                BIT_0           // Output enable in sleep mode
#define ANA_PIN_I_EN                BIT_1           // Input enable in sleep mode
#define ANA_PIN_PD_EN               BIT_2           // Pull down enable
#define ANA_PIN_PU_EN               BIT_3           // Pull up enable
#define ANA_PIN_PX_EN               0x00            // Don't pull down or up
#define ANA_PIN_FUNC_DEF            (0x00<<4)       //Function select,BIT4-5
#define ANA_PIN_FUNC_1              (0x01<<4)
#define ANA_PIN_FUNC_2              (0x02<<4)
#define ANA_PIN_FUNC_3              (0x03<<4)
#define ANA_PIN_FUN_PD_EN           BIT_6           // Weak pull down for function mode
#define ANA_PIN_FUN_PU_EN           BIT_7           // Weak pull up for function mode
#define ANA_PIN_FUN_PX_EN           0x00            // Don't pull down or up
#define ANA_PIN_DS_0                (0x00<<8)           // Driver strength level 0 BIT8-9
#define ANA_PIN_DS_1                (0x01<<8)           // Driver strength level 1 BIT8-9
#define ANA_PIN_DS_2                (0x02<<8)           // Driver strength level 2 BIT8-9
#define ANA_PIN_DS_3                (0x03<<8)           // Driver strength level 3 BIT8-9
#define PM_INVALID_VAL        0xffffffff
#define PM_INVALID_SHORT_VAL  0xffff
typedef unsigned int uint32;
typedef unsigned short uint16;

typedef struct PM_PINFUNC_tag {
	uint32 addr;
	uint32 value;
} PM_PINFUNC_T;

typedef enum {
	PM_OUTPUT,
	PM_INPUT,
	PM_INVALID_DIR
} PM_DIR_E;

typedef enum {
	PM_LEVEL,
	PM_RISING_EDGE,
	PM_FALLING_EDGE,
	PM_BOTH_EDGE,
	PM_NO_INT,
	PM_INVALID_INT
} PM_IS_E;

typedef struct GPIO_CTL_tag {
	uint16 gpio_num;
	uint16 default_val;
	PM_DIR_E dir;
	PM_IS_E int_sense;
} PM_GPIO_CTL_T;

extern const PM_PINFUNC_T pm_func[];
extern const PM_PINFUNC_T pm_default_global_map[];
extern const PM_GPIO_CTL_T pm_gpio_default_map[];

#define CHIP_REG_OR(reg_addr, value)   do{ \
                                             unsigned long flags;   \
                                             hw_local_irq_save(flags);  \
                                             (*(volatile unsigned int *)(reg_addr) |= (unsigned int)(value));   \
                                             hw_local_irq_restore(flags);   \
                                       }while(0)
#define CHIP_REG_AND(reg_addr, value)   do{ \
                                             unsigned long flags;   \
                                             hw_local_irq_save(flags);  \
                                             (*(volatile unsigned int *)(reg_addr) &= (unsigned int)(value)); \
                                             hw_local_irq_restore(flags);   \
                                       }while(0)
#define CHIP_REG_GET(reg_addr)          (*(volatile unsigned int *)(reg_addr))
#define CHIP_REG_SET(reg_addr, value)   do{ \
                                             unsigned long flags;   \
                                             hw_local_irq_save(flags);  \
                                             (*(volatile unsigned int *)(reg_addr)  = (unsigned int)(value)) ;   \
                                             hw_local_irq_restore(flags);   \
                                       }while(0)

#define SCI_ASSERT(condition) BUG_ON(!(condition))
#define SCI_PASSERT(condition, format...)  \
	do {		\
		if(!(condition)) { \
			pr_err("function :%s\r\n", __FUNCTION__);\
			BUG();	\
		} \
	}while(0)

#endif

