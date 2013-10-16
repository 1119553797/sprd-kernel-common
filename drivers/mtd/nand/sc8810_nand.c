/* linux/drivers/mtd/nand/sprd_nand.c
 *
 * Copyright (c) 2010 Spreadtrun.
 *
 * Spreadtrun NAND driver

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/cpufreq.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <mach/pm_devices.h>
#include <linux/wakelock.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>
#include <asm/io.h>
#include <mach/regs_ahb.h>
#include <mach/mfp.h>
#include <mach/dma.h>
#if (defined CONFIG_ARCH_SC8810)
#include "sc8810_nfc.h"
#endif

#define NAND_IRQ_EN
#ifdef  NAND_IRQ_EN
#include <linux/completion.h>
#include <mach/irqs.h>
#define DRIVER_NAME "sc8810_nand"
#define IRQ_TIMEOUT  100//unit:ms,IRQ timeout value
static enum NAND_ERR_CORRECT_S ret_irq_en = NAND_NO_ERROR;
#endif

static unsigned long nand_base = 0x60000000;
static char *nandflash_cmdline;
static int nandflash_parsed = 0;
static struct wake_lock nfc_wakelock;

struct sprd_nand_info {
	unsigned long			phys_base;
	struct sprd_platform_nand	*platform;
	struct clk			*clk;
	struct mtd_info			mtd;
	struct platform_device		*pdev;
};

static unsigned long nand_func_cfg8[] = {
#if ( defined CONFIG_ARCH_SC8810)
	MFP_CFG_X(NFWPN, AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
	MFP_CFG_X(NFRB,  AF0, DS1, F_PULL_UP,   S_PULL_UP,   IO_Z),
	MFP_CFG_X(NFCLE, AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
	MFP_CFG_X(NFALE, AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
	MFP_CFG_X(NFCEN, AF0, DS1, F_PULL_NONE, S_PULL_UP,   IO_Z),
	MFP_CFG_X(NFWEN, AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
	MFP_CFG_X(NFREN, AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
	MFP_CFG_X(NFD0,  AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
	MFP_CFG_X(NFD1,  AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
	MFP_CFG_X(NFD2,  AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
	MFP_CFG_X(NFD3,  AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
	MFP_CFG_X(NFD4,  AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
	MFP_CFG_X(NFD5,  AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
	MFP_CFG_X(NFD6,  AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
	MFP_CFG_X(NFD7,  AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
#endif
};

static unsigned long nand_func_cfg16[] = {
#if (defined CONFIG_ARCH_SC8810)
	MFP_CFG_X(NFD8,  AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
	MFP_CFG_X(NFD9,  AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
	MFP_CFG_X(NFD10,  AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
	MFP_CFG_X(NFD11,  AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
	MFP_CFG_X(NFD12,  AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
	MFP_CFG_X(NFD13,  AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
	MFP_CFG_X(NFD14,  AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
	MFP_CFG_X(NFD15,  AF0, DS1, F_PULL_NONE, S_PULL_DOWN, IO_Z),
#endif
};

static void sprd_config_nand_pins8(void)
{
	sprd_mfp_config(nand_func_cfg8, ARRAY_SIZE(nand_func_cfg8));
}

static void sprd_config_nand_pins16(void)
{
	sprd_mfp_config(nand_func_cfg16, ARRAY_SIZE(nand_func_cfg16));
}

/* Size of the block protected by one OOB (Spare Area in Samsung terminology) */
#define CONFIG_SYS_NAND_ECCSIZE	512
/* Number of ECC bytes per OOB - S3C6400 calculates 4 bytes ECC in 1-bit mode */
#define CONFIG_SYS_NAND_ECCBYTES	4
/* Number of ECC-blocks per NAND page */
/* 2 bit correct, sc8810 support 1, 2, 4, 8, 12,14, 24 */
#define CONFIG_SYS_NAND_ECC_MODE	2
/* Size of a single OOB region */
/* Number of ECC bytes per page */
/* ECC byte positions */

#define NFC_CMD_ENCODE		(0x0000ffff)
#define NFC_CMD_DECODE		(NFC_CMD_ENCODE + 1)

#define	NFC_ECC_EVENT  		1
#define	NFC_DONE_EVENT		2
#define	NFC_TX_DMA_EVENT	4
#define	NFC_RX_DMA_EVENT	8
#define	NFC_ERR_EVENT		16
#define	NFC_TIMEOUT_EVENT	32

/* #define NFC_TIMEOUT_VAL		(0x1000000) */
#define NFC_TIMEOUT_VAL		(0xf0000)
#define NFC_ECCENCODE_TIMEOUT	(0xfff)
#define NFC_ECCDECODE_TIMEOUT	(0xfff)
#define NFC_RESET_TIMEOUT	(0x1ff)
#define NFC_STATUS_TIMEOUT	(0x1ff)
#define NFC_READID_TIMEOUT	(0x1ff)
#define NFC_ERASE_TIMEOUT	(0x10000)
#define NFC_READ_TIMEOUT	(0x8000)
#define NFC_WRITE_TIMEOUT	(0xc000)

struct sc8810_nand_timing_param {
	u8 acs_time;
	u8 rwh_time;
	u8 rwl_time;
	u8 acr_time;
	u8 rr_time;
	u8 ceh_time;
};

struct sc8810_nand_info {
	struct clk	*clk;
	struct nand_chip *chip;
	unsigned int cfg0_setting;
	unsigned int ecc0_cfg_setting;
	unsigned int ecc1_cfg_setting;
	u8 	asy_cle; //address cycles, can be set 3, 4, 5
	u8	advance;// advance property, can be set 0, 1
	u8	bus_width; //bus width, can be 0 or 1
	u8	ecc_mode; // ecc mode can be 1, 2, 4, 8, 12, 16,24
	u8  	mc_ins_num; // micro instruction number
	u8	mc_addr_ins_num; //micro address instruction number
	u16	ecc_postion; //ecc postion
	u16 	b_pointer; // nfc buffer pointer
	u16 	addr_array[5];// the addrss of the flash to operation

};

struct sc8810_nand_page_oob {
	unsigned char m_c;
	unsigned char d_c;
	unsigned char cyc_3;
	unsigned char cyc_4;
	unsigned char cyc_5;
	int pagesize;
	int oobsize; /* total oob size */
	int eccsize; /* per ??? bytes data for ecc calcuate once time */
	int eccbit; /* ecc level per eccsize */
};

struct nand_spec_str{
    u8          mid;
    u8          did;
    u8          id3;
    u8          id4;
    u8          id5;
    struct sc8810_nand_timing_param timing_cfg;
};

#ifdef  NAND_IRQ_EN
static struct completion  sc8810_op_completion;
static enum NAND_HANDLE_STATUS_S  handle_status=NAND_HANDLE_DONE;
#endif

#define NF_MC_CMD_ID	(0xFD)
#define NF_MC_ADDR_ID	(0xF1)
#define NF_MC_WAIT_ID	(0xF2)
#define NF_MC_RWORD_ID	(0xF3)
#define NF_MC_RBLK_ID	(0xF4)
#define NF_MC_WWORD_ID	(0xF6)
#define NF_MC_WBLK_ID	(0xF7)
#define NF_MC_DEACTV_ID	(0xF9)
#define NF_MC_NOP_ID	(0xFA)
#define NF_PARA_20M        	0x7ac05      //trwl = 0  trwh = 0
#define NF_PARA_40M        	0x7ac15      //trwl = 1  trwh = 0
#define NF_PARA_53M        	0x7ad26      //trwl = 2  trwh = 1
#define NF_PARA_80M        	0x7ad37      //trwl = 3  trwh = 1
#define NF_PARA_DEFAULT    	0x7ad77      //trwl = 7  trwh = 1

#define REG_AHB_CTL0		       		(*((volatile unsigned int *)(AHB_CTL0)))
#define REG_AHB_SOFT_RST				(*((volatile unsigned int *)(AHB_SOFT_RST)))

#define REG_GR_NFC_MEM_DLY                      (*((volatile unsigned int *)(GR_NFC_MEM_DLY)))

static struct sc8810_nand_page_oob nand_config_item;
static unsigned char fix_timeout_id[8];
static unsigned long fix_timeout_reg[8];

