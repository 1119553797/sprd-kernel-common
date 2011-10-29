/* 
 * sound/soc/sc88xx/vbc-codec.c
 *
 * VBC -- SpreadTrum sc88xx intergrated Dolphin codec.
 *
 * Copyright (C) 2010 SpreadTrum Ltd.
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
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/kernel.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/soc-dapm.h>
#include <linux/delay.h>
#include <sound/tlv.h>

#include "sc88xx-asoc.h"

#define POWER_OFF_ON_STANDBY    0
#define VBC_CODEC_RESET    0xffff
#define VBC_CODEC_POWER    0xfffe
#define VBC_CODEC_POWER_ON_OUT  (1 << 0)
#define VBC_CODEC_POWER_ON_IN   (1 << 1)
#define VBC_CODEC_POWER_OFF_OUT (1 << 2)
#define VBC_CODEC_POWER_OFF_IN  (1 << 3)
#define VBC_CODEC_SPEAKER_PA 0xfffd
#define VBC_CODEC_DSP      0xfffc
/*
  ALSA SOC usually puts the device in standby mode when it's not used
  for sometime. If you define POWER_OFF_ON_STANDBY the driver will
  turn off the ADC/DAC when this callback is invoked and turn it back
  on when needed. Unfortunately this will result in a very light bump
  (it can be audible only with good earphones). If this bothers you
  just comment this line, you will have slightly higher power
  consumption . 
 */
static const unsigned int dac_tlv[] = {
    TLV_DB_RANGE_HEAD(1),
        0, 0xf, TLV_DB_LINEAR_ITEM(5, 450),
};

static const unsigned int adc_tlv[] = {
    TLV_DB_RANGE_HEAD(1),
        0, 0xf, TLV_DB_LINEAR_ITEM(0, 4500),
};

static const char *vbc_mic_sel[] = {
    "1",
    "2",
};

static const struct soc_enum vbc_mic12_enum = 
    SOC_ENUM_SINGLE(VBCR2 & 0xffff,
        MICSEL,
        2,
        vbc_mic_sel);

static const char *vbc_codec_reset_enum_sel[] = {
    "false",
    "true",
};

static const struct soc_enum vbc_codec_reset_enum =
    SOC_ENUM_SINGLE(VBC_CODEC_RESET,
        0,
        2,
        vbc_codec_reset_enum_sel);

#define VBC_PCM_CTRL(name) \
    SOC_SINGLE(name" Playback Switch", VBCR1, DAC_MUTE, 1, 1), \
    SOC_DOUBLE_TLV(name" Playback Volume", VBCGR1, 0, 4, 0x0f, 1, dac_tlv), \
    SOC_SINGLE_TLV(name" Left Playback Volume", VBCGR1, 0, 0x0f, 1, dac_tlv), \
    SOC_SINGLE_TLV(name" Right Playback Volume",VBCGR1, 4, 0x0f, 1, dac_tlv)

static const struct snd_kcontrol_new vbc_snd_controls[] = {
    // Mic
    SOC_ENUM("Micphone", vbc_mic12_enum),
    // PCM
    SOC_SINGLE("PCM Playback Switch", VBCR1, DAC_MUTE, 1, 1),
    SOC_DOUBLE_TLV("PCM Playback Volume", VBCGR1, 0, 4, 0x0f, 1, dac_tlv),
    SOC_SINGLE_TLV("PCM Left Playback Volume", VBCGR1, 0, 0x0f, 1, dac_tlv),
    SOC_SINGLE_TLV("PCM Right Playback Volume",VBCGR1, 4, 0x0f, 1, dac_tlv),
    // Speaker
    /* SOC_SINGLE("Speaker Playback Switch", VBCR1, xxx, 1, 1), */
    SOC_SINGLE("Speaker Playback Switch", VBC_CODEC_SPEAKER_PA, 0, 1, 0),
    SOC_DOUBLE_TLV("Speaker Playback Volume", VBCGR1, 0, 4, 0x0f, 1, dac_tlv),
    SOC_SINGLE_TLV("Speaker Left Playback Volume", VBCGR1, 0, 0x0f, 1, dac_tlv),
    SOC_SINGLE_TLV("Speaker Right Playback Volume",VBCGR1, 4, 0x0f, 1, dac_tlv),
    // Earpiece
    SOC_SINGLE("Earpiece Playback Switch", VBCR1, BTL_MUTE, 1, 1),
    SOC_DOUBLE_TLV("Earpiece Playback Volume", VBCGR1, 0, 4, 0x0f, 1, dac_tlv),
    SOC_SINGLE_TLV("Earpiece Left Playback Volume", VBCGR1, 0, 0x0f, 1, dac_tlv),
    SOC_SINGLE_TLV("Earpiece Right Playback Volume",VBCGR1, 4, 0x0f, 1, dac_tlv),
    // Bypass
    SOC_SINGLE("BypassFM Playback Switch", VBCR1, BYPASS, 1, 0),
    SOC_DOUBLE_R_TLV("BypassFM Playback Volume", VBCGR2, VBCGR3, 0, 0x0f, 1, dac_tlv),
    SOC_SINGLE_TLV("BypassFM Left Playback Volume", VBCGR2, 0, 0x0f, 1, dac_tlv),
    SOC_SINGLE_TLV("BypassFM Right Playback Volume",VBCGR3, 0, 0x0f, 1, dac_tlv),
    // Linein
    SOC_SINGLE("LineinFM", VBPMR1, SB_LIN, 1, 1),
    // Headset
    SOC_SINGLE("Headset Playback Switch", VBCR1, HP_DIS, 1, 1),
    SOC_DOUBLE_TLV("Headset Playback Volume", VBCGR1, 0, 4, 0x0f, 1, dac_tlv),
    SOC_SINGLE_TLV("Headset Left Playback Volume", VBCGR1, 0, 0x0f, 1, dac_tlv),
    SOC_SINGLE_TLV("Headset Right Playback Volume",VBCGR1, 4, 0x0f, 1, dac_tlv),
    // Capture
    SOC_SINGLE_TLV("Capture Capture Volume", VBCGR10, 4, 0x0f, 0, adc_tlv),
    // reset codec
    SOC_ENUM("Reset Codec", vbc_codec_reset_enum),
    // codec power
    //     poweron   poweroff
    // bit 0    1     2     3   4 ... 31
    //     out  in    out   in
    // value 1 is valid, value 0 is invalid
    SOC_SINGLE("Power Codec", VBC_CODEC_POWER, 0, 31, 0),
    SOC_SINGLE("InCall", VBC_CODEC_DSP, 0, 1, 0),
};

