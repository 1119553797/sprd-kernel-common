/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *************************************************
 * Automatically generated C header: do not edit *
 *************************************************
 */

#ifndef __REGS_AHB_H__
#define __REGS_AHB_H__

#define REGS_AHB

/* registers definitions for controller REGS_AHB */
#define REG_AHB_AHB_CTL0                SCI_ADDR(REGS_AHB_BASE, 0x0000)
#define REG_AHB_AHB_CTL1                SCI_ADDR(REGS_AHB_BASE, 0x0004)
#define REG_AHB_AHB_CTL2                SCI_ADDR(REGS_AHB_BASE, 0x0008)
#define REG_AHB_AHB_CTL3                SCI_ADDR(REGS_AHB_BASE, 0x000c)
#define REG_AHB_SOFT_RST                SCI_ADDR(REGS_AHB_BASE, 0x0010)
#define REG_AHB_AHB_PAUSE               SCI_ADDR(REGS_AHB_BASE, 0x0014)
#define REG_AHB_REMAP                   SCI_ADDR(REGS_AHB_BASE, 0x0018)
#define REG_AHB_MIPI_PHY_CTRL           SCI_ADDR(REGS_AHB_BASE, 0x001c)
#define REG_AHB_DISPC_CTRL              SCI_ADDR(REGS_AHB_BASE, 0x0020)
#define REG_AHB_ARM_CLK                 SCI_ADDR(REGS_AHB_BASE, 0x0024)
#define REG_AHB_AHB_SDIO_CTRL           SCI_ADDR(REGS_AHB_BASE, 0x0028)
#define REG_AHB_AHB_CTL4                SCI_ADDR(REGS_AHB_BASE, 0x002c)
#define REG_AHB_AHB_CTL5                SCI_ADDR(REGS_AHB_BASE, 0x0030)
#define REG_AHB_AHB_STATUS              SCI_ADDR(REGS_AHB_BASE, 0x0034)
#define REG_AHB_CA5_CFG                 SCI_ADDR(REGS_AHB_BASE, 0x0038)
#define REG_AHB_ISP_CTRL                SCI_ADDR(REGS_AHB_BASE, 0x003c)
#define REG_AHB_CHIP_ID                 SCI_ADDR(REGS_AHB_BASE, 0x01fc)

/* bits definitions for register REG_AHB_AHB_CTL0 */
#define BIT_AXIBUSMON2_EB               ( BIT(31) )
#define BIT_AXIBUSMON1_EB               ( BIT(30) )
#define BIT_AXIBUSMON0_EB               ( BIT(29) )
#define BIT_EMC_EB                      ( BIT(28) )
#define BIT_AHB_ARCH_EB                 ( BIT(27) )
#define BIT_SPINLOCK_EB                 ( BIT(25) )
#define BIT_SDIO2_EB                    ( BIT(24) )
#define BIT_EMMC_EB                     ( BIT(23) )
#define BIT_DISPC_EB                    ( BIT(22) )
#define BIT_G3D_EB                      ( BIT(21) )
#define BIT_SDIO1_EB                    ( BIT(19) )
#define BIT_DRM_EB                      ( BIT(18) )
#define BIT_BUSMON4_EB                  ( BIT(17) )
#define BIT_BUSMON2_EB                  ( BIT(15) )
#define BIT_ROT_EB                      ( BIT(14) )
#define BIT_VSP_EB                      ( BIT(13) )
#define BIT_ISP_EB                      ( BIT(12) )
#define BIT_BUSMON1_EB                  ( BIT(11) )
#define BIT_DCAM_MIPI_EB                ( BIT(10) )
#define BIT_CCIR_EB                     ( BIT(9) )
#define BIT_NFC_EB                      ( BIT(8) )
#define BIT_BUSMON0_EB                  ( BIT(7) )
#define BIT_DMA_EB                      ( BIT(6) )
#define BIT_USBD_EB                     ( BIT(5) )
#define BIT_SDIO0_EB                    ( BIT(4) )
#define BIT_LCDC_EB                     ( BIT(3) )
#define BIT_CCIR_IN_EB                  ( BIT(2) )
#define BIT_DCAM_EB                     ( BIT(1) )

