obj-y += io.o irq.o timer.o devices.o arch_asm.o arch_init.o globalregs.o \
         clock_common.o clock_sc8810.o

obj-$(CONFIG_SPRD_SOC_SC8810) += regulator.o regulator-devices.o

obj-y += dma.o reserve.o pinmap.o adi.o gpio_eic.o adc.o efuse.o \
         watchdog.o

obj-y += sys_debug.o
obj-y += sec-common.o

ifeq ($(CONFIG_NKERNEL),y)
obj-$(CONFIG_PM) += pm_vlx.o
else
obj-$(CONFIG_PM) += pm_orig.o
endif
obj-$(CONFIG_PM) += pm.o standby.o
ifeq ($(CONFIG_PM),y)
obj-$(CONFIG_SPRD_SOC_SC8810) += pm_sc8810.o pm_debug.o
obj-$(CONFIG_SPRD_SOC_SC7710) += pm_sc7710.o pm_debug_sc7710.o pm_ana_sc7710.o
endif

obj-$(CONFIG_SPRD_DCDC_CAL) += dcdc_cal.o
obj-$(CONFIG_SPRD_DCDC_DEBUG) += dcdc_debug.o

obj-$(CONFIG_DEBUG_FS) += clock_debug.o

sprdboard-$(CONFIG_MACH_SP7710GA_ORANGE) += board_sp7710ga_orange/
sprdboard-$(CONFIG_MACH_SP8810EA) += board-sp8810ea/
sprdboard-$(CONFIG_MACH_SP8810EC) += board-sp8810ec/
sprdboard-$(CONFIG_MACH_SP8810GA) += board-sp8810ga/
sprdboard-$(CONFIG_MACH_SP8810GB) += board-sp8810gb/
sprdboard-$(CONFIG_MACH_SP6820GA) += board-sp6820ga/
sprdboard-$(CONFIG_MACH_AMAZING)  += board-amazing/
sprdboard-$(CONFIG_MACH_KYLETD)  += board-kyletd/
sprdboard-$(CONFIG_MACH_KYLEW)  += board-kylew/
sprdboard-$(CONFIG_MACH_MINT)  += board-mint/
sprdboard-$(CONFIG_MACH_SP8810EB) += board-sp8810eb/
sprdboard-$(CONFIG_MACH_Z788)  += board-z788/
sprdboard-$(CONFIG_MACH_SP7710G_OPENPHONE)  += board-sp7710g_openphone/
sprdboard-$(CONFIG_MACH_SP7702) += board-sp7702/
sprdboard-$(CONFIG_MACH_OPENPHONE_SC6820) += board-sc6820-openphone/
sprdboard-$(CONFIG_MACH_SP7710GA)  += board-sp7710ga/
sprdboard-$(CONFIG_MACH_NEVISTD)  += board-nevistd/

obj-y += $(sprdboard-y)

obj-$(CONFIG_SC8810_VIBRATOR) += vibrator.o
obj-$(CONFIG_ANDROID_RAM_CONSOLE) += ram_console.o
obj-$(CONFIG_CPU_FREQ_SC8810) += cpufreq-sc8810.o
obj-$(CONFIG_SEC_DEBUG) += sec_debug.o sec_gaf.o sec_getlog.o sec-common.o sec-reboot.o
obj-$(CONFIG_BCMDHD)  += dhd_adapter.o
obj-$(CONFIG_BCMDHD89)  += dhd_adapter.o
obj-$(CONFIG_SPRD_MEM_POOL)    += sprd_mem_pool.o
ifeq ($(CONFIG_RTL8723AS),m)
obj-y  += rtl8723_adapter.o
endif


sprdboarddirs := $(patsubst %,arch/arm/mach-sc8810/%,$(sprdboard-y))
$(warning $(sprdboarddirs))
KBUILD_CPPFLAGS += $(patsubst %,-I%,$(sprdboarddirs))
$(warning $(KBUILD_CPPFLAGS))