static const struct snd_soc_dapm_widget vbc_dapm_widgets[] = {
    
};

/* vbc supported audio map */
static const struct snd_soc_dapm_route audio_map[] = {
    
};

static int vbc_add_widgets(struct snd_soc_codec *codec)
{
    snd_soc_dapm_new_controls(codec, vbc_dapm_widgets,
				  ARRAY_SIZE(vbc_dapm_widgets));

    snd_soc_dapm_add_routes(codec, audio_map, ARRAY_SIZE(audio_map));

    // snd_soc_init_card will do this.
    // snd_soc_dapm_new_widgets(codec); 

    return 0;
}

static inline void vbc_reg_VBAICR_set(u8 mode)
{
    vbc_reg_write(VBAICR, 0, mode, 0x0F);
}

static inline int vbc_reg_VBCR1_set(u32 type, u32 val)
{
    return vbc_reg_write(VBCR1, type, val, 1);
}

static void vbc_reg_VBCR2_set(u32 type, u32 val)
{
    u32 mask;

    switch (type) {
        case ADC_ADWL:
        case DAC_ADWL:
            mask = 0x03; break;
        default: 
            mask = 1; break;
    }
    vbc_reg_write(VBCR2, type, val, mask);
}

static inline void vbc_reg_VBPMR2_set(u32 type, u32 val)
{
    vbc_reg_write(VBPMR2, type, val, 1);
}

static inline void vbc_reg_VBPMR1_set(u32 type, u32 val)
{
    vbc_reg_write(VBPMR1, type, val, 1);
}

static inline void vbc_set_AD_DA_fifo_frame_num(u8 da_fifo_fnum, u8 ad_fifo_fnum)
{
    vbc_reg_write(VBBUFFSIZE, 0, ((da_fifo_fnum-1)<<8) | ((ad_fifo_fnum-1)), 0xffff);
}

static inline void vbc_reg_VBDABUFFDTA_set(u32 type, u32 val)
{
    vbc_reg_write(VBDABUFFDTA, type, val, 1);
}

static inline void vbc_set_VBADBUFFDTA_set(u32 type, u32 val)
{
    vbc_reg_write(VBADBUFFDTA, type, val, 1);
}

static inline void vbc_access_buf(int enable)
{
    // Software access ping-pong buffer enable when VBENABE bit low
    vbc_reg_VBDABUFFDTA_set(RAMSW_EN, enable ? 1:0);
}

static void vbc_buffer_clear(int id)
{
    int i;
    vbc_reg_VBDABUFFDTA_set(RAMSW_NUMB, id ? 1:0);
    for (i = 0; i < VBC_FIFO_FRAME_NUM; i++) {
        __raw_writel(0, VBDA0);
        __raw_writel(0, VBDA1);
    }
}

static void vbc_buffer_clear_all(void)
{
    vbc_access_buf(true);
    vbc_buffer_clear(1); // clear data buffer 1
    vbc_buffer_clear(0); // clear data buffer 0
    vbc_access_buf(false);
}

static void vbc_codec_mute(void)
{
    vbc_reg_VBCR1_set(DAC_MUTE, 1); // mute
}

static void vbc_codec_unmute(void)
{
    vbc_reg_VBCR1_set(DAC_MUTE, 0); // don't mute
}

static void vbc_set_ctrl2arm(void)
{
    // Enable VB DAC0/DAC1 (ARM-side)
    __raw_bits_or(ARM_VB_MCLKON|ARM_VB_ACC|ARM_VB_DA0ON|ARM_VB_ANAON|ARM_VB_DA1ON|ARM_VB_ADCON, SPRD_VBC_ALSA_CTRL2ARM_REG);
    msleep(1);
}

#if defined(CONFIG_ARCH_SC8800G)
u16 __raw_adi_read(u32 addr)
{
    u32 adi_data, phy_addr;
    check_range(addr);
    phy_addr = addr - SPRD_ADI_BASE + SPRD_ADI_PHYS;
    //enter_critical();
    __raw_writel(phy_addr, ADI_ARM_RD_CMD);
    do {
        adi_data = __raw_readl(ADI_RD_DATA);
    } while (adi_data & (1 << 31));
    //exit_critical();
//  lprintf("addr=0x%08x, phy=0x%08x, val=0x%04x\n", addr, phy_addr, adi_data & 0xffff);
    return adi_data & 0xffff;
}
EXPORT_SYMBOL_GPL(__raw_adi_read);

int __raw_adi_write(u32 data, u32 addr)
{
    check_range(addr);
    //enter_critical();
    while ((__raw_readl(ADI_FIFO_STS) & ADI_FIFO_EMPTY) == 0);
    __raw_writel(data, addr);
    //exit_critical();
    return 1;
}
EXPORT_SYMBOL_GPL(__raw_adi_write);

// adi_analogdie ANA_REG_MSK_OR
static void __raw_adi_and(u32 value, u32 addr)
{
    enter_critical();
    __raw_adi_write(__raw_adi_read(addr) & value, addr);
    exit_critical();
}