static const struct nand_spec_str nand_spec_table[] = {
    {0x2c, 0xb3, 0xd1, 0x55, 0x5a, {10, 10, 12, 10, 20, 50}},// MT29C8G96MAAFBACKD-5, MT29C4G96MAAHBACKD-5
    {0x2c, 0xba, 0x80, 0x55, 0x50, {10, 10, 12, 10, 20, 50}},// MT29C2G48MAKLCJA-5 IT
    {0x2c, 0xbc, 0x90, 0x55, 0x56, {10, 10, 12, 10, 20, 50}},// KTR0405AS-HHg1, KTR0403AS-HHg1, MT29C4G96MAZAPDJA-5 IT

    {0x98, 0xac, 0x90, 0x15, 0x76, {12, 15, 15, 10, 20, 50}},// TYBC0A111392KC
    {0x98, 0xbc, 0x90, 0x55, 0x76, {12, 15, 15, 10, 20, 50}},// TYBC0A111430KC, KSLCBBL1FB4G3A, KSLCBBL1FB2G3A
    {0x98, 0xbc, 0x90, 0x66, 0x76, {12, 15, 15, 10, 20, 50}},// KSLCCBL1FB2G3A
    {0x98, 0xba, 0x90, 0x55, 0x76, {12, 15, 15, 10, 20, 50}},

    {0xad, 0xbc, 0x90, 0x11, 0x00, {25, 15, 25, 10, 20, 50}},// H9DA4VH4JJMMCR-4EMi, H9DA4VH2GJMMCR-4EM
    {0xad, 0xbc, 0x90, 0x55, 0x54, {25, 15, 25, 10, 20, 50}},//

    {0xec, 0xb3, 0x01, 0x66, 0x5a, {21, 10, 21, 10, 20, 50}},// KBY00U00VA-B450
    {0xec, 0xbc, 0x00, 0x55, 0x54, {21, 10, 21, 10, 20, 50}},// KA100O015M-AJTT
    {0xec, 0xbc, 0x00, 0x6a, 0x56, {21, 10, 21, 10, 20, 50}},// K524G2GACH-B050
    {0xec, 0xbc, 0x01, 0x55, 0x48, {21, 15, 21, 10, 20, 50}},// KBY00N00HM-A448

    {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0}}
};

static struct sc8810_nand_info g_info ={0};
static nand_ecc_modes_t sprd_ecc_mode = NAND_ECC_NONE;
static __attribute__((aligned(4))) unsigned char io_wr_port[NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE];

static struct nand_spec_str *ptr_nand_spec = NULL;
static struct nand_spec_str *get_nand_spec(u8 *nand_id);
static void set_nfc_timing(struct sc8810_nand_timing_param *nand_timing, u32 nfc_clk_MHz);
#ifdef  NAND_IRQ_EN
static void sc8810_wait_op_done(void);
#endif

static void nfc_reg_write(unsigned int addr, unsigned int value)
{
	writel(value, addr);
}
static unsigned int nfc_reg_read(unsigned int addr)
{
	return readl(addr);
}

static void sc8810_nand_wp_en(int en)
{
	unsigned int value;
	if (en) {
		value = nfc_reg_read(NFC_CFG0);
		value &= ~ NFC_WPN;
		nfc_reg_write(NFC_CFG0, value);
	} else {
		value = nfc_reg_read(NFC_CFG0);
		value |= NFC_WPN;
		nfc_reg_write(NFC_CFG0, value);
    }
}

static void sc8810_reset_nfc(void)
{
	int ik_cnt = 0;

	REG_AHB_CTL0 |= BIT_8;
	REG_AHB_SOFT_RST |= BIT_5;
	for(ik_cnt = 0; ik_cnt < 0xffff; ik_cnt++);
	REG_AHB_SOFT_RST &= ~BIT_5;

	sc8810_nand_wp_en(0);
	//nfc_reg_write(NFC_TIMING, ((6 << 0) | (6 << 5) | (10 << 10) | (6 << 16) | (5 << 21) | (5 << 26)));
	nfc_reg_write(NFC_TIMING, ((12 << 0) | (7 << 5) | (10 << 10) | (6 << 16) | (5 << 21) | (7 << 26)));
	nfc_reg_write(NFC_TIMING + 0X4, 0xffffffff);
}

void fixon_timeout_send_readid_command(void)
{
	u8 mc_ins_num = 0;
	u8 mc_addr_ins_num = 0;
	u16 b_pointer = 0;
	u16 addr_array[5];
	u32 ins, mode;
	unsigned int offset, high_flag, value;

	/* nfc_mcr_inst_add(cmd, NF_MC_CMD_ID); */
	ins = NAND_CMD_READID;
	mode = NF_MC_CMD_ID;
	offset = mc_ins_num >> 1;
	high_flag = mc_ins_num & 0x1;
	if (high_flag) {
		value = nfc_reg_read(NFC_START_ADDR0 + (offset << 2));
		value &= 0x0000ffff;
		value |= ins << 24;
		value |= mode << 16;
	} else {
		value = nfc_reg_read(NFC_START_ADDR0 + (offset << 2));
		value &= 0xffff0000;
		value |= ins << 8;
		value |= mode;
    }
	nfc_reg_write(NFC_START_ADDR0 + (offset << 2), value);
	mc_ins_num ++;

	/* nfc_mcr_inst_add(0x00, NF_MC_ADDR_ID); */
	ins = 0x00;
	mode = NF_MC_ADDR_ID;
	offset = mc_ins_num >> 1;
	high_flag = mc_ins_num & 0x1;
	if (high_flag) {
		value = nfc_reg_read(NFC_START_ADDR0 + (offset << 2));
		value &= 0x0000ffff;
		value |= ins << 24;
		value |= mode << 16;
	} else {
		value = nfc_reg_read(NFC_START_ADDR0 + (offset << 2));
		value &= 0xffff0000;
		value |= ins << 8;
		value |= mode;
    }
	nfc_reg_write(NFC_START_ADDR0 + (offset << 2), value);
	mc_ins_num ++;

	/* nfc_mcr_inst_add(7, NF_MC_RWORD_ID); */
	ins = 7;
	mode = NF_MC_RWORD_ID;
	offset = mc_ins_num >> 1;
	high_flag = mc_ins_num & 0x1;
	if (high_flag) {
		value = nfc_reg_read(NFC_START_ADDR0 + (offset << 2));
		value &= 0x0000ffff;
		value |= ins << 24;
		value |= mode << 16;
	} else {
		value = nfc_reg_read(NFC_START_ADDR0 + (offset << 2));
		value &= 0xffff0000;
		value |= ins << 8;
		value |= mode;
    }
	nfc_reg_write(NFC_START_ADDR0 + (offset << 2), value);
	mc_ins_num ++;
    /* nfc_mcr_inst_exc_for_id(); */
	value = nfc_reg_read(NFC_CFG0);
	value &= ~NFC_BUS_WIDTH_16;
	value |= (1 << NFC_CMD_SET_OFFSET);

	nfc_reg_write(NFC_CFG0, value);
	value = NFC_CMD_VALID | ((unsigned int)NF_MC_NOP_ID) |((mc_ins_num - 1) << 16);
	nfc_reg_write(NFC_CMD, value);
}

unsigned long fixon_timeout_check_id(void)
{
	unsigned char id[5];

	memcpy(id, (void *)NFC_MBUF_ADDR, 5);

	printk("fix_timeout_id : 0x%02x,0x%02x,0x%02x,0x%02x,0x%02x\n", fix_timeout_id[0], fix_timeout_id[1], fix_timeout_id[2], fix_timeout_id[3], fix_timeout_id[4]);
	printk("newid : 0x%02x,0x%02x,0x%02x,0x%02x,0x%02x\n", id[0], id[1], id[2], id[3], id[4]);

	if ((fix_timeout_id[0] == id[0]) && (fix_timeout_id[1] == id[1]) && \
		(fix_timeout_id[2] == id[2]) && (fix_timeout_id[3] == id[3]) && (fix_timeout_id[4] == id[4]))
		return 1;
	return 0;
}

void fixon_timeout_send_reset_flash_command(void)
{
	u8 mc_ins_num = 0;
	u8 mc_addr_ins_num = 0;
	u16 b_pointer = 0;
	u16 addr_array[5];
	u32 ins, mode;
	unsigned int offset, high_flag, value;

	/* nfc_mcr_inst_add(cmd, NF_MC_CMD_ID); */
	ins = NAND_CMD_RESET;
	mode = NF_MC_CMD_ID;
	offset = mc_ins_num >> 1;
	high_flag = mc_ins_num & 0x1;
	if (high_flag) {
		value = nfc_reg_read(NFC_START_ADDR0 + (offset << 2));
		value &= 0x0000ffff;
		value |= ins << 24;
		value |= mode << 16;
	} else {
		value = nfc_reg_read(NFC_START_ADDR0 + (offset << 2));
		value &= 0xffff0000;
		value |= ins << 8;
		value |= mode;
    }
	nfc_reg_write(NFC_START_ADDR0 + (offset << 2), value);
	mc_ins_num ++;

	/* nfc_mcr_inst_exc(); */
	value = nfc_reg_read(NFC_CFG0);
    value |= NFC_BUS_WIDTH_16;
	value |= (1 << NFC_CMD_SET_OFFSET);
	nfc_reg_write(NFC_CFG0, value);
	value = NFC_CMD_VALID | ((unsigned int)NF_MC_NOP_ID) | ((mc_ins_num - 1) << 16);
	nfc_reg_write(NFC_CMD, value);
}