/* bits definitions for register REG_AHB_AHB_CTL1 */
#define BIT_ARM_DAHB_SLP_EN             ( BIT(16) )
#define BIT_MSTMTX_AUTO_GATE_EN         ( BIT(14) )
#define BIT_MCU_AUTO_GATE_EN            ( BIT(13) )
#define BIT_AHB_AUTO_GATE_EN            ( BIT(12) )
#define BIT_ARM_AUTO_GATE_EN            ( BIT(11) )
#define BIT_APB_FRC_SLEEP               ( BIT(10) )
#define BIT_EMC_CH_AUTO_GATE_EN         ( BIT(9) )
#define BIT_EMC_AUTO_GATE_EN            ( BIT(8) )

/* bits definitions for register REG_AHB_AHB_CTL2 */
#define BIT_DISPMTX_CLK_EN              ( BIT(11) )
#define BIT_MMMTX_CLK_EN                ( BIT(10) )
#define BIT_DISPC_CORE_CLK_EN           ( BIT(9) )
#define BIT_LCDC_CORE_CLK_EN            ( BIT(8) )
#define BIT_ISP_CORE_CLK_EN             ( BIT(7) )
#define BIT_VSP_CORE_CLK_EN             ( BIT(6) )
#define BIT_DCAM_CORE_CLK_EN            ( BIT(5) )
#define BITS_MCU_SHM0_CTRL(_x_)         ( (_x_) << 3 & (BIT(3)|BIT(4)) )

/* bits definitions for register REG_AHB_AHB_CTL3 */
#define BIT_CLK_ULPI_EN                 ( BIT(10) )
#define BIT_UTMI_SUSPEND_INV            ( BIT(9) )
#define BIT_UTMIFS_TX_EN_INV            ( BIT(8) )
#define BIT_CLK_UTMIFS_EN               ( BIT(7) )
#define BIT_CLK_USB_REF_EN              ( BIT(6) )
#define BIT_BUSMON_SEL1                 ( BIT(5) )
#define BIT_USB_M_HBIGENDIAN            ( BIT(2) )
#define BIT_USB_S_HBIGEIDIAN            ( BIT(1) )
#define BIT_CLK_USB_REF_SEL             ( BIT(0) )

/* bits definitions for register REG_AHB_SOFT_RST */
#define BIT_DISPMTX_SOFT_RST            ( BIT(31) )
#define BIT_MMMTX_SOFT_RST              ( BIT(30) )
#define BIT_CA5_CORE1_SOFT_RST          ( BIT(29) )
#define BIT_CA5_CORE0_SOFT_RST          ( BIT(28) )
#define BIT_MIPI_CSIHOST_SOFT_RST       ( BIT(27) )
#define BIT_MIPI_DSIHOST_SOFT_RST       ( BIT(26) )
#define BIT_SPINLOCK_SOFT_RST           ( BIT(25) )
#define BIT_CAM1_SOFT_RST               ( BIT(24) )
#define BIT_CAM0_SOFT_RST               ( BIT(23) )
#define BIT_SD2_SOFT_RST                ( BIT(22) )
#define BIT_EMMC_SOFT_RST               ( BIT(21) )
#define BIT_DISPC_SOFT_RST              ( BIT(20) )
#define BIT_G3D_SOFT_RST                ( BIT(19) )
#define BIT_DBG_SOFT_RST                ( BIT(18) )
#define BIT_SD1_SOFT_RST                ( BIT(16) )
#define BIT_VSP_SOFT_RST                ( BIT(15) )
#define BIT_ADC_SOFT_RST                ( BIT(14) )
#define BIT_DRM_SOFT_RST                ( BIT(13) )
#define BIT_SD0_SOFT_RST                ( BIT(12) )
#define BIT_EMC_SOFT_RST                ( BIT(11) )
#define BIT_ROT_SOFT_RST                ( BIT(10) )
#define BIT_ISP_SOFT_RST                ( BIT(8) )
#define BIT_USBPHY_SOFT_RST             ( BIT(7) )
#define BIT_USBD_UTMI_SOFT_RST          ( BIT(6) )
#define BIT_NFC_SOFT_RST                ( BIT(5) )
#define BIT_LCDC_SOFT_RST               ( BIT(3) )
#define BIT_CCIR_SOFT_RST               ( BIT(2) )
#define BIT_DCAM_SOFT_RST               ( BIT(1) )
#define BIT_DMA_SOFT_RST                ( BIT(0) )

/* bits definitions for register REG_AHB_AHB_PAUSE */
#define BIT_MCU_DEEP_SLP_EN             ( BIT(2) )
#define BIT_MCU_SYS_SLP_EN              ( BIT(1) )
#define BIT_MCU_CORE_FRC_SLP            ( BIT(0) )