static void __raw_adi_or(u32 value, u32 addr)
{
    enter_critical();
    __raw_adi_write(__raw_adi_read(addr) | value, addr);
    exit_critical();
}
#endif

static void vbc_ldo_on(bool on)
{
    int do_on_off = 0;
    if (on) {
#ifdef CONFIG_ARCH_SC8800S
        if (!(__raw_readl(GR_LDO_CTL0) & (1 << 17))) {
            __raw_bits_or(1 << 17, GR_LDO_CTL0); // LDO_VB_PO
            do_on_off = 1;
        }
#elif defined(CONFIG_ARCH_SC8800G)
        if (!(__raw_adi_read(ANA_LDO_PD_CTL) & (1 << 15))) {
            __raw_adi_or(1 << 15, ANA_LDO_PD_CTL);
            do_on_off = 1;
        }
#endif
    } else {
#ifdef CONFIG_ARCH_SC8800S
        if ((__raw_readl(GR_LDO_CTL0) & (1 << 17))) {
            __raw_bits_and(~(1 << 17), GR_LDO_CTL0); // LDO_VB_PO
            do_on_off = 1;
        }
#elif defined(CONFIG_ARCH_SC8800G)
        if ((__raw_adi_read(ANA_LDO_PD_CTL) & (1 << 15))) {
            __raw_adi_and(~(1 << 15), ANA_LDO_PD_CTL);
            do_on_off = 1;
        }
#endif
    }
    if (do_on_off) {
        printk("+++++++++++++ audio set ldo to %s +++++++++++++\n", on ? "on" : "off");
    }
}

static void vbc_set_mainclk_to12M(void)
{
#ifdef CONFIG_ARCH_SC8800S
	u32 vpll_clk = CHIP_GetVPllClk();
	clk_12M_divider_set(vpll_clk);
    __raw_bits_or(1 << 17, GR_LDO_CTL0); // LDO_VB_PO
#elif defined(CONFIG_ARCH_SC8800G)
    __raw_bits_or(GEN0_VB_EN | GEN0_ADI_EN, GR_GEN0); // Enable voiceband module
    __raw_bits_or(1 << 29, GR_CLK_DLY); // CLK_ADI_EN_ARM and CLK_ADI_SEL=76.8MHZ
    __raw_adi_or(1 << 15, ANA_LDO_PD_CTL);
    __raw_adi_or(VBMCLK_ARM_EN | VBCTL_SEL, ANA_CLK_CTL);
#endif
}

static inline void vbc_ready2go(void)
{
    // Enable this bit then VBC interface starts working and software 
    // can receive voice band interrupt. 
    // Better set this bit after all other register bits are programmed. 
    vbc_reg_VBDABUFFDTA_set(VBENABLE, 1);
    msleep(2);
}

extern inline int vbc_amplifier_enabled(void);
extern inline void vbc_amplifier_enable(int enable, const char *prename);
static DEFINE_MUTEX(vbc_power_lock);
static volatile int earpiece_muted = 1, headset_muted = 1, speaker_muted = 1;
void vbc_write_callback(unsigned int reg, unsigned int val)
{
    if (reg == VBCR1) {
       headset_muted  = !!(val & (1 << HP_DIS));
       earpiece_muted = !!(val & (1 << BTL_MUTE));
       printk("[headset_muted =%d]\n"
              "[earpiece_muted=%d]\n"
              "[speaker_muted =%d]\n", headset_muted, earpiece_muted, speaker_muted);
    }
}

void vbc_power_down(unsigned int value)
{
    mutex_lock(&vbc_power_lock);
    // printk("audio %s\n", __func__);
    {
        int do_sb_power = 0;
        // int VBCGR1_value;
        if ((vbc_reg_read(VBPMR1, SB_ADC, 1)
             && (value == SNDRV_PCM_STREAM_PLAYBACK && !vbc_reg_read(VBPMR1, SB_DAC, 1))) ||
            (vbc_reg_read(VBPMR1, SB_DAC, 1)
             && (value == SNDRV_PCM_STREAM_CAPTURE && !vbc_reg_read(VBPMR1, SB_ADC, 1))) ||
            (vbc_reg_read(VBPMR1, SB_ADC, 1) && vbc_reg_read(VBPMR1, SB_DAC, 1))) {
            do_sb_power = 1;
        }

        if ((value == -1) ||
            (value == SNDRV_PCM_STREAM_PLAYBACK &&
            (!vbc_reg_read(VBPMR1, SB_DAC, 1) ||
            !earpiece_muted || !headset_muted || !speaker_muted))) {
            // VBCGR1_value = vbc_reg_write(VBCGR1, 0, 0xff, 0xff); // DAC Gain
            msleep(100); // avoid quick switch from power on to off
            /*
            earpiece_muted= vbc_reg_read(VBCR1, BTL_MUTE, 1);
            headset_muted = vbc_reg_read(VBCR1, HP_DIS, 1);
            speaker_muted = vbc_amplifier_enabled();
            */
            vbc_reg_VBCR1_set(BTL_MUTE, 1); // Mute earpiece
            vbc_reg_VBCR1_set(HP_DIS, 1); // Mute headphone
            vbc_amplifier_enable(false, "vbc_power_down playback"); // Mute speaker
            vbc_codec_mute();
            msleep(50);

            vbc_reg_VBPMR1_set(SB_OUT, 1); // Power down DAC OUT
            vbc_reg_VBCR1_set(MONO, 1); // mono DAC channel
            vbc_reg_VBPMR1_set(SB_BTL, 1); // power down earphone
            msleep(100);

            vbc_reg_VBPMR1_set(SB_DAC, 1); // Power down DAC
            vbc_reg_VBPMR1_set(SB_LOUT, 1);
            vbc_reg_VBPMR1_set(SB_MIX, 1);
            // msleep(50);
            // vbc_reg_write(VBCGR1, 0, VBCGR1_value, 0xff); // DAC Gain
        }
        if ((value == -1) ||
            (value == SNDRV_PCM_STREAM_CAPTURE &&
            !vbc_reg_read(VBPMR1, SB_ADC, 1))) {
            printk("vbc_power_down capture\n");
            vbc_reg_VBPMR1_set(SB_ADC, 1); // Power down ADC
            vbc_reg_VBCR1_set(SB_MICBIAS, 1); // power down mic
        }
        if ((value == -1) ||
            do_sb_power)/* vbc_reg_read(VBPMR1, SB_ADC, 1) && vbc_reg_read(VBPMR1, SB_DAC, 1) */ {
            vbc_reg_VBPMR2_set(SB_SLEEP, 1); // SB enter sleep mode
            vbc_reg_VBPMR2_set(SB, 1); // Power down sb
            msleep(100); // avoid quick switch from power off to on
            vbc_ldo_on(0);
            printk("....................... audio full power down .......................\n");
        }
    }
    mutex_unlock(&vbc_power_lock);
}
EXPORT_SYMBOL_GPL(vbc_power_down);