void fixon_timeout_save_reg(void)
{
	unsigned long ii, total;

	for (ii = 0; ii < 8; ii ++)
		fix_timeout_reg[ii] = 0;

	total = g_info.mc_ins_num - 1;
	for (ii = 0; ii <= total; ii ++) {
		if ((ii % 2) != 0)
			continue;
		fix_timeout_reg[ii / 2] = nfc_reg_read(NFC_START_ADDR0 + ii * 2);
	}

	/*for (ii = 0; ii <= total; ii ++) {
		if ((ii % 2) != 0)
			continue;
		printk("reg[0x%08x] = 0x%08x --> timeoutreg[0x%08x] = 0x%08x\n", (NFC_START_ADDR0 + ii * 2), nfc_reg_read(NFC_START_ADDR0 + ii * 2), ii / 2, fix_timeout_reg[ii / 2]);
	}*/
}

void fixon_timeout_restore_reg(void)
{
	unsigned long ii, total;

	total = g_info.mc_ins_num - 1;
	for (ii = 0; ii <= total; ii ++) {
		if ((ii % 2) != 0)
			continue;
		nfc_reg_write((NFC_START_ADDR0 + ii * 2), fix_timeout_reg[ii / 2]);
	}

	/*for (ii = 0; ii <= total; ii ++) {
		if ((ii % 2) != 0)
			continue;
		printk("timeoutreg[0x%08x] = 0x%08x ---> reg[0x%08x] = 0x%08x\n", ii / 2, fix_timeout_reg[ii / 2], (NFC_START_ADDR0 + ii * 2), nfc_reg_read(NFC_START_ADDR0 + ii * 2));
	}*/
}

unsigned long fixon_timeout_function(unsigned int flag)
{
	unsigned long ret, nfc_cmd, nfc_clr_raw, value, nfc_clr_raw_2, nfc_cmd_2, nfc_clr_raw_3, nfc_cmd_3;

	ret = 0;
	printk("%s %d flag[0x%08x]\n", __FUNCTION__, __LINE__, flag);
	nfc_cmd = nfc_reg_read(NFC_CMD);
	printk("REG_NFC_CMD[0x%08x]\n", nfc_cmd);
	if (nfc_cmd & 0x80000000) { /* Bit31 is 1 */
		printk("Bit31 of REG_NFC_CMD is 1, memory or nfc is wrong\n");
		printk("clear the current command\n");
        value = nfc_reg_read(NFC_CFG0);
		value |= 0x2;
		nfc_reg_write(NFC_CFG0, value);
		mdelay(10);
		nfc_reg_write(NFC_CLR_RAW, 0xffff0000); /* clear all interrupt status */
		printk("send readid command\n");
		fixon_timeout_send_readid_command();
		mdelay(10);
		nfc_clr_raw = nfc_reg_read(NFC_CLR_RAW);
		printk("NFC_DONE_RAW bit[0x%08x]\n", nfc_clr_raw);
		nfc_reg_write(NFC_CLR_RAW, 0xffff0000); /* clear all interrupt status */
		if (nfc_clr_raw & NFC_DONE_RAW) { /* NFC_DONE_RAW is 1 */
			printk("NFC_DONE_RAW is 1\n");
			value = fixon_timeout_check_id();
			if (value == 1) {
				printk("Id is right, step 3\n");
				printk("send reset flash command\n");
				fixon_timeout_send_reset_flash_command();
				mdelay(20);
				nfc_clr_raw_2 = nfc_reg_read(NFC_CLR_RAW);
				nfc_cmd_2 = nfc_reg_read(NFC_CMD);
				printk("2NFC_DONE_RAW bit[0x%08x]  NFC_VALID bit[0x%08x]\n", nfc_clr_raw_2, nfc_cmd_2);
				nfc_reg_write(NFC_CLR_RAW, 0xffff0000); /* clear all interrupt status */
				if (((nfc_clr_raw_2 & NFC_DONE_RAW) == 1) && ((nfc_cmd_2 & 0x80000000) == 0)) {
					printk("2nfc/flash/hardware are all right, execute again\n");
					ret = 1;
				} else {
					printk("nfc timing is wrong, reset nfc and test again\n");
					sc8810_reset_nfc();
					mdelay(10);
					printk("send reset flash command\n");
					fixon_timeout_send_reset_flash_command();
					mdelay(20);
					nfc_clr_raw_3 = nfc_reg_read(NFC_CLR_RAW);
					nfc_cmd_3 = nfc_reg_read(NFC_CMD);
				printk("3NFC_DONE_RAW bit[0x%08x]  NFC_VALID bit[0x%08x]\n", nfc_clr_raw_3, nfc_cmd_3);
					nfc_reg_write(NFC_CLR_RAW, 0xffff0000); /* clear all interrupt status */
					if (((nfc_clr_raw_3 & NFC_DONE_RAW) == 1) && ((nfc_cmd_3 & 0x80000000) == 0)) {
						printk("3nfc/flash/hardware are all right, execute again\n");
						ret = 1;
					} else
						printk("nfc timing is wrong!!!\n");
				}
			} else
				printk("Id is wrong, please check flash, nfc or timing\n");
        } else
		printk("NFC_DONE_RAW is 0, please check flash, nfc or timing.flag:%d\n", flag);
	} else {
		nfc_clr_raw = nfc_reg_read(NFC_CLR_RAW);
		printk("REG_NFC_CLR_RAW[0x%08x]\n", nfc_clr_raw);
		if (flag == NFC_ECC_EVENT) {
			if (nfc_clr_raw & NFC_ECC_DONE_RAW) /* NFC_ECC_EVENT is 1 */
				printk("NFC_ECC_EVENT is 1, can not occur timeout\n");
			else
				printk("NFC_ECC_EVENT is 0, software clear bit%d, please check software\n", flag);
		} else if (flag == NFC_DONE_EVENT) {
			if (nfc_clr_raw & NFC_DONE_RAW) /* NFC_DONE_RAW is 1 */
				printk("NFC_DONE_RAW is 1, can not occur timeout\n");
			else
				printk("NFC_DONE_RAW is 0, software clear bit%d, please check software\n", flag);
		}
		ret = 2;
    }

	return ret;
}

static void  nfc_mcr_inst_init(void)
{
	g_info.mc_ins_num = 0;
	g_info.b_pointer = 0;
	g_info.mc_addr_ins_num = 0;
}
void  nfc_mcr_inst_add(u32 ins, u32 mode)
{
	unsigned int offset;
	unsigned int high_flag;
	unsigned int reg_value;
	offset = g_info.mc_ins_num >> 1;
	high_flag = g_info.mc_ins_num & 0x1;
	if(NF_MC_ADDR_ID == mode)
	{
		g_info.addr_array[g_info.mc_addr_ins_num ++] = ins;
	}
	if(high_flag)
	{
		reg_value = nfc_reg_read(NFC_START_ADDR0 + (offset << 2));
		reg_value &= 0x0000ffff;
		reg_value |= ins << 24;
		reg_value |= mode << 16;
	}
	else
	{
		reg_value = nfc_reg_read(NFC_START_ADDR0 + (offset << 2));
		reg_value &= 0xffff0000;
		reg_value |= ins << 8;
		reg_value |= mode;
	}
	nfc_reg_write(NFC_START_ADDR0 + (offset << 2), reg_value);
	g_info.mc_ins_num ++;
}
static unsigned int nfc_mcr_inst_exc(int irq_en)
{
	unsigned int value;
	value = nfc_reg_read(NFC_CFG0);
	if(g_info.chip->options & NAND_BUSWIDTH_16)
	{
		value |= NFC_BUS_WIDTH_16;
	}
	else
	{
		value &= ~NFC_BUS_WIDTH_16;
	}
	value |= (1 << NFC_CMD_SET_OFFSET);
	nfc_reg_write(NFC_CFG0, value);
#ifdef  NAND_IRQ_EN
        value &= ~(NFC_DONE_EN|NFC_TO_EN);
        nfc_reg_write(NFC_STS_EN, value);

        nfc_reg_write(NFC_CLR_RAW, 0xffff0000);

        if(irq_en){
            value = nfc_reg_read(NFC_STS_EN);
            value |= (NFC_DONE_EN|NFC_TO_EN);
            nfc_reg_write(NFC_STS_EN, value);

            sc8810_wait_op_done();
            if(handle_status==NAND_HANDLE_DONE){
                ret_irq_en=NAND_NO_ERROR;
            }else if(handle_status==NAND_HANDLE_TIMEOUT){
                ret_irq_en=NAND_ERR_NEED_RETRY;
            }else if(handle_status==NAND_HANDLE_ERR){
                ret_irq_en=NAND_ERR_NEED_RETRY;
            }

        }
#endif
	value = NFC_CMD_VALID | ((unsigned int)NF_MC_NOP_ID) | ((g_info.mc_ins_num - 1) << 16);
	nfc_reg_write(NFC_CMD, value);
	return 0;
}