/* bits definitions for register REG_AHB_REMAP */
#define BITS_ARM_RES_STRAPPIN(_x_)      ( (_x_) << 30 & (BIT(30)|BIT(31)) )
#define BIT_FUNC_TEST_MODE              ( BIT(7) )
#define BIT_ARM_BOOT_MD3                ( BIT(6) )
#define BIT_ARM_BOOT_MD2                ( BIT(5) )
#define BIT_ARM_BOOT_MD1                ( BIT(4) )
#define BIT_ARM_BOOT_MD0                ( BIT(3) )
#define BIT_USB_DLOAD_EN                ( BIT(2) )
#define BIT_REMAP                       ( BIT(0) )

/* bits definitions for register REG_AHB_MIPI_PHY_CTRL */
#define BIT_MIPI_CPHY_EN                ( BIT(1) )
#define BIT_MIPI_DPHY_EN                ( BIT(0) )

/* bits definitions for register REG_AHB_DISPC_CTRL */
#define BITS_CLK_DISPC_DPI_DIV(_x_)     ( (_x_) << 19 & (BIT(19)|BIT(20)|BIT(21)|BIT(22)|BIT(23)|BIT(24)|BIT(25)|BIT(26)) )
#define BITS_CLK_DISPC_DPIPLL_SEL(_x_)  ( (_x_) << 17 & (BIT(17)|BIT(18)) )
#define BITS_CLK_DISPC_DBI_DIV(_x_)     ( (_x_) << 11 & (BIT(11)|BIT(12)|BIT(13)) )
#define BITS_CLK_DISPC_DBIPLL_SEL(_x_)  ( (_x_) << 9 & (BIT(9)|BIT(10)) )
#define BITS_CLK_DISPC_DIV(_x_)         ( (_x_) << 3 & (BIT(3)|BIT(4)|BIT(5)) )
#define BITS_CLK_DISPC_PLL_SEL(_x_)     ( (_x_) << 1 & (BIT(1)|BIT(2)) )

/* bits definitions for register REG_AHB_ARM_CLK */
#define BITS_AHB_DIV_INUSE(_x_)         ( (_x_) << 27 & (BIT(27)|BIT(28)|BIT(29)) )
#define BIT_AHB_ERR_YET                 ( BIT(26) )
#define BIT_AHB_ERR_CLR                 ( BIT(25) )
#define BITS_CLK_MCU_SEL(_x_)           ( (_x_) << 23 & (BIT(23)|BIT(24)) )
#define BITS_CLK_ARM_PERI_DIV(_x_)      ( (_x_) << 20 & (BIT(20)|BIT(21)|BIT(22)) )
#define BITS_CLK_DBG_DIV(_x_)           ( (_x_) << 14 & (BIT(14)|BIT(15)|BIT(16)|BIT(17)|BIT(18)|BIT(19)) )
#define BITS_CLK_EMC_SEL(_x_)           ( (_x_) << 12 & (BIT(12)|BIT(13)) )
#define BITS_CLK_EMC_DIV(_x_)           ( (_x_) << 8 & (BIT(8)|BIT(9)|BIT(10)|BIT(11)) )
#define BITS_CLK_AHB_DIV(_x_)           ( (_x_) << 4 & (BIT(4)|BIT(5)|BIT(6)) )
#define BIT_CLK_EMC_SYNC_SEL            ( BIT(3) )
#define BITS_CLK_ARM_DIV(_x_)           ( (_x_) << 0 & (BIT(0)|BIT(1)|BIT(2)) )

/* bits definitions for register REG_AHB_AHB_SDIO_CTRL */
#define BIT_EMMC_SLOT_SEL               ( BIT(5) )
#define BIT_SDIO2_SLOT_SEL              ( BIT(4) )
#define BITS_SDIO1_SLOT_SEL(_x_)        ( (_x_) << 2 & (BIT(2)|BIT(3)) )
#define BITS_SDIO0_SLOT_SEL(_x_)        ( (_x_) << 0 & (BIT(0)|BIT(1)) )

/* bits definitions for register REG_AHB_AHB_CTL4 */
#define BIT_RX_CLK_SEL_ARM              ( BIT(31) )
#define BIT_RX_CLK_INV_ARM              ( BIT(30) )
#define BIT_RX_INV                      ( BIT(29) )