void vbc_power_on(unsigned int value)
{
    mutex_lock(&vbc_power_lock);
    vbc_ldo_on(1);
    // printk("audio %s\n", __func__);
    {
        if (value == SNDRV_PCM_STREAM_PLAYBACK &&
            (vbc_reg_read(VBPMR1, SB_DAC, 1) ||
             vbc_reg_read(VBPMR1, SB_LOUT, 1)||
             vbc_reg_read(VBPMR1, SB_OUT, 1) ||
             vbc_reg_read(VBPMR1, SB_MIX, 1) ||
             vbc_reg_read(VBPMR2, SB, 1)     ||
             vbc_reg_read(VBPMR2, SB_SLEEP, 1))) {
            int forced = 0;
            // int VBCGR1_value;

            // VBCGR1_value = vbc_reg_write(VBCGR1, 0, 0xff, 0xff); // DAC Gain
            vbc_reg_VBPMR2_set(SB, 0); // Power on sb
            vbc_reg_VBPMR2_set(SB_SLEEP, 0); // SB quit sleep mode

            vbc_codec_mute();
            /* earpiece_muted = */ vbc_reg_VBCR1_set(BTL_MUTE, 1); // Mute earpiece
            /* headset_muted =  */ vbc_reg_VBCR1_set(HP_DIS, 1); // Mute headphone
            /* speaker_muted =  */ vbc_amplifier_enable(false, "vbc_power_on playback"); // Mute speaker
            msleep(50);

            vbc_reg_VBPMR1_set(SB_DAC, 0); // Power on DAC
            msleep(50);
            vbc_reg_VBPMR1_set(SB_LOUT, 0);
            msleep(50);
            vbc_reg_VBPMR1_set(SB_MIX, 0);
            msleep(50);

            vbc_reg_VBPMR1_set(SB_OUT, 0); // Power on DAC OUT
            msleep(50);
            vbc_reg_VBCR1_set(MONO, 0); // stereo DAC left & right channel
            vbc_reg_VBPMR1_set(SB_BTL, 0); // power on earphone
            msleep(100);

            vbc_codec_unmute();
            if (!earpiece_muted || forced) vbc_reg_VBCR1_set(BTL_MUTE, 0); // unMute earpiece
            if (!headset_muted || forced) vbc_reg_VBCR1_set(HP_DIS, 0); // unMute headphone
            if (!speaker_muted || forced) vbc_amplifier_enable(true, "vbc_power_on playback"); // unMute speaker
            // vbc_reg_write(VBCGR1, 0, VBCGR1_value, 0xff); // DAC Gain
            if (speaker_muted && forced) {
                printk("[headset_muted =%d]\n"
                       "[earpiece_muted=%d]\n"
                       "[speaker_muted =%d]\n", headset_muted, earpiece_muted, speaker_muted);
            }
        }
        if (value == SNDRV_PCM_STREAM_CAPTURE &&
            vbc_reg_read(VBPMR1, SB_ADC, 1)) {
            printk("vbc_power_on capture\n");
            vbc_reg_VBPMR2_set(SB, 0); // Power on sb
            vbc_reg_VBPMR2_set(SB_SLEEP, 0); // SB quit sleep mode
            vbc_reg_VBCR1_set(SB_MICBIAS, 0); // power on mic
            vbc_reg_VBPMR1_set(SB_ADC, 0); // Power on ADC
        }
    }
    mutex_unlock(&vbc_power_lock);
}
EXPORT_SYMBOL_GPL(vbc_power_on);