static unsigned int nfc_mcr_inst_exc_for_id(int irq_en)
{
	unsigned int value;

	value = nfc_reg_read(NFC_CFG0);
	value &= ~NFC_BUS_WIDTH_16;
	value |= (1 << NFC_CMD_SET_OFFSET);

	nfc_reg_write(NFC_CFG0, value);
        #ifdef  NAND_IRQ_EN
        value &= ~(NFC_DONE_EN|NFC_TO_EN);
        nfc_reg_write(NFC_STS_EN, value);

        nfc_reg_write(NFC_CLR_RAW, 0xffff0000);

        if(irq_en){
            value = nfc_reg_read(NFC_STS_EN);
            value |= (NFC_DONE_EN|NFC_TO_EN);
            nfc_reg_write(NFC_STS_EN, value);
        }
        #endif

        value = NFC_CMD_VALID | ((unsigned int)NF_MC_NOP_ID) |((g_info.mc_ins_num - 1) << 16);
	nfc_reg_write(NFC_CMD, value);
	return 0;
}

#ifdef CONFIG_SOFT_WATCHDOG
extern int first_watchdog_fire;
unsigned long nfc_wait_times = 0, nfc_wait_long = 0;
static unsigned long func_start, func_end;

#define SPRD_SYSCNT_BASE            0xE002d000
#define SYSCNT_REG(off) (SPRD_SYSCNT_BASE + (off))
#define SYSCNT_COUNT    SYSCNT_REG(0x0004)

static unsigned long read_clock_sim()
{
    	u32 val1, val2;
        val1 = __raw_readl(SYSCNT_COUNT);
        val2 = __raw_readl(SYSCNT_COUNT);
        while(val2 != val1) {
                val1 = val2;
                val2 = __raw_readl(SYSCNT_COUNT);
        }
	return val2;
}
#endif

static int sc8810_nfc_wait_command_finish(unsigned int flag, int cmd)
{
	unsigned int event = 0;
	unsigned int value;
	unsigned int counter = 0;
	unsigned int is_timeout = 0;
	unsigned int ret = 1;

#ifdef CONFIG_SOFT_WATCHDOG
	if (first_watchdog_fire) {
		nfc_wait_times++;
		func_start = read_clock_sim();
	}
#endif

	while (((event & flag) != flag) && (counter < NFC_TIMEOUT_VAL)) {
        value = nfc_reg_read(NFC_CLR_RAW);
        if (value & NFC_ECC_DONE_RAW)
			event |= NFC_ECC_EVENT;

		if (value & NFC_DONE_RAW)
			event |= NFC_DONE_EVENT;
		counter ++;

		if (flag == NFC_DONE_EVENT) {
			if ((cmd == NAND_CMD_RESET) && (counter >= NFC_RESET_TIMEOUT))
				is_timeout = 1;
			else if ((cmd == NAND_CMD_STATUS) && (counter >= NFC_STATUS_TIMEOUT))
				is_timeout = 1;
			else if ((cmd == NAND_CMD_READID) && (counter >= NFC_READID_TIMEOUT))
				is_timeout = 1;
			else if ((cmd == NAND_CMD_ERASE2) && (counter >= NFC_ERASE_TIMEOUT))
				is_timeout = 1;
			else if ((cmd == NAND_CMD_READSTART) && (counter >= NFC_READ_TIMEOUT))
				is_timeout = 1;
			else if ((cmd == NAND_CMD_PAGEPROG) && (counter >= NFC_WRITE_TIMEOUT))
				is_timeout = 1;
		} else if (flag == NFC_ECC_EVENT) {
			if ((cmd == NFC_CMD_ENCODE) && (counter >= NFC_ECCDECODE_TIMEOUT))
				is_timeout = 1;
			else if ((cmd == NFC_CMD_DECODE) && (counter >= NFC_ECCENCODE_TIMEOUT))
				is_timeout = 1;
		}

		if (is_timeout == 1) {
			printk("nfc cmd[0x%08x] timeout[0x%08x] and reset nand controller.\n", cmd, counter);
			break;
		}
	}

	/*if (((cmd == NFC_CMD_ENCODE)) || (cmd == NFC_CMD_DECODE)) {
		printk("2cmd = 0x%08x  counter = 0x%08x\n", cmd, counter);
	}*/

	if (is_timeout == 1) {
		ret = fixon_timeout_function(flag);
		if (ret == 0) {
			panic("nfc cmd timeout, check nfc, flash, hardware\n");
			while (1);
			return -1;
		} else
			return 1;
	}

	nfc_reg_write(NFC_CLR_RAW, 0xffff0000); /* clear all interrupt status */

	if (counter > NFC_TIMEOUT_VAL) {
		panic("nfc cmd timeout!!!");
		while (1);
		return -1;
	}
#ifdef CONFIG_SOFT_WATCHDOG
	if (first_watchdog_fire) {
		func_end = read_clock_sim();
		nfc_wait_long += (func_end - func_start);
	}
#endif
	return 0;
}

#ifdef  NAND_IRQ_EN
static void sc8810_wait_op_done(void)
{
                //INIT_COMPLETION(sc8810_op_completion);
    if (!wait_for_completion_timeout(&sc8810_op_completion, msecs_to_jiffies(IRQ_TIMEOUT))) {
            handle_status=NAND_HANDLE_ERR;
            printk("%s, wait irq timeout\n", __func__);
    }
}

static irqreturn_t sc8810_nfc_irq(int irq, void *dev_id)
{
	unsigned int value;

        /*diable irq*/
        value = nfc_reg_read(NFC_STS_EN);
        value &= ~(NFC_DONE_EN|NFC_TO_EN);
        nfc_reg_write(NFC_STS_EN, value);

        value = nfc_reg_read(NFC_STS_EN);
        //printk("%s, STS_EN:0x%x\n", __func__, value);
        /*record handle status*/
        if(value & NFC_TO_STS){
            handle_status=NAND_HANDLE_TIMEOUT;
        }
        else if(value & NFC_DONE_STS){
            handle_status=NAND_HANDLE_DONE;
        }
        /*clear irq status*/
        nfc_reg_write(NFC_CLR_RAW, 0xffff0000); /* clear all interrupt status */
        complete(&sc8810_op_completion);

	return IRQ_HANDLED;
}
#endif

unsigned int ecc_mode_convert(u32 mode)
{
	u32 mode_m;
	switch(mode)
	{
	case 1:
		mode_m = 0;
		break;
	case 2:
		mode_m = 1;
		break;
	case 4:
		mode_m = 2;
		break;
	case 8:
		mode_m = 3;
		break;
	case 12:
		mode_m = 4;
		break;
	case 16:
		mode_m = 5;
		break;
	case 24:
		mode_m = 6;
		break;
	default:
		mode_m = 0;
		break;
	}
	return mode_m;
}
unsigned int sc8810_ecc_encode(struct sc8810_ecc_param *param)
{
	u32 reg;
	reg = (param->m_size - 1);
	memcpy((void *)NFC_MBUF_ADDR, param->p_mbuf, param->m_size);
	nfc_reg_write(NFC_ECC_CFG1, reg);
	reg = 0;
	reg = (ecc_mode_convert(param->mode)) << NFC_ECC_MODE_OFFSET;
	reg |= (param->ecc_pos << NFC_ECC_SP_POS_OFFSET) | ((param->sp_size - 1) << NFC_ECC_SP_SIZE_OFFSET) | ((param->ecc_num -1)<< NFC_ECC_NUM_OFFSET);
	reg |= NFC_ECC_ACTIVE;
	nfc_reg_write(NFC_ECC_CFG0, reg);
	sc8810_nfc_wait_command_finish(NFC_ECC_EVENT, NFC_CMD_ENCODE);

	memcpy(param->p_sbuf, (u8 *)NFC_SBUF_ADDR,param->sp_size);

	return 0;
}
static u32 sc8810_get_decode_sts(void)
{
	u32 err;
	err = nfc_reg_read(NFC_ECC_STS0);
	err &= 0x1f;

	if(err == 0x1f)
		return -1;

	return err;
}