/* bits definitions for register REG_AHB_AHB_CTL5 */
#define BIT_BUSMON4_BIGEND_EN           ( BIT(17) )
#define BIT_BUSMON3_BIGEND_EN           ( BIT(16) )
#define BIT_BUSMON2_BIGEND_EN           ( BIT(15) )
#define BIT_EMMC_BIGEND_EN              ( BIT(14) )
#define BIT_SDIO2_BIGEND_EN             ( BIT(13) )
#define BIT_DISPC_BIGEND_EN             ( BIT(12) )
#define BIT_SDIO1_BIGEND_EN             ( BIT(11) )
#define BIT_SHRAM0_BIGEND_EN            ( BIT(9) )
#define BIT_BUSMON1_BIGEND_EN           ( BIT(8) )
#define BIT_BUSMON0_BIGEND_EN           ( BIT(7) )
#define BIT_ROT_BIGEND_EN               ( BIT(6) )
#define BIT_VSP_BIGEND_EN               ( BIT(5) )
#define BIT_DCAM_BIGEND_EN              ( BIT(4) )
#define BIT_SDIO0_BIGEND_EN             ( BIT(3) )
#define BIT_LCDC_BIGEND_EN              ( BIT(2) )
#define BIT_NFC_BIGEND_EN               ( BIT(1) )
#define BIT_DMA_GIGEND_EN               ( BIT(0) )

/* bits definitions for register REG_AHB_AHB_STATUS */
#define BIT_APB_PERI_EN                 ( BIT(20) )
#define BIT_DSP_MAHB_SLP_EN             ( BIT(19) )
#define BIT_DMA_BUSY                    ( BIT(18) )
#define BIT_EMC_SLEEP                   ( BIT(17) )
#define BIT_EMC_STOP                    ( BIT(16) )
#define BITS_EMC_CTL_STA(_x_)           ( (_x_) << 8 & (BIT(8)|BIT(9)|BIT(10)) )
#define BIT_EMC_STOP_CH7                ( BIT(7) )
#define BIT_EMC_STOP_CH6                ( BIT(6) )
#define BIT_EMC_STOP_CH5                ( BIT(5) )
#define BIT_EMC_STOP_CH4                ( BIT(4) )
#define BIT_EMC_STOP_CH3                ( BIT(3) )
#define BIT_EMC_STOP_CH2                ( BIT(2) )
#define BIT_EMC_STOP_CH1                ( BIT(1) )
#define BIT_EMC_STOP_CH0                ( BIT(0) )

/* bits definitions for register REG_AHB_CA5_CFG */
#define BIT_CA5_WDRESET_EN              ( BIT(18) )
#define BIT_CA5_TS_EN                   ( BIT(17) )
#define BIT_CA5_CORE1_GATE_EN           ( BIT(16) )
#define BIT_CA5_CFGSDISABLE             ( BIT(13) )
#define BITS_CA5_CLK_AXI_DIV(_x_)       ( (_x_) << 11 & (BIT(11)|BIT(12)) )
#define BIT_CA5_CLK_DBG_EN_SEL          ( BIT(10) )
#define BIT_CA5_CLK_DBG_EN              ( BIT(9) )
#define BIT_CA5_DBGEN                   ( BIT(8) )
#define BIT_CA5_NIDEN                   ( BIT(7) )
#define BIT_CA5_SPIDEN                  ( BIT(6) )
#define BIT_CA5_SPNIDEN                 ( BIT(5) )
#define BIT_CA5_CPI15DISABLE            ( BIT(4) )
#define BIT_CA5_TEINIT                  ( BIT(3) )
#define BIT_CA5_L1RSTDISABLE            ( BIT(2) )
#define BIT_CA5_L2CFGEND                ( BIT(1) )
#define BIT_CA5_L2SPNIDEN               ( BIT(0) )

/* bits definitions for register REG_AHB_ISP_CTRL */
#define BITS_CLK_ISP_DIV(_x_)           ( (_x_) << 2 & (BIT(2)|BIT(3)|BIT(4)) )
#define BITS_CLK_ISPPLL_SEL(_x_)        ( (_x_) << 0 & (BIT(0)|BIT(1)) )

/* bits definitions for register REG_AHB_CHIP_ID */
#define BITS_CHIP_ID(_x_)               ( (_x_) << 0 )

/* vars definitions for controller REGS_AHB */
#define REG_AHB_SET(A)                  ( A + 0x1000 )
#define REG_AHB_CLR(A)                  ( A + 0x2000 )

#endif //__REGS_AHB_H__