static int vbc_reset(struct snd_soc_codec *codec)
{
    // 1. dial phone number
    // 2. modem will set DSP control audio codec
    // 3. DSP control audio codec
    // 4. in call
    // 5. AT+ATH to quit call
    // 6. DSP will release audio chain
    // 7. modem will set ARM control audio codec
    // 8. android will reset audio codec to ARM & setting android alsa himself needed audio parameters
    //
    // The problem occures in step 7 & 8, if 8 first occures, pop sound will be created, and
    // alsa DMA can't be work, AudioFlinger will can't obtainBuffer from alsa driver [luther.ge]
    {
        // vbc_amplifier_enable(false, "vbc_init"); // Mute Speaker
        // fix above problem
        while (!(__raw_readl(SPRD_VBC_ALSA_CTRL2ARM_REG) & ARM_VB_ACC)) {
            printk("vbc waiting DSP release audio codec ......\n");
            msleep(100);
        }
        printk("vbc waiting modem stable setting audio codec ...... start ......\n");
        msleep(200);
        printk("vbc waiting modem stable setting audio codec ...... done ......\n");
        // atomic_read
    }
    vbc_set_mainclk_to12M();
    vbc_set_ctrl2arm();
#if 0
    vbc_codec_mute();
#endif
    vbc_ready2go();

    vbc_set_AD_DA_fifo_frame_num(VBC_FIFO_FRAME_NUM, VBC_FIFO_FRAME_NUM);

    vbc_buffer_clear_all(); // must have this func, or first play will have noise

    // IIS timeing : High(right channel) for both DA1/AD1, Low(left channel) for both DA0/AD0
    // Active level of left/right channel for both ADC and DAC channel 
    vbc_set_VBADBUFFDTA_set(VBIIS_LRCK, 0);

    vbc_reg_VBAICR_set(VBCAICR_MODE_ADC_I2S    |
                       VBCAICR_MODE_DAC_I2S    |
                       VBCAICR_MODE_ADC_SERIAL |
                       VBCAICR_MODE_DAC_SERIAL);

    vbc_reg_VBCR2_set(DAC_ADWL, DAC_DATA_WIDTH_16_bit); // DAC data sample depth 16bits
    vbc_reg_VBCR2_set(ADC_ADWL, ADC_DATA_WIDTH_16_bit); // ADC data sample depth 16bits
    vbc_reg_VBCR2_set(MICSEL, MICROPHONE1); // route microphone 1 to ADC module

    vbc_reg_write(VBCCR2, 4, VBC_RATE_8000, 0xf); // 8K sample DAC
    vbc_reg_write(VBCCR2, 0, VBC_RATE_8000, 0xf); // 8K sample ADC

    vbc_reg_write(VBCGR1, 0, 0x00, 0xff); // DAC Gain
    vbc_reg_write(VBCGR8, 0, 0x00, 0x1f);
    vbc_reg_write(VBCGR9, 0, 0x00, 0x1f);
#if 0
    msleep(1);

    vbc_reg_VBPMR2_set(SB, 0); // Power on sb
    // Deleay between SB and SB_SLEEP, for stablizing the speaker output wave
    msleep(1);

    vbc_reg_VBPMR2_set(SB_SLEEP, 0); // SB quit sleep mode
    msleep(1);
    vbc_reg_VBCR1_set(SB_MICBIAS, 0); // power on mic
#else
    // vbc_power_on(SNDRV_PCM_STREAM_PLAYBACK);
#endif
    vbc_reg_VBPMR2_set(GIM, 1); // 20db gain mic amplifier
    vbc_reg_write(VBCGR10, 4, 0xf, 0xf); // set GI to max
#if 0
    // vbc_reg_write(VBPMR1, 1, 0x02, 0x7f); // power on all units, except SB_BTL
    vbc_reg_VBPMR1_set(SB_DAC, 0); // Power on DAC
//  vbc_reg_VBPMR1_set(SB_ADC, 0); // Power on ADC
    vbc_reg_VBPMR1_set(SB_ADC, 1); // Power down ADC
    vbc_reg_VBPMR1_set(SB_MIX, 0);
    vbc_reg_VBPMR1_set(SB_LOUT, 0);
    vbc_reg_VBPMR1_set(SB_OUT, 0); // Power on DAC OUT
    msleep(5);

    // mono use DA0 left channel
#ifdef CONFIG_ARCH_SC8800S
    vbc_reg_VBCR1_set(MONO, 0); // stereo DAC left & right channel
    vbc_reg_VBCR1_set(HP_DIS, 1); // not route mixer audio data to headphone outputs
    vbc_reg_VBCR1_set(BYPASS, 0); // Analog bypass not route to mixer
    vbc_reg_VBCR1_set(DACSEL, 1); // route DAC to mixer
    vbc_reg_VBCR1_set(BTL_MUTE, 1); // Mute earpiece
#elif defined(CONFIG_ARCH_SC8800G)
    vbc_reg_VBCR1_set(MONO, 0); // stereo DAC left & right channel
    vbc_reg_VBCR1_set(HP_DIS, 0); // route mixer audio data to headphone outputs
    vbc_reg_VBCR1_set(BYPASS, 0); // Analog bypass not route to mixer
    vbc_reg_VBCR1_set(DACSEL, 1); // route DAC to mixer
    vbc_reg_VBCR1_set(BTL_MUTE, 0); // not Mute earpiece

    vbc_reg_VBPMR1_set(SB_BTL, 0); // power on earphone
    vbc_reg_VBPMR1_set(SB_LIN, 0);

    vbc_reg_write(DAHPCTL, 8, 1, 1); // headphone 24bits output to sound more appealing
#endif
    vbc_codec_unmute(); // don't mute
#endif

    return 0;
}