static u32 sc8810_ecc_decode(struct sc8810_ecc_param *param)
{
	u32 reg;
	u32 ret = 0;
	s32 size = 0;

	memcpy((void *)NFC_MBUF_ADDR, param->p_mbuf, param->m_size);
	memcpy((void *)NFC_SBUF_ADDR, param->p_sbuf, param->sp_size);
	reg = (param->m_size - 1);
	nfc_reg_write(NFC_ECC_CFG1, reg);
	reg = 0;
	reg = (ecc_mode_convert(param->mode)) << NFC_ECC_MODE_OFFSET;
	reg |= (param->ecc_pos << NFC_ECC_SP_POS_OFFSET) | ((param->sp_size - 1) << NFC_ECC_SP_SIZE_OFFSET) | ((param->ecc_num - 1) << NFC_ECC_NUM_OFFSET);
	reg |= NFC_ECC_DECODE;
	reg |= NFC_ECC_ACTIVE;
	nfc_reg_write(NFC_ECC_CFG0, reg);
	sc8810_nfc_wait_command_finish(NFC_ECC_EVENT, NFC_CMD_DECODE);

	ret = sc8810_get_decode_sts();

	if (ret != 0 && ret != -1) {
//		printk(KERN_INFO "sc8810_ecc_decode sts = %x\n",ret);
	}
	if (ret == -1) {
//		printk(KERN_INFO "(%x),(%x),(%x),(%x)\n",param->p_sbuf[0],param->p_sbuf[1],param->p_sbuf[2],param->p_sbuf[3]);

		//FIXME:
		size = param->sp_size;
		if (size > 0) {
			while (size--)
			{
				if (param->p_sbuf[size] != 0xff)
					break;
			}
			if (size < 0)
			{
				size = param->m_size;
				if (size > 0)
				{
					while (size--)
					{
						if (param->p_mbuf[size] != 0xff)
							break;
					}
					if (size < 0) {
						ret = 0;
					}
				}
			}
		}

	}

	if ((ret != -1) && (ret != 0))
	{
		memcpy(param->p_mbuf, (void *)NFC_MBUF_ADDR, param->m_size);
		memcpy(param->p_sbuf, (void *)NFC_SBUF_ADDR, param->sp_size);
		ret = 0;
	}

	return ret;
}

static struct nand_spec_str *get_nand_spec(u8 *nand_id)
{
    int i = 0;
    while(nand_spec_table[i].mid != 0){
        if (
                (nand_id[0] == nand_spec_table[i].mid)
                && (nand_id[1] == nand_spec_table[i].did)
                && (nand_id[2] == nand_spec_table[i].id3)
                && (nand_id[3] == nand_spec_table[i].id4)
                && (nand_id[4] == nand_spec_table[i].id5)
           ){
                return &nand_spec_table[i];
        }
        i++;
    }
    return (struct nand_spec_str *)0;
}

#define DELAY_NFC_TO_PAD 9
#define DELAY_PAD_TO_NFC 6
#define DELAY_RWL (DELAY_NFC_TO_PAD + DELAY_PAD_TO_NFC)

static void set_nfc_timing(struct sc8810_nand_timing_param *nand_timing, u32 nfc_clk_MHz)
{
	u32 value = 0;
	u32 cycles;
	cycles = nand_timing->acs_time * nfc_clk_MHz / 1000 + 1;
	value |= ((cycles & 0x1F) << NFC_ACS_OFFSET);

	cycles = nand_timing->rwh_time * nfc_clk_MHz / 1000 + 2;
	value |= ((cycles & 0x1F) << NFC_RWH_OFFSET);

        cycles = (nand_timing->rwl_time+DELAY_RWL) * nfc_clk_MHz / 1000 + 1;
	value |= ((cycles & 0x3F) << NFC_RWL_OFFSET);

    cycles = nand_timing->acr_time * nfc_clk_MHz / 1000 + 1;
	value |= ((cycles & 0x1F) << NFC_ACR_OFFSET);

    cycles = nand_timing->rr_time * nfc_clk_MHz / 1000 + 1;
	value |= ((cycles & 0x1F) << NFC_RR_OFFSET);

    cycles = nand_timing->ceh_time * nfc_clk_MHz / 1000 + 1;
	value |= ((cycles & 0x3F) << NFC_CEH_OFFSET);

    nfc_reg_write(NFC_TIMING, value);

}

static int sprd_nand_inithw(struct sprd_nand_info *info, struct platform_device *pdev)
{
#if 0
	struct sprd_platform_nand *plat = to_nand_plat(pdev);
	unsigned long para = (plat->acs << 0) |
				(plat->ach << 2) |
				(plat->rwl << 4) |
				(plat->rwh << 8) |
				(plat->rr << 10) |
				(plat->acr << 13) |
				(plat->ceh << 16);

 	writel(para, NFCPARAMADDR);
#endif
	return 0;
}


static void sc8810_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	memcpy(buf, (void *)(g_info.b_pointer + NFC_MBUF_ADDR),len);
	g_info.b_pointer += len;
}
static void sc8810_nand_write_buf(struct mtd_info *mtd, const uint8_t *buf,
				   int len)
{
	struct nand_chip *chip = (struct nand_chip *)(mtd->priv);
	int eccsize = chip->ecc.size;
	memcpy((void *)(g_info.b_pointer + NFC_MBUF_ADDR), (unsigned char*)buf,len);
	if (g_info.b_pointer < eccsize)
		memcpy(io_wr_port, (unsigned char*)buf,len);
	g_info.b_pointer += len;
}
static u_char sc8810_nand_read_byte(struct mtd_info *mtd)
{
	u_char ch;
	ch = io_wr_port[g_info.b_pointer ++];
	return ch;
}
static u16 sc8810_nand_read_word(struct mtd_info *mtd)
{
	u16 ch = 0;
	unsigned char *port = (void *)NFC_MBUF_ADDR;

	ch = port[g_info.b_pointer ++];
	ch |= port[g_info.b_pointer ++] << 8;

	return ch;
}

static void sc8810_nand_data_add(unsigned int bytes, unsigned int bus_width, unsigned int read)
{
	unsigned int word;
	unsigned int blk;
	if(!bus_width)
	{
		blk = bytes >> 8;
		word = bytes & 0xff;
	}
	else
	{
		blk = bytes >> 9;
		word = (bytes & 0x1ff) >> 1;
	}
	if(read)
	{
		if(blk)
		{
			nfc_mcr_inst_add(blk - 1, NF_MC_RBLK_ID);
		}
		if(word)
		{
			nfc_mcr_inst_add(word - 1, NF_MC_RWORD_ID);
		}
	}
	else
	{
		if(blk)
		{
			nfc_mcr_inst_add(blk - 1, NF_MC_WBLK_ID);
		}
		if(word)
		{
			nfc_mcr_inst_add(word - 1, NF_MC_WWORD_ID);
		}
	}
}


static void read_chip_id(void)
{
	int i, cmd = NAND_CMD_READID;

	nfc_mcr_inst_init();
	nfc_mcr_inst_add(cmd, NF_MC_CMD_ID);
	nfc_mcr_inst_add(0x00, NF_MC_ADDR_ID);
	nfc_mcr_inst_add(0x10, NF_MC_NOP_ID);//add nop clk for twrh timing param
	nfc_mcr_inst_add(7, NF_MC_RWORD_ID);
	nfc_mcr_inst_exc_for_id(false);
	sc8810_nfc_wait_command_finish(NFC_DONE_EVENT, NAND_CMD_READID);
	//memcpy(io_wr_port, (void *)NFC_MBUF_ADDR, 5);
	io_wr_port[0] = nand_config_item.m_c;
	io_wr_port[1] = nand_config_item.d_c;
	io_wr_port[2] = nand_config_item.cyc_3;
	io_wr_port[3] = nand_config_item.cyc_4;
	io_wr_port[4] = nand_config_item.cyc_5;

}