static int vbc_soft_ctrl(struct snd_soc_codec *codec, unsigned int reg, unsigned int value, int dir)
{
    // printk("vbc_soft_ctrl value[%d]=%04x\n", dir, reg);
    switch (reg) {
        case VBC_CODEC_RESET:
            // After phone call, we should reset all codec related registers
            // because in phone call state dsp will control codec, and set all registers
            // so we should reset all registers again in linux side,
            // otherwise android media will not work [luther.ge]
            // if (val & (1 << VBC_CODEC_SOFT_RESET))
            if (dir == 0) return 0; // dir 0 for read, we always return 0, so every set 1 value can reach here.
            // speaker_muted = true;
            vbc_reset(codec);
            vbc_power_down(-1);
            // vbc_reset(codec);
            if (!earpiece_muted) vbc_reg_VBCR1_set(BTL_MUTE, 0); // unMute earpiece
            if (!headset_muted) vbc_reg_VBCR1_set(HP_DIS, 0); // unMute headphone
            if (!speaker_muted) vbc_amplifier_enable(true, "vbc_soft_ctrl"); // unMute speaker
            return 0;
        case VBC_CODEC_POWER:
            if (dir == 0) return 0; // dir 0 for read, we always return 0, so every set 1 value can reach here.
            printk("vbc power to 0x%08x\n", value);
            if (value & VBC_CODEC_POWER_ON_OUT) {
                vbc_power_on(SNDRV_PCM_STREAM_PLAYBACK);
            }
            if (value & VBC_CODEC_POWER_ON_IN) {
                vbc_power_on(SNDRV_PCM_STREAM_CAPTURE);
            }
            if (value & VBC_CODEC_POWER_OFF_OUT) {
                vbc_power_down(SNDRV_PCM_STREAM_PLAYBACK);
            }
            if (value & VBC_CODEC_POWER_OFF_IN) {
                vbc_power_down(SNDRV_PCM_STREAM_CAPTURE);
            }
            return value;
        case VBC_CODEC_DSP:
            return !(__raw_readl(SPRD_VBC_ALSA_CTRL2ARM_REG) & ARM_VB_ACC);
        case VBC_CODEC_SPEAKER_PA:
            if (dir) {
                vbc_amplifier_enable(value & 0x01, "vbc_soft_ctrl2");
            }
            value = vbc_amplifier_enabled();
            speaker_muted = value ? 0:1;
            return value;
        default: return -1;
    }
}

static unsigned int vbc_read(struct snd_soc_codec *codec, unsigned int reg)
{
    int ret = vbc_soft_ctrl(codec, reg, 0, 0);
    if (ret >=0) return ret;
    // Because snd_soc_update_bits reg is 16 bits short type, so muse do following convert
    reg |= ARM_VB_BASE2;
#ifdef CONFIG_ARCH_SC8800S
    return __raw_readl(reg);
#elif defined(CONFIG_ARCH_SC8800G)
    return adi_read(reg);
#endif
}

static int vbc_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int val)
{
    int ret = vbc_soft_ctrl(codec, reg, val, 1);
    if (ret >=0) return ret;
    // Because snd_soc_update_bits reg is 16 bits short type, so muse do following convert
    reg |= ARM_VB_BASE2;
    vbc_write_callback(reg, val);
#ifdef CONFIG_ARCH_SC8800S
    __raw_writel(val, reg);
#elif defined(CONFIG_ARCH_SC8800G)
    adi_write(val, reg);
#endif
    return 0;
}

static void vbc_dma_control(int chs, bool on)
{
    if (chs & AUDIO_VBDA0)
        vbc_reg_VBDABUFFDTA_set(VBDA0DMA_EN, on); // DMA write DAC0 data buffer enable/disable
    if (chs & AUDIO_VBDA1)
        vbc_reg_VBDABUFFDTA_set(VBDA1DMA_EN, on); // DMA write DAC1 data buffer enable/disable
    if (chs & AUDIO_VBAD0)
        vbc_reg_VBDABUFFDTA_set(VBAD0DMA_EN, on); // DMA read ADC0 data buffer enable/disable
    if (chs & AUDIO_VBAD1)
        vbc_reg_VBDABUFFDTA_set(VBAD1DMA_EN, on); // DAM read ADC1 data buffer enable/disable
}

static inline void vbc_dma_start(struct snd_pcm_substream *substream)
{
    vbc_dma_control(audio_playback_capture_channel(substream), 1);
}

static inline void vbc_dma_stop(struct snd_pcm_substream *substream)
{
    vbc_dma_control(audio_playback_capture_channel(substream), 0);
}

void flush_vbc_cache(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    // we could not stop vbc_dma_buffer immediately, because audio data still in cache
    if (substream->stream != SNDRV_PCM_STREAM_PLAYBACK)
        return;
//  vbc_codec_mute();
    /* clear dma cache buffer */
    memset((void*)runtime->dma_area, 0, runtime->dma_bytes);
    printk("audio flush cache buffer...\n");
    if (cpu_codec_dma_chain_operate_ready(substream)) {
        vbc_dma_start(substream); // we must restart dma
        start_cpu_dma(substream);
        /* must wait all dma cache chain filled by 0 data */
//      lprintf("Filling all dma chain cache audio data to 0\n");
        msleep(20);
//      lprintf("done!\n");
        stop_cpu_dma(substream);
        vbc_dma_stop(substream);
    }
}
EXPORT_SYMBOL_GPL(flush_vbc_cache);

static int vbc_startup(struct snd_pcm_substream *substream,
    struct snd_soc_dai *dai)
{
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        vbc_buffer_clear_all();
    }
//  vbc_codec_unmute();
    return 0;
}

static int vbc_prepare(struct snd_pcm_substream *substream,
    struct snd_soc_dai *dai)
{
    // printk("vbc_prepare......\n");
    // vbc_power_on(substream->stream);
    return 0;
}

static void vbc_shutdown(struct snd_pcm_substream *substream,
    struct snd_soc_dai *dai)
{
    printk("vbc_shutdown......\n");
    // vbc_power_down(substream->stream);
#if 0
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        vbc_amplifier_enable(false, "sprdphone_shutdown");
#endif
}

#if POWER_OFF_ON_STANDBY
static int vbc_set_bias_level(struct snd_soc_codec *codec,
				 enum snd_soc_bias_level level)
{
    return 0;
}
#endif

static int vbc_set_dai_tristate(struct snd_soc_dai *codec_dai, int tristate)
{
    return 0;
}

static int vbc_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
    return 0;
}

static int vbc_set_dai_pll(struct snd_soc_dai *codec_dai,
		int pll_id, unsigned int freq_in, unsigned int freq_out)
{
    return 0;
}

static int vbc_set_dai_clkdiv(struct snd_soc_dai *codec_dai, int div_id, int div)
{
    return 0;
}

static int vbc_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;

	switch (cmd) {
        case SNDRV_PCM_TRIGGER_START:
            vbc_power_on(substream->stream);
            vbc_dma_start(substream);
            break;
        case SNDRV_PCM_TRIGGER_STOP:
            // vbc_power_down(substream->stream);
        case SNDRV_PCM_TRIGGER_SUSPEND:
        case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
            vbc_dma_stop(substream); // Stop DMA transfer
            break;
        case SNDRV_PCM_TRIGGER_RESUME:
        case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
            vbc_dma_start(substream);
            break;
        default:
            ret = -EINVAL;
	}

	return ret;
}

static int vbc_pcm_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
    int idx;

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        idx = 1;
    else idx = 0;

    switch (params_format(params)) {
        case SNDRV_PCM_FORMAT_S16_LE:
        case SNDRV_PCM_FORMAT_S16_BE:
        case SNDRV_PCM_FORMAT_U16_LE:
        case SNDRV_PCM_FORMAT_U16_BE: break;
        default: 
            printk(KERN_EMERG "VBC codec only supports format 16bits"); 
            break;
    }

    switch (params_rate(params)) {
        case  8000: vbc_reg_write(VBCCR2, idx * 4, VBC_RATE_8000 , 0xf); break;
        case 11025: vbc_reg_write(VBCCR2, idx * 4, VBC_RATE_11025, 0xf); break;
        case 16000: vbc_reg_write(VBCCR2, idx * 4, VBC_RATE_16000, 0xf); break;
        case 22050: vbc_reg_write(VBCCR2, idx * 4, VBC_RATE_22050, 0xf); break;
        case 32000: vbc_reg_write(VBCCR2, idx * 4, VBC_RATE_32000, 0xf); break;
        case 44100: vbc_reg_write(VBCCR2, idx * 4, VBC_RATE_44100, 0xf); break;
        case 48000: vbc_reg_write(VBCCR2, idx * 4, VBC_RATE_48000, 0xf); break;
        case 96000: vbc_reg_write(VBCCR2, idx * 4, VBC_RATE_96000, 0xf); break;
        default:
            printk(KERN_EMERG "VBC codec not supports rate %d\n", params_rate(params));
            break;
    }

    // lprintf("Sample Rate is [%d]\n", params_rate(params));

    return 0;
}

int vbc_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
    flush_vbc_cache(substream);
    return 0;
}

static struct snd_soc_dai_ops vbc_dai_ops = {
    .startup    = vbc_startup,
    .prepare    = vbc_prepare,
    .trigger    = vbc_trigger,
	.hw_params  = vbc_pcm_hw_params,
    .hw_free    = vbc_hw_free,
    .shutdown   = vbc_shutdown,
	.set_clkdiv = vbc_set_dai_clkdiv,
	.set_pll    = vbc_set_dai_pll,
	.set_fmt    = vbc_set_dai_fmt,
	.set_tristate = vbc_set_dai_tristate,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
static struct early_suspend early_suspend;
static void learly_suspend(struct early_suspend *es)
{
    // vbc_power_down();
}

static void learly_resume(struct early_suspend *es)
{
    // vbc_power_on();
}

static void android_pm_init(void)
{
    early_suspend.suspend = learly_suspend;
    early_suspend.resume = learly_resume;
    early_suspend.level = INT_MAX;
    register_early_suspend(&early_suspend);
}

static void android_pm_exit(void)
{
    unregister_early_suspend(&early_suspend);
}
#else
static void android_pm_init(void) {}
static void android_pm_exit(void) {}
#endif

#if 1
#include <mach/pm_devices.h>
static struct sprd_pm_suspend sprd_suspend;
static int lsprd_suspend(struct device *pdev, pm_message_t state)
{
    // vbc_power_down();
    return 0;
}

static int lsprd_resume(struct device *pdev)
{
    // vbc_power_on();
    return 0;
}

static void android_sprd_pm_init(void)
{
    sprd_suspend.suspend = lsprd_suspend;
    sprd_suspend.resume  = lsprd_resume;
    sprd_suspend.level   = INT_MAX;
    register_sprd_pm_suspend(&sprd_suspend);
}

static void android_sprd_pm_exit(void)
{
    unregister_sprd_pm_suspend(&sprd_suspend);
}
#else
static void android_sprd_pm_init(void) {}
static void android_sprd_pm_exit(void) {}
#endif

#ifdef CONFIG_PM
int vbc_suspend(struct platform_device *pdev, pm_message_t state)
{
    // vbc_power_down();
    return 0;
}

int vbc_resume(struct platform_device *pdev)
{
    // vbc_power_on();
    return 0;
}
#else
#define vbc_suspend NULL
#define vbc_resume  NULL
#endif

#if     defined(CONFIG_ARCH_SC8800S)             || \
        defined(CONFIG_MACH_SP6810A)
#if     defined(CONFIG_ARCH_SC8800S)
static ulong gpio_amplifier = MFP_CFG_X(LCD_RSTN, GPIO, DS0, PULL_NONE/* PULL_UP */, IO_OE);
static u32 speaker_gpio = 102; // mfp_to_gpio(MFP_CFG_TO_PIN(gpio_amplifier));
#elif   defined(CONFIG_MACH_SP6810A)
static ulong gpio_amplifier = MFP_CFG_X(RFCTL6, AF3, DS2, F_PULL_DOWN, S_PULL_DOWN, IO_OE);
static u32 speaker_gpio = 96;  // GPIO_PROD_SPEAKER_PA_EN_ID
#endif
static inline void local_amplifier_init(void)
{
    sprd_mfp_config(&gpio_amplifier, 1);
    if (gpio_request(speaker_gpio, "speaker amplifier")) {
        printk(KERN_ERR "speaker amplifier gpio request fail!\n");
    }
}

static inline void local_amplifier_enable(int enable)
{
    gpio_direction_output(speaker_gpio, !!enable);
}

static inline int local_amplifier_enabled(void)
{
    if (gpio_get_value(speaker_gpio)) {
        return 1;
    } else {
        return 0;
    }
}
#elif   defined(CONFIG_MACH_SP8805GA)           || \
        defined(CONFIG_MACH_OPENPHONE)
static inline void local_amplifier_init(void)
{

}

static inline void local_amplifier_enable(int enable)
{
    if (enable) {
     // ADI_Analogdie_reg_write(ANA_PA_CTL, 0x1aa9); //classAb
        ADI_Analogdie_reg_write(ANA_PA_CTL, 0x5A5A); //classD
    } else {
        ADI_Analogdie_reg_write(ANA_PA_CTL, 0x1555);
    }
}

static inline int local_amplifier_enabled(void)
{
    u32 value = ADI_Analogdie_reg_read(ANA_PA_CTL);
    switch (value) {
        case 0x5A5A: return 1;
        default : return 0;
    }
}
#else
#error "not define this CONFIG_MACH_xxxxx"
#endif
inline void vbc_amplifier_enable(int enable, const char *prename)
{
    printk("audio %s ==> trun %s PA\n", prename, enable ? "on":"off");
    printk("[headset_muted =%d]\n"
           "[earpiece_muted=%d]\n"
           "[speaker_muted =%d]\n", headset_muted, earpiece_muted, speaker_muted);
    local_amplifier_enable(enable);
}
EXPORT_SYMBOL_GPL(vbc_amplifier_enable);
inline int vbc_amplifier_enabled(void)
{
    return local_amplifier_enabled();
}
EXPORT_SYMBOL_GPL(vbc_amplifier_enabled);

#define VBC_PCM_RATES (SNDRV_PCM_RATE_8000  |	\
			  SNDRV_PCM_RATE_11025 |	\
			  SNDRV_PCM_RATE_16000 |	\
			  SNDRV_PCM_RATE_22050 |	\
              SNDRV_PCM_RATE_32000 |    \
			  SNDRV_PCM_RATE_44100 |	\
			  SNDRV_PCM_RATE_48000 |    \
              SNDRV_PCM_RATE_96000)