static void sc8810_nand_hwcontrol(struct mtd_info *mtd, int cmd,
				   unsigned int ctrl)
{
	struct nand_chip *chip = (struct nand_chip *)(mtd->priv);
	u32 eccsize, size = 0;
	int ret = 0;
	if (ctrl & NAND_CLE) {
		switch (cmd) {
		case NAND_CMD_RESET:
			wake_lock(&nfc_wakelock);
			nfc_mcr_inst_init();
			nfc_mcr_inst_add(cmd, NF_MC_CMD_ID);
			nfc_mcr_inst_exc(false);
			sc8810_nfc_wait_command_finish(NFC_DONE_EVENT, cmd);
			wake_unlock(&nfc_wakelock);
			break;
		case NAND_CMD_STATUS:
			wake_lock(&nfc_wakelock);
			nfc_mcr_inst_init();
			//nfc_reg_write(NFC_CMD, 0x80000070);
			//sc8810_nfc_wait_command_finish(NFC_DONE_EVENT,cmd);
			//memcpy(io_wr_port, (void *)NFC_ID_STS, 1);
			nfc_mcr_inst_add(0x70, NF_MC_CMD_ID);
			nfc_mcr_inst_add(0x10, NF_MC_NOP_ID);//add nop clk for twrh timing param
			nfc_mcr_inst_add(3, NF_MC_RWORD_ID);
			nfc_mcr_inst_exc_for_id(false);
			sc8810_nfc_wait_command_finish(NFC_DONE_EVENT,cmd);
			memcpy(io_wr_port, (void *)NFC_MBUF_ADDR, 1);
			wake_unlock(&nfc_wakelock);
			break;
		case NAND_CMD_READID:
			wake_lock(&nfc_wakelock);
			nfc_mcr_inst_init();
			nfc_mcr_inst_add(cmd, NF_MC_CMD_ID);
			nfc_mcr_inst_add(0x00, NF_MC_ADDR_ID);
			nfc_mcr_inst_add(0x10, NF_MC_NOP_ID);//add nop clk for twrh timing param
			nfc_mcr_inst_add(7, NF_MC_RWORD_ID);
			nfc_mcr_inst_exc_for_id(false);
			sc8810_nfc_wait_command_finish(NFC_DONE_EVENT, cmd);
			io_wr_port[0] = nand_config_item.m_c;
			io_wr_port[1] = nand_config_item.d_c;
			io_wr_port[2] = nand_config_item.cyc_3;
			io_wr_port[3] = nand_config_item.cyc_4;
			io_wr_port[4] = nand_config_item.cyc_5;
			if (fix_timeout_id[0] == 0) {
				fix_timeout_id[0] = io_wr_port[0];
				fix_timeout_id[1] = io_wr_port[1];
			        fix_timeout_id[2] = io_wr_port[2];
				fix_timeout_id[3] = io_wr_port[3];
				fix_timeout_id[4] = io_wr_port[4];
			}
			wake_unlock(&nfc_wakelock);
			//printk("\n0x%02x  0x%02x  0x%02x  0x%02x  0x%02x\n", io_wr_port[0], io_wr_port[1], io_wr_port[2], io_wr_port[3], io_wr_port[4]);
		break;
        case NAND_CMD_ERASE1:
			wake_lock(&nfc_wakelock);
			nfc_mcr_inst_init();
			nfc_mcr_inst_add(cmd, NF_MC_CMD_ID);
			break;
		case NAND_CMD_ERASE2:
			nfc_mcr_inst_add(cmd, NF_MC_CMD_ID);
			nfc_mcr_inst_add(0, NF_MC_WAIT_ID);
			fixon_timeout_save_reg();
			nfc_mcr_inst_exc(true);
                        #ifdef  NAND_IRQ_EN
                        if (ret_irq_en == NAND_ERR_NEED_RETRY) {
                        #else
                        ret = sc8810_nfc_wait_command_finish(NFC_DONE_EVENT, cmd);
                        if (ret == 1) {
                        #endif
				fixon_timeout_restore_reg();
				nfc_mcr_inst_exc(false);
				sc8810_nfc_wait_command_finish(NFC_DONE_EVENT, cmd);
			}
			wake_unlock(&nfc_wakelock);
			break;
		case NAND_CMD_READ0:
			wake_lock(&nfc_wakelock);
			nfc_mcr_inst_init();
			nfc_mcr_inst_add(cmd, NF_MC_CMD_ID);
			break;
		case NAND_CMD_READSTART:
			nfc_mcr_inst_add(cmd, NF_MC_CMD_ID);
			nfc_mcr_inst_add(0, NF_MC_WAIT_ID);
			if((!g_info.addr_array[0]) && (!g_info.addr_array[1]) )//main part
				size = mtd->writesize + mtd->oobsize;
			else
				size = mtd->oobsize;
			sc8810_nand_data_add(size, chip->options & NAND_BUSWIDTH_16, 1);
			fixon_timeout_save_reg();
			nfc_mcr_inst_exc(false);
			ret = sc8810_nfc_wait_command_finish(NFC_DONE_EVENT, cmd);
			if (ret == 1) {
				fixon_timeout_restore_reg();
				nfc_mcr_inst_exc(false);
				sc8810_nfc_wait_command_finish(NFC_DONE_EVENT, cmd);
			}
			wake_unlock(&nfc_wakelock);
			break;
		case NAND_CMD_SEQIN:
			wake_lock(&nfc_wakelock);
			nfc_mcr_inst_init();
			nfc_mcr_inst_add(NAND_CMD_SEQIN, NF_MC_CMD_ID);
			break;
		case NAND_CMD_PAGEPROG:
			eccsize = chip->ecc.size;
			memcpy((void *)NFC_MBUF_ADDR, io_wr_port, eccsize);
			nfc_mcr_inst_add(0x10, NF_MC_NOP_ID);//add nop clk for twrh timing param
			sc8810_nand_data_add(g_info.b_pointer, chip->options & NAND_BUSWIDTH_16, 0);
			nfc_mcr_inst_add(cmd, NF_MC_CMD_ID);
			nfc_mcr_inst_add(0, NF_MC_WAIT_ID);
			fixon_timeout_save_reg();
			nfc_mcr_inst_exc(true);
                        #ifdef  NAND_IRQ_EN
                        if (ret_irq_en == NAND_ERR_NEED_RETRY) {
                        #else
                        ret = sc8810_nfc_wait_command_finish(NFC_DONE_EVENT, cmd);
                        if (ret == 1) {
                        #endif
				fixon_timeout_restore_reg();
				nfc_mcr_inst_exc(false);
				sc8810_nfc_wait_command_finish(NFC_DONE_EVENT, cmd);
			}
			wake_unlock(&nfc_wakelock);
			break;
		default :
		break;
		}
	}
	else if(ctrl & NAND_ALE) {
		nfc_mcr_inst_add(cmd & 0xff, NF_MC_ADDR_ID);
	}
}
static int sc8810_nand_devready(struct mtd_info *mtd)
{
	unsigned long value = 0;

	value = nfc_reg_read(NFC_CMD);
	if ((value & NFC_CMD_VALID) != 0)
		return 0;
	else
		return 1; /* ready */
}
static void sc8810_nand_select_chip(struct mtd_info *mtd, int chip)
{
	//struct nand_chip *this = mtd->priv;
	//struct sprd_nand_info *info = this->priv;
	/* clk_enable(info->clk) */
	int ik_cnt = 0;

	if (chip != -1) {
		REG_AHB_CTL0 |= BIT_8;//no BIT_9 /* enabel nfc clock */
		for(ik_cnt = 0; ik_cnt < 10000; ik_cnt ++);
	} else
		REG_AHB_CTL0 &= ~BIT_8; /* disabel nfc clock */

}
static int sc8810_nand_calculate_ecc(struct mtd_info *mtd, const u_char *dat, u_char *ecc_code)
{
	struct sc8810_ecc_param param;
	struct nand_chip *this = (struct nand_chip *)(mtd->priv);
	param.mode = g_info.ecc_mode;
	param.ecc_num = 1;
	param.sp_size = this->ecc.bytes;
	param.ecc_pos = 0;
	param.m_size = this->ecc.size;
	param.p_mbuf = (u8 *)dat;
	param.p_sbuf = ecc_code;

	if (sprd_ecc_mode == NAND_ECC_WRITE) {
		sc8810_ecc_encode(&param);
		sprd_ecc_mode = NAND_ECC_NONE;
	}
	return 0;
}
static void sc8810_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	sprd_ecc_mode = mode;
}
static int sc8810_nand_correct_data(struct mtd_info *mtd, uint8_t *dat,
				     uint8_t *read_ecc, uint8_t *calc_ecc)
{
	struct sc8810_ecc_param param;

	struct nand_chip *this = (struct nand_chip *)(mtd->priv);
	int ret = 0;
	param.mode = g_info.ecc_mode;
	param.ecc_num = 1;
	param.sp_size = this->ecc.bytes;
	param.ecc_pos = 0;
	param.m_size = this->ecc.size;
	param.p_mbuf = dat;
	param.p_sbuf = read_ecc;
	ret = sc8810_ecc_decode(&param);

	return ret;
}

static void sc8810_nand_hw_init(void)
{
	int ik_cnt = 0;

	REG_AHB_CTL0 |= BIT_8;//no BIT_9
	REG_AHB_SOFT_RST |= BIT_5;
	for(ik_cnt = 0; ik_cnt < 0xffff; ik_cnt++);
	REG_AHB_SOFT_RST &= ~BIT_5;

	sc8810_nand_wp_en(0);

    if (ptr_nand_spec != NULL)
        set_nfc_timing(&ptr_nand_spec->timing_cfg, 153);

	//nfc_reg_write(NFC_TIMING, ((6 << 0) | (6 << 5) | (10 << 10) | (6 << 16) | (5 << 21) | (5 << 26)));
    nfc_reg_write(NFC_TIMING, ((12 << 0) | (7 << 5) | (10 << 10) | (6 << 16) | (5 << 21) | (7 << 26)));
	nfc_reg_write(NFC_TIMING+0X4, 0xffffffff);//TIMEOUT
//	set_nfc_param(1);//53MHz
}

struct nand_ecclayout _nand_oob_64_4bit = {
	.eccbytes = 28,
	.eccpos = {
		   36, 37, 38, 39, 40, 41, 42, 43,
                   44, 45, 46, 47, 48, 49, 50, 51,
                   52, 53, 54, 55, 56, 57, 58, 59,
                   60, 61, 62, 63},
	.oobfree = {
		{.offset = 2,
		 .length = 34}}
};

static struct nand_ecclayout _nand_oob_128 = {
	.eccbytes = 56,
	.eccpos = {
		    72, 73, 74, 75, 76, 77, 78, 79,
		    80,  81,  82,  83,  84,  85,  86,  87,
		    88,  89,  90,  91,  92,  93,  94,  95,
		    96,  97,  98,  99, 100, 101, 102, 103,
		   104, 105, 106, 107, 108, 109, 110, 111,
		   112, 113, 114, 115, 116, 117, 118, 119,
		   120, 121, 122, 123, 124, 125, 126, 127},
	.oobfree = {
		{.offset = 2,
		 .length = 70}}
};

static struct nand_ecclayout _nand_oob_224 = {
	.eccbytes = 112,
	.eccpos = {
		112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125,
		126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
		140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153,
		154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167,
		168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181,
		182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195,
		196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
		210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223},
	.oobfree = {
		{.offset = 2,
		.length = 110}}
};

static struct nand_ecclayout _nand_oob_256 = {
	.eccbytes = 112,
	.eccpos = {
		144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157,
		158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171,
		172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185,
		186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199,
		200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
		214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227,
		228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241,
		242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255},
	.oobfree = {
		{.offset = 2,
		.length = 142}}
};


static int nand_check_2bitconfig_4bitecc(void)
{
        int ret = 0x0;
        unsigned char m_c = nand_config_item.m_c;
        unsigned char d_c = nand_config_item.d_c;
        unsigned char cyc_3 = nand_config_item.cyc_3;
        unsigned char cyc_4 = nand_config_item.cyc_4;
        unsigned char cyc_5 = nand_config_item.cyc_5;

        if(((m_c==0x2c) && (d_c==0xbc) &&(cyc_3==0x90) && (cyc_4 == 0x55) && (cyc_5 == 0x56)) ||\
		((m_c==0x2c) && (d_c==0xb3) &&(cyc_3==0xd1) && (cyc_4 == 0x55) && (cyc_5 == 0x56)) ||\
			((m_c==0xc8) && (d_c==0xbc) &&(cyc_3==0x90) && (cyc_4 == 0x55) && (cyc_5 == 0x54)))
        {
            ret=1;
        }

        printk("m_c=0x%x, d_c=0x%x, cyc_3=0x%x, cyc_4=0x%x, cyc_5=0x%x, ret=0x%x\n", m_c, d_c, cyc_3, cyc_4, cyc_5, ret);
        return ret;
}

void nand_hardware_config(struct mtd_info *mtd, struct nand_chip *this, u8 id[8])
{
	if (nand_config_item.pagesize == 4096) {
		this->ecc.size = nand_config_item.eccsize;
		g_info.ecc_mode = nand_config_item.eccbit;
		/* 4 bit ecc, per 512 bytes can creat 13 * 4 = 52 bit , 52 / 8 = 7 bytes
		   8 bit ecc, per 512 bytes can creat 13 * 8 = 104 bit , 104 / 8 = 13 bytes */
		switch (g_info.ecc_mode) {
			case 4:
				/* 4 bit ecc, per 512 bytes can creat 13 * 4 = 52 bit , 52 / 8 = 7 bytes */
				this->ecc.bytes = 7;
				this->ecc.layout = &_nand_oob_128;
			break;
			case 8:
				/* 8 bit ecc, per 512 bytes can creat 13 * 8 = 104 bit , 104 / 8 = 13 bytes */
				this->ecc.bytes = 14;
				if (nand_config_item.oobsize == 224)
					this->ecc.layout = &_nand_oob_224;
				else
					this->ecc.layout = &_nand_oob_256;
				mtd->oobsize = nand_config_item.oobsize;
			break;
		}
	}

	if(nand_check_2bitconfig_4bitecc())
	{
	       this->ecc.size = nand_config_item.eccsize;
	       g_info.ecc_mode = nand_config_item.eccbit;
           this->ecc.bytes = 7;
           this->ecc.layout = &_nand_oob_64_4bit;
		   printk("ecc size=%d, ecc mode=%d\n", this->ecc.size, g_info.ecc_mode);
	}
}

int board_nand_init(struct nand_chip *this)
{
	g_info.chip = this;
	g_info.ecc_mode = CONFIG_SYS_NAND_ECC_MODE;
	read_chip_id();
    ptr_nand_spec = get_nand_spec(io_wr_port);
	sc8810_nand_hw_init();

	this->IO_ADDR_R = this->IO_ADDR_W = (void __iomem*)NFC_MBUF_ADDR;
	this->cmd_ctrl = sc8810_nand_hwcontrol;
	this->dev_ready = sc8810_nand_devready;
	this->select_chip = sc8810_nand_select_chip;

	this->ecc.calculate = sc8810_nand_calculate_ecc;
	this->ecc.correct = sc8810_nand_correct_data;
	this->ecc.hwctl = sc8810_nand_enable_hwecc;
	this->ecc.mode = NAND_ECC_HW;
	this->ecc.size = CONFIG_SYS_NAND_ECCSIZE;//512;
	this->ecc.bytes = CONFIG_SYS_NAND_ECCBYTES;//3
	this->read_buf = sc8810_nand_read_buf;
	this->write_buf = sc8810_nand_write_buf;
	this->read_byte	= sc8810_nand_read_byte;
	this->read_word	= sc8810_nand_read_word;

	this->chip_delay = 20;
	this->priv = &g_info;
	this->options |= NAND_BUSWIDTH_16;
	return 0;
}
static struct sprd_platform_nand *to_nand_plat(struct platform_device *dev)
{
	return dev->dev.platform_data;
}


//linux driver layout

/*device drive  registration */
static struct mtd_info *sprd_mtd = NULL;
#ifdef CONFIG_MTD_PARTITIONS
const char *part_probes[] = { "cmdlinepart", NULL };
#endif

#ifndef MODULE
/*
 * Parse the command line.
 */
static inline unsigned long my_atoi(const char *name, int base)
{
	unsigned long val = 0;

	for (;; name++) {
		if (((*name >= '0') && (*name <= '9')) || ((*name >= 'a') && (*name <= 'f'))) {

			switch (*name) {
			case '0' ... '9':
				val = base * val + (*name - '0');
			break;
			case 'a' ... 'f':
				val = base * val + (*name - 'a' + 10);
			break;
			}
		} else
			break;
	}

	return val;
}

static int nandflash_setup_real(char *s)
{
	char *p, *vp;
	unsigned long cnt;

	nandflash_parsed = 1;

	p = strstr(s, "nandid");
	p += strlen("nandid");

	for (cnt = 0; cnt < 5; cnt ++) {
		vp = strstr(p, "0x");
		vp += strlen("0x");
		p = vp;

		switch (cnt) {
		case 0:
			nand_config_item.m_c = my_atoi(vp, 16);
		break;
		case 1:
			nand_config_item.d_c = my_atoi(vp, 16);
		break;
		case 2:
			nand_config_item.cyc_3 = my_atoi(vp, 16);
		break;
		case 3:
			nand_config_item.cyc_4 = my_atoi(vp, 16);
		break;
		case 4:
			nand_config_item.cyc_5 = my_atoi(vp, 16);
		break;
		}
    }
    p = strstr(s, "pagesize(");
	p += strlen("pagesize(");
	nand_config_item.pagesize = my_atoi(p, 10);

	p = strstr(s, "oobsize(");
	p += strlen("oobsize(");
	nand_config_item.oobsize = my_atoi(p, 10);

	p = strstr(s, "eccsize(");
	p += strlen("eccsize(");
	nand_config_item.eccsize = my_atoi(p, 10);

	p = strstr(s, "eccbit(");
	p += strlen("eccbit(");
	nand_config_item.eccbit = my_atoi(p, 10);

	return 1;
}
/**
 *	nandflash_setup - process command line options
 *	@options: string of options
 *
 *	Process command line options for nand flash subsystem.
 *
 *	NOTE: This function is a __setup and __init function.
 *            It only stores the options.  Drivers have to call
 *            nandflash_setup_real() as necessary.
 *
 *	Returns zero.
 *
 */
static int __init nandflash_setup(char *options)
{
	nandflash_cmdline = options;

	return 1;
}
__setup("nandflash=", nandflash_setup);
#endif

static int sprd_nand_probe(struct platform_device *pdev)
{
	struct nand_chip *this;
	struct sprd_nand_info info;
	struct sprd_platform_nand *plat = pdev->dev.platform_data;/* get timing */
	struct resource *regs = NULL;

	struct mtd_partition *partitions = NULL;
	int num_partitions = 0;
#ifdef  NAND_IRQ_EN
        int err = 0;
#endif
	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs) {
		dev_err(&pdev->dev,"resources unusable\n");
		goto Err;
	}
	nand_base = regs->start;

	wake_lock_init(&nfc_wakelock, WAKE_LOCK_SUSPEND, "nfc_wakelock");
#ifdef  NAND_IRQ_EN
        init_completion(&sc8810_op_completion);
        err = request_irq(IRQ_NLC_INT, sc8810_nfc_irq, 0, DRIVER_NAME, NULL);
	if (err)
		goto Err;
#endif

	if (!nandflash_parsed)
		nandflash_setup_real(nandflash_cmdline);

	/*printk("\n0x%02x  0x%02x  0x%02x  0x%02x  0x%02x  %d  %d  %d  %d\n", nand_config_item.m_c, nand_config_item.d_c, nand_config_item.cyc_3, nand_config_item.cyc_4, nand_config_item.cyc_5, nand_config_item.pagesize, nand_config_item.oobsize, nand_config_item.eccsize, nand_config_item.eccbit);*/

	memset(io_wr_port, 0xff, NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE);

	memset(&info, 0 , sizeof(struct sprd_nand_info));
	memset(fix_timeout_id, 0x0, sizeof(fix_timeout_id));

	platform_set_drvdata(pdev, &info);/* platform_device.device.driver_data IS info */
	info.platform = plat; /* nand timing */
	info.pdev = pdev;
	sprd_nand_inithw(&info, pdev);

	sprd_mtd = kmalloc(sizeof(struct mtd_info) + sizeof(struct nand_chip), GFP_KERNEL);
	this = (struct nand_chip *)(&sprd_mtd[1]);
	memset((char *)sprd_mtd, 0, sizeof(struct mtd_info));
	memset((char *)this, 0, sizeof(struct nand_chip));

	sprd_mtd->priv = this;

	if (1) {
		sprd_config_nand_pins16();
		this->options |= NAND_BUSWIDTH_16;
		this->options |= NAND_NO_READRDY;
	} else {
		sprd_config_nand_pins8();
	}

	board_nand_init(this);

	/* scan to find existance of the device */
	this->options |= NAND_USE_FLASH_BBT;
	nand_scan(sprd_mtd, 1);

	sprd_mtd->name = "sprd-nand";
	num_partitions = parse_mtd_partitions(sprd_mtd, part_probes, &partitions, 0);

	/*printk("num_partitons = %d\n", num_partitions);
	for (i = 0; i < num_partitions; i++) {
		printk("i=%d  name=%s  offset=0x%016Lx  size=0x%016Lx\n", i, partitions[i].name,
			(unsigned long long)partitions[i].offset, (unsigned long long)partitions[i].size);
	}*/

	if ((!partitions) || (num_partitions == 0)) {
		printk("No parititions defined, or unsupported device.\n");
		goto release;
	}
	add_mtd_partitions(sprd_mtd, partitions, num_partitions);

	REG_AHB_CTL0 &= ~BIT_8; /* disabel nfc clock */

	return 0;
release:
	nand_release(sprd_mtd);
Err:
	return 0;
}