// PCM Playing and Recording default in full duplex mode
struct snd_soc_dai vbc_dai[] = {
{
    .name = "VBC",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 2, // now we only want to support stereo mode [luther.ge]
		.rates = VBC_PCM_RATES,
		.formats = VBC_PCM_FORMATS,},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 1, // now we only support mono capture
		.rates = VBC_PCM_RATES,
		.formats = VBC_PCM_FORMATS,},
	.ops = &vbc_dai_ops,
	.symmetric_rates = 1,
},
};
EXPORT_SYMBOL_GPL(vbc_dai);

static int vbc_probe(struct platform_device *pdev)
{
    struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec;
    int ret = 0;

	socdev->card->codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (socdev->card->codec == NULL)
		return -ENOMEM;
	codec = socdev->card->codec;
	mutex_init(&codec->mutex);

	codec->name = "VBC";
	codec->owner = THIS_MODULE;
	codec->dai = vbc_dai;
	codec->num_dai = ARRAY_SIZE(vbc_dai);
	codec->write = vbc_write;
	codec->read = vbc_read;
#if POWER_OFF_ON_STANDBY
	codec->set_bias_level = vbc_set_bias_level;
#endif
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	/* register pcms */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0)
		goto pcm_err;

	vbc_reset(codec);
    vbc_amplifier_enable(false, "vbc_init"); // Mute Speaker
    vbc_reg_VBCR1_set(BTL_MUTE, 1); // Mute earpiece
    vbc_reg_VBCR1_set(HP_DIS, 1); // Mute headphone
    vbc_power_down(-1);
#if 0
    /* vbc_reset() must be initialized twice, or the noise when playing audio */
    vbc_reset(codec);
#endif

#if POWER_OFF_ON_STANDBY
	vbc_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
#endif
	snd_soc_add_controls(codec, vbc_snd_controls,
				ARRAY_SIZE(vbc_snd_controls));
	vbc_add_widgets(codec);
	ret = snd_soc_init_card(socdev);
	if (ret < 0)
		goto card_err;
    android_pm_init();
    android_sprd_pm_init();
	return 0;

card_err:
	snd_soc_free_pcms(socdev);

pcm_err:
	kfree(socdev->card->codec);
	socdev->card->codec = NULL;
	return ret;
}

static int vbc_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;

	if (codec == NULL)
		return 0;
#if POWER_OFF_ON_STANDBY
    vbc_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	vbc_set_bias_level(codec, SND_SOC_BIAS_OFF);
#endif
	snd_soc_dapm_free(socdev);
	snd_soc_free_pcms(socdev);
	kfree(codec);
    android_pm_exit();
    android_sprd_pm_exit();
	return 0;
}

struct snd_soc_codec_device vbc_codec= {
    .probe   =  vbc_probe,
    .remove  =  vbc_remove,
    .suspend =  vbc_suspend,
    .resume  =  vbc_resume,
};
EXPORT_SYMBOL_GPL(vbc_codec);

static int vbc_init(void)
{
    local_amplifier_init();
    return snd_soc_register_dais(vbc_dai, ARRAY_SIZE(vbc_dai));
}

static void vbc_exit(void)
{
    snd_soc_unregister_dais(vbc_dai, ARRAY_SIZE(vbc_dai));
}

module_init(vbc_init);
module_exit(vbc_exit);

MODULE_DESCRIPTION("ALSA SoC SpreadTrum VBC codec");
MODULE_AUTHOR("Luther Ge <luther.ge@spreadtrum.com>");
MODULE_LICENSE("GPL");