/* device management functions */
static int sprd_nand_remove(struct platform_device *pdev)
{
	platform_set_drvdata(pdev, NULL);

	del_mtd_partitions(sprd_mtd);
	del_mtd_device(sprd_mtd);
	kfree(sprd_mtd);

	return 0;
}

/* PM Support */
#ifdef CONFIG_PM
static unsigned long nfc_reg_cfg0;
static int sprd_nand_suspend(struct platform_device *dev, pm_message_t pm)
{
#if 0
	struct sprd_nand_info *info = platform_get_drvdata(dev);

	if (info)
		clk_disable(info->clk);
#else
	nfc_reg_cfg0 = nfc_reg_read(NFC_CFG0); /* save CS_SEL */
	nfc_reg_write(NFC_CFG0, (nfc_reg_cfg0 | BIT_6)); /* deset CS_SEL */
	REG_AHB_CTL0 &= ~BIT_8; /* disabel nfc clock */
#endif

	return 0;
}

static int sprd_nand_resume(struct platform_device *dev)
{
#if 0
	struct sprd_nand_info *info = platform_get_drvdata(dev);

	if (info) {
		clk_enable(info->clk);
		sprd_nand_inithw(info, dev);
	}
#else
	int ik_cnt = 0;

	REG_AHB_CTL0 |= BIT_8;//no BIT_9 /* enable nfc clock */
	REG_AHB_SOFT_RST |= BIT_5;
	for(ik_cnt = 0; ik_cnt < 0xffff; ik_cnt++);
	REG_AHB_SOFT_RST &= ~BIT_5;

	nfc_reg_write(NFC_CFG0, nfc_reg_cfg0); /* set CS_SEL */
	sc8810_nand_wp_en(0);

    if (ptr_nand_spec != NULL)
        set_nfc_timing(&ptr_nand_spec->timing_cfg, 153);

//	nfc_reg_write(NFC_TIMING, ((6 << 0) | (6 << 5) | (10 << 10) | (6 << 16) | (5 << 21) | (5 << 26)));
    nfc_reg_write(NFC_TIMING, ((12 << 0) | (7 << 5) | (10 << 10) | (6 << 16) | (5 << 21) | (7 << 26)));
	nfc_reg_write(NFC_TIMING+0X4, 0xffffffff);//TIMEOUT
#endif

	return 0;
}
#else
#define sprd_nand_suspend NULL
#define sprd_nand_resume NULL
#endif

static struct platform_driver sprd_nand_driver = {
	.probe		= sprd_nand_probe,
	.remove		= sprd_nand_remove,
	.suspend	= sprd_nand_suspend,
	.resume		= sprd_nand_resume,
	.driver		= {
		.name	= "sprd_nand",
		.owner	= THIS_MODULE,
	},
};

static int __init sprd_nand_init(void)
{
	printk("\nSpreadtrum NAND Driver, (c) 2011 Spreadtrum\n");
	return platform_driver_register(&sprd_nand_driver);
}

static void __exit sprd_nand_exit(void)
{
	platform_driver_unregister(&sprd_nand_driver);
}

module_init(sprd_nand_init);
module_exit(sprd_nand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("spreadtrum.com>");
MODULE_DESCRIPTION("SPRD 8810 MTD NAND driver");

