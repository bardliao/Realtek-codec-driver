/*
 * rt5679.c  --  ALC5679 ALSA SoC audio codec driver
 *
 * Copyright 2015 Realtek Semiconductor Corp.
 * Author: Oder Chiou <oder_chiou@realtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/platform_device.h>
#include <linux/firmware.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#ifdef CONFIG_SWITCH
#include <linux/switch.h>
#endif
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <sound/jack.h>

#include "rt5679.h"
#include "rt5679-spi.h"

//#define DSP_MODE_USE_SPI

#define VERSION "0.0.2"

#ifdef CONFIG_SWITCH
static struct switch_dev rt5679_headset_switch = {
	.name = "h2w",
};
#endif

static struct reg_default rt5679_init_list[] = {
	{RT5679_ASRC8				, 0x0123},
	{RT5679_CLK_TREE_CTRL3			, 0x0000},
	{RT5679_PR_REG_VREF_CTRL1		, 0x04df},
	{RT5679_PR_REG_ADC12_CLK_CTRL		, 0xa090},
	{RT5679_JACK_MIC_DET_CTRL1		, 0x70c0},
	{RT5679_MICBIAS1_CTRL1			, 0x0391},
	/* HP fine tune start */
	{RT5679_PR_REG_BIAS_CTRL9		, 0x2474},
	{RT5679_PR_REG_BIAS_CTRL10		, 0x4477},
	{RT5679_PR_REG_BIAS_CTRL11		, 0x2474},
	{RT5679_PR_REG_BIAS_CTRL12		, 0x4477},
	{RT5679_HP_CTRL2			, 0xc0c0},
	{RT5679_PR_REG_BIAS_CTRL3		, 0x0622},
	{RT5679_VREF5_L_PR_CTRL			, 0x0014},
	{RT5679_VREF5_R_PR_CTRL			, 0x0014},
	{RT5679_HP_DC_CAL_CTRL5			, 0x9e0c},
	/* HP fine tune end */
	/* MONO fine tune start */
	{RT5679_PR_REG_MONO_AMP_BIAS_CTRL	, 0x0a69},
	{RT5679_MONO_AMP_DC_CAL_CTRL3		, 0x2000},
	/* MONO fine tune end */
	{RT5679_MF_PIN_CTRL1			, 0x2009},
	{RT5679_PR_REG_PLL1_CTRL2		, 0x0044},
	{RT5679_PR_REG_PLL2_CTRL2		, 0x0044},
};

static int rt5679_reg_init(struct snd_soc_codec *codec)
{
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);
	int i;

	for (i = 0; i < ARRAY_SIZE(rt5679_init_list); i++)
		regmap_write(rt5679->regmap, rt5679_init_list[i].reg,
			rt5679_init_list[i].def);

	return 0;
}

static const struct reg_default rt5679_reg[] = {
	{0x0000, 0x0000},
	{0x0001, 0x8080},
	{0x0003, 0x0000},
	{0x0005, 0x0000},
	{0x0007, 0x0000},
	{0x0008, 0x0000},
	{0x0009, 0x0005},
	{0x000a, 0x5455},
	{0x0010, 0x0091},
	{0x0011, 0x0000},
	{0x0014, 0x5757},
	{0x0015, 0xafaf},
	{0x0016, 0xafaf},
	{0x0017, 0xafaf},
	{0x001a, 0x2f2f},
	{0x001b, 0x2f2f},
	{0x001c, 0x2f2f},
	{0x001d, 0x2f2f},
	{0x0020, 0x0000},
	{0x0021, 0x0000},
	{0x0022, 0x0000},
	{0x0028, 0x6000},
	{0x002a, 0x0000},
	{0x002b, 0x0000},
	{0x002c, 0x0000},
	{0x0030, 0x00f0},
	{0x0031, 0x0000},
	{0x0032, 0x0000},
	{0x0033, 0x0000},
	{0x0034, 0x0123},
	{0x0035, 0x4567},
	{0x0036, 0x8003},
	{0x0038, 0x00f0},
	{0x0039, 0x0000},
	{0x003a, 0x0000},
	{0x003b, 0x0000},
	{0x003c, 0x0123},
	{0x003d, 0x4567},
	{0x003e, 0x8003},
	{0x0040, 0xcaaa},
	{0x0041, 0xaa00},
	{0x0042, 0xcaaa},
	{0x0043, 0xaa00},
	{0x0044, 0xaaaa},
	{0x0045, 0xaa00},
	{0x0046, 0xb080},
	{0x0047, 0x0000},
	{0x0048, 0x0000},
	{0x0049, 0x0000},
	{0x004a, 0xc0c0},
	{0x004b, 0xc0c0},
	{0x004c, 0x0000},
	{0x004d, 0xc0c0},
	{0x004e, 0xc0c0},
	{0x0050, 0x0550},
	{0x0051, 0x0055},
	{0x0053, 0x1e00},
	{0x0055, 0x0009},
	{0x0060, 0x0000},
	{0x0061, 0x0000},
	{0x0062, 0x0000},
	{0x0063, 0x0040},
	{0x0064, 0x0000},
	{0x0065, 0x0181},
	{0x0066, 0x0000},
	{0x0067, 0x0002},
	{0x0068, 0x3703},
	{0x0069, 0x0100},
	{0x006a, 0x0000},
	{0x0070, 0x8000},
	{0x0071, 0x8000},
	{0x0072, 0x8000},
	{0x0073, 0x8000},
	{0x0074, 0x8000},
	{0x0075, 0x0000},
	{0x0076, 0x7777},
	{0x0077, 0x7777},
	{0x0078, 0x7000},
	{0x0079, 0x3000},
	{0x007a, 0x0000},
	{0x007b, 0x0000},
	{0x007c, 0x0000},
	{0x007d, 0x0000},
	{0x007e, 0x0111},
	{0x007f, 0x0333},
	{0x0080, 0x0000},
	{0x0081, 0x0000},
	{0x0083, 0x0000},
	{0x0084, 0x0000},
	{0x0085, 0x0000},
	{0x0086, 0x0000},
	{0x0087, 0x0000},
	{0x0088, 0x0000},
	{0x0089, 0x0000},
	{0x008a, 0x0000},
	{0x008b, 0x0000},
	{0x008c, 0x0000},
	{0x008d, 0x0000},
	{0x008e, 0x0008},
	{0x008f, 0x0000},
	{0x0090, 0x0000},
	{0x0091, 0x0000},
	{0x0092, 0x0000},
	{0x0093, 0x0000},
	{0x0094, 0x0000},
	{0x0095, 0x0000},
	{0x0096, 0x0000},
	{0x0097, 0x0000},
	{0x0098, 0x0000},
	{0x0099, 0x0000},
	{0x009a, 0x0000},
	{0x009b, 0x000c},
	{0x009c, 0x0002},
	{0x009d, 0x0001},
	{0x00a0, 0x7080},
	{0x00a1, 0x4a00},
	{0x00a2, 0x4e01},
	{0x00a3, 0xa000},
	{0x00b0, 0x0000},
	{0x00b1, 0x0000},
	{0x00b2, 0x0000},
	{0x00b3, 0x0000},
	{0x00b4, 0x0000},
	{0x00b5, 0x0000},
	{0x00b6, 0x0000},
	{0x00b7, 0x0000},
	{0x00b8, 0x0000},
	{0x00b9, 0x0000},
	{0x00ba, 0x0000},
	{0x00bb, 0x0000},
	{0x00bc, 0x0000},
	{0x00bd, 0x0000},
	{0x00be, 0x0000},
	{0x00bf, 0x0000},
	{0x00c0, 0x2008},
	{0x00c1, 0x8600},
	{0x00c2, 0x0000},
	{0x00c3, 0x0000},
	{0x00c4, 0x0000},
	{0x00c5, 0x0000},
	{0x00c6, 0x0000},
	{0x00c7, 0x0000},
	{0x00c8, 0x0000},
	{0x00c9, 0x0000},
	{0x00ca, 0x0000},
	{0x00cf, 0x0300},
	{0x00d0, 0xb320},
	{0x00d1, 0x0000},
	{0x00d2, 0xb300},
	{0x00d3, 0x0000},
	{0x00d4, 0xb300},
	{0x00d5, 0x0000},
	{0x00d6, 0xb300},
	{0x00d7, 0x0000},
	{0x00da, 0x0000},
	{0x00db, 0x0008},
	{0x00dc, 0x00c0},
	{0x00dd, 0x6724},
	{0x00de, 0x3131},
	{0x00df, 0x0008},
	{0x00e0, 0x4000},
	{0x00e1, 0x3131},
	{0x00e4, 0x402c},
	{0x00f7, 0xf8f8},
	{0x00f8, 0xf8f8},
	{0x00f9, 0xf8f8},
	{0x00fa, 0x0000},
	{0x00fd, 0x0000},
	{0x00fe, 0x10ec},
	{0x00ff, 0x6385},
	{0x0100, 0xc0c0},
	{0x0101, 0x0000},
	{0x0102, 0x0000},
	{0x0103, 0x0000},
	{0x0104, 0x0000},
	{0x0105, 0x0000},
	{0x0106, 0x0000},
	{0x0107, 0x0000},
	{0x0108, 0x0000},
	{0x0109, 0x0000},
	{0x010a, 0x0000},
	{0x0112, 0xe400},
	{0x011a, 0x000b},
	{0x011d, 0x0000},
	{0x0120, 0x1d22},
	{0x0121, 0x0003},
	{0x0122, 0x0003},
	{0x0123, 0x0020},
	{0x0124, 0x0080},
	{0x0125, 0x0800},
	{0x0126, 0x0000},
	{0x0127, 0x0000},
	{0x0128, 0x0000},
	{0x0129, 0x0000},
	{0x012a, 0x1d1f},
	{0x012b, 0x0000},
	{0x012c, 0x0020},
	{0x012d, 0x0080},
	{0x012e, 0x0800},
	{0x012f, 0x0000},
	{0x0130, 0x0000},
	{0x013a, 0x0000},
	{0x013b, 0x0000},
	{0x013c, 0x0000},
	{0x0150, 0x4131},
	{0x0151, 0x4131},
	{0x0152, 0x4131},
	{0x0153, 0x4131},
	{0x0154, 0x4131},
	{0x0155, 0x0000},
	{0x0156, 0x0000},
	{0x0157, 0x0000},
	{0x0158, 0x0000},
	{0x0159, 0x0000},
	{0x015a, 0x0000},
	{0x015b, 0x0000},
	{0x015c, 0x0000},
	{0x0160, 0x6000},
	{0x0161, 0x0000},
	{0x0164, 0xc000},
	{0x0165, 0x0000},
	{0x0166, 0x0000},
	{0x0167, 0x0000},
	{0x0170, 0x0000},
	{0x0171, 0x0000},
	{0x0172, 0x0000},
	{0x0173, 0x0002},
	{0x0174, 0x0001},
	{0x0175, 0x0002},
	{0x0176, 0x0001},
	{0x0177, 0x0002},
	{0x0178, 0x0001},
	{0x0179, 0x0002},
	{0x017a, 0x0001},
	{0x017b, 0x0002},
	{0x017c, 0x0001},
	{0x0180, 0x4b38},
	{0x0181, 0x0000},
	{0x0182, 0x0000},
	{0x0183, 0x0030},
	{0x0192, 0x882f},
	{0x0193, 0x0000},
	{0x0194, 0x0700},
	{0x0195, 0x0242},
	{0x0196, 0x0e22},
	{0x0197, 0x0001},
	{0x019a, 0x0000},
	{0x019b, 0x0000},
	{0x01a0, 0x433d},
	{0x01a1, 0x02a0},
	{0x01a2, 0x0000},
	{0x01a3, 0x0000},
	{0x01a4, 0x0000},
	{0x01a5, 0x0009},
	{0x01a6, 0x0018},
	{0x01a7, 0x002a},
	{0x01a8, 0x004c},
	{0x01a9, 0x0097},
	{0x01aa, 0x01c3},
	{0x01ab, 0x03e9},
	{0x01ac, 0x1389},
	{0x01ad, 0xc351},
	{0x01ae, 0x0000},
	{0x01af, 0x0000},
	{0x01b0, 0x0000},
	{0x01b1, 0x0000},
	{0x01b3, 0x40af},
	{0x01b4, 0x0702},
	{0x01b5, 0x0000},
	{0x01b6, 0x0000},
	{0x01b7, 0x5757},
	{0x01b8, 0x5757},
	{0x01b9, 0x5757},
	{0x01ba, 0x5757},
	{0x01bb, 0x5757},
	{0x01bc, 0x5757},
	{0x01bd, 0x5757},
	{0x01be, 0x5757},
	{0x01bf, 0x5757},
	{0x01c0, 0x5757},
	{0x01c1, 0x003c},
	{0x01c2, 0x5757},
	{0x01c3, 0x0000},
	{0x01d0, 0x5334},
	{0x01d1, 0x18e0},
	{0x01d2, 0x8728},
	{0x01d3, 0x7418},
	{0x01d4, 0x901f},
	{0x01d5, 0x4500},
	{0x01d6, 0x5100},
	{0x01d7, 0x0000},
	{0x01d8, 0x0000},
	{0x01d9, 0x0000},
	{0x01da, 0x0501},
	{0x01e0, 0x0111},
	{0x01e1, 0x0064},
	{0x01e2, 0xef0e},
	{0x01e3, 0xf0f0},
	{0x01e4, 0xef0e},
	{0x01e5, 0xf0f0},
	{0x01e6, 0xef0e},
	{0x01e7, 0xf0f0},
	{0x01e8, 0xf000},
	{0x01e9, 0x0000},
	{0x01f0, 0x0000},
	{0x01f1, 0x0000},
	{0x01f2, 0x0000},
	{0x0200, 0x1c10},
	{0x0201, 0x01f4},
	{0x0202, 0x1c10},
	{0x0203, 0x01f4},
	{0x0204, 0x1c10},
	{0x0205, 0x01f4},
	{0x0206, 0x1c10},
	{0x0207, 0x01f4},
	{0x0208, 0xc882},
	{0x0209, 0x1c10},
	{0x020a, 0x01f4},
	{0x020b, 0xc882},
	{0x020c, 0x1c10},
	{0x020d, 0x01f4},
	{0x020e, 0xc882},
	{0x020f, 0x1c10},
	{0x0210, 0x01f4},
	{0x0211, 0xc882},
	{0x0212, 0x1c10},
	{0x0213, 0x01f4},
	{0x0214, 0xe904},
	{0x0215, 0x1c10},
	{0x0216, 0x01f4},
	{0x0217, 0xe904},
	{0x0218, 0x1c10},
	{0x0219, 0x01f4},
	{0x021a, 0xe904},
	{0x021b, 0x1c10},
	{0x021c, 0x01f4},
	{0x021d, 0xe904},
	{0x021e, 0x1c10},
	{0x021f, 0x01f4},
	{0x0220, 0xe904},
	{0x0221, 0x1c10},
	{0x0222, 0x01f4},
	{0x0223, 0xe904},
	{0x0224, 0x1c10},
	{0x0225, 0x01f4},
	{0x0226, 0x1c10},
	{0x0227, 0x01f4},
	{0x0228, 0x1c10},
	{0x0229, 0x01f4},
	{0x022a, 0x2000},
	{0x022b, 0x0000},
	{0x022c, 0x2000},
	{0x022d, 0x2000},
	{0x022e, 0x0000},
	{0x022f, 0x2000},
	{0x0230, 0x1c10},
	{0x0231, 0x01f4},
	{0x0232, 0x1c10},
	{0x0233, 0x01f4},
	{0x0234, 0x0200},
	{0x0235, 0x0000},
	{0x0236, 0x0000},
	{0x0237, 0x0000},
	{0x0238, 0x0000},
	{0x0239, 0x0000},
	{0x023a, 0x0000},
	{0x023b, 0x0000},
	{0x023c, 0x0000},
	{0x023d, 0x0000},
	{0x023e, 0x0200},
	{0x023f, 0x0000},
	{0x0240, 0x0000},
	{0x0241, 0x0000},
	{0x0242, 0x0000},
	{0x0243, 0x0000},
	{0x0244, 0x0000},
	{0x0245, 0x0000},
	{0x0246, 0x0000},
	{0x0247, 0x0000},
	{0x0248, 0x0800},
	{0x0249, 0x0800},
	{0x024a, 0x0800},
	{0x024b, 0x0800},
	{0x024c, 0x1c10},
	{0x024d, 0x01f4},
	{0x024e, 0x1c10},
	{0x024f, 0x01f4},
	{0x0250, 0xe904},
	{0x0251, 0x1c10},
	{0x0252, 0x01f4},
	{0x0253, 0xe904},
	{0x0254, 0x1c10},
	{0x0255, 0x01f4},
	{0x0256, 0xe904},
	{0x0257, 0x1c10},
	{0x0258, 0x01f4},
	{0x0259, 0xe904},
	{0x025a, 0x1c10},
	{0x025b, 0x01f4},
	{0x025c, 0xe904},
	{0x025d, 0x1c10},
	{0x025e, 0x01f4},
	{0x025f, 0xe904},
	{0x0260, 0x1c10},
	{0x0261, 0x01f4},
	{0x0262, 0xe904},
	{0x0263, 0x1c10},
	{0x0264, 0x01f4},
	{0x0265, 0xe904},
	{0x0266, 0x1c10},
	{0x0267, 0x01f4},
	{0x0268, 0x1c10},
	{0x0269, 0x01f4},
	{0x026a, 0x1c10},
	{0x026b, 0x01f4},
	{0x026c, 0x0800},
	{0x026d, 0x0800},
	{0x026e, 0x0800},
	{0x026f, 0x0800},
	{0x0280, 0x7681},
	{0x0281, 0x0020},
	{0x0282, 0x7851},
	{0x0283, 0x01f3},
	{0x0284, 0x00fa},
	{0x0285, 0x0129},
	{0x0286, 0x0602},
	{0x0287, 0x0114},
	{0x0288, 0x0010},
	{0x0289, 0x0000},
	{0x028a, 0x120d},
	{0x028b, 0x0040},
	{0x028c, 0x0040},
	{0x028d, 0x0505},
	{0x028e, 0x1322},
	{0x028f, 0x2110},
	{0x0290, 0x3040},
	{0x0291, 0x6414},
	{0x0292, 0x2000},
	{0x0293, 0x0000},
	{0x0294, 0x0000},
	{0x0295, 0x0000},
	{0x0296, 0x0000},
	{0x02a0, 0x4089},
	{0x02a1, 0x0000},
	{0x02a2, 0x0008},
	{0x02a3, 0x0010},
	{0x02a4, 0x0596},
	{0x02a5, 0x0506},
	{0x02a6, 0x0806},
	{0x02a7, 0x1a00},
	{0x02a8, 0x0705},
	{0x02a9, 0x052d},
	{0x02aa, 0x180d},
	{0x02ab, 0x0009},
	{0x02ac, 0x140a},
	{0x02ad, 0x0e4b},
	{0x02ae, 0x0000},
	{0x02b0, 0x0000},
	{0x02b1, 0x0000},
	{0x02b2, 0x8060},
	{0x02b3, 0x0040},
	{0x02b4, 0x0001},
	{0x02b5, 0x0001},
	{0x02b6, 0x0400},
	{0x02b7, 0x0000},
	{0x02b8, 0x0000},
	{0x02b9, 0x4022},
	{0x02ba, 0x0002},
	{0x02bb, 0x0000},
	{0x02bc, 0x0000},
	{0x02bd, 0x0000},
	{0x02c5, 0x802c},
	{0x02d0, 0x7fff},
	{0x02d1, 0x0000},
	{0x02d2, 0x0000},
	{0x02d3, 0x006a},
	{0x02d4, 0x0000},
	{0x02d5, 0x0000},
	{0x02d6, 0x0000},
	{0x02e0, 0x6000},
	{0x02e1, 0x4040},
	{0x02e2, 0x4000},
	{0x02e3, 0x0000},
	{0x02e4, 0xc350},
	{0x02e5, 0x0064},
	{0x02e6, 0x0040},
	{0x02e7, 0x0000},
	{0x02e8, 0x5280},
	{0x02e9, 0x0001},
	{0x02ea, 0x86a0},
	{0x02eb, 0x0fd3},
	{0x0300, 0x3c10},
	{0x0301, 0x7dc2},
	{0x0302, 0xa178},
	{0x0303, 0x5383},
	{0x0304, 0x003e},
	{0x0305, 0x02c1},
	{0x0306, 0xd37d},
	{0x0307, 0x68d3},
	{0x0308, 0x82f6},
	{0x0309, 0xcd3b},
	{0x030a, 0x0035},
	{0x030b, 0xebf4},
	{0x030c, 0x2d6e},
	{0x0310, 0x5254},
	{0x0311, 0x0300},
	{0x0312, 0x5f5f},
	{0x0313, 0x133e},
	{0x0314, 0x32ff},
	{0x0315, 0x040c},
	{0x0316, 0x7418},
	{0x0317, 0x1800},
	{0x0318, 0x0000},
	{0x0319, 0x0045},
	{0x031a, 0x0000},
	{0x031b, 0x0000},
	{0x0320, 0x5254},
	{0x0321, 0x0300},
	{0x0322, 0x5f5f},
	{0x0323, 0x133e},
	{0x0324, 0x32ff},
	{0x0325, 0x040c},
	{0x0326, 0x7418},
	{0x0327, 0x1800},
	{0x0328, 0x0000},
	{0x0329, 0x0045},
	{0x032a, 0x0000},
	{0x032b, 0x0000},
	{0x0330, 0x5254},
	{0x0331, 0x0300},
	{0x0332, 0x5f5f},
	{0x0333, 0x133e},
	{0x0334, 0x32ff},
	{0x0335, 0x040c},
	{0x0336, 0x7418},
	{0x0337, 0x1800},
	{0x0338, 0x0000},
	{0x0339, 0x0045},
	{0x033a, 0x0000},
	{0x033b, 0x0000},
	{0x0340, 0x4951},
	{0x0341, 0x1860},
	{0x0342, 0x5f5f},
	{0x0343, 0x0032},
	{0x0344, 0x0450},
	{0x0345, 0x00ff},
	{0x0346, 0x040c},
	{0x0347, 0x7418},
	{0x0348, 0x0000},
	{0x0349, 0x8596},
	{0x034a, 0x0075},
	{0x034b, 0x0080},
	{0x034c, 0x0000},
	{0x034d, 0x0000},
	{0x034e, 0x0000},
	{0x0350, 0x4905},
	{0x0351, 0xe150},
	{0x0352, 0x0100},
	{0x0353, 0x5f5f},
	{0x0354, 0x0603},
	{0x0355, 0x0022},
	{0x0356, 0x45ff},
	{0x0357, 0x040c},
	{0x0358, 0x7418},
	{0x0359, 0x0000},
	{0x035a, 0x0000},
	{0x035b, 0x8700},
	{0x035c, 0x5080},
	{0x035d, 0x0000},
	{0x035e, 0x0000},
	{0x035f, 0x0000},
	{0x0400, 0x7f08},
	{0x0401, 0x0030},
	{0x0402, 0x0000},
	{0x0403, 0x0261},
	{0x0404, 0x1c3c},
	{0x0405, 0x0000},
	{0x0406, 0x0000},
	{0x0407, 0x0000},
	{0x0408, 0x0000},
	{0x0409, 0x0000},
	{0x040a, 0x0000},
	{0x040b, 0x0000},
	{0x040c, 0x0000},
	{0x040d, 0x0000},
	{0x040e, 0x0000},
	{0x040f, 0x0000},
	{0x0410, 0x0000},
	{0x0411, 0x0000},
	{0x0412, 0x0000},
	{0x0413, 0x0000},
	{0x0414, 0x0000},
	{0x0415, 0x0000},
	{0x0416, 0x0000},
	{0x0417, 0x0000},
	{0x0418, 0x0000},
	{0x0420, 0x7914},
	{0x0421, 0x0261},
	{0x0422, 0x6000},
	{0x0423, 0x0000},
	{0x0424, 0x0000},
	{0x0425, 0x0000},
	{0x0426, 0x0000},
	{0x0427, 0x0000},
	{0x0428, 0x0000},
	{0x0429, 0x0000},
	{0x0500, 0x0000},
	{0x0501, 0x0000},
	{0x0502, 0x0000},
	{0x0503, 0x2f2f},
	{0x0504, 0x2f2f},
	{0x0505, 0x2f2f},
	{0x0506, 0x2f2f},
	{0x0507, 0x2f2f},
	{0x0508, 0x1800},
	{0x0509, 0x0000},
	{0x050a, 0x0000},
	{0x050b, 0x0800},
	{0x050c, 0x1800},
	{0x050d, 0x0000},
	{0x050e, 0x0000},
	{0x050f, 0x0800},
	{0x0510, 0x1800},
	{0x0511, 0x0000},
	{0x0512, 0x0000},
	{0x0513, 0x0800},
	{0x0514, 0x1800},
	{0x0515, 0x0000},
	{0x0516, 0x0000},
	{0x0517, 0x0800},
	{0x0518, 0x1800},
	{0x0519, 0x0000},
	{0x051a, 0x0000},
	{0x051b, 0x0800},
	{0x0520, 0x0041},
	{0x0530, 0x0000},
	{0x0540, 0x0000},
	{0x0541, 0x0000},
	{0x0542, 0x0000},
	{0x0543, 0x0000},
	{0x0544, 0x0000},
	{0x0545, 0x0000},
	{0x0546, 0x0000},
	{0x0547, 0x0000},
	{0x0548, 0x0000},
	{0x0549, 0x0000},
	{0x054a, 0x0000},
	{0x0550, 0x0000},
	{0x0551, 0x0000},
	{0x0552, 0x0000},
	{0x0553, 0x0000},
	{0x0554, 0x0000},
	{0x0555, 0x0000},
	{0x0556, 0x0000},
	{0x0557, 0x0000},
	{0x0558, 0x0000},
	{0x0559, 0x0000},
	{0x055a, 0x0000},
	{0x055b, 0x0000},
	{0x055c, 0x0000},
	{0x055d, 0x0000},
	{0x055e, 0x0000},
	{0x055f, 0x0000},
	{0x0560, 0x0000},
	{0x0561, 0x0000},
	{0x0562, 0x0000},
	{0x0563, 0x0000},
	{0x0564, 0x0000},
	{0x0565, 0x0000},
	{0x0566, 0x0000},
	{0x0567, 0x0000},
	{0x0568, 0x0000},
	{0x0569, 0x0000},
	{0x056a, 0x0000},
	{0x056b, 0x0000},
	{0x056c, 0x0000},
	{0x0600, 0x8a69},
	{0x0601, 0xaa66},
	{0x0602, 0x00aa},
	{0x0603, 0x0666},
	{0x0604, 0xaaa6},
	{0x0605, 0xaaaa},
	{0x0606, 0xaaaa},
	{0x0607, 0xa666},
	{0x0608, 0x4000},
	{0x0609, 0x4444},
	{0x060a, 0x4444},
	{0x060b, 0x4444},
	{0x060c, 0x4444},
	{0x060d, 0xa860},
	{0x0610, 0xa490},
	{0x0611, 0xa490},
	{0x0612, 0x0410},
	{0x0613, 0x0000},
	{0x0614, 0xa490},
	{0x0619, 0x3303},
	{0x061a, 0x0054},
	{0x061b, 0x3303},
	{0x061c, 0x0054},
	{0x061d, 0x045f},
	{0x061e, 0x081f},
	{0x0620, 0x0095},
	{0x0621, 0x0095},
	{0x0622, 0x0095},
	{0x0622, 0x0095},
	{0x0636, 0xaaa0},
	{0x0637, 0xaaaa},
	{0x063c, 0x0000},
	{0x063d, 0x0000},
	{0x063e, 0x0000},
	{0x063f, 0x0003},
	{0x0640, 0x0000},
	{0x0641, 0x0000},
	{0x0642, 0x0000},
	{0x0643, 0x0000},
	{0x0644, 0x0000},
	{0x0652, 0x3002},
	{0x0653, 0x0000},
	{0x0654, 0x0000},
	{0x0655, 0x0000},
	{0x0656, 0x0000},
	{0x0657, 0x0000},
	{0x0660, 0x0010},
	{0x0661, 0x0010},
	{0x0662, 0x0010},
	{0x0663, 0x0010},
	{0x0664, 0x0010},
	{0x0665, 0x0000},
	{0x0670, 0x0000},
	{0x0671, 0x30c2},
	{0x0672, 0x0803},
	{0x0673, 0xaaaa},
	{0x0674, 0x1100},
	{0x0675, 0x0a54},
	{0x0680, 0x0000},
	{0x0681, 0x0002},
	{0x0682, 0x0002},
	{0x0683, 0x0000},
	{0x0684, 0x0114},
	{0x0689, 0x0010},
	{0x068a, 0x0210},
	{0x068c, 0x0067},
	{0x068d, 0x0711},
	{0x068f, 0x1024},
	{0x0690, 0x1024},
	{0x0700, 0x0000},
	{0x0701, 0x0000},
	{0x0702, 0x0000},
	{0x0710, 0x0055},
	{0x07f0, 0x0020},
	{0x07f1, 0x0000},
	{0x07f2, 0x0000},
	{0x07f3, 0x0000},
};

/**
 * rt5679_dsp_mode_i2c_write_addr - Write value to address on DSP mode.
 * @rt5679: Private Data.
 * @addr: Address index.
 * @value: Address data.
 *
 *
 * Returns 0 for success or negative error code.
 */
static int rt5679_dsp_mode_i2c_write_addr(struct rt5679_priv *rt5679,
		unsigned int addr, unsigned int value, unsigned int opcode)
{
	struct snd_soc_codec *codec = rt5679->codec;
	int ret;

	mutex_lock(&rt5679->dsp_lock);

	ret = regmap_write(rt5679->regmap_physical, RT5679_DSP_I2C_ADDR_MSB,
		addr >> 16);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to set addr msb value: %d\n", ret);
		goto err;
	}

	ret = regmap_write(rt5679->regmap_physical, RT5679_DSP_I2C_ADDR_LSB,
		addr & 0xffff);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to set addr lsb value: %d\n", ret);
		goto err;
	}

	ret = regmap_write(rt5679->regmap_physical, RT5679_DSP_I2C_DATA_MSB,
		value >> 16);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to set data msb value: %d\n", ret);
		goto err;
	}

	ret = regmap_write(rt5679->regmap_physical, RT5679_DSP_I2C_DATA_LSB,
		value & 0xffff);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to set data lsb value: %d\n", ret);
		goto err;
	}

	ret = regmap_write(rt5679->regmap_physical, RT5679_DSP_I2C_OP_CODE,
		opcode);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to set op code value: %d\n", ret);
		goto err;
	}

err:
	mutex_unlock(&rt5679->dsp_lock);

	return ret;
}

/**
 * rt5679_dsp_mode_i2c_read_addr - Read value from address on DSP mode.
 * rt5679: Private Data.
 * @addr: Address index.
 * @value: Address data.
 *
 *
 * Returns 0 for success or negative error code.
 */
static int rt5679_dsp_mode_i2c_read_addr(
	struct rt5679_priv *rt5679, unsigned int addr, unsigned int *value)
{
	struct snd_soc_codec *codec = rt5679->codec;
	int ret;
	unsigned int msb, lsb;

	mutex_lock(&rt5679->dsp_lock);

	ret = regmap_write(rt5679->regmap_physical, RT5679_DSP_I2C_ADDR_MSB,
		addr >> 16);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to set addr msb value: %d\n", ret);
		goto err;
	}

	ret = regmap_write(rt5679->regmap_physical, RT5679_DSP_I2C_ADDR_LSB,
		addr & 0xffff);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to set addr lsb value: %d\n", ret);
		goto err;
	}

	ret = regmap_write(rt5679->regmap_physical, RT5679_DSP_I2C_OP_CODE,
		0x0002);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to set op code value: %d\n", ret);
		goto err;
	}

	regmap_read(rt5679->regmap_physical, RT5679_DSP_I2C_DATA_MSB, &msb);
	regmap_read(rt5679->regmap_physical, RT5679_DSP_I2C_DATA_LSB, &lsb);
	*value = (msb << 16) | lsb;

err:
	mutex_unlock(&rt5679->dsp_lock);

	return ret;
}

/**
 * rt5679_dsp_mode_i2c_write - Write register on DSP mode.
 * rt5679: Private Data.
 * @reg: Register index.
 * @value: Register data.
 *
 *
 * Returns 0 for success or negative error code.
 */
static int rt5679_dsp_mode_i2c_write(struct rt5679_priv *rt5679,
		unsigned int reg, unsigned int value)
{
	return rt5679_dsp_mode_i2c_write_addr(rt5679, 0x1800c000 + reg * 2,
		value << 16 | value, 0x1);
}

/**
 * rt5679_dsp_mode_i2c_read - Read register on DSP mode.
 * @codec: SoC audio codec device.
 * @reg: Register index.
 * @value: Register data.
 *
 *
 * Returns 0 for success or negative error code.
 */
static int rt5679_dsp_mode_i2c_read(
	struct rt5679_priv *rt5679, unsigned int reg, unsigned int *value)
{
	int ret = rt5679_dsp_mode_i2c_read_addr(rt5679, 0x1800c000 + reg * 2,
		value);

	*value &= 0xffff;

	return ret;
}

static void rt5679_set_dsp_mode(struct snd_soc_codec *codec, bool on)
{
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);

	if (on) {
		regmap_update_bits(rt5679->regmap, RT5679_PWR_ANA1,
			RT5679_PWR_CLK_25M, RT5679_PWR_CLK_25M);
		regmap_update_bits(rt5679->regmap, RT5679_LDO8_LDO9_PR_CTRL,
			0x0010, 0);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_LDO1,
			RT5679_PWR_LDO3_ON, RT5679_PWR_LDO3_ON);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_LDO2,
			RT5679_PWR_LDO9, RT5679_PWR_LDO9);
		regmap_write(rt5679->regmap, RT5679_DSP_CLK_SOURCE1, 0x0333);
		regmap_write(rt5679->regmap, RT5679_DSP_CLK_SOURCE2, 0x0333);
		regmap_update_bits(rt5679->regmap, RT5679_GLB_CLK2, 0x1, 0x1);
		regmap_write(rt5679->regmap, RT5679_PWR_LDO3, 0x7707);
		regmap_update_bits(rt5679->regmap, RT5679_HIFI_MINI_DSP_CTRL_ST,
			0x30, 0x30);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_DSP,
			RT5679_PWR_DCVDD9_ISO | RT5679_PWR_DCVDD3_ISO |
			RT5679_PWR_SRAM | RT5679_PWR_MINI_DSP |
			RT5679_PWR_EP_DSP, RT5679_PWR_DCVDD9_ISO |
			RT5679_PWR_DCVDD3_ISO | RT5679_PWR_SRAM |
			RT5679_PWR_MINI_DSP | RT5679_PWR_EP_DSP);
		rt5679->is_dsp_mode = true;
	} else {
		regmap_update_bits(rt5679->regmap, RT5679_PWR_DSP,
			RT5679_PWR_DCVDD9_ISO | RT5679_PWR_DCVDD3_ISO |
			RT5679_PWR_SRAM | RT5679_PWR_MINI_DSP |
			RT5679_PWR_EP_DSP, 0);
		rt5679->is_dsp_mode = false;
		regmap_update_bits(rt5679->regmap, RT5679_HIFI_MINI_DSP_CTRL_ST,
			0x30, 0);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_LDO2,
			RT5679_PWR_LDO9, 0);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_LDO1,
			RT5679_PWR_LDO3_ON, 0);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_ANA1,
			RT5679_PWR_CLK_25M, 0);
	}
}

static bool rt5679_volatile_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case RT5679_RESET:
	case RT5679_SPDIF_IN_CTRL:
	case RT5679_FRAC_DIV_CTRL2:
	case RT5679_JACK_MIC_DET_CTRL2:
	case RT5679_JACK_MIC_DET_CTRL4:
	case RT5679_IRQ_ST1:
	case RT5679_IRQ_ST2:
	case RT5679_GPIO_ST1:
 	case RT5679_GPIO_ST2:
  	case RT5679_IL_CMD1:
  	case RT5679_4BTN_IL_CMD1:
  	case RT5679_PS_IL_CMD1:
  	case RT5679_VENDOR_ID:
  	case RT5679_VENDOR_ID1:
  	case RT5679_VENDOR_ID2:
  	case RT5679_PDM1_CTRL1:
  	case RT5679_PDM1_CTRL2:
  	case RT5679_PDM1_CTRL5:
  	case RT5679_PDM2_CTRL1:
  	case RT5679_PDM2_CTRL2:
  	case RT5679_PDM2_CTRL5:
  	case RT5679_MCLK_DET_PROTECT_CTRL:
  	case RT5679_STO_HP_NG2_ST1 ... RT5679_STO_HP_NG2_ST3:
	case RT5679_MONO_AMP_NG2_ST1:
	case RT5679_MONO_AMP_NG2_ST2:
	case RT5679_IF_INPUT_DET_ST1 ... RT5679_IF_INPUT_DET_ST3:
	case RT5679_STO_DAC_SIL_DET_CTRL ... RT5679_DD_MIXERR_SIL_DET_CTRL:
	case RT5679_ADC_EQ_CTRL1:
	case RT5679_DAC_EQ_CTRL1:
	case RT5679_DAC_EQ_CTRL2:
	case RT5679_I2S_MASTER_CLK_CTRL5:
	case RT5679_I2S_MASTER_CLK_CTRL7:
	case RT5679_I2S_MASTER_CLK_CTRL9:
	case RT5679_I2S_MASTER_CLK_CTRL11:
	case RT5679_I2S_MASTER_CLK_CTRL13:
	case RT5679_VAD_CLK_SETTING1:
	case RT5679_VAD_ADC_PLL3_CTRL2:
	case RT5679_HP_IMP_SENS_CTRL1:
	case RT5679_HP_IMP_SENS_CTRL3 ... RT5679_HP_IMP_SENS_CTRL5:
	case RT5679_HP_IMP_SENS_DIG_CTRL2:
	case RT5679_HP_IMP_SENS_DIG_CTRL16:
	case RT5679_HP_IMP_SENS_DIG_CTRL17:
	case RT5679_ALC_PGA_ST1 ... RT5679_ALC_PGA_ST3:
	case RT5679_HAPTIC_GEN_CTRL1:
	case RT5679_HAPTIC_GEN_CTRL2:
	case RT5679_PITCH_HELLO_DET_CTRL1:
	case RT5679_PITCH_HELLO_DET_CTRL19:
	case RT5679_PITCH_HELLO_DET_CTRL20:
	case RT5679_PITCH_HELLO_DET_CTRL22:
	case RT5679_PITCH_HELLO_DET_CTRL23:
	case RT5679_OK_DET_CTRL1:
	case RT5679_OK_DET_CTRL2:
	case RT5679_OK_DET_CTRL15:
	case RT5679_DFLL_CAL_CTRL2:
	case RT5679_DFLL_CAL_CTRL10:
	case RT5679_DFLL_CAL_CTRL12 ... RT5679_DFLL_CAL_CTRL14:
	case RT5679_VAD_FUNCTION_CTRL1:
	case RT5679_DELAY_BUFFER_SRAM_CTRL2:
	case RT5679_DELAY_BUFFER_SRAM_CTRL3:
	case RT5679_DELAY_BUFFER_SRAM_CTRL5:
	case RT5679_DELAY_BUFFER_SRAM_CTRL6:
	case RT5679_DMIC_CLK_ON_OFF_CTRL12:
	case RT5679_DAC_MULTI_DRC_HB_CTRL9:
	case RT5679_DAC_MULTI_DRC_HB_CTRL11:
	case RT5679_DAC_MULTI_DRC_HB_CTRL12:
	case RT5679_DAC_MULTI_DRC_MB_CTRL9:
	case RT5679_DAC_MULTI_DRC_MB_CTRL11:
	case RT5679_DAC_MULTI_DRC_MB_CTRL12:
	case RT5679_DAC_MULTI_DRC_BB_CTRL9:
	case RT5679_DAC_MULTI_DRC_BB_CTRL11:
	case RT5679_DAC_MULTI_DRC_BB_CTRL12:
	case RT5679_DAC_MULTI_DRC_POS_ST1:
	case RT5679_DAC_MULTI_DRC_POS_ST2:
	case RT5679_ADC_ALC_ST1:
	case RT5679_ADC_ALC_ST2:
	case RT5679_HP_DC_CAL_CTRL1:
	case RT5679_HP_DC_CAL_CTRL10:
	case RT5679_HP_DC_CAL_ST1 ... RT5679_HP_DC_CAL_ST13:
	case RT5679_MONO_AMP_DC_CAL_CTRL1:
	case RT5679_MONO_AMP_DC_CAL_CTRL5:
	case RT5679_MONO_AMP_DC_CAL_ST1 ... RT5679_MONO_AMP_DC_CAL_ST3:
	case RT5679_HIFI_MINI_DSP_CTRL_ST:
	case RT5679_SPI_SLAVE_CRC_CHECK_CTRL:
	case RT5679_EFUSE_CTRL1:
	case RT5679_EFUSE_CTRL6 ... RT5679_EFUSE_CTRL9:
	case RT5679_EFUSE_CTRL11:
	case RT5679_SLIMBUS_PARAMETER:
	case RT5679_DUMMY_REG_1:
		return true;

	default:
		return false;
	}
}

static bool rt5679_readable_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case RT5679_RESET:
	case RT5679_LOUT:
	case RT5679_HP_OUT:
	case RT5679_MONO_OUT:
	case RT5679_BST12_CTRL:
	case RT5679_BST34_CTRL:
	case RT5679_VAD_INBUF_CTRL:
	case RT5679_CAL_ADC_MIXER_CTRL:
	case RT5679_MICBIAS1_CTRL1:
	case RT5679_MICBIAS1_CTRL2:
	case RT5679_DAC1_POST_DIG_VOL:
	case RT5679_DAC1_DIG_VOL:
	case RT5679_DAC2_DIG_VOL:
	case RT5679_DAC3_DIG_VOL:
	case RT5679_STO1_ADC_DIG_VOL:
	case RT5679_MONO_ADC_DIG_VOL:
	case RT5679_STO2_ADC_DIG_VOL:
	case RT5679_STO3_ADC_DIG_VOL:
	case RT5679_ADC_BST_GAIN_CTRL1:
	case RT5679_ADC_BST_GAIN_CTRL2:
	case RT5679_ADC_BST_GAIN_CTRL3:
	case RT5679_SPDIF_IN_CTRL:
	case RT5679_IF3_DATA_CTRL:
	case RT5679_IF4_DATA_CTRL:
	case RT5679_IF5_DATA_CTRL:
	case RT5679_TDM1_CTRL1:
	case RT5679_TDM1_CTRL2:
	case RT5679_TDM1_CTRL3:
	case RT5679_TDM1_CTRL4:
	case RT5679_TDM1_CTRL5:
	case RT5679_TDM1_CTRL6:
	case RT5679_TDM1_CTRL7:
	case RT5679_TDM2_CTRL1:
	case RT5679_TDM2_CTRL2:
	case RT5679_TDM2_CTRL3:
	case RT5679_TDM2_CTRL4:
	case RT5679_TDM2_CTRL5:
	case RT5679_TDM2_CTRL6:
	case RT5679_TDM2_CTRL7:
	case RT5679_STO1_DAC_MIXER_CTRL1:
	case RT5679_STO1_DAC_MIXER_CTRL2:
	case RT5679_MONO_DAC_MIXER_CTRL1:
	case RT5679_MONO_DAC_MIXER_CTRL2:
	case RT5679_DD_MIXER_CTRL1:
	case RT5679_DD_MIXER_CTRL2:
	case RT5679_DAC1_MIXER_CTRL:
	case RT5679_DAC2_MIXER_CTRL:
	case RT5679_DAC3_MIXER_CTRL:
	case RT5679_DAC_SOURCE_CTRL:
	case RT5679_STO1_ADC_MIXER_CTRL:
	case RT5679_MONO_ADC_MIXER_CTRL1:
	case RT5679_MONO_ADC_MIXER_CTRL2:
	case RT5679_STO2_ADC_MIXER_CTRL:
	case RT5679_STO3_ADC_MIXER_CTRL:
	case RT5679_DMIC_CTRL1:
	case RT5679_DMIC_CTRL2:
	case RT5679_HPF_CTRL1:
	case RT5679_SV_ZCD_CTRL1:
	case RT5679_PWR_ADC:
	case RT5679_PWR_DIG1:
	case RT5679_PWR_DIG2:
	case RT5679_PWR_ANA1:
	case RT5679_PWR_ANA2:
	case RT5679_PWR_DSP:
	case RT5679_PWR_LDO1:
	case RT5679_PWR_LDO2:
	case RT5679_PWR_LDO3:
	case RT5679_PWR_LDO4:
	case RT5679_PWR_LDO5:
	case RT5679_I2S1_SDP:
	case RT5679_I2S2_SDP:
	case RT5679_I2S3_SDP:
	case RT5679_I2S4_SDP:
	case RT5679_I2S5_SDP:
	case RT5679_I2S_LRCK_BCLK_SOURCE:
	case RT5679_CLK_TREE_CTRL1:
	case RT5679_CLK_TREE_CTRL2:
	case RT5679_CLK_TREE_CTRL3:
	case RT5679_CLK_TREE_CTRL4:
	case RT5679_PLL1_CTRL1:
	case RT5679_PLL1_CTRL2:
	case RT5679_PLL2_CTRL1:
	case RT5679_PLL2_CTRL2:
	case RT5679_DSP_CLK_SOURCE1:
	case RT5679_DSP_CLK_SOURCE2:
	case RT5679_GLB_CLK1:
	case RT5679_GLB_CLK2:
	case RT5679_ASRC1:
	case RT5679_ASRC2:
	case RT5679_ASRC3:
	case RT5679_ASRC4:
	case RT5679_ASRC5:
	case RT5679_ASRC6:
	case RT5679_ASRC7:
	case RT5679_ASRC8:
	case RT5679_ASRC9:
	case RT5679_ASRC10:
	case RT5679_ASRC11:
	case RT5679_ASRC12:
	case RT5679_ASRC13:
	case RT5679_ASRC14:
	case RT5679_ASRC15:
	case RT5679_ASRC16:
	case RT5679_ASRC17:
	case RT5679_ASRC18:
	case RT5679_ASRC19:
	case RT5679_ASRC20:
	case RT5679_ASRC21:
	case RT5679_ASRC22:
	case RT5679_ASRC23:
	case RT5679_ASRC24:
	case RT5679_ASRC25:
	case RT5679_FRAC_DIV_CTRL1:
	case RT5679_FRAC_DIV_CTRL2:
	case RT5679_JACK_MIC_DET_CTRL1:
	case RT5679_JACK_MIC_DET_CTRL2:
	case RT5679_JACK_MIC_DET_CTRL3:
	case RT5679_JACK_MIC_DET_CTRL4:
	case RT5679_JACK_DET_CTRL1:
	case RT5679_JACK_DET_CTRL2:
	case RT5679_JACK_DET_CTRL3:
	case RT5679_JACK_DET_CTRL4:
	case RT5679_JACK_DET_CTRL5:
	case RT5679_IRQ_ST1:
	case RT5679_IRQ_ST2:
	case RT5679_IRQ_CTRL1:
	case RT5679_IRQ_CTRL2:
	case RT5679_IRQ_CTRL3:
	case RT5679_IRQ_CTRL4:
	case RT5679_IRQ_CTRL5:
	case RT5679_IRQ_CTRL6:
	case RT5679_IRQ_CTRL7:
	case RT5679_IRQ_CTRL8:
	case RT5679_IRQ_CTRL9:
	case RT5679_MF_PIN_CTRL1:
	case RT5679_MF_PIN_CTRL2:
	case RT5679_MF_PIN_CTRL3:
	case RT5679_GPIO_CTRL1:
	case RT5679_GPIO_CTRL2:
	case RT5679_GPIO_CTRL3:
	case RT5679_GPIO_CTRL4:
	case RT5679_GPIO_CTRL5:
	case RT5679_GPIO_CTRL6:
	case RT5679_GPIO_ST1:
	case RT5679_GPIO_ST2:
	case RT5679_LP_DET_CTRL:
	case RT5679_STO1_ADC_HPF_CTRL1:
	case RT5679_STO1_ADC_HPF_CTRL2:
	case RT5679_MONO_ADC_HPF_CTRL1:
	case RT5679_MONO_ADC_HPF_CTRL2:
	case RT5679_STO2_ADC_HPF_CTRL1:
	case RT5679_STO2_ADC_HPF_CTRL2:
	case RT5679_STO3_ADC_HPF_CTRL1:
	case RT5679_STO3_ADC_HPF_CTRL2:
	case RT5679_ZCD_CTRL:
	case RT5679_IL_CMD1:
	case RT5679_IL_CMD2:
	case RT5679_IL_CMD3:
	case RT5679_IL_CMD4:
	case RT5679_4BTN_IL_CMD1:
	case RT5679_4BTN_IL_CMD2:
	case RT5679_4BTN_IL_CMD3:
	case RT5679_PS_IL_CMD1:
	case RT5679_DSP_OUTB_0123_MIXER_CTRL:
	case RT5679_DSP_OUTB_45_MIXER_CTRL:
	case RT5679_DSP_OUTB_67_MIXER_CTRL:
	case RT5679_MCLK_GATING_CTRL:
	case RT5679_VENDOR_ID:
	case RT5679_VENDOR_ID1:
	case RT5679_VENDOR_ID2:
	case RT5679_PDM_OUTPUT_CTRL:
	case RT5679_PDM1_CTRL1:
	case RT5679_PDM1_CTRL2:
	case RT5679_PDM1_CTRL3:
	case RT5679_PDM1_CTRL4:
	case RT5679_PDM1_CTRL5:
	case RT5679_PDM2_CTRL1:
	case RT5679_PDM2_CTRL2:
	case RT5679_PDM2_CTRL3:
	case RT5679_PDM2_CTRL4:
	case RT5679_PDM2_CTRL5:
	case RT5679_STO_DAC_POST_VOL_CTRL:
	case RT5679_ST_CTRL:
	case RT5679_MCLK_DET_PROTECT_CTRL:
	case RT5679_STO_HP_NG2_CTRL1:
	case RT5679_STO_HP_NG2_CTRL2:
	case RT5679_STO_HP_NG2_CTRL3:
	case RT5679_STO_HP_NG2_CTRL4:
	case RT5679_STO_HP_NG2_CTRL5:
	case RT5679_STO_HP_NG2_CTRL6:
	case RT5679_STO_HP_NG2_ST1:
	case RT5679_STO_HP_NG2_ST2:
	case RT5679_STO_HP_NG2_ST3:
	case RT5679_NG2_ENV_DITHER_CTRL:
	case RT5679_MONO_AMP_NG2_CTRL1:
	case RT5679_MONO_AMP_NG2_CTRL2:
	case RT5679_MONO_AMP_NG2_CTRL3:
	case RT5679_MONO_AMP_NG2_CTRL4:
	case RT5679_MONO_AMP_NG2_CTRL5:
	case RT5679_MONO_AMP_NG2_ST1:
	case RT5679_MONO_AMP_NG2_ST2:
	case RT5679_IF_INPUT_DET_ST1:
	case RT5679_IF_INPUT_DET_ST2:
	case RT5679_IF_INPUT_DET_ST3:
	case RT5679_STO_DAC_SIL_DET_CTRL:
	case RT5679_MONO_DACL_SIL_DET_CTRL:
	case RT5679_MONO_DACR_SIL_DET_CTRL:
	case RT5679_DD_MIXERL_SIL_DET_CTRL:
	case RT5679_DD_MIXERR_SIL_DET_CTRL:
	case RT5679_SIL_DET_CTRLOUTPUT1:
	case RT5679_SIL_DET_CTRLOUTPUT2:
	case RT5679_SIL_DET_CTRLOUTPUT3:
	case RT5679_SIL_DET_CTRLOUTPUT4:
	case RT5679_SIL_DET_CTRLOUTPUT5:
	case RT5679_SIL_DET_CTRLOUTPUT6:
	case RT5679_SIL_DET_CTRLOUTPUT7:
	case RT5679_SIL_DET_CTRLOUTPUT8:
	case RT5679_ADC_EQ_CTRL1:
	case RT5679_ADC_EQ_CTRL2:
	case RT5679_DAC_EQ_CTRL1:
	case RT5679_DAC_EQ_CTRL2:
	case RT5679_DAC_EQ_CTRL3:
	case RT5679_DAC_EQ_CTRL4:
	case RT5679_I2S_MASTER_CLK_CTRL1:
	case RT5679_I2S_MASTER_CLK_CTRL2:
	case RT5679_I2S_MASTER_CLK_CTRL3:
	case RT5679_I2S_MASTER_CLK_CTRL4:
	case RT5679_I2S_MASTER_CLK_CTRL5:
	case RT5679_I2S_MASTER_CLK_CTRL6:
	case RT5679_I2S_MASTER_CLK_CTRL7:
	case RT5679_I2S_MASTER_CLK_CTRL8:
	case RT5679_I2S_MASTER_CLK_CTRL9:
	case RT5679_I2S_MASTER_CLK_CTRL10:
	case RT5679_I2S_MASTER_CLK_CTRL11:
	case RT5679_I2S_MASTER_CLK_CTRL12:
	case RT5679_I2S_MASTER_CLK_CTRL13:
	case RT5679_HP_DECR_DECOUP_CTRL1:
	case RT5679_HP_DECR_DECOUP_CTRL2:
	case RT5679_HP_DECR_DECOUP_CTRL3:
	case RT5679_HP_DECR_DECOUP_CTRL4:
	case RT5679_VAD_ADC_FILTER_CTRL1:
	case RT5679_VAD_ADC_FILTER_CTRL2:
	case RT5679_VAD_CLK_SETTING1:
	case RT5679_VAD_CLK_SETTING2:
	case RT5679_VAD_ADC_PLL3_CTRL1:
	case RT5679_VAD_ADC_PLL3_CTRL2:
	case RT5679_HP_BL_CTRL1:
	case RT5679_HP_BL_CTRL2:
	case RT5679_HP_IMP_SENS_CTRL1:
	case RT5679_HP_IMP_SENS_CTRL2:
	case RT5679_HP_IMP_SENS_CTRL3:
	case RT5679_HP_IMP_SENS_CTRL4:
	case RT5679_HP_IMP_SENS_CTRL5:
	case RT5679_HP_IMP_SENS_CTRL6:
	case RT5679_HP_IMP_SENS_CTRL7:
	case RT5679_HP_IMP_SENS_CTRL8:
	case RT5679_HP_IMP_SENS_CTRL9:
	case RT5679_HP_IMP_SENS_CTRL10:
	case RT5679_HP_IMP_SENS_CTRL11:
	case RT5679_HP_IMP_SENS_CTRL12:
	case RT5679_HP_IMP_SENS_CTRL13:
	case RT5679_HP_IMP_SENS_CTRL14:
	case RT5679_HP_IMP_SENS_CTRL15:
	case RT5679_HP_IMP_SENS_CTRL16:
	case RT5679_HP_IMP_SENS_CTRL17:
	case RT5679_HP_IMP_SENS_CTRL18:
	case RT5679_HP_IMP_SENS_DIG_CTRL1:
	case RT5679_HP_IMP_SENS_DIG_CTRL2:
	case RT5679_HP_IMP_SENS_DIG_CTRL3:
	case RT5679_HP_IMP_SENS_DIG_CTRL4:
	case RT5679_HP_IMP_SENS_DIG_CTRL5:
	case RT5679_HP_IMP_SENS_DIG_CTRL6:
	case RT5679_HP_IMP_SENS_DIG_CTRL7:
	case RT5679_HP_IMP_SENS_DIG_CTRL8:
	case RT5679_HP_IMP_SENS_DIG_CTRL9:
	case RT5679_HP_IMP_SENS_DIG_CTRL10:
	case RT5679_HP_IMP_SENS_DIG_CTRL11:
	case RT5679_HP_IMP_SENS_DIG_CTRL12:
	case RT5679_HP_IMP_SENS_DIG_CTRL13:
	case RT5679_HP_IMP_SENS_DIG_CTRL14:
	case RT5679_HP_IMP_SENS_DIG_CTRL15:
	case RT5679_HP_IMP_SENS_DIG_CTRL16:
	case RT5679_HP_IMP_SENS_DIG_CTRL17:
	case RT5679_ALC_PGA_CTRL1:
	case RT5679_ALC_PGA_CTRL2:
	case RT5679_ALC_PGA_CTRL3:
	case RT5679_ALC_PGA_CTRL4:
	case RT5679_ALC_PGA_CTRL5:
	case RT5679_ALC_PGA_CTRL6:
	case RT5679_ALC_PGA_CTRL7:
	case RT5679_ALC_PGA_ST1:
	case RT5679_ALC_PGA_ST2:
	case RT5679_ALC_PGA_ST3:
	case RT5679_ALC_PGA_SOURCE_CTRL1:
	case RT5679_HAPTIC_GEN_CTRL1:
	case RT5679_HAPTIC_GEN_CTRL2:
	case RT5679_HAPTIC_GEN_CTRL3:
	case RT5679_HAPTIC_GEN_CTRL4:
	case RT5679_HAPTIC_GEN_CTRL5:
	case RT5679_HAPTIC_GEN_CTRL6:
	case RT5679_HAPTIC_GEN_CTRL7:
	case RT5679_HAPTIC_GEN_CTRL8:
	case RT5679_HAPTIC_GEN_CTRL9:
	case RT5679_HAPTIC_GEN_CTRL10:
	case RT5679_AUTO_RC_CLK_CTRL1:
	case RT5679_AUTO_RC_CLK_CTRL2:
	case RT5679_AUTO_RC_CLK_CTRL3:
	case RT5679_DAC_L_EQ_LPF1_A1:
	case RT5679_DAC_L_EQ_LPF1_H0:
	case RT5679_DAC_R_EQ_LPF1_A1:
	case RT5679_DAC_R_EQ_LPF1_H0:
	case RT5679_DAC_L_EQ_LPF2_A1:
	case RT5679_DAC_L_EQ_LPF2_H0:
	case RT5679_DAC_R_EQ_LPF2_A1:
	case RT5679_DAC_R_EQ_LPF2_H0:
	case RT5679_DAC_L_EQ_BPF1_A1:
	case RT5679_DAC_L_EQ_BPF1_A2:
	case RT5679_DAC_L_EQ_BPF1_H0:
	case RT5679_DAC_R_EQ_BPF1_A1:
	case RT5679_DAC_R_EQ_BPF1_A2:
	case RT5679_DAC_R_EQ_BPF1_H0:
	case RT5679_DAC_L_EQ_BPF2_A1:
	case RT5679_DAC_L_EQ_BPF2_A2:
	case RT5679_DAC_L_EQ_BPF2_H0:
	case RT5679_DAC_R_EQ_BPF2_A1:
	case RT5679_DAC_R_EQ_BPF2_A2:
	case RT5679_DAC_R_EQ_BPF2_H0:
	case RT5679_DAC_L_EQ_BPF3_A1:
	case RT5679_DAC_L_EQ_BPF3_A2:
	case RT5679_DAC_L_EQ_BPF3_H0:
	case RT5679_DAC_R_EQ_BPF3_A1:
	case RT5679_DAC_R_EQ_BPF3_A2:
	case RT5679_DAC_R_EQ_BPF3_H0:
	case RT5679_DAC_L_EQ_BPF4_A1:
	case RT5679_DAC_L_EQ_BPF4_A2:
	case RT5679_DAC_L_EQ_BPF4_H0:
	case RT5679_DAC_R_EQ_BPF4_A1:
	case RT5679_DAC_R_EQ_BPF4_A2:
	case RT5679_DAC_R_EQ_BPF4_H0:
	case RT5679_DAC_L_EQ_BPF5_A1:
	case RT5679_DAC_L_EQ_BPF5_A2:
	case RT5679_DAC_L_EQ_BPF5_H0:
	case RT5679_DAC_R_EQ_BPF5_A1:
	case RT5679_DAC_R_EQ_BPF5_A2:
	case RT5679_DAC_R_EQ_BPF5_H0:
	case RT5679_DAC_L_EQ_HPF1_A1:
	case RT5679_DAC_L_EQ_HPF1_H0:
	case RT5679_DAC_R_EQ_HPF1_A1:
	case RT5679_DAC_R_EQ_HPF1_H0:
	case RT5679_DAC_L_EQ_HPF2_A1:
	case RT5679_DAC_L_EQ_HPF2_A2:
	case RT5679_DAC_L_EQ_HPF2_H0:
	case RT5679_DAC_R_EQ_HPF2_A1:
	case RT5679_DAC_R_EQ_HPF2_A2:
	case RT5679_DAC_R_EQ_HPF2_H0:
	case RT5679_DAC_L_EQ_HPF3_A1:
	case RT5679_DAC_L_EQ_HPF3_H0:
	case RT5679_DAC_R_EQ_HPF3_A1:
	case RT5679_DAC_R_EQ_HPF3_H0:
	case RT5679_DAC_L_BI_EQ_H0_1:
	case RT5679_DAC_L_BI_EQ_H0_2:
	case RT5679_DAC_L_BI_EQ_B1_1:
	case RT5679_DAC_L_BI_EQ_B1_2:
	case RT5679_DAC_L_BI_EQ_B2_1:
	case RT5679_DAC_L_BI_EQ_B2_2:
	case RT5679_DAC_L_BI_EQ_A1_1:
	case RT5679_DAC_L_BI_EQ_A1_2:
	case RT5679_DAC_L_BI_EQ_A2_1:
	case RT5679_DAC_L_BI_EQ_A2_2:
	case RT5679_DAC_R_BI_EQ_H0_1:
	case RT5679_DAC_R_BI_EQ_H0_2:
	case RT5679_DAC_R_BI_EQ_B1_1:
	case RT5679_DAC_R_BI_EQ_B1_2:
	case RT5679_DAC_R_BI_EQ_B2_1:
	case RT5679_DAC_R_BI_EQ_B2_2:
	case RT5679_DAC_R_BI_EQ_A1_1:
	case RT5679_DAC_R_BI_EQ_A1_2:
	case RT5679_DAC_R_BI_EQ_A2_1:
	case RT5679_DAC_R_BI_EQ_A2_2:
	case RT5679_DAC_L_EQ_PRE_VOL_CTRL:
	case RT5679_DAC_R_EQ_PRE_VOL_CTRL:
	case RT5679_DAC_L_EQ_POST_VOL_CTRL:
	case RT5679_DAC_R_EQ_POST_VOL_CTRL:
	case RT5679_ADC_L_EQ_LPF_A1:
	case RT5679_ADC_L_EQ_LPF_H0:
	case RT5679_ADC_R_EQ_LPF_A1:
	case RT5679_ADC_R_EQ_LPF_H0:
	case RT5679_ADC_L_EQ_BPF1_A1:
	case RT5679_ADC_L_EQ_BPF1_A2:
	case RT5679_ADC_L_EQ_BPF1_H0:
	case RT5679_ADC_R_EQ_BPF1_A1:
	case RT5679_ADC_R_EQ_BPF1_A2:
	case RT5679_ADC_R_EQ_BPF1_H0:
	case RT5679_ADC_L_EQ_BPF2_A1:
	case RT5679_ADC_L_EQ_BPF2_A2:
	case RT5679_ADC_L_EQ_BPF2_H0:
	case RT5679_ADC_R_EQ_BPF2_A1:
	case RT5679_ADC_R_EQ_BPF2_A2:
	case RT5679_ADC_R_EQ_BPF2_H0:
	case RT5679_ADC_L_EQ_BPF3_A1:
	case RT5679_ADC_L_EQ_BPF3_A2:
	case RT5679_ADC_L_EQ_BPF3_H0:
	case RT5679_ADC_R_EQ_BPF3_A1:
	case RT5679_ADC_R_EQ_BPF3_A2:
	case RT5679_ADC_R_EQ_BPF3_H0:
	case RT5679_ADC_L_EQ_BPF4_A1:
	case RT5679_ADC_L_EQ_BPF4_A2:
	case RT5679_ADC_L_EQ_BPF4_H0:
	case RT5679_ADC_R_EQ_BPF4_A1:
	case RT5679_ADC_R_EQ_BPF4_A2:
	case RT5679_ADC_R_EQ_BPF4_H0:
	case RT5679_ADC_L_EQ_HPF1_A1:
	case RT5679_ADC_L_EQ_HPF1_H0:
	case RT5679_ADC_R_EQ_HPF1_A1:
	case RT5679_ADC_R_EQ_HPF1_H0:
	case RT5679_ADC_L_EQ_PRE_VOL_CTRL:
	case RT5679_ADC_R_EQ_PRE_VOL_CTRL:
	case RT5679_ADC_L_EQ_POST_VOL_CTRL:
	case RT5679_ADC_R_EQ_POST_VOL_CTRL:
	case RT5679_PITCH_HELLO_DET_CTRL1:
	case RT5679_PITCH_HELLO_DET_CTRL2:
	case RT5679_PITCH_HELLO_DET_CTRL3:
	case RT5679_PITCH_HELLO_DET_CTRL4:
	case RT5679_PITCH_HELLO_DET_CTRL5:
	case RT5679_PITCH_HELLO_DET_CTRL6:
	case RT5679_PITCH_HELLO_DET_CTRL7:
	case RT5679_PITCH_HELLO_DET_CTRL8:
	case RT5679_PITCH_HELLO_DET_CTRL9:
	case RT5679_PITCH_HELLO_DET_CTRL10:
	case RT5679_PITCH_HELLO_DET_CTRL11:
	case RT5679_PITCH_HELLO_DET_CTRL12:
	case RT5679_PITCH_HELLO_DET_CTRL13:
	case RT5679_PITCH_HELLO_DET_CTRL14:
	case RT5679_PITCH_HELLO_DET_CTRL15:
	case RT5679_PITCH_HELLO_DET_CTRL16:
	case RT5679_PITCH_HELLO_DET_CTRL17:
	case RT5679_PITCH_HELLO_DET_CTRL18:
	case RT5679_PITCH_HELLO_DET_CTRL19:
	case RT5679_PITCH_HELLO_DET_CTRL20:
	case RT5679_PITCH_HELLO_DET_CTRL21:
	case RT5679_PITCH_HELLO_DET_CTRL22:
	case RT5679_PITCH_HELLO_DET_CTRL23:
	case RT5679_OK_DET_CTRL1:
	case RT5679_OK_DET_CTRL2:
	case RT5679_OK_DET_CTRL3:
	case RT5679_OK_DET_CTRL4:
	case RT5679_OK_DET_CTRL5:
	case RT5679_OK_DET_CTRL6:
	case RT5679_OK_DET_CTRL7:
	case RT5679_OK_DET_CTRL8:
	case RT5679_OK_DET_CTRL9:
	case RT5679_OK_DET_CTRL10:
	case RT5679_OK_DET_CTRL11:
	case RT5679_OK_DET_CTRL12:
	case RT5679_OK_DET_CTRL13:
	case RT5679_OK_DET_CTRL14:
	case RT5679_OK_DET_CTRL15:
	case RT5679_DFLL_CAL_CTRL1:
	case RT5679_DFLL_CAL_CTRL2:
	case RT5679_DFLL_CAL_CTRL3:
	case RT5679_DFLL_CAL_CTRL4:
	case RT5679_DFLL_CAL_CTRL5:
	case RT5679_DFLL_CAL_CTRL6:
	case RT5679_DFLL_CAL_CTRL7:
	case RT5679_DFLL_CAL_CTRL8:
	case RT5679_DFLL_CAL_CTRL9:
	case RT5679_DFLL_CAL_CTRL10:
	case RT5679_DFLL_CAL_CTRL11:
	case RT5679_DFLL_CAL_CTRL12:
	case RT5679_DFLL_CAL_CTRL13:
	case RT5679_DFLL_CAL_CTRL14:
	case RT5679_VAD_FUNCTION_CTRL1:
	case RT5679_DELAY_BUFFER_SRAM_CTRL1:
	case RT5679_DELAY_BUFFER_SRAM_CTRL2:
	case RT5679_DELAY_BUFFER_SRAM_CTRL3:
	case RT5679_DELAY_BUFFER_SRAM_CTRL4:
	case RT5679_DELAY_BUFFER_SRAM_CTRL5:
	case RT5679_DELAY_BUFFER_SRAM_CTRL6:
	case RT5679_DELAY_BUFFER_SRAM_CTRL7:
	case RT5679_DMIC_CLK_ON_OFF_CTRL1:
	case RT5679_DMIC_CLK_ON_OFF_CTRL2:
	case RT5679_DMIC_CLK_ON_OFF_CTRL3:
	case RT5679_DMIC_CLK_ON_OFF_CTRL4:
	case RT5679_DMIC_CLK_ON_OFF_CTRL5:
	case RT5679_DMIC_CLK_ON_OFF_CTRL6:
	case RT5679_DMIC_CLK_ON_OFF_CTRL7:
	case RT5679_DMIC_CLK_ON_OFF_CTRL8:
	case RT5679_DMIC_CLK_ON_OFF_CTRL9:
	case RT5679_DMIC_CLK_ON_OFF_CTRL10:
	case RT5679_DMIC_CLK_ON_OFF_CTRL11:
	case RT5679_DMIC_CLK_ON_OFF_CTRL12:
	case RT5679_DAC_MULTI_DRC_MISC_CTRL:
	case RT5679_DAC_MULTI_DRC_COEF_FB1_CTRL1:
	case RT5679_DAC_MULTI_DRC_COEF_FB1_CTRL2:
	case RT5679_DAC_MULTI_DRC_COEF_FB1_CTRL3:
	case RT5679_DAC_MULTI_DRC_COEF_FB1_CTRL4:
	case RT5679_DAC_MULTI_DRC_COEF_FB1_CTRL5:
	case RT5679_DAC_MULTI_DRC_COEF_FB1_CTRL6:
	case RT5679_DAC_MULTI_DRC_COEF_FB2_CTRL7:
	case RT5679_DAC_MULTI_DRC_COEF_FB2_CTRL8:
	case RT5679_DAC_MULTI_DRC_COEF_FB2_CTRL9:
	case RT5679_DAC_MULTI_DRC_COEF_FB2_CTRL10:
	case RT5679_DAC_MULTI_DRC_COEF_FB2_CTRL11:
	case RT5679_DAC_MULTI_DRC_COEF_FB2_CTRL12:
	case RT5679_DAC_MULTI_DRC_HB_CTRL1:
	case RT5679_DAC_MULTI_DRC_HB_CTRL2:
	case RT5679_DAC_MULTI_DRC_HB_CTRL3:
	case RT5679_DAC_MULTI_DRC_HB_CTRL4:
	case RT5679_DAC_MULTI_DRC_HB_CTRL5:
	case RT5679_DAC_MULTI_DRC_HB_CTRL6:
	case RT5679_DAC_MULTI_DRC_HB_CTRL7:
	case RT5679_DAC_MULTI_DRC_HB_CTRL8:
	case RT5679_DAC_MULTI_DRC_HB_CTRL9:
	case RT5679_DAC_MULTI_DRC_HB_CTRL10:
	case RT5679_DAC_MULTI_DRC_HB_CTRL11:
	case RT5679_DAC_MULTI_DRC_HB_CTRL12:
	case RT5679_DAC_MULTI_DRC_MB_CTRL1:
	case RT5679_DAC_MULTI_DRC_MB_CTRL2:
	case RT5679_DAC_MULTI_DRC_MB_CTRL3:
	case RT5679_DAC_MULTI_DRC_MB_CTRL4:
	case RT5679_DAC_MULTI_DRC_MB_CTRL5:
	case RT5679_DAC_MULTI_DRC_MB_CTRL6:
	case RT5679_DAC_MULTI_DRC_MB_CTRL7:
	case RT5679_DAC_MULTI_DRC_MB_CTRL8:
	case RT5679_DAC_MULTI_DRC_MB_CTRL9:
	case RT5679_DAC_MULTI_DRC_MB_CTRL10:
	case RT5679_DAC_MULTI_DRC_MB_CTRL11:
	case RT5679_DAC_MULTI_DRC_MB_CTRL12:
	case RT5679_DAC_MULTI_DRC_BB_CTRL1:
	case RT5679_DAC_MULTI_DRC_BB_CTRL2:
	case RT5679_DAC_MULTI_DRC_BB_CTRL3:
	case RT5679_DAC_MULTI_DRC_BB_CTRL4:
	case RT5679_DAC_MULTI_DRC_BB_CTRL5:
	case RT5679_DAC_MULTI_DRC_BB_CTRL6:
	case RT5679_DAC_MULTI_DRC_BB_CTRL7:
	case RT5679_DAC_MULTI_DRC_BB_CTRL8:
	case RT5679_DAC_MULTI_DRC_BB_CTRL9:
	case RT5679_DAC_MULTI_DRC_BB_CTRL10:
	case RT5679_DAC_MULTI_DRC_BB_CTRL11:
	case RT5679_DAC_MULTI_DRC_BB_CTRL12:
	case RT5679_DAC_MULTI_DRC_POS_CTRL1:
	case RT5679_DAC_MULTI_DRC_POS_CTRL2:
	case RT5679_DAC_MULTI_DRC_POS_CTRL3:
	case RT5679_DAC_MULTI_DRC_POS_CTRL4:
	case RT5679_DAC_MULTI_DRC_POS_CTRL5:
	case RT5679_DAC_MULTI_DRC_POS_CTRL6:
	case RT5679_DAC_MULTI_DRC_POS_CTRL7:
	case RT5679_DAC_MULTI_DRC_POS_CTRL8:
	case RT5679_DAC_MULTI_DRC_POS_CTRL9:
	case RT5679_DAC_MULTI_DRC_POS_CTRL10:
	case RT5679_DAC_MULTI_DRC_POS_CTRL11:
	case RT5679_DAC_MULTI_DRC_POS_CTRL12:
	case RT5679_DAC_MULTI_DRC_POS_CTRL13:
	case RT5679_DAC_MULTI_DRC_POS_ST1:
	case RT5679_DAC_MULTI_DRC_POS_ST2:
	case RT5679_ADC_ALC_CTRL1:
	case RT5679_ADC_ALC_CTRL2:
	case RT5679_ADC_ALC_CTRL3:
	case RT5679_ADC_ALC_CTRL4:
	case RT5679_ADC_ALC_CTRL5:
	case RT5679_ADC_ALC_CTRL6:
	case RT5679_ADC_ALC_CTRL7:
	case RT5679_ADC_ALC_CTRL8:
	case RT5679_ADC_ALC_CTRL9:
	case RT5679_ADC_ALC_CTRL10:
	case RT5679_ADC_ALC_CTRL11:
	case RT5679_ADC_ALC_CTRL12:
	case RT5679_ADC_ALC_CTRL13:
	case RT5679_ADC_ALC_CTRL14:
	case RT5679_ADC_ALC_ST1:
	case RT5679_ADC_ALC_ST2:
	case RT5679_HP_DC_CAL_CTRL1:
	case RT5679_HP_DC_CAL_CTRL2:
	case RT5679_HP_DC_CAL_CTRL3:
	case RT5679_HP_DC_CAL_CTRL4:
	case RT5679_HP_DC_CAL_CTRL5:
	case RT5679_HP_DC_CAL_CTRL6:
	case RT5679_HP_DC_CAL_CTRL7:
	case RT5679_HP_DC_CAL_CTRL8:
	case RT5679_HP_DC_CAL_CTRL9:
	case RT5679_HP_DC_CAL_CTRL10:
	case RT5679_HP_DC_CAL_CTRL11:
	case RT5679_HP_DC_CAL_CTRL12:
	case RT5679_HP_DC_CAL_ST1:
	case RT5679_HP_DC_CAL_ST2:
	case RT5679_HP_DC_CAL_ST3:
	case RT5679_HP_DC_CAL_ST4:
	case RT5679_HP_DC_CAL_ST5:
	case RT5679_HP_DC_CAL_ST6:
	case RT5679_HP_DC_CAL_ST7:
	case RT5679_HP_DC_CAL_ST8:
	case RT5679_HP_DC_CAL_ST9:
	case RT5679_HP_DC_CAL_ST10:
	case RT5679_HP_DC_CAL_ST11:
	case RT5679_HP_DC_CAL_ST12:
	case RT5679_HP_DC_CAL_ST13:
	case RT5679_MONO_AMP_DC_CAL_CTRL1:
	case RT5679_MONO_AMP_DC_CAL_CTRL2:
	case RT5679_MONO_AMP_DC_CAL_CTRL3:
	case RT5679_MONO_AMP_DC_CAL_CTRL4:
	case RT5679_MONO_AMP_DC_CAL_CTRL5:
	case RT5679_MONO_AMP_DC_CAL_CTRL6:
	case RT5679_MONO_AMP_DC_CAL_CTRL7:
	case RT5679_MONO_AMP_DC_CAL_ST1:
	case RT5679_MONO_AMP_DC_CAL_ST2:
	case RT5679_MONO_AMP_DC_CAL_ST3:
	case RT5679_DSP_IB_CTRL1:
	case RT5679_DSP_IB_CTRL2:
	case RT5679_DSP_IN_OB_CTRL:
	case RT5679_DSP_OB01_DIG_VOL:
	case RT5679_DSP_OB23_DIG_VOL:
	case RT5679_DSP_OB45_DIG_VOL:
	case RT5679_DSP_OB67_DIG_VOL:
	case RT5679_MINI_DSP_OB01_DIG_VOL:
	case RT5679_DSP_IB1_SRC_CTRL1:
	case RT5679_DSP_IB1_SRC_CTRL2:
	case RT5679_DSP_IB1_SRC_CTRL3:
	case RT5679_DSP_IB1_SRC_CTRL4:
	case RT5679_DSP_IB2_SRC_CTRL1:
	case RT5679_DSP_IB2_SRC_CTRL2:
	case RT5679_DSP_IB2_SRC_CTRL3:
	case RT5679_DSP_IB2_SRC_CTRL4:
	case RT5679_DSP_IB3_SRC_CTRL1:
	case RT5679_DSP_IB3_SRC_CTRL2:
	case RT5679_DSP_IB3_SRC_CTRL3:
	case RT5679_DSP_IB3_SRC_CTRL4:
	case RT5679_DSP_OB1_SRC_CTRL1:
	case RT5679_DSP_OB1_SRC_CTRL2:
	case RT5679_DSP_OB1_SRC_CTRL3:
	case RT5679_DSP_OB1_SRC_CTRL4:
	case RT5679_DSP_OB2_SRC_CTRL1:
	case RT5679_DSP_OB2_SRC_CTRL2:
	case RT5679_DSP_OB2_SRC_CTRL3:
	case RT5679_DSP_OB2_SRC_CTRL4:
	case RT5679_HIFI_MINI_DSP_CTRL_ST:
	case RT5679_SPI_SLAVE_CRC_CHECK_CTRL:
	case RT5679_EFUSE_CTRL1:
	case RT5679_EFUSE_CTRL2:
	case RT5679_EFUSE_CTRL3:
	case RT5679_EFUSE_CTRL4:
	case RT5679_EFUSE_CTRL5:
	case RT5679_EFUSE_CTRL6:
	case RT5679_EFUSE_CTRL7:
	case RT5679_EFUSE_CTRL8:
	case RT5679_EFUSE_CTRL9:
	case RT5679_EFUSE_CTRL10:
	case RT5679_EFUSE_CTRL11:
	case RT5679_I2C_AND_SPI_SCRAM_CTRL:
	case RT5679_I2C_SCRAM_WRITE_KEY1_MSB:
	case RT5679_I2C_SCRAM_WRITE_KEY1_LSB:
	case RT5679_I2C_SCRAM_WRITE_KEY2_MSB:
	case RT5679_I2C_SCRAM_WRITE_KEY2_LSB:
	case RT5679_I2C_SCRAM_READ_KEY1_MSB:
	case RT5679_I2C_SCRAM_READ_KEY1_LSB:
	case RT5679_I2C_SCRAM_READ_KEY2_MSB:
	case RT5679_I2C_SCRAM_READ_KEY2_LSB:
	case RT5679_SPI_SCRAM_WRITE_KEY1_1:
	case RT5679_SPI_SCRAM_WRITE_KEY1_2:
	case RT5679_SPI_SCRAM_WRITE_KEY1_3:
	case RT5679_SPI_SCRAM_WRITE_KEY1_4:
	case RT5679_SPI_SCRAM_WRITE_KEY2_1:
	case RT5679_SPI_SCRAM_WRITE_KEY2_2:
	case RT5679_SPI_SCRAM_WRITE_KEY2_3:
	case RT5679_SPI_SCRAM_WRITE_KEY2_4:
	case RT5679_SPI_SCRAM_READ_KEY1_1:
	case RT5679_SPI_SCRAM_READ_KEY1_2:
	case RT5679_SPI_SCRAM_READ_KEY1_3:
	case RT5679_SPI_SCRAM_READ_KEY1_4:
	case RT5679_SPI_SCRAM_READ_KEY2_1:
	case RT5679_SPI_SCRAM_READ_KEY2_2:
	case RT5679_SPI_SCRAM_READ_KEY2_3:
	case RT5679_SPI_SCRAM_READ_KEY2_4:
	case RT5679_GPIO1_TEST_OUTPUT_SEL1:
	case RT5679_GPIO1_TEST_OUTPUT_SEL2:
	case RT5679_GPIO1_TEST_OUTPUT_SEL3:
	case RT5679_GPIO1_TEST_OUTPUT_SEL4:
	case RT5679_PR_REG_MONO_AMP_BIAS_CTRL:
	case RT5679_PR_REG_BIAS_CTRL1:
	case RT5679_PR_REG_BIAS_CTRL2:
	case RT5679_PR_REG_BIAS_CTRL3:
	case RT5679_PR_REG_BIAS_CTRL4:
	case RT5679_PR_REG_BIAS_CTRL5:
	case RT5679_PR_REG_BIAS_CTRL6:
	case RT5679_PR_REG_BIAS_CTRL7:
	case RT5679_PR_REG_BIAS_CTRL8:
	case RT5679_PR_REG_BIAS_CTRL9:
	case RT5679_PR_REG_BIAS_CTRL10:
	case RT5679_PR_REG_BIAS_CTRL11:
	case RT5679_PR_REG_BIAS_CTRL12:
	case RT5679_PR_REG_BIAS_CTRL13:
	case RT5679_PR_REG_ADC12_CLK_CTRL:
	case RT5679_PR_REG_ADC34_CLK_CTRL:
	case RT5679_PR_REG_ADC5_CLK_CTRL1:
	case RT5679_PR_REG_ADC5_CLK_CTRL2:
	case RT5679_PR_REG_ADC67_CLK_CTRL:
	case RT5679_PR_REG_PLL1_CTRL1:
	case RT5679_PR_REG_PLL1_CTRL2:
	case RT5679_PR_REG_PLL2_CTRL1:
	case RT5679_PR_REG_PLL2_CTRL2:
	case RT5679_PR_REG_VREF_CTRL1:
	case RT5679_PR_REG_VREF_CTRL2:
	case RT5679_PR_REG_BST1_CTRL:
	case RT5679_PR_REG_BST2_CTRL:
	case RT5679_PR_REG_BST3_CTRL:
	case RT5679_PR_REG_BST4_CTRL:
	case RT5679_DAC_ADC_DIG_VOL1:
	case RT5679_DAC_ADC_DIG_VOL2:
	case RT5679_VAD_SRAM_TEST:
	case RT5679_PAD_DRIVING_CTRL1:
	case RT5679_PAD_DRIVING_CTRL2:
	case RT5679_PAD_DRIVING_CTRL3:
	case RT5679_DIG_INPUT_PIN_ST_CTRL1:
	case RT5679_DIG_INPUT_PIN_ST_CTRL2:
	case RT5679_DIG_INPUT_PIN_ST_CTRL3:
	case RT5679_DIG_INPUT_PIN_ST_CTRL4:
	case RT5679_DIG_INPUT_PIN_ST_CTRL5:
	case RT5679_TEST_MODE_CTRL1:
	case RT5679_TEST_MODE_CTRL2:
	case RT5679_GPIO1_GPIO3_TEST_MODE_CTRL:
	case RT5679_GPIO5_GPIO6_TEST_MODE_CTRL:
	case RT5679_GPIO6_GPIO7_TEST_MODE_CTRL:
	case RT5679_CODEC_DOMAIN_REG_RW_CTRL:
	case RT5679_DAC1_CLK_AND_CHOPPER_CTRL:
	case RT5679_DAC2_CLK_AND_CHOPPER_CTRL:
	case RT5679_DAC3_CLK_AND_CHOPPER_CTRL:
	case RT5679_DAC4_CLK_AND_CHOPPER_CTRL:
	case RT5679_DAC5_CLK_AND_CHOPPER_CTRL:
	case RT5679_DAC1_DAC2_DUMMY_REG:
	case RT5679_HP_CTRL1:
	case RT5679_HP_CTRL2:
	case RT5679_HP_CTRL3:
	case RT5679_HP_CTRL4:
	case RT5679_HP_CTRL5:
	case RT5679_HP_CTRL6:
	case RT5679_LDO6_PR_CTRL1:
	case RT5679_LDO6_PR_CTRL2:
	case RT5679_LDO6_PR_CTRL3:
	case RT5679_LDO6_PR_CTRL4:
	case RT5679_LDO_AVDD1_PR_CTRL:
	case RT5679_LDO_HV2_PR_CTRL:
	case RT5679_LDO_HV3_PR_CTRL:
	case RT5679_LDO1_LDO3_LDO4_PR_CTRL:
	case RT5679_LDO8_LDO9_PR_CTRL:
	case RT5679_VREF5_L_PR_CTRL:
	case RT5679_VREF5_R_PR_CTRL:
	case RT5679_SLIMBUS_PARAMETER:
	case RT5679_SLIMBUS_RX:
	case RT5679_SLIMBUS_CTRL:
	case RT5679_LOUT_CTRL:
	case RT5679_DUMMY_REG_1:
	case RT5679_DUMMY_REG_2:
	case RT5679_DUMMY_REG_3:
	case RT5679_DUMMY_REG_4:
	case RT5679_DSP_I2C_DATA_MSB:
		return true;

	default:
		return false;
	}
}

/**
 * rt5679_headset_detect - Detect headset.
 * @codec: SoC audio codec device.
 * @jack_insert: Jack insert or not.
 *
 * Detect whether is headset or not when jack inserted.
 *
 * Returns detect status.
 */
int rt5679_headset_detect(struct snd_soc_codec *codec, int jack_insert)
{
	struct snd_soc_dapm_context *dapm = snd_soc_codec_get_dapm(codec);
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);
	int val = 0, i = 0;

	if (jack_insert) {
		snd_soc_dapm_force_enable_pin(dapm, "Mic Det Power");
		snd_soc_dapm_sync(dapm);

		regmap_write(rt5679->regmap, RT5679_JACK_MIC_DET_CTRL1, 0xb880);
		regmap_update_bits(rt5679->regmap, RT5679_JACK_MIC_DET_CTRL2,
			0x0070, 0x0060);
		regmap_update_bits(rt5679->regmap, RT5679_JACK_MIC_DET_CTRL4,
			0x8000, 0x0);
		regmap_update_bits(rt5679->regmap, RT5679_DUMMY_REG_1, 0x2000,
			0x2000);
		regmap_update_bits(rt5679->regmap, RT5679_JACK_MIC_DET_CTRL1,
			0x0020, 0x0020);

		while (i < 20) {
			regmap_read(rt5679->regmap, RT5679_JACK_MIC_DET_CTRL4,
				&val);

			if (val & 0x1)
				break;

			i++;
			msleep(10);
		}

		regmap_read(rt5679->regmap, RT5679_JACK_MIC_DET_CTRL2, &val);

		switch (val & 0x3) {
		case 1:
			rt5679->jack_type = SND_JACK_HEADSET;
			break;

		default:
			rt5679->jack_type = SND_JACK_HEADPHONE;
			regmap_write(rt5679->regmap, RT5679_JACK_MIC_DET_CTRL1,
				0x7080);
			snd_soc_dapm_disable_pin(dapm, "Mic Det Power");
			snd_soc_dapm_sync(dapm);
			break;
		}
	} else {
		regmap_write(rt5679->regmap, RT5679_JACK_MIC_DET_CTRL1, 0x7080);
		snd_soc_dapm_disable_pin(dapm, "Mic Det Power");
		snd_soc_dapm_sync(dapm);
		rt5679->jack_type = 0;
	}

	pr_debug("jack_type = %d\n", rt5679->jack_type);
	return rt5679->jack_type;
}
EXPORT_SYMBOL(rt5679_headset_detect);

static void rt5679_jack_detect_work(struct work_struct *work)
{
	struct rt5679_priv *rt5679 =
		container_of(work, struct rt5679_priv, jack_detect_work.work);
	struct snd_soc_codec *codec = rt5679->codec;
	unsigned int value;

	regmap_read(rt5679->regmap, RT5679_IRQ_ST1, &value);

	if (!(value & 0x8000)) {
		if (rt5679->jack_type) {
			regmap_read(rt5679->regmap, RT5679_4BTN_IL_CMD1, &value);
			regmap_write(rt5679->regmap, RT5679_4BTN_IL_CMD1, value);

			rt5679->jack_type = SND_JACK_HEADSET;

			switch (value & 0xfff0) {
			case 0x8000:
			case 0x4000:
			case 0x2000:
				rt5679->jack_type |= SND_JACK_BTN_0;
				break;
			case 0x1000:
			case 0x0800:
			case 0x0400:
				rt5679->jack_type |= SND_JACK_BTN_1;
				break;
			case 0x0200:
			case 0x0100:
			case 0x0080:
				rt5679->jack_type |= SND_JACK_BTN_2;
				break;
			case 0x0040:
			case 0x0020:
			case 0x0010:
				rt5679->jack_type |= SND_JACK_BTN_3;
				break;
			case 0x0000:
				break;
			default:
				dev_err(rt5679->codec->dev,
					"Unexpected button code 0x%04x\n",
					rt5679->jack_type);
				break;
			}
		} else {
			rt5679->jack_type = rt5679_headset_detect(codec, 1);

			if (rt5679->jack_type == SND_JACK_HEADSET) {
#ifdef CONFIG_SWITCH
				switch_set_state(&rt5679_headset_switch, 1);
#endif
				regmap_update_bits(rt5679->regmap,
					RT5679_4BTN_IL_CMD2, 0xc000, 0xc000);
				regmap_update_bits(rt5679->regmap,
					RT5679_IRQ_CTRL3, 0x20, 0x20);
			} else {
#ifdef CONFIG_SWITCH
				switch_set_state(&rt5679_headset_switch, 2);
#endif
			}

			rt5679->irq_work_delay_time = 0;
		}
	} else {
			rt5679->jack_type = rt5679_headset_detect(codec, 0);
#ifdef CONFIG_SWITCH
			switch_set_state(&rt5679_headset_switch, 0);
#endif
			regmap_update_bits(rt5679->regmap, RT5679_IRQ_CTRL3,
				0x20, 0);
			regmap_update_bits(rt5679->regmap, RT5679_4BTN_IL_CMD2,
				0xc000, 0);

			rt5679->irq_work_delay_time = 250;
	}

	snd_soc_jack_report(rt5679->hp_jack, rt5679->jack_type,
		SND_JACK_HEADSET | SND_JACK_BTN_0 | SND_JACK_BTN_1 |
		SND_JACK_BTN_2 | SND_JACK_BTN_3);
}

static irqreturn_t rt5679_irq(int irq, void *data)
{
	struct rt5679_priv *rt5679 = data;

	queue_delayed_work(system_wq, &rt5679->jack_detect_work,
		msecs_to_jiffies(rt5679->irq_work_delay_time));

	return IRQ_HANDLED;
}

int rt5679_set_jack_detect(struct snd_soc_codec *codec,
	struct snd_soc_jack *hp_jack)
{
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);

	rt5679->hp_jack = hp_jack;
	rt5679->irq_work_delay_time = 250;

	regmap_update_bits(rt5679->regmap, RT5679_DUMMY_REG_1, 0x0840, 0x0840);
	regmap_update_bits(rt5679->regmap, RT5679_JACK_DET_CTRL1, 0xc000,
		0x4000);
	regmap_update_bits(rt5679->regmap, RT5679_JACK_DET_CTRL5, 0x3000,
		0x1000);
	regmap_update_bits(rt5679->regmap, RT5679_IRQ_CTRL1, 0x8800, 0x8800);
	regmap_update_bits(rt5679->regmap, RT5679_MF_PIN_CTRL1,
		RT5679_GP1_TYPE_MASK, RT5679_GP1_TYPE_IRQ1);
	regmap_update_bits(rt5679->regmap, RT5679_JACK_MIC_DET_CTRL4, 0x72,
		0x72);
#ifdef CONFIG_SWITCH
	switch_dev_register(&rt5679_headset_switch);
#endif
	rt5679_irq(0, rt5679);

	return 0;
}
EXPORT_SYMBOL_GPL(rt5679_set_jack_detect);

static const DECLARE_TLV_DB_SCALE(ng2_vol_tlv, -2325, 75, 0);
static const DECLARE_TLV_DB_SCALE(dac_vol_tlv, -65625, 375, 0);
static const DECLARE_TLV_DB_SCALE(bst_tlv, -1200, 75, 0);
static const DECLARE_TLV_DB_SCALE(adc_vol_tlv, -17625, 375, 0);
static const DECLARE_TLV_DB_SCALE(adc_bst_tlv, 0, 1200, 0);

static const char * const rt5679_asrc_clk_source[] = {
	"clk_sys_div_out", "clk_i2s1_track", "clk_i2s2_track", "clk_i2s3_track",
	"clk_i2s4_track", "clk_i2s5_track", "clk_i2s6_track", "clk_i2s7_track",
	"clk_sys2", "clk_sys3", "clk_sys4", "clk_sys5", "clk_sys6", "clk_sys7",
	"clk_sys8"
};

static const SOC_ENUM_SINGLE_DECL(rt5679_da_sto1_asrc_enum, RT5679_ASRC3,
	RT5679_DA_FILTER_STEREO_TRACK_SFT, rt5679_asrc_clk_source);

static const SOC_ENUM_SINGLE_DECL(rt5679_da_mono2l_asrc_enum, RT5679_ASRC3,
	RT5679_DA_FILTER_MONO2L_TRACK_SFT, rt5679_asrc_clk_source);

static const SOC_ENUM_SINGLE_DECL(rt5679_da_mono2r_asrc_enum, RT5679_ASRC3,
	RT5679_DA_FILTER_MONO2R_TRACK_SFT, rt5679_asrc_clk_source);

static const SOC_ENUM_SINGLE_DECL(rt5679_da_mono3l_asrc_enum, RT5679_ASRC4,
	RT5679_DA_FILTER_MONO3L_TRACK_SFT, rt5679_asrc_clk_source);

static const SOC_ENUM_SINGLE_DECL(rt5679_da_mono3r_asrc_enum, RT5679_ASRC4,
	RT5679_DA_FILTER_MONO3R_TRACK_SFT, rt5679_asrc_clk_source);

static const SOC_ENUM_SINGLE_DECL(rt5679_ad_sto1_asrc_enum, RT5679_ASRC5,
	RT5679_AD_FILTER_STEREO1_TRACK_SFT, rt5679_asrc_clk_source);

static const SOC_ENUM_SINGLE_DECL(rt5679_ad_sto2_asrc_enum, RT5679_ASRC5,
	RT5679_AD_FILTER_STEREO2_TRACK_SFT, rt5679_asrc_clk_source);

static const SOC_ENUM_SINGLE_DECL(rt5679_ad_sto3_asrc_enum, RT5679_ASRC5,
	RT5679_AD_FILTER_STEREO3_TRACK_SFT, rt5679_asrc_clk_source);

static const SOC_ENUM_SINGLE_DECL(rt5679_ad_monol_asrc_enum, RT5679_ASRC6,
	RT5679_AD_FILTER_MONOL_TRACK_SFT, rt5679_asrc_clk_source);

static const SOC_ENUM_SINGLE_DECL(rt5679_ad_monor_asrc_enum, RT5679_ASRC6,
	RT5679_AD_FILTER_MONOR_TRACK_SFT, rt5679_asrc_clk_source);

static const SOC_ENUM_SINGLE_DECL(rt5679_ob_0_3_asrc_enum, RT5679_ASRC7,
	RT5679_DSP_OUT_FS_SEC1_SFT, rt5679_asrc_clk_source);

static const SOC_ENUM_SINGLE_DECL(rt5679_ob_4_7_asrc_enum, RT5679_ASRC7,
	RT5679_DSP_OUT_FS_SEC2_SFT, rt5679_asrc_clk_source);

static int rt5679_da_sto1_asrc_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	switch (ucontrol->value.integer.value[0]) {
	case 1 ... 7: /*enable*/
		if (snd_soc_read(codec, RT5679_PWR_DIG2) & RT5679_PWR_DAC_S1F)
			snd_soc_update_bits(codec, RT5679_ASRC2,
				RT5679_EN_STO_DAC_ASRC,	RT5679_EN_STO_DAC_ASRC);
		break;
	default: /*disable*/
		snd_soc_update_bits(codec, RT5679_ASRC2, RT5679_EN_STO_DAC_ASRC,
			0);
		break;
	}

	return snd_soc_put_enum_double(kcontrol, ucontrol);
}

static int rt5679_da_mono2l_asrc_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	switch (ucontrol->value.integer.value[0]) {
	case 1 ... 7: /*enable*/
		if (snd_soc_read(codec, RT5679_PWR_DIG2) & RT5679_PWR_DAC_M2F_L)
			snd_soc_update_bits(codec, RT5679_ASRC2,
				RT5679_EN_MONO_DAC_L_ASRC,
				RT5679_EN_MONO_DAC_L_ASRC);
		break;
	default: /*disable*/
		snd_soc_update_bits(codec, RT5679_ASRC2,
			RT5679_EN_MONO_DAC_L_ASRC, 0);
		break;
	}

	return snd_soc_put_enum_double(kcontrol, ucontrol);
}

static int rt5679_da_mono2r_asrc_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	switch (ucontrol->value.integer.value[0]) {
	case 1 ... 7: /*enable*/
		if (snd_soc_read(codec, RT5679_PWR_DIG2) & RT5679_PWR_DAC_M2F_R)
			snd_soc_update_bits(codec, RT5679_ASRC2,
				RT5679_EN_MONO_DAC_R_ASRC,
				RT5679_EN_MONO_DAC_R_ASRC);
		break;
	default: /*disable*/
		snd_soc_update_bits(codec, RT5679_ASRC2,
			RT5679_EN_MONO_DAC_R_ASRC, 0);
		break;
	}

	return snd_soc_put_enum_double(kcontrol, ucontrol);
}

static int rt5679_da_mono3l_asrc_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	switch (ucontrol->value.integer.value[0]) {
	case 1 ... 7: /*enable*/
		if (snd_soc_read(codec, RT5679_PWR_DIG2) & RT5679_PWR_DAC_M3F_L)
			snd_soc_update_bits(codec, RT5679_ASRC1,
				RT5679_EN_MONO_DA_3L_ASRC,
				RT5679_EN_MONO_DA_3L_ASRC);
		break;
	default: /*disable*/
		snd_soc_update_bits(codec, RT5679_ASRC1,
			RT5679_EN_MONO_DA_3L_ASRC, 0);
		break;
	}

	return snd_soc_put_enum_double(kcontrol, ucontrol);
}

static int rt5679_da_mono3r_asrc_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	switch (ucontrol->value.integer.value[0]) {
	case 1 ... 7: /*enable*/
		if (snd_soc_read(codec, RT5679_PWR_DIG2) & RT5679_PWR_DAC_M3F_R)
			snd_soc_update_bits(codec, RT5679_ASRC1,
				RT5679_EN_MONO_DA_3R_ASRC,
				RT5679_EN_MONO_DA_3R_ASRC);
		break;
	default: /*disable*/
		snd_soc_update_bits(codec, RT5679_ASRC1,
			RT5679_EN_MONO_DA_3R_ASRC, 0);
		break;
	}

	return snd_soc_put_enum_double(kcontrol, ucontrol);
}

static int rt5679_ad_sto1_asrc_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	switch (ucontrol->value.integer.value[0]) {
	case 1 ... 7: /*enable*/
		if (snd_soc_read(codec, RT5679_PWR_DIG2) & RT5679_PWR_ADC_S1F)
			snd_soc_update_bits(codec, RT5679_ASRC2,
				RT5679_EN_ADC_ASRC_STO1,
				RT5679_EN_ADC_ASRC_STO1);
		break;
	default: /*disable*/
		snd_soc_update_bits(codec, RT5679_ASRC2,
			RT5679_EN_ADC_ASRC_STO1, 0);
		break;
	}

	return snd_soc_put_enum_double(kcontrol, ucontrol);
}

static int rt5679_ad_sto2_asrc_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	switch (ucontrol->value.integer.value[0]) {
	case 1 ... 7: /*enable*/
		if (snd_soc_read(codec, RT5679_PWR_DIG2) & RT5679_PWR_ADC_S2F)
			snd_soc_update_bits(codec, RT5679_ASRC2,
				RT5679_EN_ADC_ASRC_STO2,
				RT5679_EN_ADC_ASRC_STO2);
		break;
	default: /*disable*/
		snd_soc_update_bits(codec, RT5679_ASRC2,
			RT5679_EN_ADC_ASRC_STO2, 0);
		break;
	}

	return snd_soc_put_enum_double(kcontrol, ucontrol);
}

static int rt5679_ad_sto3_asrc_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	switch (ucontrol->value.integer.value[0]) {
	case 1 ... 7: /*enable*/
		if (snd_soc_read(codec, RT5679_PWR_DIG2) & RT5679_PWR_ADC_S3F)
			snd_soc_update_bits(codec, RT5679_ASRC2,
				RT5679_EN_ADC_ASRC_STO3,
				RT5679_EN_ADC_ASRC_STO3);
		break;
	default: /*disable*/
		snd_soc_update_bits(codec, RT5679_ASRC2,
			RT5679_EN_ADC_ASRC_STO3, 0);
		break;
	}

	return snd_soc_put_enum_double(kcontrol, ucontrol);
}

static int rt5679_ad_monol_asrc_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	switch (ucontrol->value.integer.value[0]) {
	case 1 ... 7: /*enable*/
		if (snd_soc_read(codec, RT5679_PWR_DIG2) & RT5679_PWR_ADC_MF_L)
			snd_soc_update_bits(codec, RT5679_ASRC2,
				RT5679_EN_ADC_ASRC_MONOL,
				RT5679_EN_ADC_ASRC_MONOL);
		break;
	default: /*disable*/
		snd_soc_update_bits(codec, RT5679_ASRC2,
			RT5679_EN_ADC_ASRC_MONOL, 0);
		break;
	}

	return snd_soc_put_enum_double(kcontrol, ucontrol);
}

static int rt5679_ad_monor_asrc_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	switch (ucontrol->value.integer.value[0]) {
	case 1 ... 7: /*enable*/
		if (snd_soc_read(codec, RT5679_PWR_DIG2) & RT5679_PWR_ADC_MF_R)
			snd_soc_update_bits(codec, RT5679_ASRC2,
				RT5679_EN_ADC_ASRC_MONOR,
				RT5679_EN_ADC_ASRC_MONOR);
		break;
	default: /*disable*/
		snd_soc_update_bits(codec, RT5679_ASRC2,
			RT5679_EN_ADC_ASRC_MONOR, 0);
		break;
	}

	return snd_soc_put_enum_double(kcontrol, ucontrol);
}

static const char *rt5679_jack_type_mode[] = {
	"Disable", "Read"
};

static const SOC_ENUM_SINGLE_DECL(rt5679_jack_type_enum, 0, 0,
	rt5679_jack_type_mode);

static int rt5679_jack_type_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = 0;

	return 0;
}

static int rt5679_jack_type_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int jack_insert = ucontrol->value.integer.value[0];

	dev_info(codec->dev, "ret=0x%x\n",
		rt5679_headset_detect(codec, jack_insert));

	return 0;
}

static int rt5679_dsp_mode_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	rt5679_set_dsp_mode(codec, ucontrol->value.integer.value[0]);

	return 0;
}

static int rt5679_hp_vol_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int ret = snd_soc_put_volsw(kcontrol, ucontrol);

	if (snd_soc_read(codec, RT5679_STO_HP_NG2_CTRL1) & 0x8000) {
		snd_soc_update_bits(codec, RT5679_STO_HP_NG2_CTRL1, 0x8000,
			0x0000);
		snd_soc_update_bits(codec, RT5679_STO_HP_NG2_CTRL1, 0x8000,
			0x8000);
	}

	return ret;
}

static int rt5679_mono_vol_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int ret = snd_soc_put_volsw(kcontrol, ucontrol);

	if (snd_soc_read(codec, RT5679_MONO_AMP_NG2_CTRL1) & 0x8000) {
		snd_soc_update_bits(codec, RT5679_MONO_AMP_NG2_CTRL1, 0x8000,
			0x0000);
		snd_soc_update_bits(codec, RT5679_MONO_AMP_NG2_CTRL1, 0x8000,
			0x8000);
	}

	return ret;
}

static const struct snd_kcontrol_new rt5679_snd_controls[] = {
	/* Headphone Output Volume */
	SOC_DOUBLE_R_EXT_TLV("Headphone Playback Volume", RT5679_STO_HP_NG2_CTRL2,
		RT5679_STO_HP_NG2_CTRL3, RT5679_G_HP_SFT, 34, 1, snd_soc_get_volsw,
		rt5679_hp_vol_put, ng2_vol_tlv),

	/* Mono Output Volume */
	SOC_SINGLE_EXT_TLV("Mono Playback Volume", RT5679_MONO_AMP_NG2_CTRL2,
		RT5679_G_MONO_SFT, 34, 1, snd_soc_get_volsw, rt5679_mono_vol_put,
		ng2_vol_tlv),

	/* DAC Digital Volume */
	SOC_DOUBLE_TLV("DAC1 Playback Volume", RT5679_DAC1_DIG_VOL,
			RT5679_L_VOL_SFT, RT5679_R_VOL_SFT,
			175, 0, dac_vol_tlv),
	SOC_DOUBLE_TLV("DAC2 Playback Volume", RT5679_DAC2_DIG_VOL,
			RT5679_L_VOL_SFT, RT5679_R_VOL_SFT,
			175, 0, dac_vol_tlv),
	SOC_DOUBLE_TLV("DAC3 Playback Volume", RT5679_DAC3_DIG_VOL,
			RT5679_L_VOL_SFT, RT5679_R_VOL_SFT,
			175, 0, dac_vol_tlv),

	/* IN Boost Control */
	SOC_SINGLE_TLV("IN1 Capture Volume", RT5679_BST12_CTRL, RT5679_BST1_SFT,
		69, 0, bst_tlv),
	SOC_SINGLE_TLV("IN2 Capture Volume", RT5679_BST12_CTRL, RT5679_BST2_SFT,
		69, 0, bst_tlv),
	SOC_SINGLE_TLV("IN3 Capture Volume", RT5679_BST34_CTRL, RT5679_BST3_SFT,
		69, 0, bst_tlv),
	SOC_SINGLE_TLV("IN4 Capture Volume", RT5679_BST34_CTRL, RT5679_BST4_SFT,
		69, 0, bst_tlv),

	/* ADC Digital Volume Control */
	SOC_DOUBLE("ADC1 Capture Switch", RT5679_STO1_ADC_DIG_VOL,
		RT5679_L_MUTE_SFT, RT5679_R_MUTE_SFT, 1, 1),
	SOC_DOUBLE("ADC2 Capture Switch", RT5679_STO2_ADC_DIG_VOL,
		RT5679_L_MUTE_SFT, RT5679_R_MUTE_SFT, 1, 1),
	SOC_DOUBLE("ADC3 Capture Switch", RT5679_STO3_ADC_DIG_VOL,
		RT5679_L_MUTE_SFT, RT5679_R_MUTE_SFT, 1, 1),
	SOC_DOUBLE("Mono ADC Capture Switch", RT5679_MONO_ADC_DIG_VOL,
		RT5679_L_MUTE_SFT, RT5679_R_MUTE_SFT, 1, 1),

	SOC_DOUBLE_TLV("ADC1 Capture Volume", RT5679_STO1_ADC_DIG_VOL,
			RT5679_STO1_ADC_L_VOL_SFT, RT5679_STO1_ADC_R_VOL_SFT,
			127, 0, adc_vol_tlv),
	SOC_DOUBLE_TLV("ADC2 Capture Volume", RT5679_STO2_ADC_DIG_VOL,
			RT5679_STO2_ADC_L_VOL_SFT, RT5679_STO2_ADC_R_VOL_SFT,
			127, 0, adc_vol_tlv),
	SOC_DOUBLE_TLV("ADC3 Capture Volume", RT5679_STO3_ADC_DIG_VOL,
			RT5679_STO3_ADC_L_VOL_SFT, RT5679_STO3_ADC_R_VOL_SFT,
			127, 0, adc_vol_tlv),
	SOC_DOUBLE_TLV("Mono ADC Capture Volume", RT5679_MONO_ADC_DIG_VOL,
			RT5679_MONO_ADC_L_VOL_SFT, RT5679_MONO_ADC_R_VOL_SFT,
			127, 0, adc_vol_tlv),

	/* ADC Boost Volume Control */
	SOC_DOUBLE_TLV("STO1 ADC Boost Volume", RT5679_ADC_BST_GAIN_CTRL1,
			RT5679_STO1_ADC_L_BST_SFT, RT5679_STO1_ADC_R_BST_SFT,
			3, 0, adc_bst_tlv),
	SOC_DOUBLE_TLV("STO2 ADC Boost Volume", RT5679_ADC_BST_GAIN_CTRL1,
			RT5679_STO2_ADC_L_BST_SFT, RT5679_STO2_ADC_R_BST_SFT,
			3, 0, adc_bst_tlv),
	SOC_DOUBLE_TLV("STO3 ADC Boost Volume", RT5679_ADC_BST_GAIN_CTRL3,
			RT5679_STO3_ADC_L_BST_SFT, RT5679_STO3_ADC_R_BST_SFT,
			3, 0, adc_bst_tlv),
	SOC_DOUBLE_TLV("Mono ADC Boost Volume", RT5679_ADC_BST_GAIN_CTRL2,
			RT5679_MONO_ADC_L_BST_SFT, RT5679_MONO_ADC_R_BST_SFT,
			3, 0, adc_bst_tlv),

	SOC_ENUM_EXT("DA STO1 ASRC Switch", rt5679_da_sto1_asrc_enum,
		snd_soc_get_enum_double, rt5679_da_sto1_asrc_put),
	SOC_ENUM_EXT("DA MONO2L ASRC Switch", rt5679_da_mono2l_asrc_enum,
		snd_soc_get_enum_double, rt5679_da_mono2l_asrc_put),
	SOC_ENUM_EXT("DA MONO2R ASRC Switch", rt5679_da_mono2r_asrc_enum,
		snd_soc_get_enum_double, rt5679_da_mono2r_asrc_put),
	SOC_ENUM_EXT("DA MONO3L ASRC Switch", rt5679_da_mono3l_asrc_enum,
		snd_soc_get_enum_double, rt5679_da_mono3l_asrc_put),
	SOC_ENUM_EXT("DA MONO3R ASRC Switch", rt5679_da_mono3r_asrc_enum,
		snd_soc_get_enum_double, rt5679_da_mono3r_asrc_put),
	SOC_ENUM_EXT("AD STO1 ASRC Switch", rt5679_ad_sto1_asrc_enum,
		snd_soc_get_enum_double, rt5679_ad_sto1_asrc_put),
	SOC_ENUM_EXT("AD STO2 ASRC Switch", rt5679_ad_sto2_asrc_enum,
		snd_soc_get_enum_double, rt5679_ad_sto2_asrc_put),
	SOC_ENUM_EXT("AD STO3 ASRC Switch", rt5679_ad_sto3_asrc_enum,
		snd_soc_get_enum_double, rt5679_ad_sto3_asrc_put),
	SOC_ENUM_EXT("AD MONOL ASRC Switch", rt5679_ad_monol_asrc_enum,
		snd_soc_get_enum_double, rt5679_ad_monol_asrc_put),
	SOC_ENUM_EXT("AD MONOR ASRC Switch", rt5679_ad_monor_asrc_enum,
		snd_soc_get_enum_double, rt5679_ad_monor_asrc_put),
	SOC_ENUM("OB 0-3 ASRC Switch", rt5679_ob_0_3_asrc_enum),
	SOC_ENUM("OB 4-7 ASRC Switch", rt5679_ob_4_7_asrc_enum),

	SOC_ENUM_EXT("jack type", rt5679_jack_type_enum,
		rt5679_jack_type_get, rt5679_jack_type_put),
	SOC_ENUM_EXT("dsp mode", rt5679_jack_type_enum,
		rt5679_jack_type_get, rt5679_dsp_mode_put),
};

static int calc_dmic_clk(int rate)
{
	int div[] = {2, 3, 4, 6, 8, 12};
	int i;

	if (rate < 1000000 * div[0]) {
		pr_warn("Base clock rate %d is too low\n", rate);
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(div); i++) {
		/* find divider that gives DMIC frequency below 2.048MHz */
		if (2048000 * div[i] >= rate)
			return i;
	}

	pr_warn("Base clock rate %d is too high\n", rate);
	return -EINVAL;
}

int get_pre_div(struct regmap *map, unsigned int reg, int sft)
{
	int pd, val;

	regmap_read(map, reg, &val);

	val = (val >> sft) & 0x7;

	switch (val) {
	case 0:
	case 1:
	case 2:
	case 3:
		pd = val + 1;
		break;

	case 4:
		pd = 6;
		break;

	case 5:
		pd = 8;
		break;

	case 6:
		pd = 12;
		break;

	case 7:
		pd = 16;
		break;

	default:
		pd = -EINVAL;
		break;
	}

	return pd;
}

/**
 * set_dmic_clk - Set parameter of dmic.
 *
 * @w: DAPM widget.
 * @kcontrol: The kcontrol of this widget.
 * @event: Event id.
 *
 * Choose dmic clock between 1MHz and 3MHz.
 * It is better for clock to approximate 3MHz.
 */
static int rt5679_set_dmic_clk(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);
	int idx, rate;

	rate = rt5679->sysclk / get_pre_div(rt5679->regmap,
		RT5679_CLK_TREE_CTRL1, RT5679_I2S_PD1_SFT);
	idx = calc_dmic_clk(rate);
	if (idx < 0)
		dev_err(codec->dev, "Failed to set DMIC clock\n");
	else
		regmap_update_bits(rt5679->regmap, RT5679_DMIC_CTRL1,
			RT5679_DMIC_CLK_MASK, idx << RT5679_DMIC_CLK_SFT);
	return idx;
}

static int rt5679_is_sys_clk_from_pll(struct snd_soc_dapm_widget *source,
			 struct snd_soc_dapm_widget *sink)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(source->dapm);
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);
	unsigned int val;

	regmap_read(rt5679->regmap, RT5679_GLB_CLK1, &val);
	val &= RT5679_SCLK_SRC_MASK;
	if (val == RT5679_SCLK_SRC_PLL1)
		return 1;
	else
		return 0;
}

static int rt5679_is_using_asrc(struct snd_soc_dapm_widget *source,
			 struct snd_soc_dapm_widget *sink)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(source->dapm);
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);
	unsigned int reg, shift, val;
	int ret = 0;

	if (source->reg == RT5679_ASRC1) {
		switch (source->shift) {
		case RT5679_EN_MONO_DA_3L_ASRC_BIT:
			reg = RT5679_ASRC4;
			shift = RT5679_DA_FILTER_MONO3L_TRACK_SFT;
			break;

		case RT5679_EN_MONO_DA_3R_ASRC_BIT:
			reg = RT5679_ASRC4;
			shift = RT5679_DA_FILTER_MONO3R_TRACK_SFT;
			break;

		default:
			return 0;
		}
	} else {
		switch (source->shift) {
		case RT5679_EN_STO_DAC_ASRC_BIT:
			reg = RT5679_ASRC3;
			shift = RT5679_DA_FILTER_STEREO_TRACK_SFT;
			break;

		case RT5679_EN_MONO_DAC_L_ASRC_BIT:
			reg = RT5679_ASRC3;
			shift = RT5679_DA_FILTER_MONO2L_TRACK_SFT;
			break;

		case RT5679_EN_MONO_DAC_R_ASRC_BIT:
			reg = RT5679_ASRC3;
			shift = RT5679_DA_FILTER_MONO2R_TRACK_SFT;
			break;

		case RT5679_EN_ADC_ASRC_STO1_BIT:
			reg = RT5679_ASRC5;
			shift = RT5679_AD_FILTER_STEREO1_TRACK_SFT;
			break;

		case RT5679_EN_ADC_ASRC_STO2_BIT:
			reg = RT5679_ASRC5;
			shift = RT5679_AD_FILTER_STEREO2_TRACK_SFT;
			break;

		case RT5679_EN_ADC_ASRC_STO3_BIT:
			reg = RT5679_ASRC5;
			shift = RT5679_AD_FILTER_STEREO3_TRACK_SFT;
			break;

		case RT5679_EN_ADC_ASRC_MONOL_BIT:
			reg = RT5679_ASRC6;
			shift = RT5679_AD_FILTER_MONOL_TRACK_SFT;
			break;

		case RT5679_EN_ADC_ASRC_MONOR_BIT:
			reg = RT5679_ASRC6;
			shift = RT5679_AD_FILTER_MONOR_TRACK_SFT;
			break;

		default:
			return 0;
		}
	}

	regmap_read(rt5679->regmap, reg, &val);
	val = (val >> shift) & 0xf;

	switch (val) {
	case 1 ... 7:
		ret = 1;
		break;

	default:
		break;
	}

	return ret;
}

static int rt5679_can_use_asrc(struct snd_soc_dapm_widget *source,
			 struct snd_soc_dapm_widget *sink)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(source->dapm);
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);
	unsigned int aif = RT5679_AIF1;

	switch (sink->shift) {
	case RT5679_PWR_I2S1_BIT:
		aif = RT5679_AIF1;
		break;

	case RT5679_PWR_I2S2_BIT:
		aif = RT5679_AIF2;
		break;

	case RT5679_PWR_I2S3_BIT:
		aif = RT5679_AIF3;
		break;

	case RT5679_PWR_I2S4_BIT:
		aif = RT5679_AIF4;
		break;

	case RT5679_PWR_I2S5_BIT:
		aif = RT5679_AIF5;
		break;

	default:
		break;
	}

	if (rt5679->sysclk > rt5679->lrck[aif] * 384)
		return 1;

	return 0;
}

static int rt5679_dmic_is_using_asrc(struct snd_soc_dapm_widget *source,
			 struct snd_soc_dapm_widget *sink)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(source->dapm);
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);
	unsigned int asrc_setting;

	switch (source->shift) {
	case RT5679_EN_DMIC_ASRC_STO1_BIT:
		regmap_read(rt5679->regmap, RT5679_ASRC5, &asrc_setting);
		asrc_setting =
			(asrc_setting & RT5679_AD_FILTER_STEREO1_TRACK_MASK) >>
			RT5679_AD_FILTER_STEREO1_TRACK_SFT;
		if (asrc_setting >= 1 && asrc_setting <= 7)
			return 1;
		break;

	case RT5679_EN_DMIC_ASRC_STO2_BIT:
		regmap_read(rt5679->regmap, RT5679_ASRC5, &asrc_setting);
		asrc_setting =
			(asrc_setting & RT5679_AD_FILTER_STEREO2_TRACK_MASK) >>
			RT5679_AD_FILTER_STEREO2_TRACK_SFT;
		if (asrc_setting >= 1 && asrc_setting <= 7)
			return 1;
		break;

	case RT5679_EN_DMIC_ASRC_STO3_BIT:
		regmap_read(rt5679->regmap, RT5679_ASRC5, &asrc_setting);
		asrc_setting =
			(asrc_setting & RT5679_AD_FILTER_STEREO3_TRACK_MASK) >>
			RT5679_AD_FILTER_STEREO3_TRACK_SFT;
		if (asrc_setting >= 1 && asrc_setting <= 7)
			return 1;
		break;

	case RT5679_EN_DMIC_ASRC_MONOL_BIT:
		regmap_read(rt5679->regmap, RT5679_ASRC6, &asrc_setting);
		asrc_setting =
			(asrc_setting & RT5679_AD_FILTER_MONOL_TRACK_MASK) >>
			RT5679_AD_FILTER_MONOL_TRACK_SFT;
		if (asrc_setting >= 1 && asrc_setting <= 7)
			return 1;
		break;

	case RT5679_EN_DMIC_ASRC_MONOR_BIT:
		regmap_read(rt5679->regmap, RT5679_ASRC6, &asrc_setting);
		asrc_setting =
			(asrc_setting & RT5679_AD_FILTER_MONOR_TRACK_MASK) >>
			RT5679_AD_FILTER_MONOR_TRACK_SFT;
		if (asrc_setting >= 1 && asrc_setting <= 7)
			return 1;
		break;

	default:
		break;
	}

	return 0;
}

/* Digital Mixer */
static const struct snd_kcontrol_new rt5679_sto1_adc_l_mix[] = {
	SOC_DAPM_SINGLE("ADC1 Switch", RT5679_STO1_ADC_MIXER_CTRL,
			RT5679_M_STO1_ADC_L1_SFT, 1, 1),
	SOC_DAPM_SINGLE("ADC2 Switch", RT5679_STO1_ADC_MIXER_CTRL,
			RT5679_M_STO1_ADC_L2_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_sto1_adc_r_mix[] = {
	SOC_DAPM_SINGLE("ADC1 Switch", RT5679_STO1_ADC_MIXER_CTRL,
			RT5679_M_STO1_ADC_R1_SFT, 1, 1),
	SOC_DAPM_SINGLE("ADC2 Switch", RT5679_STO1_ADC_MIXER_CTRL,
			RT5679_M_STO1_ADC_R2_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_sto2_adc_l_mix[] = {
	SOC_DAPM_SINGLE("ADC1 Switch", RT5679_STO2_ADC_MIXER_CTRL,
			RT5679_M_STO2_ADC_L1_SFT, 1, 1),
	SOC_DAPM_SINGLE("ADC2 Switch", RT5679_STO2_ADC_MIXER_CTRL,
			RT5679_M_STO2_ADC_L2_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_sto2_adc_r_mix[] = {
	SOC_DAPM_SINGLE("ADC1 Switch", RT5679_STO2_ADC_MIXER_CTRL,
			RT5679_M_STO2_ADC_R1_SFT, 1, 1),
	SOC_DAPM_SINGLE("ADC2 Switch", RT5679_STO2_ADC_MIXER_CTRL,
			RT5679_M_STO2_ADC_R2_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_sto3_adc_l_mix[] = {
	SOC_DAPM_SINGLE("ADC1 Switch", RT5679_STO3_ADC_MIXER_CTRL,
			RT5679_M_STO3_ADC_L1_SFT, 1, 1),
	SOC_DAPM_SINGLE("ADC2 Switch", RT5679_STO3_ADC_MIXER_CTRL,
			RT5679_M_STO3_ADC_L2_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_sto3_adc_r_mix[] = {
	SOC_DAPM_SINGLE("ADC1 Switch", RT5679_STO3_ADC_MIXER_CTRL,
			RT5679_M_STO3_ADC_R1_SFT, 1, 1),
	SOC_DAPM_SINGLE("ADC2 Switch", RT5679_STO3_ADC_MIXER_CTRL,
			RT5679_M_STO3_ADC_R2_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_mono_adc_l_mix[] = {
	SOC_DAPM_SINGLE("ADC1 Switch", RT5679_MONO_ADC_MIXER_CTRL1,
			RT5679_M_MONO_ADC_L1_SFT, 1, 1),
	SOC_DAPM_SINGLE("ADC2 Switch", RT5679_MONO_ADC_MIXER_CTRL1,
			RT5679_M_MONO_ADC_L2_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_mono_adc_r_mix[] = {
	SOC_DAPM_SINGLE("ADC1 Switch", RT5679_MONO_ADC_MIXER_CTRL1,
			RT5679_M_MONO_ADC_R1_SFT, 1, 1),
	SOC_DAPM_SINGLE("ADC2 Switch", RT5679_MONO_ADC_MIXER_CTRL1,
			RT5679_M_MONO_ADC_R2_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_dac_l_mix[] = {
	SOC_DAPM_SINGLE("Stereo ADC Switch", RT5679_DAC1_MIXER_CTRL,
			RT5679_M_ADDA_MIXER1_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC1 Switch", RT5679_DAC1_MIXER_CTRL,
			RT5679_M_DAC1_L_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_dac_r_mix[] = {
	SOC_DAPM_SINGLE("Stereo ADC Switch", RT5679_DAC1_MIXER_CTRL,
			RT5679_M_ADDA_MIXER1_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC1 Switch", RT5679_DAC1_MIXER_CTRL,
			RT5679_M_DAC1_R_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_sto1_dac_l_mix[] = {
	SOC_DAPM_SINGLE("ST L Switch", RT5679_STO1_DAC_MIXER_CTRL1,
			RT5679_M_ST_DAC1_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC1 L Switch", RT5679_STO1_DAC_MIXER_CTRL1,
			RT5679_M_DAC_L1_STO1_MIXL_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC1 R Switch", RT5679_STO1_DAC_MIXER_CTRL1,
			RT5679_M_DAC_R1_STO1_MIXL_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC2 L Switch", RT5679_STO1_DAC_MIXER_CTRL1,
			RT5679_M_DAC_L2_STO1_MIXL_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC2 R Switch", RT5679_STO1_DAC_MIXER_CTRL2,
			RT5679_M_DAC_R2_STO1_MIXL_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC3 L Switch", RT5679_STO1_DAC_MIXER_CTRL2,
			RT5679_M_DAC_L3_STO1_MIXL_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_sto1_dac_r_mix[] = {
	SOC_DAPM_SINGLE("ST R Switch", RT5679_STO1_DAC_MIXER_CTRL1,
			RT5679_M_ST_DAC1_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC1 R Switch", RT5679_STO1_DAC_MIXER_CTRL1,
			RT5679_M_DAC_R1_STO1_MIXR_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC1 L Switch", RT5679_STO1_DAC_MIXER_CTRL1,
			RT5679_M_DAC_L1_STO1_MIXR_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC2 R Switch", RT5679_STO1_DAC_MIXER_CTRL1,
			RT5679_M_DAC_R2_STO1_MIXR_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC2 L Switch", RT5679_STO1_DAC_MIXER_CTRL2,
			RT5679_M_DAC_L2_STO1_MIXR_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC3 R Switch", RT5679_STO1_DAC_MIXER_CTRL2,
			RT5679_M_DAC_R3_STO1_MIXR_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_mono_dac_l_mix[] = {
	SOC_DAPM_SINGLE("ST L Switch", RT5679_MONO_DAC_MIXER_CTRL1,
			RT5679_M_ST_DAC2_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC2 L Switch", RT5679_MONO_DAC_MIXER_CTRL1,
			RT5679_M_DAC_L2_MONO_MIXL_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC1 L Switch", RT5679_MONO_DAC_MIXER_CTRL1,
			RT5679_M_DAC_L1_MONO_MIXL_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC2 R Switch", RT5679_MONO_DAC_MIXER_CTRL1,
			RT5679_M_DAC_R2_MONO_MIXL_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC3 L Switch", RT5679_MONO_DAC_MIXER_CTRL2,
			RT5679_M_DACL3_MONO_MIXL_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC3 R Switch", RT5679_MONO_DAC_MIXER_CTRL2,
			RT5679_M_DACR3_MONO_MIXL_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_mono_dac_r_mix[] = {
	SOC_DAPM_SINGLE("ST R Switch", RT5679_MONO_DAC_MIXER_CTRL1,
			RT5679_M_ST_DAC2_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC2 R Switch", RT5679_MONO_DAC_MIXER_CTRL1,
			RT5679_M_DAC_R2_MONO_MIXR_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC1 R Switch", RT5679_MONO_DAC_MIXER_CTRL1,
			RT5679_M_DAC_R1_MONO_MIXR_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC2 L Switch", RT5679_MONO_DAC_MIXER_CTRL1,
			RT5679_M_DAC_L2_MONO_MIXR_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC3 R Switch", RT5679_MONO_DAC_MIXER_CTRL2,
			RT5679_M_DACR3_MONO_MIXR_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC3 L Switch", RT5679_MONO_DAC_MIXER_CTRL2,
			RT5679_M_DACL3_MONO_MIXR_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_dd_l_mix[] = {
	SOC_DAPM_SINGLE("Sto DAC Mix L Switch", RT5679_DD_MIXER_CTRL1,
			RT5679_M_STO1_MIXL_TO_DD_MIXL_SFT, 1, 1),
	SOC_DAPM_SINGLE("Mono DAC Mix L Switch", RT5679_DD_MIXER_CTRL1,
			RT5679_M_MONO_MIXL_TO_DD_MIXL_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC3 L Switch", RT5679_DD_MIXER_CTRL1,
			RT5679_M_DAC_L3_TO_DD_MIXL_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC3 R Switch", RT5679_DD_MIXER_CTRL1,
			RT5679_M_DAC_R3_TO_DD_MIXL_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC1 L Switch", RT5679_DD_MIXER_CTRL2,
			RT5679_M_DAC_L1_TO_DDMIXL_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC2 L Switch", RT5679_DD_MIXER_CTRL2,
			RT5679_M_DAC_L2_TO_DDMIXL_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_dd_r_mix[] = {
	SOC_DAPM_SINGLE("Sto DAC Mix R Switch", RT5679_DD_MIXER_CTRL1,
			RT5679_M_STO1_MIXR_TO_DD_MIXR_SFT, 1, 1),
	SOC_DAPM_SINGLE("Mono DAC Mix R Switch", RT5679_DD_MIXER_CTRL1,
			RT5679_M_MONO_MIXR_TO_DD_MIXR_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC3 R Switch", RT5679_DD_MIXER_CTRL1,
			RT5679_M_DAC_R3_TO_DD_MIXR_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC3 L Switch", RT5679_DD_MIXER_CTRL1,
			RT5679_M_DAC_L3_TO_DD_MIXR_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC1 R Switch", RT5679_DD_MIXER_CTRL2,
			RT5679_M_DAC_R1_TO_DDMIXR_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC2 R Switch", RT5679_DD_MIXER_CTRL2,
			RT5679_M_DAC_R2_TO_DDMIXR_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_ob_01_mix[] = {
	SOC_DAPM_SINGLE("IB01 Switch", RT5679_DSP_OUTB_0123_MIXER_CTRL,
			RT5679_DSP_IB_01_H_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB23 Switch", RT5679_DSP_OUTB_0123_MIXER_CTRL,
			RT5679_DSP_IB_23_H_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB45 Switch", RT5679_DSP_OUTB_0123_MIXER_CTRL,
			RT5679_DSP_IB_45_H_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB6 Switch", RT5679_DSP_OUTB_0123_MIXER_CTRL,
			RT5679_DSP_IB_6_H_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB7 Switch", RT5679_DSP_OUTB_0123_MIXER_CTRL,
			RT5679_DSP_IB_7_H_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_ob_23_mix[] = {
	SOC_DAPM_SINGLE("IB01 Switch", RT5679_DSP_OUTB_0123_MIXER_CTRL,
			RT5679_DSP_IB_01_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB23 Switch", RT5679_DSP_OUTB_0123_MIXER_CTRL,
			RT5679_DSP_IB_23_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB45 Switch", RT5679_DSP_OUTB_0123_MIXER_CTRL,
			RT5679_DSP_IB_45_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB6 Switch", RT5679_DSP_OUTB_0123_MIXER_CTRL,
			RT5679_DSP_IB_6_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB7 Switch", RT5679_DSP_OUTB_0123_MIXER_CTRL,
			RT5679_DSP_IB_7_L_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_ob_4_mix[] = {
	SOC_DAPM_SINGLE("IB01 Switch", RT5679_DSP_OUTB_45_MIXER_CTRL,
			RT5679_DSP_IB_01_H_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB23 Switch", RT5679_DSP_OUTB_45_MIXER_CTRL,
			RT5679_DSP_IB_23_H_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB45 Switch", RT5679_DSP_OUTB_45_MIXER_CTRL,
			RT5679_DSP_IB_45_H_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB6 Switch", RT5679_DSP_OUTB_45_MIXER_CTRL,
			RT5679_DSP_IB_6_H_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB7 Switch", RT5679_DSP_OUTB_45_MIXER_CTRL,
			RT5679_DSP_IB_7_H_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_ob_5_mix[] = {
	SOC_DAPM_SINGLE("IB01 Switch", RT5679_DSP_OUTB_45_MIXER_CTRL,
			RT5679_DSP_IB_01_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB23 Switch", RT5679_DSP_OUTB_45_MIXER_CTRL,
			RT5679_DSP_IB_23_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB45 Switch", RT5679_DSP_OUTB_45_MIXER_CTRL,
			RT5679_DSP_IB_45_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB6 Switch", RT5679_DSP_OUTB_45_MIXER_CTRL,
			RT5679_DSP_IB_6_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB7 Switch", RT5679_DSP_OUTB_45_MIXER_CTRL,
			RT5679_DSP_IB_7_L_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_ob_6_mix[] = {
	SOC_DAPM_SINGLE("IB01 Switch", RT5679_DSP_OUTB_67_MIXER_CTRL,
			RT5679_DSP_IB_01_H_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB23 Switch", RT5679_DSP_OUTB_67_MIXER_CTRL,
			RT5679_DSP_IB_23_H_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB45 Switch", RT5679_DSP_OUTB_67_MIXER_CTRL,
			RT5679_DSP_IB_45_H_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB6 Switch", RT5679_DSP_OUTB_67_MIXER_CTRL,
			RT5679_DSP_IB_6_H_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB7 Switch", RT5679_DSP_OUTB_67_MIXER_CTRL,
			RT5679_DSP_IB_7_H_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5679_ob_7_mix[] = {
	SOC_DAPM_SINGLE("IB01 Switch", RT5679_DSP_OUTB_67_MIXER_CTRL,
			RT5679_DSP_IB_01_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB23 Switch", RT5679_DSP_OUTB_67_MIXER_CTRL,
			RT5679_DSP_IB_23_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB45 Switch", RT5679_DSP_OUTB_67_MIXER_CTRL,
			RT5679_DSP_IB_45_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB6 Switch", RT5679_DSP_OUTB_67_MIXER_CTRL,
			RT5679_DSP_IB_6_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("IB7 Switch", RT5679_DSP_OUTB_67_MIXER_CTRL,
			RT5679_DSP_IB_7_L_SFT, 1, 1),
};

/* Out Switch */
static const struct snd_kcontrol_new rt5679_lout1_switch =
	SOC_DAPM_SINGLE("Switch", RT5679_LOUT, RT5679_L_MUTE_SFT, 1, 1);

static const struct snd_kcontrol_new rt5679_lout2_switch =
	SOC_DAPM_SINGLE("Switch", RT5679_LOUT, RT5679_R_MUTE_SFT, 1, 1);

static const struct snd_kcontrol_new rt5679_pdm1_l_switch =
	SOC_DAPM_SINGLE("Switch", RT5679_PDM_OUTPUT_CTRL, RT5679_M_PDM1_L_SFT,
		1, 1);

static const struct snd_kcontrol_new rt5679_pdm1_r_switch =
	SOC_DAPM_SINGLE("Switch", RT5679_PDM_OUTPUT_CTRL, RT5679_M_PDM1_R_SFT,
		1, 1);

static const struct snd_kcontrol_new rt5679_pdm2_l_switch =
	SOC_DAPM_SINGLE("Switch", RT5679_PDM_OUTPUT_CTRL, RT5679_M_PDM2_L_SFT,
		1, 1);

static const struct snd_kcontrol_new rt5679_pdm2_r_switch =
	SOC_DAPM_SINGLE("Switch", RT5679_PDM_OUTPUT_CTRL, RT5679_M_PDM2_R_SFT,
		1, 1);

/* Mux */
/* DAC1 L/R Source */ /* MX0046 [10:8] */
static const char * const rt5679_dac1_src[] = {
	"IF1 DAC01", "IF2 DAC01", "IF3 DAC", "IF4 DAC", "IF5 DAC", "SLB DAC01",
	"OB01"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_dac1_enum, RT5679_DAC1_MIXER_CTRL,
	RT5679_DAC1_SEL_SFT, rt5679_dac1_src);

static const struct snd_kcontrol_new rt5679_dac1_mux =
	SOC_DAPM_ENUM("DAC1 Source", rt5679_dac1_enum);

/* ADDA1 L/R Source */ /* MX0046 [1:0] */
static const char * const rt5679_adda1_src[] = {
	"STO1 ADC MIX", "STO2 ADC MIX", "OB01", "OB01 MINI"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_adda1_enum, RT5679_DAC1_MIXER_CTRL,
	RT5679_ADDA1_SEL_SFT, rt5679_adda1_src);

static const struct snd_kcontrol_new rt5679_adda1_mux =
	SOC_DAPM_ENUM("ADDA1 Source", rt5679_adda1_enum);


/* DAC2 L/R Source */ /* MX0047 [7:4] [3:0] */
static const char * const rt5679_dac2_src[] = {
	"IF1 DAC23", "IF2 DAC23", "IF3 DAC", "IF4 DAC", "IF5 DAC", "SLB DAC23",
	"OB23", "OB45", "VAD ADC/HAPTIC GEN", "MONO ADC MIX"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_dac2l_enum, RT5679_DAC2_MIXER_CTRL,
	RT5679_SEL_DAC2_L_SRC_SFT, rt5679_dac2_src);

static const struct snd_kcontrol_new rt5679_dac2_l_mux =
	SOC_DAPM_ENUM("DAC2 L Source", rt5679_dac2l_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_dac2r_enum, RT5679_DAC2_MIXER_CTRL,
	RT5679_SEL_DAC2_R_SRC_SFT, rt5679_dac2_src);

static const struct snd_kcontrol_new rt5679_dac2_r_mux =
	SOC_DAPM_ENUM("DAC2 R Source", rt5679_dac2r_enum);

/* DAC3 L/R Source */ /* MX0048 [7:4] [3:0] */
static const char * const rt5679_dac3_src[] = {
	"IF1 DAC45", "IF2 DAC45", "IF3 DAC", "IF4 DAC", "IF5 DAC", "IF1 DAC67",
	"IF2 DAC67", "SLB DAC45", "SLB DAC67", "OB23", "OB45", "STO3 ADC MIX"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_dac3l_enum, RT5679_DAC3_MIXER_CTRL,
	RT5679_SEL_DAC3_L_SRC_SFT, rt5679_dac3_src);

static const struct snd_kcontrol_new rt5679_dac3_l_mux =
	SOC_DAPM_ENUM("DAC3 L Source", rt5679_dac3l_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_dac3r_enum, RT5679_DAC3_MIXER_CTRL,
	RT5679_SEL_DAC3_R_SRC_SFT, rt5679_dac3_src);

static const struct snd_kcontrol_new rt5679_dac3_r_mux =
	SOC_DAPM_ENUM("DAC3 R Source", rt5679_dac3r_enum);

/* In/OutBound Source Pass SRC */ /* MX0502 [3] [4] [0] [1] [2] */
static const char * const rt5679_iob_bypass_src[] = {
	"Bypass", "Pass SRC"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_ob01_bypass_src_enum, RT5679_DSP_IN_OB_CTRL,
	RT5679_SEL_SRC_OB01_SFT, rt5679_iob_bypass_src);

static const struct snd_kcontrol_new rt5679_ob01_bypass_src_mux =
	SOC_DAPM_ENUM("OB01 Bypass Source", rt5679_ob01_bypass_src_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_ob23_bypass_src_enum, RT5679_DSP_IN_OB_CTRL,
	RT5679_SEL_SRC_OB23_SFT, rt5679_iob_bypass_src);

static const struct snd_kcontrol_new rt5679_ob23_bypass_src_mux =
	SOC_DAPM_ENUM("OB23 Bypass Source", rt5679_ob23_bypass_src_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_ib01_bypass_src_enum, RT5679_DSP_IN_OB_CTRL,
	RT5679_SEL_SRC_IB01_SFT, rt5679_iob_bypass_src);

static const struct snd_kcontrol_new rt5679_ib01_bypass_src_mux =
	SOC_DAPM_ENUM("IB01 Bypass Source", rt5679_ib01_bypass_src_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_ib23_bypass_src_enum, RT5679_DSP_IN_OB_CTRL,
	RT5679_SEL_SRC_IB23_SFT, rt5679_iob_bypass_src);

static const struct snd_kcontrol_new rt5679_ib23_bypass_src_mux =
	SOC_DAPM_ENUM("IB23 Bypass Source", rt5679_ib23_bypass_src_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_ib45_bypass_src_enum, RT5679_DSP_IN_OB_CTRL,
	RT5679_SEL_SRC_IB45_SFT, rt5679_iob_bypass_src);

static const struct snd_kcontrol_new rt5679_ib45_bypass_src_mux =
	SOC_DAPM_ENUM("IB45 Bypass Source", rt5679_ib45_bypass_src_enum);

/* DMIC Source */ /* MX004C [11:10][3:2] MX004A MX004D MX004E [5:4] */
/* VAD DMIC Source */ /* MX0193 [7:6] */
static const char * const rt5679_dmic_src[] = {
	"DMIC1", "DMIC2", "DMIC3", "DMIC4"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_mono_dmic_l_enum, RT5679_MONO_ADC_MIXER_CTRL2,
	RT5679_SEL_MONOL_DMIC_SFT, rt5679_dmic_src);

static const struct snd_kcontrol_new rt5679_mono_dmic_l_mux =
	SOC_DAPM_ENUM("Mono DMIC L Source", rt5679_mono_dmic_l_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_mono_dmic_r_enum, RT5679_MONO_ADC_MIXER_CTRL2,
	RT5679_SEL_MONOR_DMIC_SFT, rt5679_dmic_src);

static const struct snd_kcontrol_new rt5679_mono_dmic_r_mux =
	SOC_DAPM_ENUM("Mono DMIC R Source", rt5679_mono_dmic_r_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_stereo1_dmic_enum, RT5679_STO1_ADC_MIXER_CTRL,
	RT5679_SEL_STO1_DMIC_SFT, rt5679_dmic_src);

static const struct snd_kcontrol_new rt5679_sto1_dmic_mux =
	SOC_DAPM_ENUM("Stereo1 DMIC Source", rt5679_stereo1_dmic_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_stereo2_dmic_enum, RT5679_STO2_ADC_MIXER_CTRL,
	RT5679_SEL_STO2_DMIC_SFT, rt5679_dmic_src);

static const struct snd_kcontrol_new rt5679_sto2_dmic_mux =
	SOC_DAPM_ENUM("Stereo2 DMIC Source", rt5679_stereo2_dmic_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_stereo3_dmic_enum, RT5679_STO3_ADC_MIXER_CTRL,
	RT5679_SEL_STO3_DMIC_SFT, rt5679_dmic_src);

static const struct snd_kcontrol_new rt5679_sto3_dmic_mux =
	SOC_DAPM_ENUM("Stereo3 DMIC Source", rt5679_stereo3_dmic_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_vad_dmic_enum, RT5679_VAD_ADC_FILTER_CTRL2,
	6, rt5679_dmic_src);

static const struct snd_kcontrol_new rt5679_vad_dmic_src_mux =
	SOC_DAPM_ENUM("VAD DMIC Source", rt5679_vad_dmic_enum);

/* ADC Source */ /* MX004C [15:14][7:6] MX004A MX004D MX004E [11:10] */
static const char * const rt5679_adc_src[] = {
	"ADC12", "ADC34", "ADC23", "ADC67"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_mono_adc_l_enum, RT5679_MONO_ADC_MIXER_CTRL2,
	RT5679_SEL_MONOL_ANA_ADC_SFT, rt5679_adc_src);

static const struct snd_kcontrol_new rt5679_mono_adc_l_mux =
	SOC_DAPM_ENUM("Mono ADC L Source", rt5679_mono_adc_l_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_mono_adc_r_enum, RT5679_MONO_ADC_MIXER_CTRL2,
	RT5679_SEL_MONOR_ANA_ADC_SFT, rt5679_adc_src);

static const struct snd_kcontrol_new rt5679_mono_adc_r_mux =
	SOC_DAPM_ENUM("Mono ADC R Source", rt5679_mono_adc_r_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_stereo1_adc_enum, RT5679_STO1_ADC_MIXER_CTRL,
	RT5679_SEL_STO1_ANA_ADC_SFT, rt5679_adc_src);

static const struct snd_kcontrol_new rt5679_sto1_adc_mux =
	SOC_DAPM_ENUM("Stereo1 ADC Source", rt5679_stereo1_adc_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_stereo2_adc_enum, RT5679_STO2_ADC_MIXER_CTRL,
	RT5679_SEL_STO2_ANA_ADC_SFT, rt5679_adc_src);

static const struct snd_kcontrol_new rt5679_sto2_adc_mux =
	SOC_DAPM_ENUM("Stereo2 ADC Source", rt5679_stereo2_adc_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_stereo3_adc_enum, RT5679_STO3_ADC_MIXER_CTRL,
	RT5679_SEL_STO3_ANA_ADC_SFT, rt5679_adc_src);

static const struct snd_kcontrol_new rt5679_sto3_adc_mux =
	SOC_DAPM_ENUM("Stereo3 ADC Source", rt5679_stereo3_adc_enum);

/* DD1 Source */ /* MX004C [13:12][5:4] MX004A MX004D MX004E [9:8] */
static const char * const rt5679_dd1_src[] = {
	"DD MIX", "DAC MIX"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_mono_dd1_l_enum, RT5679_MONO_ADC_MIXER_CTRL2,
	RT5679_SEL_MONOL_DD1_SFT, rt5679_dd1_src);

static const struct snd_kcontrol_new rt5679_mono_dd1_l_mux =
	SOC_DAPM_ENUM("Mono DD1 L Source", rt5679_mono_dd1_l_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_mono_dd1_r_enum, RT5679_MONO_ADC_MIXER_CTRL2,
	RT5679_SEL_MONOR_DD1_SFT, rt5679_dd1_src);

static const struct snd_kcontrol_new rt5679_mono_dd1_r_mux =
	SOC_DAPM_ENUM("Mono DD1 R Source", rt5679_mono_dd1_r_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_stereo1_dd1_enum, RT5679_STO1_ADC_MIXER_CTRL,
	RT5679_SEL_STO1_DD1_SFT, rt5679_dd1_src);

static const struct snd_kcontrol_new rt5679_sto1_dd1_mux =
	SOC_DAPM_ENUM("Stereo1 DD1 Source", rt5679_stereo1_dd1_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_stereo2_dd1_enum, RT5679_STO2_ADC_MIXER_CTRL,
	RT5679_SEL_STO2_DD1_SFT, rt5679_dd1_src);

static const struct snd_kcontrol_new rt5679_sto2_dd1_mux =
	SOC_DAPM_ENUM("Stereo2 DD1 Source", rt5679_stereo2_dd1_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_stereo3_dd1_enum, RT5679_STO3_ADC_MIXER_CTRL,
	RT5679_SEL_STO3_DD1_SFT, rt5679_dd1_src);

static const struct snd_kcontrol_new rt5679_sto3_dd1_mux =
	SOC_DAPM_ENUM("Stereo3 ADC Source", rt5679_stereo3_dd1_enum);

/* DD2 Source */ /* MX004C [9:8][1:0] MX004A MX004D MX004E [3:2] */
static const char * const rt5679_dd2_src[] = {
	"DD MIX", "Stereo DAC MIX", "Mono DAC MIX"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_mono_dd2_l_enum, RT5679_MONO_ADC_MIXER_CTRL2,
	RT5679_SEL_MONOL_DD2_SFT, rt5679_dd2_src);

static const struct snd_kcontrol_new rt5679_mono_dd2_l_mux =
	SOC_DAPM_ENUM("Mono DD2 L Source", rt5679_mono_dd2_l_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_mono_dd2_r_enum, RT5679_MONO_ADC_MIXER_CTRL2,
	RT5679_SEL_MONOR_DD2_SFT, rt5679_dd2_src);

static const struct snd_kcontrol_new rt5679_mono_dd2_r_mux =
	SOC_DAPM_ENUM("Mono DD2 R Source", rt5679_mono_dd2_r_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_stereo1_dd2_enum, RT5679_STO1_ADC_MIXER_CTRL,
	RT5679_SEL_STO1_DD2_SFT, rt5679_dd2_src);

static const struct snd_kcontrol_new rt5679_sto1_dd2_mux =
	SOC_DAPM_ENUM("Stereo1 DD2 Source", rt5679_stereo1_dd2_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_stereo2_dd2_enum, RT5679_STO2_ADC_MIXER_CTRL,
	RT5679_SEL_STO2_DD2_SFT, rt5679_dd2_src);

static const struct snd_kcontrol_new rt5679_sto2_dd2_mux =
	SOC_DAPM_ENUM("Stereo2 DD2 Source", rt5679_stereo2_dd2_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_stereo3_dd2_enum, RT5679_STO3_ADC_MIXER_CTRL,
	RT5679_SEL_STO3_DD2_SFT, rt5679_dd2_src);

static const struct snd_kcontrol_new rt5679_sto3_dd2_mux =
	SOC_DAPM_ENUM("Stereo3 DD2 Source", rt5679_stereo3_dd2_enum);

/* Stereo1 ADC Source 1 */ /* MX004B [13][12][5][4] MX004A  MX004D MX004E [13][12] */
static const char * const rt5679_adc12_src[] = {
	"ADC/DMIC", "DD/DAC"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_mono_adc1_l_enum, RT5679_MONO_ADC_MIXER_CTRL1,
	RT5679_SEL_MONO_ADC_L1_SFT, rt5679_adc12_src);

static const struct snd_kcontrol_new rt5679_mono_adc1_l_mux =
	SOC_DAPM_ENUM("Mono ADC1 L Source", rt5679_mono_adc1_l_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_mono_adc2_l_enum, RT5679_MONO_ADC_MIXER_CTRL1,
	RT5679_SEL_MONO_ADC_L2_SFT, rt5679_adc12_src);

static const struct snd_kcontrol_new rt5679_mono_adc2_l_mux =
	SOC_DAPM_ENUM("Mono ADC2 L Source", rt5679_mono_adc2_l_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_mono_adc1_r_enum, RT5679_MONO_ADC_MIXER_CTRL1,
	RT5679_SEL_MONO_ADC_R1_SFT, rt5679_adc12_src);

static const struct snd_kcontrol_new rt5679_mono_adc1_r_mux =
	SOC_DAPM_ENUM("Mono ADC1 R Source", rt5679_mono_adc1_r_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_mono_adc2_r_enum, RT5679_MONO_ADC_MIXER_CTRL1,
	RT5679_SEL_MONO_ADC_R2_SFT, rt5679_adc12_src);

static const struct snd_kcontrol_new rt5679_mono_adc2_r_mux =
	SOC_DAPM_ENUM("Mono ADC2 R Source", rt5679_mono_adc2_r_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_stereo1_adc1_enum, RT5679_STO1_ADC_MIXER_CTRL,
	RT5679_SEL_STO1_ADC1_SFT, rt5679_adc12_src);

static const struct snd_kcontrol_new rt5679_sto1_adc1_mux =
	SOC_DAPM_ENUM("Stereo1 ADC1 Source", rt5679_stereo1_adc1_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_stereo1_adc2_enum, RT5679_STO1_ADC_MIXER_CTRL,
	RT5679_SEL_STO1_ADC2_SFT, rt5679_adc12_src);

static const struct snd_kcontrol_new rt5679_sto1_adc2_mux =
	SOC_DAPM_ENUM("Stereo1 ADC2 Source", rt5679_stereo1_adc2_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_stereo2_adc1_enum, RT5679_STO2_ADC_MIXER_CTRL,
	RT5679_SEL_STO2_ADC1_SFT, rt5679_adc12_src);

static const struct snd_kcontrol_new rt5679_sto2_adc1_mux =
	SOC_DAPM_ENUM("Stereo2 ADC1 Source", rt5679_stereo2_adc1_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_stereo2_adc2_enum, RT5679_STO2_ADC_MIXER_CTRL,
	RT5679_SEL_STO2_ADC2_SFT, rt5679_adc12_src);

static const struct snd_kcontrol_new rt5679_sto2_adc2_mux =
	SOC_DAPM_ENUM("Stereo2 ADC2 Source", rt5679_stereo2_adc2_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_stereo3_adc1_enum, RT5679_STO3_ADC_MIXER_CTRL,
	RT5679_SEL_STO3_ADC1_SFT, rt5679_adc12_src);

static const struct snd_kcontrol_new rt5679_sto3_adc1_mux =
	SOC_DAPM_ENUM("Stereo3 ADC1 Source", rt5679_stereo3_adc1_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_stereo3_adc2_enum, RT5679_STO3_ADC_MIXER_CTRL,
	RT5679_SEL_STO3_ADC2_SFT, rt5679_adc12_src);

static const struct snd_kcontrol_new rt5679_sto3_adc2_mux =
	SOC_DAPM_ENUM("Stereo3 ADC2 Source", rt5679_stereo3_adc2_enum);

/* InBound0/1 Source */ /* MX0500 [15:12] */
static const char * const rt5679_inbound01_src[] = {
	"IF1 DAC01", "IF2 DAC01", "SLB DAC01", "STO1 ADC MIX",
	"MONO ADC MIX", "VAD ADC", "DAC1 FS", "VAD ADC FILTER", "SPDIF IN"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_inbound01_enum, RT5679_DSP_IB_CTRL1,
	RT5679_IB01_SRC_SFT, rt5679_inbound01_src);

static const struct snd_kcontrol_new rt5679_ib01_src_mux =
	SOC_DAPM_ENUM("InBound0/1 Source", rt5679_inbound01_enum);

/* InBound0/1 Mini Source */ /* MX0501 [2:0] */
static const char * const rt5679_inbound01_mini_src[] = {
	"VAD ADC", "IF1 DAC01", "IF3 DAC", "SLB DAC01", "MONO ADC MIX",
	"STO1 ADC MIX", "VAD ADC FILTER", "DAC1 FS"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_inbound01_mini_enum, RT5679_DSP_IB_CTRL2,
	RT5679_IB01_MINI_SRC_SFT, rt5679_inbound01_mini_src);

static const struct snd_kcontrol_new rt5679_ib01_mini_src_mux =
	SOC_DAPM_ENUM("InBound0/1 Mini Source", rt5679_inbound01_mini_enum);

/* InBound2/3 Source */ /* MX0500 [10:8] */
static const char * const rt5679_inbound23_src[] = {
	"IF1 DAC23", "IF2 DAC23", "IF5 DAC", "SLB DAC23", "STO2 ADC MIX",
	"STO3 ADC MIX", "DAC1 FS", "MONO ADC MIX", "SPDIF IN"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_inbound23_enum, RT5679_DSP_IB_CTRL1,
	RT5679_IB23_SRC_SFT, rt5679_inbound23_src);

static const struct snd_kcontrol_new rt5679_ib23_src_mux =
	SOC_DAPM_ENUM("InBound2/3 Source", rt5679_inbound23_enum);

/* InBound4/5 Source */ /* MX0500 [6:4] */
static const char * const rt5679_inbound45_src[] = {
	"IF1 DAC45", "IF2 DAC45", "IF3 DAC", "IF4 DAC", "SLB DAC45",
	"STO1 ADC MIX", "MONO ADC MIX", "STO3 ADC MIX"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_inbound45_enum, RT5679_DSP_IB_CTRL1,
	RT5679_IB45_SRC_SFT, rt5679_inbound45_src);

static const struct snd_kcontrol_new rt5679_ib45_src_mux =
	SOC_DAPM_ENUM("InBound4/5 Source", rt5679_inbound45_enum);

/* InBound6 Source */ /* MX0500 [2:0] */
static const char * const rt5679_inbound6_src[] = {
	"IF1 DAC 6", "IF2 DAC 6", "IF4 DAC L", "IF5 DAC L", "SLB DAC 6",
	"MONO ADC MIX L", "STO2 ADC MIX L", "STO3 ADC MIX L"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_inbound6_enum, RT5679_DSP_IB_CTRL1,
	RT5679_IB6_SRC_SFT, rt5679_inbound6_src);

static const struct snd_kcontrol_new rt5679_ib6_src_mux =
	SOC_DAPM_ENUM("InBound6 Source", rt5679_inbound6_enum);

/* InBound7 Source */ /* MX0501 [14:12] */
static const char * const rt5679_inbound7_src[] = {
	"IF1 DAC 7", "IF2 DAC 7", "IF4 DAC R", "IF5 DAC R", "SLB DAC 7",
	"MONO ADC MIX R", "STO2 ADC MIX R", "STO3 ADC MIX R"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_inbound7_enum, RT5679_DSP_IB_CTRL2,
	RT5679_IB7_SRC_SFT, rt5679_inbound7_src);

static const struct snd_kcontrol_new rt5679_ib7_src_mux =
	SOC_DAPM_ENUM("InBound7 Source", rt5679_inbound7_enum);

/* VAD Source */ /* MX02C5 [6:4] */
static const char * const rt5679_vad_src[] = {
	"MONO ADC MIX L", "MONO ADC MIX R", "VAD ADC FILTER", "STO1 ADC MIX L",
	"STO2 ADC MIX L", "STO3 ADC MIX L"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_vad_enum, RT5679_VAD_FUNCTION_CTRL1,
	RT5679_VAD_SRC_SFT, rt5679_vad_src);

static const struct snd_kcontrol_new rt5679_vad_src_mux =
	SOC_DAPM_ENUM("VAD Source", rt5679_vad_enum);

/* VAD INBUF Source */ /* MX0009 [4] */
static const char * const rt5679_vad_inbuf_src[] = {
	"IN3P", "IN4P"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_vad_inbuf_enum, RT5679_VAD_INBUF_CTRL,
	RT5679_SEL_VIN_INBUF_SFT, rt5679_vad_inbuf_src);

static const struct snd_kcontrol_new rt5679_vad_inbuf_src_mux =
	SOC_DAPM_ENUM("VAD Inbuf Source", rt5679_vad_inbuf_enum);

/* VAD Filter Source */ /* MX0193 [0] */
static const char * const rt5679_vad_filter_src[] = {
	"ADC", "DMIC"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_vad_filter_enum, RT5679_VAD_ADC_FILTER_CTRL2,
	RT5679_AD_INPUT_SEL_SFT, rt5679_vad_filter_src);

static const struct snd_kcontrol_new rt5679_vad_filter_src_mux =
	SOC_DAPM_ENUM("VAD Filter Source", rt5679_vad_filter_enum);

/* Sidetone Source */ /* MX011A [11:9] */
static const char * const rt5679_sidetone_src[] = {
	"DMIC1L", "DMIC2L", "DMIC3L", "DMIC4L", "ADC1", "ADC2", "ADC3", "ADC4"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_sidetone_enum, RT5679_ST_CTRL,
	RT5679_ST_SEL_SFT, rt5679_sidetone_src);

static const struct snd_kcontrol_new rt5679_sidetone_mux =
	SOC_DAPM_ENUM("Sidetone Source", rt5679_sidetone_enum);

/* DAC1/2/3/4 Source */ /* MX0049 [15:14][13:12][11:10] */
static const char * const rt5679_dac1234_out_src[] = {
	"STO1 DAC MIX", "MONO DAC MIX", "DD MIX"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_dac12_enum, RT5679_DAC_SOURCE_CTRL,
	RT5679_SEL_DAC12_SOURCE_SFT, rt5679_dac1234_out_src);

static const struct snd_kcontrol_new rt5679_dac12_mux =
	SOC_DAPM_ENUM("Analog DAC1/2 Source", rt5679_dac12_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_dac3_enum, RT5679_DAC_SOURCE_CTRL,
	RT5679_SEL_DAC3_SOURCE_SFT, rt5679_dac1234_out_src);

static const struct snd_kcontrol_new rt5679_dac3_mux =
	SOC_DAPM_ENUM("Analog DAC3 Source", rt5679_dac3_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_dac4_enum, RT5679_DAC_SOURCE_CTRL,
	RT5679_SEL_DAC4_SOURCE_SFT, rt5679_dac1234_out_src);

static const struct snd_kcontrol_new rt5679_dac4_mux =
	SOC_DAPM_ENUM("Analog DAC4 Source", rt5679_dac4_enum);

/* DAC5 Source */ /* MX0049 [9:8] */
static const char * const rt5679_dac5_out_src[] = {
	"STO1 DAC MIXL", "MONO DAC MIXL", "MONO DAC MIXR"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_dac5_enum, RT5679_DAC_SOURCE_CTRL,
	RT5679_SEL_DAC5_SOURCE_SFT, rt5679_dac5_out_src);

static const struct snd_kcontrol_new rt5679_dac5_mux =
	SOC_DAPM_ENUM("Analog DAC5 Source", rt5679_dac5_enum);

/* PDM channel Source */ /* MX0100 [13:12][11:0][5:4][3:2] */
static const char * const rt5679_pdm_src[] = {
	"STO1 DAC MIX", "MONO DAC MIX", "DD MIX"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_pdm1_l_enum, RT5679_PDM_OUTPUT_CTRL,
	RT5679_SEL_PDM1_L_SFT, rt5679_pdm_src);

static const struct snd_kcontrol_new rt5679_pdm1_l_mux =
	SOC_DAPM_ENUM("PDM1 Source", rt5679_pdm1_l_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_pdm2_l_enum, RT5679_PDM_OUTPUT_CTRL,
	RT5679_SEL_PDM2_L_SFT, rt5679_pdm_src);

static const struct snd_kcontrol_new rt5679_pdm2_l_mux =
	SOC_DAPM_ENUM("PDM2 Source", rt5679_pdm2_l_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_pdm1_r_enum, RT5679_PDM_OUTPUT_CTRL,
	RT5679_SEL_PDM1_R_SFT, rt5679_pdm_src);

static const struct snd_kcontrol_new rt5679_pdm1_r_mux =
	SOC_DAPM_ENUM("PDM1 Source", rt5679_pdm1_r_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_pdm2_r_enum, RT5679_PDM_OUTPUT_CTRL,
	RT5679_SEL_PDM2_R_SFT, rt5679_pdm_src);

static const struct snd_kcontrol_new rt5679_pdm2_r_mux =
	SOC_DAPM_ENUM("PDM2 Source", rt5679_pdm2_r_enum);

/* TDM IF1/2 ADC1 Data Selection */ /* MX0032 MX003A [10:8] */
static const char * const rt5679_if12_adc1_src[] = {
	"STO1 ADC MIX", "OB01", "OB01 MINI", "VAD ADC", "IF1/2 DAC01",
	"IF3 DAC", "IF4 DAC", "IF5 DAC"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if1_adc1_enum, RT5679_TDM1_CTRL3,
	RT5679_SEL_I2S1_RX_ADC1_SFT, rt5679_if12_adc1_src);

static const struct snd_kcontrol_new rt5679_if1_adc1_mux =
	SOC_DAPM_ENUM("IF1 ADC1 Source", rt5679_if1_adc1_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if2_adc1_enum, RT5679_TDM2_CTRL3,
	RT5679_SEL_I2S2_RX_ADC1_SFT, rt5679_if12_adc1_src);

static const struct snd_kcontrol_new rt5679_if2_adc1_mux =
	SOC_DAPM_ENUM("IF2 ADC1 Source", rt5679_if2_adc1_enum);

/* SLB ADC1 Data Selection */ /* MX0701 [1:0] */
static const char * const rt5679_slb_adc1_src[] = {
	"STO1 ADC MIX", "OB01", "OB01 MINI", "VAD ADC"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_slb_adc1_enum, RT5679_SLIMBUS_RX,
	RT5679_SLB_ADC1_SFT, rt5679_slb_adc1_src);

static const struct snd_kcontrol_new rt5679_slb_adc1_mux =
	SOC_DAPM_ENUM("SLB ADC1 Source", rt5679_slb_adc1_enum);

/* TDM IF1/2 SLB ADC2 Data Selection */ /* MX0032 MX003A [11] MX0701 [2] */
static const char * const rt5679_if12_adc2_src[] = {
	"STO2 ADC MIX", "OB23"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if1_adc2_enum, RT5679_TDM1_CTRL3,
	RT5679_SEL_I2S1_RX_ADC2_SFT, rt5679_if12_adc2_src);

static const struct snd_kcontrol_new rt5679_if1_adc2_mux =
	SOC_DAPM_ENUM("IF1 ADC2 Source", rt5679_if1_adc2_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if2_adc2_enum, RT5679_TDM2_CTRL3,
	RT5679_SEL_I2S2_RX_ADC2_SFT, rt5679_if12_adc2_src);

static const struct snd_kcontrol_new rt5679_if2_adc2_mux =
	SOC_DAPM_ENUM("IF2 ADC2 Source", rt5679_if2_adc2_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_slb_adc2_enum, RT5679_SLIMBUS_RX,
	RT5679_SLB_ADC2_SFT, rt5679_if12_adc2_src);

static const struct snd_kcontrol_new rt5679_slb_adc2_mux =
	SOC_DAPM_ENUM("SLB ADC2 Source", rt5679_slb_adc2_enum);

/* TDM IF1/2 SLB ADC3 Data Selection */ /* MX0032 MX003A [13:12] MX0701 [5:4] */
static const char * const rt5679_if12_adc3_src[] = {
	"MONO ADC MIX", "OB45", "DAC1 FS", "VAD ADC FILTER"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if1_adc3_enum, RT5679_TDM1_CTRL3,
	RT5679_SEL_I2S1_RX_ADC3_SFT, rt5679_if12_adc3_src);

static const struct snd_kcontrol_new rt5679_if1_adc3_mux =
	SOC_DAPM_ENUM("IF1 ADC3 Source", rt5679_if1_adc3_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if2_adc3_enum, RT5679_TDM2_CTRL3,
	RT5679_SEL_I2S2_RX_ADC3_SFT, rt5679_if12_adc3_src);

static const struct snd_kcontrol_new rt5679_if2_adc3_mux =
	SOC_DAPM_ENUM("IF2 ADC3 Source", rt5679_if2_adc3_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_slb_adc3_enum, RT5679_SLIMBUS_RX,
	RT5679_SLB_ADC3_SFT, rt5679_if12_adc3_src);

static const struct snd_kcontrol_new rt5679_slb_adc3_mux =
	SOC_DAPM_ENUM("SLB ADC3 Source", rt5679_slb_adc3_enum);

/* TDM IF1/2 SLB ADC4 Data Selection */ /* MX0032 MX003A [15:14] MX0701 [7:6] */
static const char * const rt5679_if12_adc4_src[] = {
	"STO3 ADC MIX", "DAC1 FS", "OB67"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if1_adc4_enum, RT5679_TDM1_CTRL3,
	RT5679_SEL_I2S1_RX_ADC4_SFT, rt5679_if12_adc4_src);

static const struct snd_kcontrol_new rt5679_if1_adc4_mux =
	SOC_DAPM_ENUM("IF1 ADC4 Source", rt5679_if1_adc4_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if2_adc4_enum, RT5679_TDM2_CTRL3,
	RT5679_SEL_I2S2_RX_ADC4_SFT, rt5679_if12_adc4_src);

static const struct snd_kcontrol_new rt5679_if2_adc4_mux =
	SOC_DAPM_ENUM("IF2 ADC4 Source", rt5679_if2_adc4_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_slb_adc4_enum, RT5679_SLIMBUS_RX,
	RT5679_SLB_ADC4_SFT, rt5679_if12_adc4_src);

static const struct snd_kcontrol_new rt5679_slb_adc4_mux =
	SOC_DAPM_ENUM("SLB ADC4 Source", rt5679_slb_adc4_enum);

/* Interface3/4/5 ADC Data Input */ /* MX002A MX002B MX002C [3:0] */
static const char * const rt5679_if345_adc_src[] = {
	"STO1 ADC MIX", "STO2 ADC MIX", "STO3 ADC MIX", "MONO ADC MIX", "OB01",
	"OB23", "OB45", "OB01 MINI", "VAD ADC", "IF1 DAC01", "IF2 DAC01",
	"IF3/4 DAC", "IF4/5 DAC", "OB67"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if3_adc_enum, RT5679_IF3_DATA_CTRL,
	RT5679_IF3_ADC_IN_SFT, rt5679_if345_adc_src);

static const struct snd_kcontrol_new rt5679_if3_adc_mux =
	SOC_DAPM_ENUM("IF3 ADC Source", rt5679_if3_adc_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if4_adc_enum, RT5679_IF4_DATA_CTRL,
	RT5679_IF4_ADC_IN_SFT, rt5679_if345_adc_src);

static const struct snd_kcontrol_new rt5679_if4_adc_mux =
	SOC_DAPM_ENUM("IF4 ADC Source", rt5679_if4_adc_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if5_adc_enum, RT5679_IF5_DATA_CTRL,
	RT5679_IF5_ADC_IN_SFT, rt5679_if345_adc_src);

static const struct snd_kcontrol_new rt5679_if5_adc_mux =
	SOC_DAPM_ENUM("IF5 ADC Source", rt5679_if5_adc_enum);

/* TDM IF1/2 ADC Data Selection */ /* MX0031 MX0039 [15:14][13:12][11:10][9:8]
*/
/* IF3/4/5 DAC/ADC Data Selection */ /* MX002A MX002B MX002C [7:6][5:4] */
static const char * const rt5679_if12_adc_swap_src[] = {
	"L/R", "R/L", "L/L", "R/R"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if1_adc1_swap_enum, RT5679_TDM1_CTRL2,
	RT5679_IF1_ADC1_SWAP_SFT, rt5679_if12_adc_swap_src);

static const struct snd_kcontrol_new rt5679_if1_adc1_swap_mux =
	SOC_DAPM_ENUM("IF1 ADC1 Swap Source", rt5679_if1_adc1_swap_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if1_adc2_swap_enum, RT5679_TDM1_CTRL2,
	RT5679_IF1_ADC2_SWAP_SFT, rt5679_if12_adc_swap_src);

static const struct snd_kcontrol_new rt5679_if1_adc2_swap_mux =
	SOC_DAPM_ENUM("IF1 ADC2 Swap Source", rt5679_if1_adc2_swap_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if1_adc3_swap_enum, RT5679_TDM1_CTRL2,
	RT5679_IF1_ADC3_SWAP_SFT, rt5679_if12_adc_swap_src);

static const struct snd_kcontrol_new rt5679_if1_adc3_swap_mux =
	SOC_DAPM_ENUM("IF1 ADC3 Swap Source", rt5679_if1_adc3_swap_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if1_adc4_swap_enum, RT5679_TDM1_CTRL2,
	RT5679_IF1_ADC4_SWAP_SFT, rt5679_if12_adc_swap_src);

static const struct snd_kcontrol_new rt5679_if1_adc4_swap_mux =
	SOC_DAPM_ENUM("IF1 ADC4 Swap Source", rt5679_if1_adc4_swap_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if2_adc1_swap_enum, RT5679_TDM2_CTRL2,
	RT5679_IF2_ADC1_SWAP_SFT, rt5679_if12_adc_swap_src);

static const struct snd_kcontrol_new rt5679_if2_adc1_swap_mux =
	SOC_DAPM_ENUM("IF2 ADC1 Swap Source", rt5679_if2_adc1_swap_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if2_adc2_swap_enum, RT5679_TDM2_CTRL2,
	RT5679_IF2_ADC2_SWAP_SFT, rt5679_if12_adc_swap_src);

static const struct snd_kcontrol_new rt5679_if2_adc2_swap_mux =
	SOC_DAPM_ENUM("IF2 ADC2 Swap Source", rt5679_if2_adc2_swap_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if2_adc3_swap_enum, RT5679_TDM2_CTRL2,
	RT5679_IF2_ADC3_SWAP_SFT, rt5679_if12_adc_swap_src);

static const struct snd_kcontrol_new rt5679_if2_adc3_swap_mux =
	SOC_DAPM_ENUM("IF2 ADC3 Swap Source", rt5679_if2_adc3_swap_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if2_adc4_swap_enum, RT5679_TDM2_CTRL2,
	RT5679_IF2_ADC4_SWAP_SFT, rt5679_if12_adc_swap_src);

static const struct snd_kcontrol_new rt5679_if2_adc4_swap_mux =
	SOC_DAPM_ENUM("IF2 ADC4 Swap Source", rt5679_if2_adc4_swap_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if3_dac_swap_enum, RT5679_IF3_DATA_CTRL,
	RT5679_IF3_DAC_SEL_SFT, rt5679_if12_adc_swap_src);

static const struct snd_kcontrol_new rt5679_if3_dac_swap_mux =
	SOC_DAPM_ENUM("IF3 DAC Swap Source", rt5679_if3_dac_swap_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if3_adc_swap_enum, RT5679_IF3_DATA_CTRL,
	RT5679_IF3_ADC_SEL_SFT, rt5679_if12_adc_swap_src);

static const struct snd_kcontrol_new rt5679_if3_adc_swap_mux =
	SOC_DAPM_ENUM("IF3 ADC Swap Source", rt5679_if3_adc_swap_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if4_dac_swap_enum, RT5679_IF4_DATA_CTRL,
	RT5679_IF4_DAC_SEL_SFT, rt5679_if12_adc_swap_src);

static const struct snd_kcontrol_new rt5679_if4_dac_swap_mux =
	SOC_DAPM_ENUM("IF4 DAC Swap Source", rt5679_if4_dac_swap_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if4_adc_swap_enum, RT5679_IF4_DATA_CTRL,
	RT5679_IF4_ADC_SEL_SFT, rt5679_if12_adc_swap_src);

static const struct snd_kcontrol_new rt5679_if4_adc_swap_mux =
	SOC_DAPM_ENUM("IF4 ADC Swap Source", rt5679_if4_adc_swap_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if5_dac_swap_enum, RT5679_IF5_DATA_CTRL,
	RT5679_IF5_DAC_SEL_SFT, rt5679_if12_adc_swap_src);

static const struct snd_kcontrol_new rt5679_if5_dac_swap_mux =
	SOC_DAPM_ENUM("IF5 DAC Swap Source", rt5679_if5_dac_swap_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if5_adc_swap_enum, RT5679_IF4_DATA_CTRL,
	RT5679_IF5_ADC_SEL_SFT, rt5679_if12_adc_swap_src);

static const struct snd_kcontrol_new rt5679_if5_adc_swap_mux =
	SOC_DAPM_ENUM("IF5 ADC Swap Source", rt5679_if5_adc_swap_enum);

/* TDM IF1/2 ADC Data Selection */ /* MX0032 MX003A [4:0] */
static const char * const rt5679_if12_adc_tdm_swap_src[] = {
	"1/2/3/4", "1/2/4/3", "1/3/2/4", "1/3/4/2", "1/4/3/2", "1/4/2/3",
	"2/1/3/4", "2/1/4/3", "2/3/1/4", "2/3/4/1", "2/4/3/1", "2/4/1/3",
	"3/1/2/4", "3/1/4/2", "3/2/1/4", "3/2/4/1", "3/4/2/1", "3/4/1/2",
	"4/1/2/3", "4/1/3/2", "4/2/1/3", "4/2/3/1", "4/3/2/1", "4/3/1/2",
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if1_adc_tdm_swap_enum, RT5679_TDM1_CTRL3,
	RT5679_SEL_RX_I2S1_SFT, rt5679_if12_adc_tdm_swap_src);

static const struct snd_kcontrol_new rt5679_if1_adc_tdm_swap_mux =
	SOC_DAPM_ENUM("IF1 ADC TDM Swap Source", rt5679_if1_adc_tdm_swap_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if2_adc_tdm_swap_enum, RT5679_TDM2_CTRL3,
	RT5679_SEL_RX_I2S2_SFT, rt5679_if12_adc_tdm_swap_src);

static const struct snd_kcontrol_new rt5679_if2_adc_tdm_swap_mux =
	SOC_DAPM_ENUM("IF2 ADC TDM Swap Source", rt5679_if2_adc_tdm_swap_enum);

/* TDM IF1/2 DAC Data Selection */ /* MX0034 [14:12][10:8][6:4][2:0]
					MX0035 [14:12][10:8][6:4][2:0]
					MX003C [14:12][10:8][6:4][2:0]
					MX003D [14:12][10:8][6:4][2:0] */
static const char * const rt5679_if12_dac_tdm_sel_src[] = {
	"Slot0", "Slot1", "Slot2", "Slot3", "Slot4", "Slot5", "Slot6", "Slot7"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if1_dac0_tdm_sel_enum, RT5679_TDM1_CTRL5,
	RT5679_IF1_DAC0_SFT, rt5679_if12_dac_tdm_sel_src);

static const struct snd_kcontrol_new rt5679_if1_dac0_tdm_sel_mux =
	SOC_DAPM_ENUM("IF1 DAC0 TDM Source", rt5679_if1_dac0_tdm_sel_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if1_dac1_tdm_sel_enum, RT5679_TDM1_CTRL5,
	RT5679_IF1_DAC1_SFT, rt5679_if12_dac_tdm_sel_src);

static const struct snd_kcontrol_new rt5679_if1_dac1_tdm_sel_mux =
	SOC_DAPM_ENUM("IF1 DAC1 TDM Source", rt5679_if1_dac1_tdm_sel_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if1_dac2_tdm_sel_enum, RT5679_TDM1_CTRL5,
	RT5679_IF1_DAC2_SFT, rt5679_if12_dac_tdm_sel_src);

static const struct snd_kcontrol_new rt5679_if1_dac2_tdm_sel_mux =
	SOC_DAPM_ENUM("IF1 DAC2 TDM Source", rt5679_if1_dac2_tdm_sel_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if1_dac3_tdm_sel_enum, RT5679_TDM1_CTRL5,
	RT5679_IF1_DAC3_SFT, rt5679_if12_dac_tdm_sel_src);

static const struct snd_kcontrol_new rt5679_if1_dac3_tdm_sel_mux =
	SOC_DAPM_ENUM("IF1 DAC3 TDM Source", rt5679_if1_dac3_tdm_sel_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if1_dac4_tdm_sel_enum, RT5679_TDM1_CTRL6,
	RT5679_IF1_DAC4_SFT, rt5679_if12_dac_tdm_sel_src);

static const struct snd_kcontrol_new rt5679_if1_dac4_tdm_sel_mux =
	SOC_DAPM_ENUM("IF1 DAC4 TDM Source", rt5679_if1_dac4_tdm_sel_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if1_dac5_tdm_sel_enum, RT5679_TDM1_CTRL6,
	RT5679_IF1_DAC5_SFT, rt5679_if12_dac_tdm_sel_src);

static const struct snd_kcontrol_new rt5679_if1_dac5_tdm_sel_mux =
	SOC_DAPM_ENUM("IF1 DAC5 TDM Source", rt5679_if1_dac5_tdm_sel_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if1_dac6_tdm_sel_enum, RT5679_TDM1_CTRL6,
	RT5679_IF1_DAC6_SFT, rt5679_if12_dac_tdm_sel_src);

static const struct snd_kcontrol_new rt5679_if1_dac6_tdm_sel_mux =
	SOC_DAPM_ENUM("IF1 DAC6 TDM Source", rt5679_if1_dac6_tdm_sel_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if1_dac7_tdm_sel_enum, RT5679_TDM1_CTRL6,
	RT5679_IF1_DAC7_SFT, rt5679_if12_dac_tdm_sel_src);

static const struct snd_kcontrol_new rt5679_if1_dac7_tdm_sel_mux =
	SOC_DAPM_ENUM("IF1 DAC7 TDM Source", rt5679_if1_dac7_tdm_sel_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if2_dac0_tdm_sel_enum, RT5679_TDM2_CTRL5,
	RT5679_IF2_DAC0_SFT, rt5679_if12_dac_tdm_sel_src);

static const struct snd_kcontrol_new rt5679_if2_dac0_tdm_sel_mux =
	SOC_DAPM_ENUM("IF2 DAC0 TDM Source", rt5679_if2_dac0_tdm_sel_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if2_dac1_tdm_sel_enum, RT5679_TDM2_CTRL5,
	RT5679_IF2_DAC1_SFT, rt5679_if12_dac_tdm_sel_src);

static const struct snd_kcontrol_new rt5679_if2_dac1_tdm_sel_mux =
	SOC_DAPM_ENUM("IF2 DAC1 TDM Source", rt5679_if2_dac1_tdm_sel_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if2_dac2_tdm_sel_enum, RT5679_TDM2_CTRL5,
	RT5679_IF2_DAC2_SFT, rt5679_if12_dac_tdm_sel_src);

static const struct snd_kcontrol_new rt5679_if2_dac2_tdm_sel_mux =
	SOC_DAPM_ENUM("IF2 DAC2 TDM Source", rt5679_if2_dac2_tdm_sel_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if2_dac3_tdm_sel_enum, RT5679_TDM2_CTRL5,
	RT5679_IF2_DAC3_SFT, rt5679_if12_dac_tdm_sel_src);

static const struct snd_kcontrol_new rt5679_if2_dac3_tdm_sel_mux =
	SOC_DAPM_ENUM("IF2 DAC3 TDM Source", rt5679_if2_dac3_tdm_sel_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if2_dac4_tdm_sel_enum, RT5679_TDM2_CTRL6,
	RT5679_IF2_DAC4_SFT, rt5679_if12_dac_tdm_sel_src);

static const struct snd_kcontrol_new rt5679_if2_dac4_tdm_sel_mux =
	SOC_DAPM_ENUM("IF2 DAC4 TDM Source", rt5679_if2_dac4_tdm_sel_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if2_dac5_tdm_sel_enum, RT5679_TDM2_CTRL6,
	RT5679_IF2_DAC5_SFT, rt5679_if12_dac_tdm_sel_src);

static const struct snd_kcontrol_new rt5679_if2_dac5_tdm_sel_mux =
	SOC_DAPM_ENUM("IF2 DAC5 TDM Source", rt5679_if2_dac5_tdm_sel_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if2_dac6_tdm_sel_enum, RT5679_TDM2_CTRL6,
	RT5679_IF2_DAC6_SFT, rt5679_if12_dac_tdm_sel_src);

static const struct snd_kcontrol_new rt5679_if2_dac6_tdm_sel_mux =
	SOC_DAPM_ENUM("IF2 DAC6 TDM Source", rt5679_if2_dac6_tdm_sel_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_if2_dac7_tdm_sel_enum, RT5679_TDM2_CTRL6,
	RT5679_IF2_DAC7_SFT, rt5679_if12_dac_tdm_sel_src);

static const struct snd_kcontrol_new rt5679_if2_dac7_tdm_sel_mux =
	SOC_DAPM_ENUM("IF2 DAC7 TDM Source", rt5679_if2_dac7_tdm_sel_enum);

/* DAC1 Mixer Source */ /* MX0040 [13][12] */
static const char * const rt5679_dac_mixer_src[] = {
	"DAC1", "Mixer"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5679_dac1l_mixer_enum, RT5679_STO1_DAC_MIXER_CTRL1,
	RT5679_SEL_DAC_L1_MIXER_SFT, rt5679_dac_mixer_src);

static const struct snd_kcontrol_new rt5679_dac1l_mixer_mux =
	SOC_DAPM_ENUM("DAC1 L Mixer Source", rt5679_dac1l_mixer_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5679_dac1r_mixer_enum, RT5679_STO1_DAC_MIXER_CTRL1,
	RT5679_SEL_DAC_R1_MIXER_SFT, rt5679_dac_mixer_src);

static const struct snd_kcontrol_new rt5679_dac1r_mixer_mux =
	SOC_DAPM_ENUM("DAC1 R Mixer Source", rt5679_dac1r_mixer_enum);

static int rt5679_vref_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(rt5679->regmap, RT5679_PWR_ANA1,
			RT5679_PWR_VREF1 | RT5679_PWR_FV1 | RT5679_PWR_MB |
			RT5679_PWR_VREF2 | RT5679_PWR_FV2,
			RT5679_PWR_VREF1 | RT5679_PWR_MB | RT5679_PWR_VREF2);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_LDO1,
			RT5679_PWR_VREF5_L | RT5679_PWR_FV5_L |
			RT5679_PWR_BG_VREF5_L | RT5679_PWR_VREF5_R |
			RT5679_PWR_FV5_R | RT5679_PWR_BG_VREF5_R,
			RT5679_PWR_VREF5_L | RT5679_PWR_BG_VREF5_L |
			RT5679_PWR_VREF5_R | RT5679_PWR_BG_VREF5_R);
		msleep(20);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_ANA1,
			RT5679_PWR_FV1 | RT5679_PWR_FV2, RT5679_PWR_FV1 |
			RT5679_PWR_FV2);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_LDO1,
			RT5679_PWR_FV5_L | RT5679_PWR_FV5_R,
			RT5679_PWR_FV5_L | RT5679_PWR_FV5_R);
		break;

	case SND_SOC_DAPM_POST_PMD:
		if (rt5679->jack_type != SND_JACK_HEADSET)
			regmap_update_bits(rt5679->regmap, RT5679_PWR_ANA1,
				RT5679_PWR_VREF1 | RT5679_PWR_FV1 |
				RT5679_PWR_MB | RT5679_PWR_VREF2 |
				RT5679_PWR_FV2, 0);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_LDO1,
			RT5679_PWR_VREF5_L | RT5679_PWR_FV5_L |
			RT5679_PWR_BG_VREF5_L | RT5679_PWR_VREF5_R |
			RT5679_PWR_FV5_R | RT5679_PWR_BG_VREF5_R, 0);
		break;

	default:
		return 0;
	}

	return 0;
}

static int rt5679_mic_det_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(rt5679->regmap, RT5679_PWR_ANA1,
			RT5679_PWR_VREF1 | RT5679_PWR_MB | RT5679_PWR_VREF2,
			RT5679_PWR_VREF1 | RT5679_PWR_MB | RT5679_PWR_VREF2);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_ANA2,
			RT5679_PWR_VREF3 | RT5679_PWR_MB1,
			RT5679_PWR_VREF3 | RT5679_PWR_MB1);

		break;

	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(rt5679->regmap, RT5679_PWR_ANA1,
			RT5679_PWR_VREF1 | RT5679_PWR_MB | RT5679_PWR_VREF2, 0);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_ANA2,
			RT5679_PWR_VREF3 | RT5679_PWR_MB1, 0);
		break;

	default:
		return 0;
	}

	return 0;
}

static int rt5679_set_pll1_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(rt5679->regmap, RT5679_PLL1_CTRL2,
			RT5679_UPDATE_PLL, RT5679_UPDATE_PLL);
		break;

	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(rt5679->regmap, RT5679_PLL1_CTRL2,
			RT5679_UPDATE_PLL, 0);
		break;

	default:
		return 0;
	}

	return 0;
}

static int rt5679_set_pll2_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(rt5679->regmap, RT5679_PLL2_CTRL2,
			RT5679_UPDATE_PLL, RT5679_UPDATE_PLL);
		break;

	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(rt5679->regmap, RT5679_PLL2_CTRL2,
			RT5679_UPDATE_PLL, 0);
		break;

	default:
		return 0;
	}

	return 0;
}

static int rt5679_micbias1_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(rt5679->regmap, RT5679_PWR_ANA2,
			RT5679_PWR_VREF3, RT5679_PWR_VREF3);
		msleep(20);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_ANA2,
			RT5679_PWR_FV3, RT5679_PWR_FV3);
		break;

	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(rt5679->regmap, RT5679_PWR_ANA2,
			RT5679_PWR_VREF3 | RT5679_PWR_FV3, 0);
		break;

	default:
		return 0;
	}

	return 0;
}

static int rt5679_hp_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(rt5679->regmap, RT5679_HP_BL_CTRL2, 0x3,
			0x3);
		break;

	default:
		return 0;
	}

	return 0;
}

static int rt5679_lout_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(rt5679->regmap, RT5679_LOUT_CTRL, 0x200,
			0x200);
		break;

	default:
		return 0;
	}

	return 0;
}

static int rt5679_mono_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(rt5679->regmap, RT5679_MONO_OUT, 0x1000,
			0x1000);
		break;

	default:
		return 0;
	}

	return 0;
}

static int rt5679_dsp_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);
	const struct firmware *fw;
	int ret;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		// 24.576Mhz to 150Mhz
		regmap_write(rt5679->regmap, RT5679_PLL2_CTRL1, 0x0ee0);
		regmap_write(rt5679->regmap, RT5679_PLL2_CTRL2, 0x8002);
		regmap_write(rt5679->regmap, RT5679_PLL2_CTRL2, 0x8000);
		regmap_update_bits(rt5679->regmap, RT5679_LDO8_LDO9_PR_CTRL,
			0x0010, 0);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_LDO1,
			RT5679_PWR_LDO3_ON, RT5679_PWR_LDO3_ON);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_LDO2,
			RT5679_LDO_PLL2 | RT5679_PWR_LDO9,
			RT5679_LDO_PLL2 | RT5679_PWR_LDO9);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_ANA1,
			RT5679_RST_PLL2 | RT5679_PWR_PLL2,
			RT5679_RST_PLL2 | RT5679_PWR_PLL2);
		regmap_write(rt5679->regmap, RT5679_DSP_CLK_SOURCE1, 0x0111);
		regmap_write(rt5679->regmap, RT5679_DSP_CLK_SOURCE2, 0x0555);
		regmap_update_bits(rt5679->regmap, RT5679_GLB_CLK2, 0x1, 0x1);
		regmap_write(rt5679->regmap, RT5679_PWR_LDO3, 0x7707);
		regmap_update_bits(rt5679->regmap, RT5679_HIFI_MINI_DSP_CTRL_ST,
			0x30, 0x30);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_DSP,
			RT5679_PWR_DCVDD9_ISO | RT5679_PWR_DCVDD3_ISO |
			RT5679_PWR_SRAM | RT5679_PWR_MINI_DSP |
			RT5679_PWR_EP_DSP, RT5679_PWR_DCVDD9_ISO |
			RT5679_PWR_DCVDD3_ISO | RT5679_PWR_SRAM |
			RT5679_PWR_MINI_DSP | RT5679_PWR_EP_DSP);
		rt5679->is_dsp_mode = true;

		ret = request_firmware(&fw, "ALC5679_50000000", codec->dev);
		if (ret == 0) {
			rt5679_spi_burst_write(0x50000000, fw->data, fw->size);
			release_firmware(fw);
		}

		ret = request_firmware(&fw, "ALC5679_60000000", codec->dev);
		if (ret == 0) {
			rt5679_spi_burst_write(0x60000000, fw->data, fw->size);
			release_firmware(fw);
		}

		regmap_update_bits(rt5679->regmap, RT5679_PWR_DSP, 0x1, 0x0);
		break;

	case SND_SOC_DAPM_PRE_PMD:
		regmap_update_bits(rt5679->regmap, RT5679_PWR_DSP, 0x1, 0x1);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_DSP,
			RT5679_PWR_DCVDD9_ISO | RT5679_PWR_DCVDD3_ISO |
			RT5679_PWR_SRAM | RT5679_PWR_MINI_DSP |
			RT5679_PWR_EP_DSP, 0);
		rt5679->is_dsp_mode = false;
		regmap_update_bits(rt5679->regmap, RT5679_HIFI_MINI_DSP_CTRL_ST,
			0x30, 0);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_ANA1,
			RT5679_RST_PLL2 | RT5679_PWR_PLL2, 0);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_LDO2,
			RT5679_LDO_PLL2 | RT5679_PWR_LDO9, 0);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_LDO1,
			RT5679_PWR_LDO3_ON, 0);
		break;

	default:
		return 0;
	}
	return 0;
}

static const struct snd_soc_dapm_widget rt5679_dapm_widgets[] = {
	SND_SOC_DAPM_SUPPLY("Vref", SND_SOC_NOPM, 0, 0, rt5679_vref_event,
		SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_SUPPLY("PLL1 LDO", RT5679_PWR_LDO2, RT5679_LDO_PLL1_BIT,
		0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("PLL1 RST", RT5679_PWR_ANA1, RT5679_RST_PLL1_BIT,
		0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("PLL1", RT5679_PWR_ANA1, RT5679_PWR_PLL1_BIT,
		0, rt5679_set_pll1_event, SND_SOC_DAPM_PRE_PMU |
		SND_SOC_DAPM_POST_PMU),
	SND_SOC_DAPM_SUPPLY("PLL2", RT5679_PWR_ANA1, RT5679_PWR_PLL2_BIT,
		0, rt5679_set_pll2_event, SND_SOC_DAPM_PRE_PMU |
		SND_SOC_DAPM_POST_PMU),
	SND_SOC_DAPM_SUPPLY("Mic Det Power", RT5679_PWR_ANA2,
		RT5679_PWR_MIC_DET_BIT, 0, rt5679_mic_det_event,
		SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	/* ASRC */
	SND_SOC_DAPM_SUPPLY_S("I2S1 ASRC", 1, RT5679_ASRC1,
		RT5679_EN_I2S1_ASRC_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("I2S2 ASRC", 1, RT5679_ASRC1,
		RT5679_EN_I2S2_ASRC_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("I2S3 ASRC", 1, RT5679_ASRC1,
		RT5679_EN_I2S3_ASRC_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("I2S4 ASRC", 1, RT5679_ASRC1,
		RT5679_EN_I2S4_ASRC_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("I2S5 ASRC", 1, RT5679_ASRC1,
		RT5679_EN_I2S5_ASRC_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("DAC STO ASRC", 1, RT5679_ASRC2,
		RT5679_EN_STO_DAC_ASRC_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("DAC MONO2 L ASRC", 1, RT5679_ASRC2,
		RT5679_EN_MONO_DAC_L_ASRC_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("DAC MONO2 R ASRC", 1, RT5679_ASRC2,
		RT5679_EN_MONO_DAC_R_ASRC_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("DAC MONO3 L ASRC", 1, RT5679_ASRC1,
		RT5679_EN_MONO_DA_3L_ASRC_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("DAC MONO3 R ASRC", 1, RT5679_ASRC1,
		RT5679_EN_MONO_DA_3R_ASRC_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("DMIC STO1 ASRC", 1, RT5679_ASRC2,
		RT5679_EN_DMIC_ASRC_STO1_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("DMIC STO2 ASRC", 1, RT5679_ASRC2,
		RT5679_EN_DMIC_ASRC_STO2_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("DMIC STO3 ASRC", 1, RT5679_ASRC2,
		RT5679_EN_DMIC_ASRC_STO3_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("DMIC MONO L ASRC", 1, RT5679_ASRC2,
		RT5679_EN_DMIC_ASRC_MONOL_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("DMIC MONO R ASRC", 1, RT5679_ASRC2,
		RT5679_EN_DMIC_ASRC_MONOR_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("ADC STO1 ASRC", 1, RT5679_ASRC2,
		RT5679_EN_ADC_ASRC_STO1_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("ADC STO2 ASRC", 1, RT5679_ASRC2,
		RT5679_EN_ADC_ASRC_STO2_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("ADC STO3 ASRC", 1, RT5679_ASRC2,
		RT5679_EN_ADC_ASRC_STO3_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("ADC MONO L ASRC", 1, RT5679_ASRC2,
		RT5679_EN_ADC_ASRC_MONOL_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("ADC MONO R ASRC", 1, RT5679_ASRC2,
		RT5679_EN_ADC_ASRC_MONOR_BIT, 0, NULL, 0),

	/* Input Side */
	/* micbias */
	SND_SOC_DAPM_SUPPLY("MICBIAS1", RT5679_PWR_ANA2, RT5679_PWR_MB1_BIT, 0,
		rt5679_micbias1_event, SND_SOC_DAPM_PRE_PMU |
		SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_SUPPLY("MICBIAS2", RT5679_PWR_LDO1, RT5679_PWR_LDO4_1_BIT,
		0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("MICBIAS3", RT5679_PWR_LDO1, RT5679_PWR_LDO4_2_BIT,
		0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("MICBIAS4", RT5679_PWR_LDO1, RT5679_PWR_LDO4_3_BIT,
		0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("LDO4", RT5679_PWR_LDO1, RT5679_PWR_BG_LDO4_BIT,
		0, NULL, 0),

	/* Input Lines */
	SND_SOC_DAPM_INPUT("DMIC1L"),
	SND_SOC_DAPM_INPUT("DMIC1R"),
	SND_SOC_DAPM_INPUT("DMIC2L"),
	SND_SOC_DAPM_INPUT("DMIC2R"),
	SND_SOC_DAPM_INPUT("DMIC3L"),
	SND_SOC_DAPM_INPUT("DMIC3R"),
	SND_SOC_DAPM_INPUT("DMIC4L"),
	SND_SOC_DAPM_INPUT("DMIC4R"),

	SND_SOC_DAPM_INPUT("IN1P"),
	SND_SOC_DAPM_INPUT("IN1N"),
	SND_SOC_DAPM_INPUT("IN2P"),
	SND_SOC_DAPM_INPUT("IN2N"),
	SND_SOC_DAPM_INPUT("IN3P"),
	SND_SOC_DAPM_INPUT("IN3N"),
	SND_SOC_DAPM_INPUT("IN4P"),
	SND_SOC_DAPM_INPUT("IN4N"),
	SND_SOC_DAPM_INPUT("HAPTIC GEN"),
	SND_SOC_DAPM_INPUT("SPDIF IN"),

	SND_SOC_DAPM_SUPPLY("DMIC1 Power", RT5679_DMIC_CTRL1,
		RT5679_DMIC_1_EN_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("DMIC2 Power", RT5679_DMIC_CTRL1,
		RT5679_DMIC_2_EN_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("DMIC3 Power", RT5679_DMIC_CTRL1,
		RT5679_DMIC_3_EN_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("DMIC4 Power", RT5679_DMIC_CTRL1,
		RT5679_DMIC_4_EN_SFT, 0, NULL, 0),

	SND_SOC_DAPM_PGA("DMIC1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("DMIC2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("DMIC3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("DMIC4", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_SUPPLY("DMIC CLK", SND_SOC_NOPM, 0, 0,
		rt5679_set_dmic_clk, SND_SOC_DAPM_PRE_PMU),

	/* Boost */
	SND_SOC_DAPM_PGA("BST1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("BST2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("BST3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("BST4", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("INPUT BUF", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_SUPPLY("BST1 Power", RT5679_PWR_ANA2, RT5679_PWR_BST1_BIT,
		0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("BST2 Power", RT5679_PWR_ANA2, RT5679_PWR_BST2_BIT,
		0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("BST3 Power", RT5679_PWR_ANA2, RT5679_PWR_BST3_BIT,
		0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("BST4 Power", RT5679_PWR_ANA2, RT5679_PWR_BST4_BIT,
		0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("INPUT BUF Power", RT5679_PWR_ADC,
		RT5679_PWR_INBUF_BIT, 0, NULL, 0),

	/* ADCs */
	SND_SOC_DAPM_ADC("ADC1", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_ADC("ADC2", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_ADC("ADC3", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_ADC("ADC4", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_ADC("ADC5", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_PGA("ADC12", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("ADC34", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("ADC23", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_SUPPLY("ADC1 Power", RT5679_PWR_ADC, RT5679_PWR_ADC1_BIT,
		0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("ADC2 Power", RT5679_PWR_ADC, RT5679_PWR_ADC2_BIT,
		0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("ADC3 Power", RT5679_PWR_ADC, RT5679_PWR_ADC3_BIT,
		0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("ADC4 Power", RT5679_PWR_ADC, RT5679_PWR_ADC4_BIT,
		0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("ADC5 Power", RT5679_PWR_ADC, RT5679_PWR_ADC5_BIT,
		0, NULL, 0),

	SND_SOC_DAPM_SUPPLY("ADC12 Clock", RT5679_PR_REG_ADC12_CLK_CTRL, 12, 0,
		NULL, 0),
	SND_SOC_DAPM_SUPPLY("ADC34 Clock", RT5679_PR_REG_ADC34_CLK_CTRL, 12, 0,
		NULL, 0),
	SND_SOC_DAPM_SUPPLY("ADC5 Clock", RT5679_PR_REG_ADC5_CLK_CTRL1, 12, 0,
		NULL, 0),

	/* ADC Mux */
	SND_SOC_DAPM_MUX("Stereo1 DMIC Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_sto1_dmic_mux),
	SND_SOC_DAPM_MUX("Stereo1 ADC Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_sto1_adc_mux),
	SND_SOC_DAPM_MUX("Stereo1 DD1 Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_sto1_dd1_mux),
	SND_SOC_DAPM_MUX("Stereo1 DD2 Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_sto1_dd2_mux),
	SND_SOC_DAPM_MUX("Stereo1 ADC1 Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_sto1_adc1_mux),
	SND_SOC_DAPM_MUX("Stereo1 ADC2 Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_sto1_adc2_mux),

	SND_SOC_DAPM_MUX("Stereo2 DMIC Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_sto2_dmic_mux),
	SND_SOC_DAPM_MUX("Stereo2 ADC Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_sto2_adc_mux),
	SND_SOC_DAPM_MUX("Stereo2 DD1 Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_sto2_dd1_mux),
	SND_SOC_DAPM_MUX("Stereo2 DD2 Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_sto2_dd2_mux),
	SND_SOC_DAPM_MUX("Stereo2 ADC1 Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_sto2_adc1_mux),
	SND_SOC_DAPM_MUX("Stereo2 ADC2 Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_sto2_adc2_mux),

	SND_SOC_DAPM_MUX("Stereo3 DMIC Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_sto3_dmic_mux),
	SND_SOC_DAPM_MUX("Stereo3 ADC Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_sto3_adc_mux),
	SND_SOC_DAPM_MUX("Stereo3 DD1 Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_sto3_dd1_mux),
	SND_SOC_DAPM_MUX("Stereo3 DD2 Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_sto3_dd2_mux),
	SND_SOC_DAPM_MUX("Stereo3 ADC1 Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_sto3_adc1_mux),
	SND_SOC_DAPM_MUX("Stereo3 ADC2 Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_sto3_adc2_mux),

	SND_SOC_DAPM_MUX("Mono DMIC L Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_mono_dmic_l_mux),
	SND_SOC_DAPM_MUX("Mono DMIC R Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_mono_dmic_r_mux),
	SND_SOC_DAPM_MUX("Mono ADC L Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_mono_adc_l_mux),
	SND_SOC_DAPM_MUX("Mono ADC R Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_mono_adc_r_mux),
	SND_SOC_DAPM_MUX("Mono DD1 L Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_mono_dd1_l_mux),
	SND_SOC_DAPM_MUX("Mono DD1 R Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_mono_dd1_r_mux),
	SND_SOC_DAPM_MUX("Mono DD2 L Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_mono_dd2_l_mux),
	SND_SOC_DAPM_MUX("Mono DD2 R Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_mono_dd2_r_mux),
	SND_SOC_DAPM_MUX("Mono ADC2 L Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_mono_adc2_l_mux),
	SND_SOC_DAPM_MUX("Mono ADC1 L Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_mono_adc1_l_mux),
	SND_SOC_DAPM_MUX("Mono ADC1 R Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_mono_adc1_r_mux),
	SND_SOC_DAPM_MUX("Mono ADC2 R Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_mono_adc2_r_mux),

	/* ADC Mixer */
	SND_SOC_DAPM_SUPPLY("adc stereo1 filter", RT5679_PWR_DIG2,
		RT5679_PWR_ADC_S1F_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("adc stereo2 filter", RT5679_PWR_DIG2,
		RT5679_PWR_ADC_S2F_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("adc stereo3 filter", RT5679_PWR_DIG2,
		RT5679_PWR_ADC_S3F_BIT, 0, NULL, 0),

	SND_SOC_DAPM_MIXER("Sto1 ADC MIXL", SND_SOC_NOPM, 0, 0,
		rt5679_sto1_adc_l_mix, ARRAY_SIZE(rt5679_sto1_adc_l_mix)),
	SND_SOC_DAPM_MIXER("Sto1 ADC MIXR", SND_SOC_NOPM, 0, 0,
		rt5679_sto1_adc_r_mix, ARRAY_SIZE(rt5679_sto1_adc_r_mix)),
	SND_SOC_DAPM_MIXER("Sto2 ADC MIXL", SND_SOC_NOPM, 0, 0,
		rt5679_sto2_adc_l_mix, ARRAY_SIZE(rt5679_sto2_adc_l_mix)),
	SND_SOC_DAPM_MIXER("Sto2 ADC MIXR", SND_SOC_NOPM, 0, 0,
		rt5679_sto2_adc_r_mix, ARRAY_SIZE(rt5679_sto2_adc_r_mix)),
	SND_SOC_DAPM_MIXER("Sto3 ADC MIXL", SND_SOC_NOPM, 0, 0,
		rt5679_sto3_adc_l_mix, ARRAY_SIZE(rt5679_sto3_adc_l_mix)),
	SND_SOC_DAPM_MIXER("Sto3 ADC MIXR", SND_SOC_NOPM, 0, 0,
		rt5679_sto3_adc_r_mix, ARRAY_SIZE(rt5679_sto3_adc_r_mix)),

	SND_SOC_DAPM_SUPPLY("adc mono left filter", RT5679_PWR_DIG2,
		RT5679_PWR_ADC_MF_L_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("adc mono right filter", RT5679_PWR_DIG2,
		RT5679_PWR_ADC_MF_R_BIT, 0, NULL, 0),

	SND_SOC_DAPM_MIXER("Mono ADC MIXL", SND_SOC_NOPM, 0, 0,
		rt5679_mono_adc_l_mix, ARRAY_SIZE(rt5679_mono_adc_l_mix)),
	SND_SOC_DAPM_MIXER("Mono ADC MIXR", SND_SOC_NOPM, 0, 0,
		rt5679_mono_adc_r_mix, ARRAY_SIZE(rt5679_mono_adc_r_mix)),

	SND_SOC_DAPM_ADC("Mono ADC MIXL ADC", NULL, RT5679_MONO_ADC_DIG_VOL,
		RT5679_L_MUTE_SFT, 1),
	SND_SOC_DAPM_ADC("Mono ADC MIXR ADC", NULL, RT5679_MONO_ADC_DIG_VOL,
		RT5679_R_MUTE_SFT, 1),

	SND_SOC_DAPM_ADC("Stereo1 ADC MIXL", NULL, RT5679_STO1_ADC_DIG_VOL,
		RT5679_L_MUTE_SFT, 1),
	SND_SOC_DAPM_ADC("Stereo1 ADC MIXR", NULL, RT5679_STO1_ADC_DIG_VOL,
		RT5679_R_MUTE_SFT, 1),
	SND_SOC_DAPM_ADC("Stereo2 ADC MIXL", NULL, RT5679_STO2_ADC_DIG_VOL,
		RT5679_L_MUTE_SFT, 1),
	SND_SOC_DAPM_ADC("Stereo2 ADC MIXR", NULL, RT5679_STO2_ADC_DIG_VOL,
		RT5679_R_MUTE_SFT, 1),
	SND_SOC_DAPM_ADC("Stereo3 ADC MIXL", NULL, RT5679_STO3_ADC_DIG_VOL,
		RT5679_L_MUTE_SFT, 1),
	SND_SOC_DAPM_ADC("Stereo3 ADC MIXR", NULL, RT5679_STO3_ADC_DIG_VOL,
		RT5679_R_MUTE_SFT, 1),

	/* ADC PGA */
	SND_SOC_DAPM_PGA("Stereo1 ADC MIX", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Stereo2 ADC MIX", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Stereo3 ADC MIX", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Mono ADC MIX", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 ADC", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 ADC", SND_SOC_NOPM, 0, 0, NULL, 0),

	/* DSP */
	SND_SOC_DAPM_MUX("IB7 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_ib7_src_mux),
	SND_SOC_DAPM_MUX("IB6 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_ib6_src_mux),
	SND_SOC_DAPM_MUX("IB45 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_ib45_src_mux),
	SND_SOC_DAPM_MUX("IB23 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_ib23_src_mux),
	SND_SOC_DAPM_MUX("IB01 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_ib01_src_mux),
	SND_SOC_DAPM_MUX("IB01 Mini Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_ib01_mini_src_mux),
	SND_SOC_DAPM_MUX("IB45 Bypass Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_ib45_bypass_src_mux),
	SND_SOC_DAPM_MUX("IB23 Bypass Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_ib23_bypass_src_mux),
	SND_SOC_DAPM_MUX("IB01 Bypass Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_ib01_bypass_src_mux),
	SND_SOC_DAPM_MUX("OB23 Bypass Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_ob23_bypass_src_mux),
	SND_SOC_DAPM_MUX("OB01 Bypass Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_ob01_bypass_src_mux),

	SND_SOC_DAPM_PGA("OB45", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("OB67", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_PGA("OutBound2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("OutBound3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("OutBound4", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("OutBound5", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("OutBound6", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("OutBound7", SND_SOC_NOPM, 0, 0, NULL, 0),

	/* Digital Interface */
	SND_SOC_DAPM_SUPPLY("I2S1", RT5679_PWR_DIG1,
		RT5679_PWR_I2S1_BIT, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 DAC0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 DAC1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 DAC2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 DAC3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 DAC4", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 DAC5", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 DAC6", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 DAC7", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 DAC01", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 DAC23", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 DAC45", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 DAC67", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 ADC1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 ADC2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 ADC3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 ADC4", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_SUPPLY("I2S2", RT5679_PWR_DIG1,
		RT5679_PWR_I2S2_BIT, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 DAC0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 DAC1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 DAC2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 DAC3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 DAC4", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 DAC5", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 DAC6", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 DAC7", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 DAC01", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 DAC23", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 DAC45", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 DAC67", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 ADC1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 ADC2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 ADC3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 ADC4", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_SUPPLY("I2S3", RT5679_PWR_DIG1,
		RT5679_PWR_I2S3_BIT, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF3 DAC", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF3 DAC L", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF3 DAC R", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF3 ADC", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF3 ADC L", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF3 ADC R", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_SUPPLY("I2S4", RT5679_PWR_DIG1,
		RT5679_PWR_I2S4_BIT, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF4 DAC", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF4 DAC L", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF4 DAC R", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF4 ADC", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF4 ADC L", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF4 ADC R", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_SUPPLY("I2S5", RT5679_PWR_DIG1,
		RT5679_PWR_I2S5_BIT, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF5 DAC", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF5 DAC L", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF5 DAC R", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF5 ADC", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF5 ADC L", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF5 ADC R", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_SUPPLY("SLB", RT5679_PWR_DIG1,
		RT5679_PWR_SLB_BIT, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SLB DAC0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SLB DAC1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SLB DAC2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SLB DAC3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SLB DAC4", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SLB DAC5", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SLB DAC6", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SLB DAC7", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SLB DAC01", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SLB DAC23", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SLB DAC45", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SLB DAC67", SND_SOC_NOPM, 0, 0, NULL, 0),

	/* Digital Interface Select */
	SND_SOC_DAPM_MUX("IF1 ADC1 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if1_adc1_mux),
	SND_SOC_DAPM_MUX("IF1 ADC2 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if1_adc2_mux),
	SND_SOC_DAPM_MUX("IF1 ADC3 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if1_adc3_mux),
	SND_SOC_DAPM_MUX("IF1 ADC4 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if1_adc4_mux),
	SND_SOC_DAPM_MUX("IF1 ADC1 Swap Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if1_adc1_swap_mux),
	SND_SOC_DAPM_MUX("IF1 ADC2 Swap Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if1_adc2_swap_mux),
	SND_SOC_DAPM_MUX("IF1 ADC3 Swap Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if1_adc3_swap_mux),
	SND_SOC_DAPM_MUX("IF1 ADC4 Swap Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if1_adc4_swap_mux),
	SND_SOC_DAPM_MUX_E("IF1 ADC TDM Swap Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if1_adc_tdm_swap_mux, NULL, 0),
	SND_SOC_DAPM_MUX("IF2 ADC1 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if2_adc1_mux),
	SND_SOC_DAPM_MUX("IF2 ADC2 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if2_adc2_mux),
	SND_SOC_DAPM_MUX("IF2 ADC3 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if2_adc3_mux),
	SND_SOC_DAPM_MUX("IF2 ADC4 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if2_adc4_mux),
	SND_SOC_DAPM_MUX("IF2 ADC1 Swap Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if2_adc1_swap_mux),
	SND_SOC_DAPM_MUX("IF2 ADC2 Swap Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if2_adc2_swap_mux),
	SND_SOC_DAPM_MUX("IF2 ADC3 Swap Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if2_adc3_swap_mux),
	SND_SOC_DAPM_MUX("IF2 ADC4 Swap Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if2_adc4_swap_mux),
	SND_SOC_DAPM_MUX_E("IF2 ADC TDM Swap Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if2_adc_tdm_swap_mux, NULL, 0),
	SND_SOC_DAPM_MUX("IF3 ADC Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if3_adc_mux),
	SND_SOC_DAPM_MUX("IF4 ADC Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if4_adc_mux),
	SND_SOC_DAPM_MUX("IF5 ADC Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if5_adc_mux),
	SND_SOC_DAPM_MUX("SLB ADC1 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_slb_adc1_mux),
	SND_SOC_DAPM_MUX("SLB ADC2 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_slb_adc2_mux),
	SND_SOC_DAPM_MUX("SLB ADC3 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_slb_adc3_mux),
	SND_SOC_DAPM_MUX("SLB ADC4 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_slb_adc4_mux),

	SND_SOC_DAPM_MUX("IF1 DAC0 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if1_dac0_tdm_sel_mux),
	SND_SOC_DAPM_MUX("IF1 DAC1 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if1_dac1_tdm_sel_mux),
	SND_SOC_DAPM_MUX("IF1 DAC2 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if1_dac2_tdm_sel_mux),
	SND_SOC_DAPM_MUX("IF1 DAC3 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if1_dac3_tdm_sel_mux),
	SND_SOC_DAPM_MUX("IF1 DAC4 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if1_dac4_tdm_sel_mux),
	SND_SOC_DAPM_MUX("IF1 DAC5 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if1_dac5_tdm_sel_mux),
	SND_SOC_DAPM_MUX("IF1 DAC6 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if1_dac6_tdm_sel_mux),
	SND_SOC_DAPM_MUX("IF1 DAC7 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if1_dac7_tdm_sel_mux),

	SND_SOC_DAPM_MUX("IF2 DAC0 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if2_dac0_tdm_sel_mux),
	SND_SOC_DAPM_MUX("IF2 DAC1 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if2_dac1_tdm_sel_mux),
	SND_SOC_DAPM_MUX("IF2 DAC2 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if2_dac2_tdm_sel_mux),
	SND_SOC_DAPM_MUX("IF2 DAC3 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if2_dac3_tdm_sel_mux),
	SND_SOC_DAPM_MUX("IF2 DAC4 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if2_dac4_tdm_sel_mux),
	SND_SOC_DAPM_MUX("IF2 DAC5 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if2_dac5_tdm_sel_mux),
	SND_SOC_DAPM_MUX("IF2 DAC6 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if2_dac6_tdm_sel_mux),
	SND_SOC_DAPM_MUX("IF2 DAC7 Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if2_dac7_tdm_sel_mux),

	SND_SOC_DAPM_MUX("IF3 DAC Swap Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if3_dac_swap_mux),
	SND_SOC_DAPM_MUX("IF3 ADC Swap Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if3_adc_swap_mux),

	SND_SOC_DAPM_MUX("IF4 DAC Swap Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if4_dac_swap_mux),
	SND_SOC_DAPM_MUX("IF4 ADC Swap Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if4_adc_swap_mux),

	SND_SOC_DAPM_MUX("IF5 DAC Swap Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if5_dac_swap_mux),
	SND_SOC_DAPM_MUX("IF5 ADC Swap Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_if5_adc_swap_mux),

	/* Audio Interface */
	SND_SOC_DAPM_AIF_IN("AIF1RX", "AIF1 Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("AIF1TX", "AIF1 Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("AIF2RX", "AIF2 Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("AIF2TX", "AIF2 Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("AIF3RX", "AIF3 Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("AIF3TX", "AIF3 Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("AIF4RX", "AIF4 Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("AIF4TX", "AIF4 Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("AIF5RX", "AIF5 Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("AIF5TX", "AIF5 Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("SLBRX", "SLIMBus Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("SLBTX", "SLIMBus Capture", 0, SND_SOC_NOPM, 0, 0),

	/* Sidetone Mux */
	SND_SOC_DAPM_MUX("Sidetone Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_sidetone_mux),
	/* VAD Mux*/
	SND_SOC_DAPM_MUX("VAD ADC Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_vad_src_mux),
	SND_SOC_DAPM_MUX("VAD ADC INBUF Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_vad_inbuf_src_mux),
	SND_SOC_DAPM_MUX("VAD ADC DMIC Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_vad_dmic_src_mux),
	SND_SOC_DAPM_MUX("VAD ADC FILTER Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_vad_filter_src_mux),

	/* Tensilica DSP */
	SND_SOC_DAPM_MIXER("OB01 MIX", SND_SOC_NOPM, 0, 0,
		rt5679_ob_01_mix, ARRAY_SIZE(rt5679_ob_01_mix)),
	SND_SOC_DAPM_MIXER("OB23 MIX", SND_SOC_NOPM, 0, 0,
		rt5679_ob_23_mix, ARRAY_SIZE(rt5679_ob_23_mix)),
	SND_SOC_DAPM_MIXER("OB4 MIX", SND_SOC_NOPM, 0, 0,
		rt5679_ob_4_mix, ARRAY_SIZE(rt5679_ob_4_mix)),
	SND_SOC_DAPM_MIXER("OB5 MIX", SND_SOC_NOPM, 0, 0,
		rt5679_ob_5_mix, ARRAY_SIZE(rt5679_ob_5_mix)),
	SND_SOC_DAPM_MIXER("OB6 MIX", SND_SOC_NOPM, 0, 0,
		rt5679_ob_6_mix, ARRAY_SIZE(rt5679_ob_6_mix)),
	SND_SOC_DAPM_MIXER("OB7 MIX", SND_SOC_NOPM, 0, 0,
		rt5679_ob_7_mix, ARRAY_SIZE(rt5679_ob_7_mix)),

	SND_SOC_DAPM_SUPPLY("DSP event", SND_SOC_NOPM, 0, 0, rt5679_dsp_event,
		SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),

	/* Output Side */
	/* DAC mixer before sound effect  */
	SND_SOC_DAPM_MIXER("DAC1 MIXL", SND_SOC_NOPM, 0, 0,
		rt5679_dac_l_mix, ARRAY_SIZE(rt5679_dac_l_mix)),
	SND_SOC_DAPM_MIXER("DAC1 MIXR", SND_SOC_NOPM, 0, 0,
		rt5679_dac_r_mix, ARRAY_SIZE(rt5679_dac_r_mix)),
	SND_SOC_DAPM_PGA("DAC1 FS", SND_SOC_NOPM, 0, 0, NULL, 0),

	/* DAC1 Mux */
	SND_SOC_DAPM_MUX("DAC1 Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_dac1_mux),
	SND_SOC_DAPM_MUX("ADDA1 Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_adda1_mux),

	/* DAC2 Mux */
	SND_SOC_DAPM_MUX("DAC2 L Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_dac2_l_mux),
	SND_SOC_DAPM_MUX("DAC2 R Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_dac2_r_mux),

	/* DAC3 Mux */
	SND_SOC_DAPM_MUX("DAC3 L Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_dac3_l_mux),
	SND_SOC_DAPM_MUX("DAC3 R Mux", SND_SOC_NOPM, 0, 0,
			&rt5679_dac3_r_mux),

	/* Source Mux */
	SND_SOC_DAPM_MUX("DAC12 Source Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_dac12_mux),
	SND_SOC_DAPM_MUX("DAC3 Source Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_dac3_mux),
	SND_SOC_DAPM_MUX("DAC4 Source Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_dac4_mux),
	SND_SOC_DAPM_MUX("DAC5 Source Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_dac5_mux),

	SND_SOC_DAPM_MUX("DAC1 L Mixer Source Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_dac1l_mixer_mux),
	SND_SOC_DAPM_MUX("DAC1 R Mixer Source Mux", SND_SOC_NOPM, 0, 0,
				&rt5679_dac1r_mixer_mux),

	/* DAC Mixer */
	SND_SOC_DAPM_SUPPLY("dac stereo1 filter", RT5679_PWR_DIG2,
		RT5679_PWR_DAC_S1F_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("dac mono2 left filter", RT5679_PWR_DIG2,
		RT5679_PWR_DAC_M2F_L_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("dac mono2 right filter", RT5679_PWR_DIG2,
		RT5679_PWR_DAC_M2F_R_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("dac mono3 left filter", RT5679_PWR_DIG2,
		RT5679_PWR_DAC_M3F_L_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("dac mono3 right filter", RT5679_PWR_DIG2,
		RT5679_PWR_DAC_M3F_R_BIT, 0, NULL, 0),

	SND_SOC_DAPM_MIXER("Stereo DAC MIXL", SND_SOC_NOPM, 0, 0,
		rt5679_sto1_dac_l_mix, ARRAY_SIZE(rt5679_sto1_dac_l_mix)),
	SND_SOC_DAPM_MIXER("Stereo DAC MIXR", SND_SOC_NOPM, 0, 0,
		rt5679_sto1_dac_r_mix, ARRAY_SIZE(rt5679_sto1_dac_r_mix)),
	SND_SOC_DAPM_MIXER("Mono DAC MIXL", SND_SOC_NOPM, 0, 0,
		rt5679_mono_dac_l_mix, ARRAY_SIZE(rt5679_mono_dac_l_mix)),
	SND_SOC_DAPM_MIXER("Mono DAC MIXR", SND_SOC_NOPM, 0, 0,
		rt5679_mono_dac_r_mix, ARRAY_SIZE(rt5679_mono_dac_r_mix)),
	SND_SOC_DAPM_MIXER("DD MIXL", SND_SOC_NOPM, 0, 0,
		rt5679_dd_l_mix, ARRAY_SIZE(rt5679_dd_l_mix)),
	SND_SOC_DAPM_MIXER("DD MIXR", SND_SOC_NOPM, 0, 0,
		rt5679_dd_r_mix, ARRAY_SIZE(rt5679_dd_r_mix)),

	SND_SOC_DAPM_PGA("Stereo DAC MIX", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Mono DAC MIX", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("DD MIX", SND_SOC_NOPM, 0, 0, NULL, 0),

	/* DACs */
	SND_SOC_DAPM_DAC("DAC 1", NULL, RT5679_PWR_DIG1,
		RT5679_PWR_DAC1_BIT, 0),
	SND_SOC_DAPM_DAC("DAC 2", NULL, RT5679_PWR_DIG1,
		RT5679_PWR_DAC2_BIT, 0),
	SND_SOC_DAPM_DAC("DAC 3", NULL, RT5679_PWR_DIG1,
		RT5679_PWR_DAC3_BIT, 0),
	SND_SOC_DAPM_DAC("DAC 4", NULL, RT5679_PWR_DIG1,
		RT5679_PWR_DAC4_BIT, 0),
	SND_SOC_DAPM_DAC("DAC 5", NULL, RT5679_PWR_DIG1,
		RT5679_PWR_DAC5_BIT, 0),

	SND_SOC_DAPM_SUPPLY("LDO DACREF1", RT5679_PWR_LDO1,
		RT5679_PWR_LDO_DACREF1_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("LDO DACREF2", RT5679_PWR_LDO1,
		RT5679_PWR_LDO_DACREF2_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("LDO ADDA", RT5679_PWR_LDO1,
		RT5679_PWR_LDO_ADDA_BIT, 0, NULL, 0),

	SND_SOC_DAPM_SUPPLY("DAC 1 Clock", RT5679_DAC1_CLK_AND_CHOPPER_CTRL,
		RT5679_EN_CKGEN_DAC_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("DAC 2 Clock", RT5679_DAC2_CLK_AND_CHOPPER_CTRL,
		RT5679_EN_CKGEN_DAC_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("DAC 3 Clock", RT5679_DAC3_CLK_AND_CHOPPER_CTRL,
		RT5679_EN_CKGEN_DAC_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("DAC 4 Clock", RT5679_DAC4_CLK_AND_CHOPPER_CTRL,
		RT5679_EN_CKGEN_DAC_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("DAC 5 Clock", RT5679_DAC5_CLK_AND_CHOPPER_CTRL,
		RT5679_EN_CKGEN_DAC_SFT, 0, NULL, 0),

	/* PDM */
	SND_SOC_DAPM_SUPPLY("PDM1 Power", RT5679_PWR_DIG2,
		RT5679_PWR_PDM1_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("PDM2 Power", RT5679_PWR_DIG2,
		RT5679_PWR_PDM2_BIT, 0, NULL, 0),

	SND_SOC_DAPM_MUX_E("PDM1 L Mux", SND_SOC_NOPM, 0, 0, &rt5679_pdm1_l_mux,
		NULL, 0),
	SND_SOC_DAPM_MUX_E("PDM1 R Mux", SND_SOC_NOPM, 0, 0, &rt5679_pdm1_r_mux,
		NULL, 0),
	SND_SOC_DAPM_MUX_E("PDM2 L Mux", SND_SOC_NOPM, 0, 0, &rt5679_pdm2_l_mux,
		NULL, 0),
	SND_SOC_DAPM_MUX_E("PDM2 R Mux", SND_SOC_NOPM, 0, 0, &rt5679_pdm2_r_mux,
		NULL, 0),

	SND_SOC_DAPM_SUPPLY("Mono Power", RT5679_PWR_ANA2, RT5679_PWR_MA_BIT,
		0, NULL, 0),
	SND_SOC_DAPM_PGA_S("Mono Amp", 1, RT5679_MONO_OUT,
		RT5679_EN_DAC5_MONO_SFT, 0, rt5679_mono_event,
		SND_SOC_DAPM_PRE_PMU),

	SND_SOC_DAPM_PGA_S("HPL Amp", 1, RT5679_HP_OUT, RT5679_EN_DAC1_HPL_SFT,
		0, rt5679_hp_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_PGA_S("HPR Amp", 1, RT5679_HP_OUT, RT5679_EN_DAC2_HPR_SFT,
		0, rt5679_hp_event, SND_SOC_DAPM_PRE_PMU),

	SND_SOC_DAPM_SUPPLY("LOUT1 Power", RT5679_PWR_ANA1, RT5679_PWR_LO1_BIT,
		0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("LOUT2 Power", RT5679_PWR_ANA1, RT5679_PWR_LO2_BIT,
		0, NULL, 0),
	SND_SOC_DAPM_PGA_S("LOUT1 Amp", 1, RT5679_LOUT,
		RT5679_EN_DAC3_LOUT1_SFT, 0, rt5679_lout_event,
		SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_PGA_S("LOUT2 Amp", 1, RT5679_LOUT,
		RT5679_EN_DAC4_LOUT2_SFT, 0, rt5679_lout_event,
		SND_SOC_DAPM_PRE_PMU),

	SND_SOC_DAPM_SWITCH("LOUT1 Playback", SND_SOC_NOPM, 0, 0,
		&rt5679_lout1_switch),
	SND_SOC_DAPM_SWITCH("LOUT2 Playback", SND_SOC_NOPM, 0, 0,
		&rt5679_lout2_switch),
	SND_SOC_DAPM_SWITCH("PDM1 L Playback", SND_SOC_NOPM, 0, 0,
		&rt5679_pdm1_l_switch),
	SND_SOC_DAPM_SWITCH("PDM1 R Playback", SND_SOC_NOPM, 0, 0,
		&rt5679_pdm1_r_switch),
	SND_SOC_DAPM_SWITCH("PDM2 L Playback", SND_SOC_NOPM, 0, 0,
		&rt5679_pdm2_l_switch),
	SND_SOC_DAPM_SWITCH("PDM2 R Playback", SND_SOC_NOPM, 0, 0,
		&rt5679_pdm2_r_switch),

	/* Output Lines */
	SND_SOC_DAPM_OUTPUT("HPOL"),
	SND_SOC_DAPM_OUTPUT("HPOR"),
	SND_SOC_DAPM_OUTPUT("LOUT1"),
	SND_SOC_DAPM_OUTPUT("LOUT2"),
	SND_SOC_DAPM_OUTPUT("MONOOUT"),
	SND_SOC_DAPM_OUTPUT("PDM1L"),
	SND_SOC_DAPM_OUTPUT("PDM1R"),
	SND_SOC_DAPM_OUTPUT("PDM2L"),
	SND_SOC_DAPM_OUTPUT("PDM2R"),
};

static const struct snd_soc_dapm_route rt5679_dapm_routes[] = {
	{ "Stereo1 DMIC Mux", NULL, "DMIC STO1 ASRC",
		rt5679_dmic_is_using_asrc },
	{ "Stereo2 DMIC Mux", NULL, "DMIC STO2 ASRC",
		rt5679_dmic_is_using_asrc },
	{ "Stereo3 DMIC Mux", NULL, "DMIC STO3 ASRC",
		rt5679_dmic_is_using_asrc },
	{ "Mono DMIC L Mux", NULL, "DMIC MONO L ASRC",
		rt5679_dmic_is_using_asrc },
	{ "Mono DMIC R Mux", NULL, "DMIC MONO R ASRC",
		rt5679_dmic_is_using_asrc },

	{ "I2S1", NULL, "I2S1 ASRC", rt5679_can_use_asrc},
	{ "I2S2", NULL, "I2S2 ASRC", rt5679_can_use_asrc},
	{ "I2S3", NULL, "I2S3 ASRC", rt5679_can_use_asrc},
	{ "I2S4", NULL, "I2S4 ASRC", rt5679_can_use_asrc},
	{ "I2S5", NULL, "I2S5 ASRC", rt5679_can_use_asrc},

	{ "dac stereo1 filter", NULL, "DAC STO ASRC", rt5679_is_using_asrc },
	{ "dac mono2 left filter", NULL, "DAC MONO2 L ASRC",
		rt5679_is_using_asrc },
	{ "dac mono2 right filter", NULL, "DAC MONO2 R ASRC",
		rt5679_is_using_asrc },
	{ "dac mono3 left filter", NULL, "DAC MONO3 L ASRC",
		rt5679_is_using_asrc },
	{ "dac mono3 right filter", NULL, "DAC MONO3 R ASRC",
		rt5679_is_using_asrc },
	{ "adc stereo1 filter", NULL, "ADC STO1 ASRC", rt5679_is_using_asrc },
	{ "adc stereo2 filter", NULL, "ADC STO2 ASRC", rt5679_is_using_asrc },
	{ "adc stereo3 filter", NULL, "ADC STO3 ASRC", rt5679_is_using_asrc },
	{ "adc mono left filter", NULL, "ADC MONO L ASRC",
		rt5679_is_using_asrc },
	{ "adc mono right filter", NULL, "ADC MONO R ASRC",
		rt5679_is_using_asrc },

	{ "MICBIAS2", NULL, "LDO4" },
	{ "MICBIAS2", NULL, "Vref" },
	{ "MICBIAS3", NULL, "LDO4" },
	{ "MICBIAS3", NULL, "Vref" },
	{ "MICBIAS4", NULL, "LDO4" },
	{ "MICBIAS4", NULL, "Vref" },

	{ "PLL1", NULL, "PLL1 RST" },
	{ "PLL1", NULL, "PLL1 LDO" },

	{ "DMIC1", NULL, "DMIC1L" },
	{ "DMIC1", NULL, "DMIC1R" },
	{ "DMIC2", NULL, "DMIC2L" },
	{ "DMIC2", NULL, "DMIC2R" },
	{ "DMIC3", NULL, "DMIC3L" },
	{ "DMIC3", NULL, "DMIC3R" },
	{ "DMIC4", NULL, "DMIC4L" },
	{ "DMIC4", NULL, "DMIC4R" },

	{ "DMIC1L", NULL, "DMIC CLK" },
	{ "DMIC1R", NULL, "DMIC CLK" },
	{ "DMIC2L", NULL, "DMIC CLK" },
	{ "DMIC2R", NULL, "DMIC CLK" },
	{ "DMIC3L", NULL, "DMIC CLK" },
	{ "DMIC3R", NULL, "DMIC CLK" },
	{ "DMIC4L", NULL, "DMIC CLK" },
	{ "DMIC4R", NULL, "DMIC CLK" },
	{ "DMIC1L", NULL, "DMIC1 Power" },
	{ "DMIC1R", NULL, "DMIC1 Power" },
	{ "DMIC2L", NULL, "DMIC2 Power" },
	{ "DMIC2R", NULL, "DMIC2 Power" },
	{ "DMIC3L", NULL, "DMIC3 Power" },
	{ "DMIC3R", NULL, "DMIC3 Power" },
	{ "DMIC4L", NULL, "DMIC4 Power" },
	{ "DMIC4R", NULL, "DMIC4 Power" },

	{ "BST1", NULL, "IN1P" },
	{ "BST1", NULL, "IN1N" },
	{ "BST1", NULL, "BST1 Power" },
	{ "BST2", NULL, "IN2P" },
	{ "BST2", NULL, "IN2N" },
	{ "BST2", NULL, "BST2 Power" },
	{ "BST3", NULL, "IN3P" },
	{ "BST3", NULL, "IN3N" },
	{ "BST3", NULL, "BST3 Power" },
	{ "BST4", NULL, "IN4P" },
	{ "BST4", NULL, "IN4N" },
	{ "BST4", NULL, "BST4 Power" },

	{ "ADC1", NULL, "BST1" },
	{ "ADC1", NULL, "ADC1 Power" },
	{ "ADC1", NULL, "ADC12 Clock" },
	{ "ADC1", NULL, "Vref" },
	{ "ADC1", NULL, "LDO ADDA" },
	{ "ADC2", NULL, "BST2" },
	{ "ADC2", NULL, "ADC2 Power" },
	{ "ADC2", NULL, "ADC12 Clock" },
	{ "ADC2", NULL, "Vref" },
	{ "ADC2", NULL, "LDO ADDA" },
	{ "ADC3", NULL, "BST3" },
	{ "ADC3", NULL, "ADC3 Power" },
	{ "ADC3", NULL, "ADC34 Clock" },
	{ "ADC3", NULL, "Vref" },
	{ "ADC3", NULL, "LDO ADDA" },
	{ "ADC4", NULL, "BST4" },
	{ "ADC4", NULL, "ADC4 Power" },
	{ "ADC4", NULL, "ADC34 Clock" },
	{ "ADC4", NULL, "Vref" },
	{ "ADC4", NULL, "LDO ADDA" },
	{ "ADC5", NULL, "INPUT BUF" },
	{ "ADC5", NULL, "ADC5 Power" },
	{ "ADC5", NULL, "ADC5 Clock" },
	{ "ADC5", NULL, "Vref" },
	{ "ADC5", NULL, "LDO ADDA" },

	{ "ADC12", NULL, "ADC1" },
	{ "ADC12", NULL, "ADC2" },
	{ "ADC34", NULL, "ADC3" },
	{ "ADC34", NULL, "ADC4" },
	{ "ADC23", NULL, "ADC2" },
	{ "ADC23", NULL, "ADC3" },

	{ "Stereo1 DMIC Mux", "DMIC1", "DMIC1" },
	{ "Stereo1 DMIC Mux", "DMIC2", "DMIC2" },
	{ "Stereo1 DMIC Mux", "DMIC3", "DMIC3" },
	{ "Stereo1 DMIC Mux", "DMIC4", "DMIC4" },

	{ "Stereo2 DMIC Mux", "DMIC1", "DMIC1" },
	{ "Stereo2 DMIC Mux", "DMIC2", "DMIC2" },
	{ "Stereo2 DMIC Mux", "DMIC3", "DMIC3" },
	{ "Stereo2 DMIC Mux", "DMIC4", "DMIC4" },

	{ "Stereo3 DMIC Mux", "DMIC1", "DMIC1" },
	{ "Stereo3 DMIC Mux", "DMIC2", "DMIC2" },
	{ "Stereo3 DMIC Mux", "DMIC3", "DMIC3" },
	{ "Stereo3 DMIC Mux", "DMIC4", "DMIC4" },

	{ "Mono DMIC L Mux", "DMIC1", "DMIC1" },
	{ "Mono DMIC L Mux", "DMIC2", "DMIC2" },
	{ "Mono DMIC L Mux", "DMIC3", "DMIC3" },
	{ "Mono DMIC L Mux", "DMIC4", "DMIC4" },

	{ "Mono DMIC R Mux", "DMIC1", "DMIC1" },
	{ "Mono DMIC R Mux", "DMIC2", "DMIC2" },
	{ "Mono DMIC R Mux", "DMIC3", "DMIC3" },
	{ "Mono DMIC R Mux", "DMIC4", "DMIC4" },

	{ "Stereo1 ADC Mux", "ADC12", "ADC12" },
	{ "Stereo1 ADC Mux", "ADC34", "ADC34" },
	{ "Stereo1 ADC Mux", "ADC23", "ADC23" },

	{ "Stereo2 ADC Mux", "ADC12", "ADC12" },
	{ "Stereo2 ADC Mux", "ADC34", "ADC34" },
	{ "Stereo2 ADC Mux", "ADC23", "ADC23" },

	{ "Stereo3 ADC Mux", "ADC12", "ADC12" },
	{ "Stereo3 ADC Mux", "ADC34", "ADC34" },

	{ "Mono ADC L Mux", "ADC12", "ADC1" },
	{ "Mono ADC L Mux", "ADC34", "ADC3" },

	{ "Mono ADC R Mux", "ADC12", "ADC2" },
	{ "Mono ADC R Mux", "ADC34", "ADC4" },

	{ "Stereo1 DD1 Mux", "DD MIX", "DD MIX" },
	{ "Stereo1 DD1 Mux", "DAC MIX", "Stereo DAC MIX" },

	{ "Stereo2 DD1 Mux", "DD MIX", "DD MIX" },
	{ "Stereo2 DD1 Mux", "DAC MIX", "Stereo DAC MIX" },

	{ "Stereo3 DD1 Mux", "DD MIX", "DD MIX" },
	{ "Stereo3 DD1 Mux", "DAC MIX", "Stereo DAC MIX" },

	{ "Mono DD1 L Mux", "DD MIX", "DD MIXL" },
	{ "Mono DD1 L Mux", "DAC MIX", "Mono DAC MIXL" },

	{ "Mono DD1 R Mux", "DD MIX", "DD MIXR" },
	{ "Mono DD1 R Mux", "DAC MIX", "Mono DAC MIXR" },

	{ "Stereo1 DD2 Mux", "DD MIX", "DD MIX" },
	{ "Stereo1 DD2 Mux", "Stereo DAC MIX", "Stereo DAC MIX" },
	{ "Stereo1 DD2 Mux", "Mono DAC MIX", "Mono DAC MIX" },

	{ "Stereo2 DD2 Mux", "DD MIX", "DD MIX" },
	{ "Stereo2 DD2 Mux", "Stereo DAC MIX", "Stereo DAC MIX" },
	{ "Stereo2 DD2 Mux", "Mono DAC MIX", "Mono DAC MIX" },

	{ "Stereo3 DD2 Mux", "DD MIX", "DD MIX" },
	{ "Stereo3 DD2 Mux", "Stereo DAC MIX", "Stereo DAC MIX" },
	{ "Stereo3 DD2 Mux", "Mono DAC MIX", "Mono DAC MIX" },

	{ "Mono DD2 L Mux", "DD MIX", "DD MIXL" },
	{ "Mono DD2 L Mux", "Stereo DAC MIX", "DAC1 L Mixer Source Mux" },
	{ "Mono DD2 L Mux", "Mono DAC MIX", "Mono DAC MIXL" },

	{ "Mono DD2 R Mux", "DD MIX", "DD MIXR" },
	{ "Mono DD2 R Mux", "Stereo DAC MIX", "DAC1 R Mixer Source Mux" },
	{ "Mono DD2 R Mux", "Mono DAC MIX", "Mono DAC MIXR" },

	{ "Stereo1 ADC1 Mux", "ADC/DMIC", "Stereo1 ADC Mux" },
	{ "Stereo1 ADC1 Mux", "DD/DAC", "Stereo1 DD1 Mux" },

	{ "Stereo1 ADC2 Mux", "ADC/DMIC", "Stereo1 DMIC Mux" },
	{ "Stereo1 ADC2 Mux", "DD/DAC", "Stereo1 DD2 Mux" },

	{ "Stereo2 ADC1 Mux", "ADC/DMIC", "Stereo2 ADC Mux" },
	{ "Stereo2 ADC1 Mux", "DD/DAC", "Stereo2 DD1 Mux" },

	{ "Stereo2 ADC2 Mux", "ADC/DMIC", "Stereo2 DMIC Mux" },
	{ "Stereo2 ADC2 Mux", "DD/DAC", "Stereo2 DD2 Mux" },

	{ "Stereo3 ADC1 Mux", "ADC/DMIC", "Stereo3 ADC Mux" },
	{ "Stereo3 ADC1 Mux", "DD/DAC", "Stereo3 DD1 Mux" },

	{ "Stereo3 ADC2 Mux", "ADC/DMIC", "Stereo3 DMIC Mux" },
	{ "Stereo3 ADC2 Mux", "DD/DAC", "Stereo3 DD2 Mux" },

	{ "Mono ADC2 L Mux", "ADC/DMIC", "Mono DMIC L Mux" },
	{ "Mono ADC2 L Mux", "DD/DAC", "Mono DD2 L Mux" },

	{ "Mono ADC1 L Mux", "ADC/DMIC", "Mono ADC L Mux" },
	{ "Mono ADC1 L Mux", "DD/DAC", "Mono DD1 L Mux" },

	{ "Mono ADC1 R Mux", "ADC/DMIC", "Mono ADC R Mux" },
	{ "Mono ADC1 R Mux", "DD/DAC", "Mono DD1 R Mux" },

	{ "Mono ADC2 R Mux", "ADC/DMIC", "Mono DMIC R Mux" },
	{ "Mono ADC2 R Mux", "DD/DAC", "Mono DD2 R Mux" },

	{ "Sto1 ADC MIXL", "ADC1 Switch", "Stereo1 ADC1 Mux" },
	{ "Sto1 ADC MIXL", "ADC2 Switch", "Stereo1 ADC2 Mux" },
	{ "Sto1 ADC MIXR", "ADC1 Switch", "Stereo1 ADC1 Mux" },
	{ "Sto1 ADC MIXR", "ADC2 Switch", "Stereo1 ADC2 Mux" },

	{ "Stereo1 ADC MIXL", NULL, "Sto1 ADC MIXL" },
	{ "Stereo1 ADC MIXL", NULL, "adc stereo1 filter" },
	{ "Stereo1 ADC MIXR", NULL, "Sto1 ADC MIXR" },
	{ "Stereo1 ADC MIXR", NULL, "adc stereo1 filter" },
	{ "adc stereo1 filter", NULL, "PLL1", rt5679_is_sys_clk_from_pll },

	{ "Stereo1 ADC MIX", NULL, "Stereo1 ADC MIXL" },
	{ "Stereo1 ADC MIX", NULL, "Stereo1 ADC MIXR" },

	{ "Sto2 ADC MIXL", "ADC1 Switch", "Stereo2 ADC1 Mux" },
	{ "Sto2 ADC MIXL", "ADC2 Switch", "Stereo2 ADC2 Mux" },
	{ "Sto2 ADC MIXR", "ADC1 Switch", "Stereo2 ADC1 Mux" },
	{ "Sto2 ADC MIXR", "ADC2 Switch", "Stereo2 ADC2 Mux" },

	{ "Stereo2 ADC MIXL", NULL, "Sto2 ADC MIXL" },
	{ "Stereo2 ADC MIXL", NULL, "adc stereo2 filter" },
	{ "Stereo2 ADC MIXR", NULL, "Sto2 ADC MIXR" },
	{ "Stereo2 ADC MIXR", NULL, "adc stereo2 filter" },
	{ "adc stereo2 filter", NULL, "PLL1", rt5679_is_sys_clk_from_pll },

	{ "Stereo2 ADC MIX", NULL, "Stereo2 ADC MIXL" },
	{ "Stereo2 ADC MIX", NULL, "Stereo2 ADC MIXR" },

	{ "Sto3 ADC MIXL", "ADC1 Switch", "Stereo3 ADC1 Mux" },
	{ "Sto3 ADC MIXL", "ADC2 Switch", "Stereo3 ADC2 Mux" },
	{ "Sto3 ADC MIXR", "ADC1 Switch", "Stereo3 ADC1 Mux" },
	{ "Sto3 ADC MIXR", "ADC2 Switch", "Stereo3 ADC2 Mux" },

	{ "Stereo3 ADC MIXL", NULL, "Sto3 ADC MIXL" },
	{ "Stereo3 ADC MIXL", NULL, "adc stereo3 filter" },
	{ "Stereo3 ADC MIXR", NULL, "Sto3 ADC MIXR" },
	{ "Stereo3 ADC MIXR", NULL, "adc stereo3 filter" },
	{ "adc stereo3 filter", NULL, "PLL1", rt5679_is_sys_clk_from_pll },

	{ "Stereo3 ADC MIX", NULL, "Stereo3 ADC MIXL" },
	{ "Stereo3 ADC MIX", NULL, "Stereo3 ADC MIXR" },

	{ "Mono ADC MIXL", "ADC1 Switch", "Mono ADC1 L Mux" },
	{ "Mono ADC MIXL", "ADC2 Switch", "Mono ADC2 L Mux" },
	{ "Mono ADC MIXL", NULL, "adc mono left filter" },
	{ "adc mono left filter", NULL, "PLL1", rt5679_is_sys_clk_from_pll },

	{ "Mono ADC MIXR", "ADC1 Switch", "Mono ADC1 R Mux" },
	{ "Mono ADC MIXR", "ADC2 Switch", "Mono ADC2 R Mux" },
	{ "Mono ADC MIXR", NULL, "adc mono right filter" },
	{ "adc mono right filter", NULL, "PLL1", rt5679_is_sys_clk_from_pll },

	{ "Mono ADC MIXL ADC", NULL, "Mono ADC MIXL" },
	{ "Mono ADC MIXR ADC", NULL, "Mono ADC MIXR" },

	{ "Mono ADC MIX", NULL, "Mono ADC MIXL ADC" },
	{ "Mono ADC MIX", NULL, "Mono ADC MIXR ADC" },

	{ "VAD ADC Mux", "MONO ADC MIX L", "Mono ADC MIXL ADC" },
	{ "VAD ADC Mux", "MONO ADC MIX R", "Mono ADC MIXR ADC" },
	{ "VAD ADC Mux", "STO1 ADC MIX L", "Stereo1 ADC MIXL" },
	{ "VAD ADC Mux", "STO2 ADC MIX L", "Stereo2 ADC MIXL" },
	{ "VAD ADC Mux", "STO3 ADC MIX L", "Stereo3 ADC MIXL" },

	{ "VAD ADC INBUF Mux", "IN3P", "IN3P" },
	{ "VAD ADC INBUF Mux", "IN4P", "IN4P" },

	{ "INPUT BUF", NULL, "VAD ADC INBUF Mux" },
	{ "INPUT BUF", NULL, "INPUT BUF Power" },

	{ "VAD ADC DMIC Mux", "DMIC1", "DMIC1" },
	{ "VAD ADC DMIC Mux", "DMIC2", "DMIC2" },
	{ "VAD ADC DMIC Mux", "DMIC3", "DMIC3" },
	{ "VAD ADC DMIC Mux", "DMIC4", "DMIC4" },

	{ "VAD ADC FILTER Mux", "ADC", "ADC5" },
	{ "VAD ADC FILTER Mux", "DMIC", "VAD ADC DMIC Mux" },

	{ "IF1 ADC1 Mux", "STO1 ADC MIX", "Stereo1 ADC MIX" },
	{ "IF1 ADC1 Mux", "OB01", "OB01 Bypass Mux" },
	{ "IF1 ADC1 Mux", "OB01 MINI", "IB01 Mini Mux" },
	{ "IF1 ADC1 Mux", "VAD ADC", "VAD ADC Mux" },
	{ "IF1 ADC1 Mux", "IF1/2 DAC01", "IF2 DAC01" },
	{ "IF1 ADC1 Mux", "IF3 DAC", "IF3 DAC" },
	{ "IF1 ADC1 Mux", "IF4 DAC", "IF4 DAC" },
	{ "IF1 ADC1 Mux", "IF5 DAC", "IF5 DAC" },

	{ "IF1 ADC2 Mux", "STO2 ADC MIX", "Stereo2 ADC MIX" },
	{ "IF1 ADC2 Mux", "OB23", "OB23 Bypass Mux" },

	{ "IF1 ADC3 Mux", "MONO ADC MIX", "Mono ADC MIX" },
	{ "IF1 ADC3 Mux", "OB45", "OB45" },
	{ "IF1 ADC3 Mux", "DAC1 FS", "DAC1 FS" },
	{ "IF1 ADC3 Mux", "VAD ADC FILTER", "VAD ADC FILTER Mux" },

	{ "IF1 ADC4 Mux", "STO3 ADC MIX", "Stereo3 ADC MIX" },
	{ "IF1 ADC4 Mux", "DAC1 FS", "DAC1 FS" },
	{ "IF1 ADC4 Mux", "OB67", "OB67" },

	{ "IF1 ADC1 Swap Mux", "L/R", "IF1 ADC1 Mux" },
	{ "IF1 ADC1 Swap Mux", "R/L", "IF1 ADC1 Mux" },
	{ "IF1 ADC1 Swap Mux", "L/L", "IF1 ADC1 Mux" },
	{ "IF1 ADC1 Swap Mux", "R/R", "IF1 ADC1 Mux" },

	{ "IF1 ADC2 Swap Mux", "L/R", "IF1 ADC2 Mux" },
	{ "IF1 ADC2 Swap Mux", "R/L", "IF1 ADC2 Mux" },
	{ "IF1 ADC2 Swap Mux", "L/L", "IF1 ADC2 Mux" },
	{ "IF1 ADC2 Swap Mux", "R/R", "IF1 ADC2 Mux" },

	{ "IF1 ADC3 Swap Mux", "L/R", "IF1 ADC3 Mux" },
	{ "IF1 ADC3 Swap Mux", "R/L", "IF1 ADC3 Mux" },
	{ "IF1 ADC3 Swap Mux", "L/L", "IF1 ADC3 Mux" },
	{ "IF1 ADC3 Swap Mux", "R/R", "IF1 ADC3 Mux" },

	{ "IF1 ADC4 Swap Mux", "L/R", "IF1 ADC4 Mux" },
	{ "IF1 ADC4 Swap Mux", "R/L", "IF1 ADC4 Mux" },
	{ "IF1 ADC4 Swap Mux", "L/L", "IF1 ADC4 Mux" },
	{ "IF1 ADC4 Swap Mux", "R/R", "IF1 ADC4 Mux" },

	{ "IF1 ADC", NULL, "IF1 ADC1 Swap Mux" },
	{ "IF1 ADC", NULL, "IF1 ADC2 Swap Mux" },
	{ "IF1 ADC", NULL, "IF1 ADC3 Swap Mux" },
	{ "IF1 ADC", NULL, "IF1 ADC4 Swap Mux" },

	{ "IF1 ADC TDM Swap Mux", "1/2/3/4", "IF1 ADC" },
	{ "IF1 ADC TDM Swap Mux", "1/2/4/3", "IF1 ADC" },
	{ "IF1 ADC TDM Swap Mux", "1/3/2/4", "IF1 ADC" },
	{ "IF1 ADC TDM Swap Mux", "1/3/4/2", "IF1 ADC" },
	{ "IF1 ADC TDM Swap Mux", "1/4/3/2", "IF1 ADC" },
	{ "IF1 ADC TDM Swap Mux", "1/4/2/3", "IF1 ADC" },

	{ "IF1 ADC TDM Swap Mux", "2/1/3/4", "IF1 ADC" },
	{ "IF1 ADC TDM Swap Mux", "2/1/4/3", "IF1 ADC" },
	{ "IF1 ADC TDM Swap Mux", "2/3/1/4", "IF1 ADC" },
	{ "IF1 ADC TDM Swap Mux", "2/3/4/1", "IF1 ADC" },
	{ "IF1 ADC TDM Swap Mux", "2/4/3/1", "IF1 ADC" },
	{ "IF1 ADC TDM Swap Mux", "2/4/1/3", "IF1 ADC" },

	{ "IF1 ADC TDM Swap Mux", "3/1/2/4", "IF1 ADC" },
	{ "IF1 ADC TDM Swap Mux", "3/1/4/2", "IF1 ADC" },
	{ "IF1 ADC TDM Swap Mux", "3/2/1/4", "IF1 ADC" },
	{ "IF1 ADC TDM Swap Mux", "3/2/4/1", "IF1 ADC" },
	{ "IF1 ADC TDM Swap Mux", "3/4/2/1", "IF1 ADC" },
	{ "IF1 ADC TDM Swap Mux", "3/4/1/2", "IF1 ADC" },

	{ "IF1 ADC TDM Swap Mux", "4/1/2/3", "IF1 ADC" },
	{ "IF1 ADC TDM Swap Mux", "4/1/3/2", "IF1 ADC" },
	{ "IF1 ADC TDM Swap Mux", "4/2/1/3", "IF1 ADC" },
	{ "IF1 ADC TDM Swap Mux", "4/2/3/1", "IF1 ADC" },
	{ "IF1 ADC TDM Swap Mux", "4/3/2/1", "IF1 ADC" },
	{ "IF1 ADC TDM Swap Mux", "4/3/1/2", "IF1 ADC" },

	{ "AIF1TX", NULL, "I2S1" },
	{ "AIF1TX", NULL, "IF1 ADC TDM Swap Mux" },

	{ "IF2 ADC1 Mux", "STO1 ADC MIX", "Stereo1 ADC MIX" },
	{ "IF2 ADC1 Mux", "OB01", "OB01 Bypass Mux" },
	{ "IF2 ADC1 Mux", "OB01 MINI", "IB01 Mini Mux" },
	{ "IF2 ADC1 Mux", "VAD ADC", "VAD ADC Mux" },
	{ "IF2 ADC1 Mux", "IF1/2 DAC01", "IF1 DAC01" },
	{ "IF2 ADC1 Mux", "IF3 DAC", "IF3 DAC" },
	{ "IF2 ADC1 Mux", "IF4 DAC", "IF4 DAC" },
	{ "IF2 ADC1 Mux", "IF5 DAC", "IF5 DAC" },

	{ "IF2 ADC2 Mux", "STO2 ADC MIX", "Stereo2 ADC MIX" },
	{ "IF2 ADC2 Mux", "OB23", "OB23 Bypass Mux" },

	{ "IF2 ADC3 Mux", "MONO ADC MIX", "Mono ADC MIX" },
	{ "IF2 ADC3 Mux", "OB45", "OB45" },
	{ "IF2 ADC3 Mux", "DAC1 FS", "DAC1 FS" },

	{ "IF2 ADC4 Mux", "STO3 ADC MIX", "Stereo3 ADC MIX" },
	{ "IF2 ADC4 Mux", "DAC1 FS", "DAC1 FS" },
	{ "IF2 ADC4 Mux", "OB67", "OB67" },

	{ "IF2 ADC1 Swap Mux", "L/R", "IF2 ADC1 Mux" },
	{ "IF2 ADC1 Swap Mux", "R/L", "IF2 ADC1 Mux" },
	{ "IF2 ADC1 Swap Mux", "L/L", "IF2 ADC1 Mux" },
	{ "IF2 ADC1 Swap Mux", "R/R", "IF2 ADC1 Mux" },

	{ "IF2 ADC2 Swap Mux", "L/R", "IF2 ADC2 Mux" },
	{ "IF2 ADC2 Swap Mux", "R/L", "IF2 ADC2 Mux" },
	{ "IF2 ADC2 Swap Mux", "L/L", "IF2 ADC2 Mux" },
	{ "IF2 ADC2 Swap Mux", "R/R", "IF2 ADC2 Mux" },

	{ "IF2 ADC3 Swap Mux", "L/R", "IF2 ADC3 Mux" },
	{ "IF2 ADC3 Swap Mux", "R/L", "IF2 ADC3 Mux" },
	{ "IF2 ADC3 Swap Mux", "L/L", "IF2 ADC3 Mux" },
	{ "IF2 ADC3 Swap Mux", "R/R", "IF2 ADC3 Mux" },

	{ "IF2 ADC4 Swap Mux", "L/R", "IF2 ADC4 Mux" },
	{ "IF2 ADC4 Swap Mux", "R/L", "IF2 ADC4 Mux" },
	{ "IF2 ADC4 Swap Mux", "L/L", "IF2 ADC4 Mux" },
	{ "IF2 ADC4 Swap Mux", "R/R", "IF2 ADC4 Mux" },

	{ "IF2 ADC", NULL, "IF2 ADC1 Swap Mux" },
	{ "IF2 ADC", NULL, "IF2 ADC2 Swap Mux" },
	{ "IF2 ADC", NULL, "IF2 ADC3 Swap Mux" },
	{ "IF2 ADC", NULL, "IF2 ADC4 Swap Mux" },

	{ "IF2 ADC TDM Swap Mux", "1/2/3/4", "IF2 ADC" },
	{ "IF2 ADC TDM Swap Mux", "1/2/4/3", "IF2 ADC" },
	{ "IF2 ADC TDM Swap Mux", "1/3/2/4", "IF2 ADC" },
	{ "IF2 ADC TDM Swap Mux", "1/3/4/2", "IF2 ADC" },
	{ "IF2 ADC TDM Swap Mux", "1/4/3/2", "IF2 ADC" },
	{ "IF2 ADC TDM Swap Mux", "1/4/2/3", "IF2 ADC" },

	{ "IF2 ADC TDM Swap Mux", "2/1/3/4", "IF2 ADC" },
	{ "IF2 ADC TDM Swap Mux", "2/1/4/3", "IF2 ADC" },
	{ "IF2 ADC TDM Swap Mux", "2/3/1/4", "IF2 ADC" },
	{ "IF2 ADC TDM Swap Mux", "2/3/4/1", "IF2 ADC" },
	{ "IF2 ADC TDM Swap Mux", "2/4/3/1", "IF2 ADC" },
	{ "IF2 ADC TDM Swap Mux", "2/4/1/3", "IF2 ADC" },

	{ "IF2 ADC TDM Swap Mux", "3/1/2/4", "IF2 ADC" },
	{ "IF2 ADC TDM Swap Mux", "3/1/4/2", "IF2 ADC" },
	{ "IF2 ADC TDM Swap Mux", "3/2/1/4", "IF2 ADC" },
	{ "IF2 ADC TDM Swap Mux", "3/2/4/1", "IF2 ADC" },
	{ "IF2 ADC TDM Swap Mux", "3/4/2/1", "IF2 ADC" },
	{ "IF2 ADC TDM Swap Mux", "3/4/1/2", "IF2 ADC" },

	{ "IF2 ADC TDM Swap Mux", "4/1/2/3", "IF2 ADC" },
	{ "IF2 ADC TDM Swap Mux", "4/1/3/2", "IF2 ADC" },
	{ "IF2 ADC TDM Swap Mux", "4/2/1/3", "IF2 ADC" },
	{ "IF2 ADC TDM Swap Mux", "4/2/3/1", "IF2 ADC" },
	{ "IF2 ADC TDM Swap Mux", "4/3/2/1", "IF2 ADC" },
	{ "IF2 ADC TDM Swap Mux", "4/3/1/2", "IF2 ADC" },

	{ "AIF2TX", NULL, "I2S2" },
	{ "AIF2TX", NULL, "IF2 ADC TDM Swap Mux" },

	{ "IF3 ADC Mux", "STO1 ADC MIX", "Stereo1 ADC MIX" },
	{ "IF3 ADC Mux", "STO2 ADC MIX", "Stereo2 ADC MIX" },
	{ "IF3 ADC Mux", "STO3 ADC MIX", "Stereo3 ADC MIX" },
	{ "IF3 ADC Mux", "MONO ADC MIX", "Mono ADC MIX" },
	{ "IF3 ADC Mux", "OB01", "OB01 Bypass Mux" },
	{ "IF3 ADC Mux", "OB23", "OB23 Bypass Mux" },
	{ "IF3 ADC Mux", "OB45", "OB45" },
	{ "IF3 ADC Mux", "OB01 MINI", "IB01 Mini Mux" },
	{ "IF3 ADC Mux", "VAD ADC", "VAD ADC Mux" },
	{ "IF3 ADC Mux", "IF1 DAC01", "IF1 DAC01" },
	{ "IF3 ADC Mux", "IF2 DAC01", "IF2 DAC01" },
	{ "IF3 ADC Mux", "IF3/4 DAC", "IF4 DAC" },
	{ "IF3 ADC Mux", "IF4/5 DAC", "IF5 DAC" },
	{ "IF3 ADC Mux", "OB67", "OB67" },

	{ "IF3 ADC Swap Mux", "L/R", "IF3 ADC Mux" },
	{ "IF3 ADC Swap Mux", "R/L", "IF3 ADC Mux" },
	{ "IF3 ADC Swap Mux", "L/L", "IF3 ADC Mux" },
	{ "IF3 ADC Swap Mux", "R/R", "IF3 ADC Mux" },

	{ "AIF3TX", NULL, "I2S3" },
	{ "AIF3TX", NULL, "IF3 ADC Swap Mux" },

	{ "IF4 ADC Mux", "STO1 ADC MIX", "Stereo1 ADC MIX" },
	{ "IF4 ADC Mux", "STO2 ADC MIX", "Stereo2 ADC MIX" },
	{ "IF4 ADC Mux", "STO3 ADC MIX", "Stereo3 ADC MIX" },
	{ "IF4 ADC Mux", "MONO ADC MIX", "Mono ADC MIX" },
	{ "IF4 ADC Mux", "OB01", "OB01 Bypass Mux" },
	{ "IF4 ADC Mux", "OB23", "OB23 Bypass Mux" },
	{ "IF4 ADC Mux", "OB45", "OB45" },
	{ "IF4 ADC Mux", "OB01 MINI", "IB01 Mini Mux" },
	{ "IF4 ADC Mux", "VAD ADC", "VAD ADC Mux" },
	{ "IF4 ADC Mux", "IF1 DAC01", "IF1 DAC01" },
	{ "IF4 ADC Mux", "IF2 DAC01", "IF2 DAC01" },
	{ "IF4 ADC Mux", "IF3/4 DAC", "IF3 DAC" },
	{ "IF4 ADC Mux", "IF4/5 DAC", "IF5 DAC" },
	{ "IF4 ADC Mux", "OB67", "OB67" },

	{ "IF4 ADC Swap Mux", "L/R", "IF4 ADC Mux" },
	{ "IF4 ADC Swap Mux", "R/L", "IF4 ADC Mux" },
	{ "IF4 ADC Swap Mux", "L/L", "IF4 ADC Mux" },
	{ "IF4 ADC Swap Mux", "R/R", "IF4 ADC Mux" },

	{ "AIF4TX", NULL, "I2S4" },
	{ "AIF4TX", NULL, "IF4 ADC Swap Mux" },

	{ "IF5 ADC Mux", "STO1 ADC MIX", "Stereo1 ADC MIX" },
	{ "IF5 ADC Mux", "STO2 ADC MIX", "Stereo2 ADC MIX" },
	{ "IF5 ADC Mux", "STO3 ADC MIX", "Stereo3 ADC MIX" },
	{ "IF5 ADC Mux", "MONO ADC MIX", "Mono ADC MIX" },
	{ "IF5 ADC Mux", "OB01", "OB01 Bypass Mux" },
	{ "IF5 ADC Mux", "OB23", "OB23 Bypass Mux" },
	{ "IF5 ADC Mux", "OB45", "OB45" },
	{ "IF5 ADC Mux", "OB01 MINI", "IB01 Mini Mux" },
	{ "IF5 ADC Mux", "VAD ADC", "VAD ADC Mux" },
	{ "IF5 ADC Mux", "IF1 DAC01", "IF1 DAC01" },
	{ "IF5 ADC Mux", "IF2 DAC01", "IF2 DAC01" },
	{ "IF5 ADC Mux", "IF3/4 DAC", "IF3 DAC" },
	{ "IF5 ADC Mux", "IF4/5 DAC", "IF4 DAC" },
	{ "IF5 ADC Mux", "OB67", "OB67" },

	{ "IF5 ADC Swap Mux", "L/R", "IF5 ADC Mux" },
	{ "IF5 ADC Swap Mux", "R/L", "IF5 ADC Mux" },
	{ "IF5 ADC Swap Mux", "L/L", "IF5 ADC Mux" },
	{ "IF5 ADC Swap Mux", "R/R", "IF5 ADC Mux" },

	{ "AIF5TX", NULL, "I2S5" },
	{ "AIF5TX", NULL, "IF5 ADC Swap Mux" },

	{ "SLB ADC1 Mux", "STO1 ADC MIX", "Stereo1 ADC MIX" },
	{ "SLB ADC1 Mux", "OB01", "OB01 Bypass Mux" },
	{ "SLB ADC1 Mux", "OB01 MINI", "IB01 Mini Mux" },
	{ "SLB ADC1 Mux", "VAD ADC", "VAD ADC Mux" },

	{ "SLB ADC2 Mux", "STO2 ADC MIX", "Stereo2 ADC MIX" },
	{ "SLB ADC2 Mux", "OB23", "OB23 Bypass Mux" },

	{ "SLB ADC3 Mux", "MONO ADC MIX", "Mono ADC MIX" },
	{ "SLB ADC3 Mux", "OB45", "OB45" },
	{ "SLB ADC3 Mux", "DAC1 FS", "DAC1 FS" },

	{ "SLB ADC4 Mux", "STO3 ADC MIX", "Stereo3 ADC MIX" },
	{ "SLB ADC4 Mux", "DAC1 FS", "DAC1 FS" },
	{ "SLB ADC4 Mux", "OB67", "OB67" },

	{ "SLBTX", NULL, "SLB" },
	{ "SLBTX", NULL, "SLB ADC1 Mux" },
	{ "SLBTX", NULL, "SLB ADC2 Mux" },
	{ "SLBTX", NULL, "SLB ADC3 Mux" },
	{ "SLBTX", NULL, "SLB ADC4 Mux" },

	{ "IB01 Mux", "IF1 DAC01", "IF1 DAC01" },
	{ "IB01 Mux", "IF2 DAC01", "IF2 DAC01" },
	{ "IB01 Mux", "SLB DAC01", "SLB DAC01" },
	{ "IB01 Mux", "STO1 ADC MIX", "Stereo1 ADC MIX" },
	{ "IB01 Mux", "MONO ADC MIX", "Mono ADC MIX" },
	{ "IB01 Mux", "VAD ADC", "VAD ADC Mux" },
	{ "IB01 Mux", "DAC1 FS", "DAC1 FS" },
	{ "IB01 Mux", "VAD ADC FILTER", "VAD ADC FILTER Mux" },
	{ "IB01 Mux", "SPDIF IN", "SPDIF IN"},

	{ "IB01 Bypass Mux", "Bypass", "IB01 Mux" },
	{ "IB01 Bypass Mux", "Pass SRC", "IB01 Mux" },

	{ "IB23 Mux", "IF1 DAC23", "IF1 DAC23" },
	{ "IB23 Mux", "IF2 DAC23", "IF2 DAC23" },
	{ "IB23 Mux", "IF5 DAC", "IF5 DAC" },
	{ "IB23 Mux", "SLB DAC23", "SLB DAC23" },
	{ "IB23 Mux", "STO2 ADC MIX", "Stereo2 ADC MIX" },
	{ "IB23 Mux", "STO3 ADC MIX", "Stereo3 ADC MIX" },
	{ "IB23 Mux", "DAC1 FS", "DAC1 FS" },
	{ "IB23 Mux", "MONO ADC MIX", "Mono ADC MIX" },
	{ "IB23 Mux", "SPDIF IN", "SPDIF IN"},

	{ "IB23 Bypass Mux", "Bypass", "IB23 Mux" },
	{ "IB23 Bypass Mux", "Pass SRC", "IB23 Mux" },

	{ "IB45 Mux", "IF1 DAC45", "IF1 DAC45" },
	{ "IB45 Mux", "IF2 DAC45", "IF2 DAC45" },
	{ "IB45 Mux", "IF3 DAC", "IF3 DAC" },
	{ "IB45 Mux", "IF4 DAC", "IF4 DAC" },
	{ "IB45 Mux", "SLB DAC45", "SLB DAC45" },
	{ "IB45 Mux", "STO1 ADC MIX", "Stereo1 ADC MIX" },
	{ "IB45 Mux", "MONO ADC MIX", "Mono ADC MIX" },
	{ "IB45 Mux", "STO3 ADC MIX", "Stereo3 ADC MIX" },

	{ "IB45 Bypass Mux", "Bypass", "IB45 Mux" },
	{ "IB45 Bypass Mux", "Pass SRC", "IB45 Mux" },

	{ "IB6 Mux", "IF1 DAC 6", "IF1 DAC6 Mux" },
	{ "IB6 Mux", "IF2 DAC 6", "IF2 DAC6 Mux" },
	{ "IB6 Mux", "IF4 DAC L", "IF4 DAC L" },
	{ "IB6 Mux", "IF5 DAC L", "IF5 DAC L" },
	{ "IB6 Mux", "SLB DAC 6", "SLB DAC6" },
	{ "IB6 Mux", "MONO ADC MIX L", "Mono ADC MIXL ADC" },
	{ "IB6 Mux", "STO2 ADC MIX L", "Stereo2 ADC MIXL" },
	{ "IB6 Mux", "STO3 ADC MIX L", "Stereo3 ADC MIXL" },

	{ "IB7 Mux", "IF1 DAC 7", "IF1 DAC7 Mux" },
	{ "IB7 Mux", "IF2 DAC 7", "IF2 DAC7 Mux" },
	{ "IB7 Mux", "IF4 DAC R", "IF4 DAC R" },
	{ "IB7 Mux", "IF5 DAC R", "IF5 DAC R" },
	{ "IB7 Mux", "SLB DAC 7", "SLB DAC7" },
	{ "IB7 Mux", "MONO ADC MIX R", "Mono ADC MIXR ADC" },
	{ "IB7 Mux", "STO2 ADC MIX R", "Stereo2 ADC MIXR" },
	{ "IB7 Mux", "STO3 ADC MIX R", "Stereo3 ADC MIXR" },

	{ "IB01 Mini Mux", "VAD ADC", "VAD ADC Mux" },
	{ "IB01 Mini Mux", "IF1 DAC01", "IF1 DAC01" },
	{ "IB01 Mini Mux", "IF3 DAC", "IF3 DAC" },
	{ "IB01 Mini Mux", "SLB DAC01", "SLB DAC01" },
	{ "IB01 Mini Mux", "MONO ADC MIX", "Mono ADC MIX" },
	{ "IB01 Mini Mux", "STO1 ADC MIX", "Stereo1 ADC MIX" },
	{ "IB01 Mini Mux", "VAD ADC FILTER", "VAD ADC FILTER Mux" },
	{ "IB01 Mini Mux", "DAC1 FS", "DAC1 FS" },

	{ "OB01 MIX", NULL, "DSP event" },
	{ "OB23 MIX", NULL, "DSP event" },

	{ "OB01 MIX", "IB01 Switch", "IB01 Bypass Mux" },
	{ "OB01 MIX", "IB23 Switch", "IB23 Bypass Mux" },
	{ "OB01 MIX", "IB45 Switch", "IB45 Bypass Mux" },
	{ "OB01 MIX", "IB6 Switch", "IB6 Mux" },
	{ "OB01 MIX", "IB7 Switch", "IB7 Mux" },

	{ "OB23 MIX", "IB01 Switch", "IB01 Bypass Mux" },
	{ "OB23 MIX", "IB23 Switch", "IB23 Bypass Mux" },
	{ "OB23 MIX", "IB45 Switch", "IB45 Bypass Mux" },
	{ "OB23 MIX", "IB6 Switch", "IB6 Mux" },
	{ "OB23 MIX", "IB7 Switch", "IB7 Mux" },

	{ "OB4 MIX", "IB01 Switch", "IB01 Bypass Mux" },
	{ "OB4 MIX", "IB23 Switch", "IB23 Bypass Mux" },
	{ "OB4 MIX", "IB45 Switch", "IB45 Bypass Mux" },
	{ "OB4 MIX", "IB6 Switch", "IB6 Mux" },
	{ "OB4 MIX", "IB7 Switch", "IB7 Mux" },

	{ "OB5 MIX", "IB01 Switch", "IB01 Bypass Mux" },
	{ "OB5 MIX", "IB23 Switch", "IB23 Bypass Mux" },
	{ "OB5 MIX", "IB45 Switch", "IB45 Bypass Mux" },
	{ "OB5 MIX", "IB6 Switch", "IB6 Mux" },
	{ "OB5 MIX", "IB7 Switch", "IB7 Mux" },

	{ "OB6 MIX", "IB01 Switch", "IB01 Bypass Mux" },
	{ "OB6 MIX", "IB23 Switch", "IB23 Bypass Mux" },
	{ "OB6 MIX", "IB45 Switch", "IB45 Bypass Mux" },
	{ "OB6 MIX", "IB6 Switch", "IB6 Mux" },
	{ "OB6 MIX", "IB7 Switch", "IB7 Mux" },

	{ "OB7 MIX", "IB01 Switch", "IB01 Bypass Mux" },
	{ "OB7 MIX", "IB23 Switch", "IB23 Bypass Mux" },
	{ "OB7 MIX", "IB45 Switch", "IB45 Bypass Mux" },
	{ "OB7 MIX", "IB6 Switch", "IB6 Mux" },
	{ "OB7 MIX", "IB7 Switch", "IB7 Mux" },

	{ "OB01 Bypass Mux", "Bypass", "OB01 MIX" },
	{ "OB01 Bypass Mux", "Pass SRC", "OB01 MIX" },
	{ "OB23 Bypass Mux", "Bypass", "OB23 MIX" },
	{ "OB23 Bypass Mux", "Pass SRC", "OB23 MIX" },

	{ "OutBound2", NULL, "OB23 Bypass Mux" },
	{ "OutBound3", NULL, "OB23 Bypass Mux" },
	{ "OutBound4", NULL, "OB4 MIX" },
	{ "OutBound5", NULL, "OB5 MIX" },
	{ "OutBound6", NULL, "OB6 MIX" },
	{ "OutBound7", NULL, "OB7 MIX" },

	{ "OB45", NULL, "OutBound4" },
	{ "OB45", NULL, "OutBound5" },
	{ "OB67", NULL, "OutBound6" },
	{ "OB67", NULL, "OutBound7" },

	{ "IF1 DAC0", NULL, "AIF1RX" },
	{ "IF1 DAC1", NULL, "AIF1RX" },
	{ "IF1 DAC2", NULL, "AIF1RX" },
	{ "IF1 DAC3", NULL, "AIF1RX" },
	{ "IF1 DAC4", NULL, "AIF1RX" },
	{ "IF1 DAC5", NULL, "AIF1RX" },
	{ "IF1 DAC6", NULL, "AIF1RX" },
	{ "IF1 DAC7", NULL, "AIF1RX" },
	{ "IF1 DAC0", NULL, "I2S1" },
	{ "IF1 DAC1", NULL, "I2S1" },
	{ "IF1 DAC2", NULL, "I2S1" },
	{ "IF1 DAC3", NULL, "I2S1" },
	{ "IF1 DAC4", NULL, "I2S1" },
	{ "IF1 DAC5", NULL, "I2S1" },
	{ "IF1 DAC6", NULL, "I2S1" },
	{ "IF1 DAC7", NULL, "I2S1" },

	{ "IF1 DAC0 Mux", "Slot0", "IF1 DAC0" },
	{ "IF1 DAC0 Mux", "Slot1", "IF1 DAC1" },
	{ "IF1 DAC0 Mux", "Slot2", "IF1 DAC2" },
	{ "IF1 DAC0 Mux", "Slot3", "IF1 DAC3" },
	{ "IF1 DAC0 Mux", "Slot4", "IF1 DAC4" },
	{ "IF1 DAC0 Mux", "Slot5", "IF1 DAC5" },
	{ "IF1 DAC0 Mux", "Slot6", "IF1 DAC6" },
	{ "IF1 DAC0 Mux", "Slot7", "IF1 DAC7" },

	{ "IF1 DAC1 Mux", "Slot0", "IF1 DAC0" },
	{ "IF1 DAC1 Mux", "Slot1", "IF1 DAC1" },
	{ "IF1 DAC1 Mux", "Slot2", "IF1 DAC2" },
	{ "IF1 DAC1 Mux", "Slot3", "IF1 DAC3" },
	{ "IF1 DAC1 Mux", "Slot4", "IF1 DAC4" },
	{ "IF1 DAC1 Mux", "Slot5", "IF1 DAC5" },
	{ "IF1 DAC1 Mux", "Slot6", "IF1 DAC6" },
	{ "IF1 DAC1 Mux", "Slot7", "IF1 DAC7" },

	{ "IF1 DAC2 Mux", "Slot0", "IF1 DAC0" },
	{ "IF1 DAC2 Mux", "Slot1", "IF1 DAC1" },
	{ "IF1 DAC2 Mux", "Slot2", "IF1 DAC2" },
	{ "IF1 DAC2 Mux", "Slot3", "IF1 DAC3" },
	{ "IF1 DAC2 Mux", "Slot4", "IF1 DAC4" },
	{ "IF1 DAC2 Mux", "Slot5", "IF1 DAC5" },
	{ "IF1 DAC2 Mux", "Slot6", "IF1 DAC6" },
	{ "IF1 DAC2 Mux", "Slot7", "IF1 DAC7" },

	{ "IF1 DAC3 Mux", "Slot0", "IF1 DAC0" },
	{ "IF1 DAC3 Mux", "Slot1", "IF1 DAC1" },
	{ "IF1 DAC3 Mux", "Slot2", "IF1 DAC2" },
	{ "IF1 DAC3 Mux", "Slot3", "IF1 DAC3" },
	{ "IF1 DAC3 Mux", "Slot4", "IF1 DAC4" },
	{ "IF1 DAC3 Mux", "Slot5", "IF1 DAC5" },
	{ "IF1 DAC3 Mux", "Slot6", "IF1 DAC6" },
	{ "IF1 DAC3 Mux", "Slot7", "IF1 DAC7" },

	{ "IF1 DAC4 Mux", "Slot0", "IF1 DAC0" },
	{ "IF1 DAC4 Mux", "Slot1", "IF1 DAC1" },
	{ "IF1 DAC4 Mux", "Slot2", "IF1 DAC2" },
	{ "IF1 DAC4 Mux", "Slot3", "IF1 DAC3" },
	{ "IF1 DAC4 Mux", "Slot4", "IF1 DAC4" },
	{ "IF1 DAC4 Mux", "Slot5", "IF1 DAC5" },
	{ "IF1 DAC4 Mux", "Slot6", "IF1 DAC6" },
	{ "IF1 DAC4 Mux", "Slot7", "IF1 DAC7" },

	{ "IF1 DAC5 Mux", "Slot0", "IF1 DAC0" },
	{ "IF1 DAC5 Mux", "Slot1", "IF1 DAC1" },
	{ "IF1 DAC5 Mux", "Slot2", "IF1 DAC2" },
	{ "IF1 DAC5 Mux", "Slot3", "IF1 DAC3" },
	{ "IF1 DAC5 Mux", "Slot4", "IF1 DAC4" },
	{ "IF1 DAC5 Mux", "Slot5", "IF1 DAC5" },
	{ "IF1 DAC5 Mux", "Slot6", "IF1 DAC6" },
	{ "IF1 DAC5 Mux", "Slot7", "IF1 DAC7" },

	{ "IF1 DAC6 Mux", "Slot0", "IF1 DAC0" },
	{ "IF1 DAC6 Mux", "Slot1", "IF1 DAC1" },
	{ "IF1 DAC6 Mux", "Slot2", "IF1 DAC2" },
	{ "IF1 DAC6 Mux", "Slot3", "IF1 DAC3" },
	{ "IF1 DAC6 Mux", "Slot4", "IF1 DAC4" },
	{ "IF1 DAC6 Mux", "Slot5", "IF1 DAC5" },
	{ "IF1 DAC6 Mux", "Slot6", "IF1 DAC6" },
	{ "IF1 DAC6 Mux", "Slot7", "IF1 DAC7" },

	{ "IF1 DAC7 Mux", "Slot0", "IF1 DAC0" },
	{ "IF1 DAC7 Mux", "Slot1", "IF1 DAC1" },
	{ "IF1 DAC7 Mux", "Slot2", "IF1 DAC2" },
	{ "IF1 DAC7 Mux", "Slot3", "IF1 DAC3" },
	{ "IF1 DAC7 Mux", "Slot4", "IF1 DAC4" },
	{ "IF1 DAC7 Mux", "Slot5", "IF1 DAC5" },
	{ "IF1 DAC7 Mux", "Slot6", "IF1 DAC6" },
	{ "IF1 DAC7 Mux", "Slot7", "IF1 DAC7" },

	{ "IF1 DAC01", NULL, "IF1 DAC0 Mux" },
	{ "IF1 DAC01", NULL, "IF1 DAC1 Mux" },
	{ "IF1 DAC23", NULL, "IF1 DAC2 Mux" },
	{ "IF1 DAC23", NULL, "IF1 DAC3 Mux" },
	{ "IF1 DAC45", NULL, "IF1 DAC4 Mux" },
	{ "IF1 DAC45", NULL, "IF1 DAC5 Mux" },
	{ "IF1 DAC67", NULL, "IF1 DAC6 Mux" },
	{ "IF1 DAC67", NULL, "IF1 DAC7 Mux" },

	{ "IF2 DAC0", NULL, "AIF2RX" },
	{ "IF2 DAC1", NULL, "AIF2RX" },
	{ "IF2 DAC2", NULL, "AIF2RX" },
	{ "IF2 DAC3", NULL, "AIF2RX" },
	{ "IF2 DAC4", NULL, "AIF2RX" },
	{ "IF2 DAC5", NULL, "AIF2RX" },
	{ "IF2 DAC6", NULL, "AIF2RX" },
	{ "IF2 DAC7", NULL, "AIF2RX" },
	{ "IF2 DAC0", NULL, "I2S2" },
	{ "IF2 DAC1", NULL, "I2S2" },
	{ "IF2 DAC2", NULL, "I2S2" },
	{ "IF2 DAC3", NULL, "I2S2" },
	{ "IF2 DAC4", NULL, "I2S2" },
	{ "IF2 DAC5", NULL, "I2S2" },
	{ "IF2 DAC6", NULL, "I2S2" },
	{ "IF2 DAC7", NULL, "I2S2" },

	{ "IF2 DAC0 Mux", "Slot0", "IF2 DAC0" },
	{ "IF2 DAC0 Mux", "Slot1", "IF2 DAC1" },
	{ "IF2 DAC0 Mux", "Slot2", "IF2 DAC2" },
	{ "IF2 DAC0 Mux", "Slot3", "IF2 DAC3" },
	{ "IF2 DAC0 Mux", "Slot4", "IF2 DAC4" },
	{ "IF2 DAC0 Mux", "Slot5", "IF2 DAC5" },
	{ "IF2 DAC0 Mux", "Slot6", "IF2 DAC6" },
	{ "IF2 DAC0 Mux", "Slot7", "IF2 DAC7" },

	{ "IF2 DAC1 Mux", "Slot0", "IF2 DAC0" },
	{ "IF2 DAC1 Mux", "Slot1", "IF2 DAC1" },
	{ "IF2 DAC1 Mux", "Slot2", "IF2 DAC2" },
	{ "IF2 DAC1 Mux", "Slot3", "IF2 DAC3" },
	{ "IF2 DAC1 Mux", "Slot4", "IF2 DAC4" },
	{ "IF2 DAC1 Mux", "Slot5", "IF2 DAC5" },
	{ "IF2 DAC1 Mux", "Slot6", "IF2 DAC6" },
	{ "IF2 DAC1 Mux", "Slot7", "IF2 DAC7" },

	{ "IF2 DAC2 Mux", "Slot0", "IF2 DAC0" },
	{ "IF2 DAC2 Mux", "Slot1", "IF2 DAC1" },
	{ "IF2 DAC2 Mux", "Slot2", "IF2 DAC2" },
	{ "IF2 DAC2 Mux", "Slot3", "IF2 DAC3" },
	{ "IF2 DAC2 Mux", "Slot4", "IF2 DAC4" },
	{ "IF2 DAC2 Mux", "Slot5", "IF2 DAC5" },
	{ "IF2 DAC2 Mux", "Slot6", "IF2 DAC6" },
	{ "IF2 DAC2 Mux", "Slot7", "IF2 DAC7" },

	{ "IF2 DAC3 Mux", "Slot0", "IF2 DAC0" },
	{ "IF2 DAC3 Mux", "Slot1", "IF2 DAC1" },
	{ "IF2 DAC3 Mux", "Slot2", "IF2 DAC2" },
	{ "IF2 DAC3 Mux", "Slot3", "IF2 DAC3" },
	{ "IF2 DAC3 Mux", "Slot4", "IF2 DAC4" },
	{ "IF2 DAC3 Mux", "Slot5", "IF2 DAC5" },
	{ "IF2 DAC3 Mux", "Slot6", "IF2 DAC6" },
	{ "IF2 DAC3 Mux", "Slot7", "IF2 DAC7" },

	{ "IF2 DAC4 Mux", "Slot0", "IF2 DAC0" },
	{ "IF2 DAC4 Mux", "Slot1", "IF2 DAC1" },
	{ "IF2 DAC4 Mux", "Slot2", "IF2 DAC2" },
	{ "IF2 DAC4 Mux", "Slot3", "IF2 DAC3" },
	{ "IF2 DAC4 Mux", "Slot4", "IF2 DAC4" },
	{ "IF2 DAC4 Mux", "Slot5", "IF2 DAC5" },
	{ "IF2 DAC4 Mux", "Slot6", "IF2 DAC6" },
	{ "IF2 DAC4 Mux", "Slot7", "IF2 DAC7" },

	{ "IF2 DAC5 Mux", "Slot0", "IF2 DAC0" },
	{ "IF2 DAC5 Mux", "Slot1", "IF2 DAC1" },
	{ "IF2 DAC5 Mux", "Slot2", "IF2 DAC2" },
	{ "IF2 DAC5 Mux", "Slot3", "IF2 DAC3" },
	{ "IF2 DAC5 Mux", "Slot4", "IF2 DAC4" },
	{ "IF2 DAC5 Mux", "Slot5", "IF2 DAC5" },
	{ "IF2 DAC5 Mux", "Slot6", "IF2 DAC6" },
	{ "IF2 DAC5 Mux", "Slot7", "IF2 DAC7" },

	{ "IF2 DAC6 Mux", "Slot0", "IF2 DAC0" },
	{ "IF2 DAC6 Mux", "Slot1", "IF2 DAC1" },
	{ "IF2 DAC6 Mux", "Slot2", "IF2 DAC2" },
	{ "IF2 DAC6 Mux", "Slot3", "IF2 DAC3" },
	{ "IF2 DAC6 Mux", "Slot4", "IF2 DAC4" },
	{ "IF2 DAC6 Mux", "Slot5", "IF2 DAC5" },
	{ "IF2 DAC6 Mux", "Slot6", "IF2 DAC6" },
	{ "IF2 DAC6 Mux", "Slot7", "IF2 DAC7" },

	{ "IF2 DAC7 Mux", "Slot0", "IF2 DAC0" },
	{ "IF2 DAC7 Mux", "Slot1", "IF2 DAC1" },
	{ "IF2 DAC7 Mux", "Slot2", "IF2 DAC2" },
	{ "IF2 DAC7 Mux", "Slot3", "IF2 DAC3" },
	{ "IF2 DAC7 Mux", "Slot4", "IF2 DAC4" },
	{ "IF2 DAC7 Mux", "Slot5", "IF2 DAC5" },
	{ "IF2 DAC7 Mux", "Slot6", "IF2 DAC6" },
	{ "IF2 DAC7 Mux", "Slot7", "IF2 DAC7" },

	{ "IF2 DAC01", NULL, "IF2 DAC0 Mux" },
	{ "IF2 DAC01", NULL, "IF2 DAC1 Mux" },
	{ "IF2 DAC23", NULL, "IF2 DAC2 Mux" },
	{ "IF2 DAC23", NULL, "IF2 DAC3 Mux" },
	{ "IF2 DAC45", NULL, "IF2 DAC4 Mux" },
	{ "IF2 DAC45", NULL, "IF2 DAC5 Mux" },
	{ "IF2 DAC67", NULL, "IF2 DAC6 Mux" },
	{ "IF2 DAC67", NULL, "IF2 DAC7 Mux" },

	{ "IF3 DAC", NULL, "AIF3RX" },
	{ "IF3 DAC", NULL, "I2S3" },

	{ "IF4 DAC", NULL, "AIF4RX" },
	{ "IF4 DAC", NULL, "I2S4" },

	{ "IF5 DAC", NULL, "AIF5RX" },
	{ "IF5 DAC", NULL, "I2S5" },

	{ "IF3 DAC Swap Mux", "L/R", "IF3 DAC" },
	{ "IF3 DAC Swap Mux", "R/L", "IF3 DAC" },
	{ "IF3 DAC Swap Mux", "L/L", "IF3 DAC" },
	{ "IF3 DAC Swap Mux", "R/R", "IF3 DAC" },

	{ "IF4 DAC Swap Mux", "L/R", "IF4 DAC" },
	{ "IF4 DAC Swap Mux", "R/L", "IF4 DAC" },
	{ "IF4 DAC Swap Mux", "L/L", "IF4 DAC" },
	{ "IF4 DAC Swap Mux", "R/R", "IF4 DAC" },

	{ "IF5 DAC Swap Mux", "L/R", "IF5 DAC" },
	{ "IF5 DAC Swap Mux", "R/L", "IF5 DAC" },
	{ "IF5 DAC Swap Mux", "L/L", "IF5 DAC" },
	{ "IF5 DAC Swap Mux", "R/R", "IF5 DAC" },

	{ "IF3 DAC L", NULL, "IF3 DAC Swap Mux" },
	{ "IF3 DAC R", NULL, "IF3 DAC Swap Mux" },

	{ "IF4 DAC L", NULL, "IF4 DAC Swap Mux" },
	{ "IF4 DAC R", NULL, "IF4 DAC Swap Mux" },

	{ "IF5 DAC L", NULL, "IF5 DAC Swap Mux" },
	{ "IF5 DAC R", NULL, "IF5 DAC Swap Mux" },

	{ "SLB DAC0", NULL, "SLBRX" },
	{ "SLB DAC1", NULL, "SLBRX" },
	{ "SLB DAC2", NULL, "SLBRX" },
	{ "SLB DAC3", NULL, "SLBRX" },
	{ "SLB DAC4", NULL, "SLBRX" },
	{ "SLB DAC5", NULL, "SLBRX" },
	{ "SLB DAC6", NULL, "SLBRX" },
	{ "SLB DAC7", NULL, "SLBRX" },
	{ "SLB DAC0", NULL, "SLB" },
	{ "SLB DAC1", NULL, "SLB" },
	{ "SLB DAC2", NULL, "SLB" },
	{ "SLB DAC3", NULL, "SLB" },
	{ "SLB DAC4", NULL, "SLB" },
	{ "SLB DAC5", NULL, "SLB" },
	{ "SLB DAC6", NULL, "SLB" },
	{ "SLB DAC7", NULL, "SLB" },

	{ "SLB DAC01", NULL, "SLB DAC0" },
	{ "SLB DAC01", NULL, "SLB DAC1" },
	{ "SLB DAC23", NULL, "SLB DAC2" },
	{ "SLB DAC23", NULL, "SLB DAC3" },
	{ "SLB DAC45", NULL, "SLB DAC4" },
	{ "SLB DAC45", NULL, "SLB DAC5" },
	{ "SLB DAC67", NULL, "SLB DAC6" },
	{ "SLB DAC67", NULL, "SLB DAC7" },

	{ "ADDA1 Mux", "STO1 ADC MIX", "Stereo1 ADC MIX" },
	{ "ADDA1 Mux", "STO2 ADC MIX", "Stereo2 ADC MIX" },
	{ "ADDA1 Mux", "OB01", "OB01 Bypass Mux" },
	{ "ADDA1 Mux", "OB01 MINI", "IB01 Mini Mux" },

	{ "DAC1 Mux", "IF1 DAC01", "IF1 DAC01" },
	{ "DAC1 Mux", "IF2 DAC01", "IF2 DAC01" },
	{ "DAC1 Mux", "IF3 DAC", "IF3 DAC" },
	{ "DAC1 Mux", "IF4 DAC", "IF4 DAC" },
	{ "DAC1 Mux", "IF5 DAC", "IF5 DAC" },
	{ "DAC1 Mux", "SLB DAC01", "SLB DAC01" },
	{ "DAC1 Mux", "OB01", "OB01 Bypass Mux" },

	{ "DAC1 MIXL", "Stereo ADC Switch", "ADDA1 Mux" },
	{ "DAC1 MIXL", "DAC1 Switch", "DAC1 Mux" },
	{ "DAC1 MIXL", NULL, "dac stereo1 filter" },
	{ "DAC1 MIXR", "Stereo ADC Switch", "ADDA1 Mux" },
	{ "DAC1 MIXR", "DAC1 Switch", "DAC1 Mux" },
	{ "DAC1 MIXR", NULL, "dac stereo1 filter" },
	{ "dac stereo1 filter", NULL, "PLL1", rt5679_is_sys_clk_from_pll },

	{ "DAC1 FS", NULL, "DAC1 MIXL" },
	{ "DAC1 FS", NULL, "DAC1 MIXR" },

	{ "DAC2 L Mux", "IF1 DAC23", "IF1 DAC2 Mux" },
	{ "DAC2 L Mux", "IF2 DAC23", "IF2 DAC2 Mux" },
	{ "DAC2 L Mux", "IF3 DAC", "IF3 DAC L" },
	{ "DAC2 L Mux", "IF4 DAC", "IF4 DAC L" },
	{ "DAC2 L Mux", "IF5 DAC", "IF5 DAC L" },
	{ "DAC2 L Mux", "SLB DAC23", "SLB DAC2" },
	{ "DAC2 L Mux", "OB23", "OutBound2" },
	{ "DAC2 L Mux", "OB45", "OutBound4" },
	{ "DAC2 L Mux", "VAD ADC/HAPTIC GEN", "VAD ADC Mux" },
	{ "DAC2 L Mux", "MONO ADC MIX", "Mono ADC MIXL ADC" },
	{ "DAC2 L Mux", NULL, "dac mono2 left filter" },
	{ "dac mono2 left filter", NULL, "PLL1", rt5679_is_sys_clk_from_pll },

	{ "DAC2 R Mux", "IF1 DAC23", "IF1 DAC3 Mux" },
	{ "DAC2 R Mux", "IF2 DAC23", "IF2 DAC3 Mux" },
	{ "DAC2 R Mux", "IF3 DAC", "IF3 DAC R" },
	{ "DAC2 R Mux", "IF4 DAC", "IF4 DAC R" },
	{ "DAC2 R Mux", "IF5 DAC", "IF5 DAC R" },
	{ "DAC2 R Mux", "SLB DAC23", "SLB DAC3" },
	{ "DAC2 R Mux", "OB23", "OutBound3" },
	{ "DAC2 R Mux", "OB45", "OutBound3" },
	{ "DAC2 R Mux", "VAD ADC/HAPTIC GEN", "HAPTIC GEN" },
	{ "DAC2 R Mux", "MONO ADC MIX", "Mono ADC MIXR ADC" },
	{ "DAC2 R Mux", NULL, "dac mono2 right filter" },
	{ "dac mono2 right filter", NULL, "PLL1", rt5679_is_sys_clk_from_pll },

	{ "DAC3 L Mux", "IF1 DAC45", "IF1 DAC4 Mux" },
	{ "DAC3 L Mux", "IF2 DAC45", "IF2 DAC4 Mux" },
	{ "DAC3 L Mux", "IF3 DAC", "IF3 DAC L" },
	{ "DAC3 L Mux", "IF4 DAC", "IF4 DAC L" },
	{ "DAC3 L Mux", "IF5 DAC", "IF5 DAC L" },
	{ "DAC3 L Mux", "IF1 DAC67", "IF1 DAC6 Mux" },
	{ "DAC3 L Mux", "IF2 DAC67", "IF2 DAC6 Mux" },
	{ "DAC3 L Mux", "SLB DAC45", "SLB DAC4" },
	{ "DAC3 L Mux", "SLB DAC67", "SLB DAC6" },
	{ "DAC3 L Mux", "OB23", "OutBound2" },
	{ "DAC3 L Mux", "OB45", "OutBound4" },
	{ "DAC3 L Mux", "STO3 ADC MIX", "Stereo3 ADC MIXL" },
	{ "DAC3 L Mux", NULL, "dac mono3 left filter" },
	{ "dac mono3 left filter", NULL, "PLL1", rt5679_is_sys_clk_from_pll },

	{ "DAC3 R Mux", "IF1 DAC45", "IF1 DAC5 Mux" },
	{ "DAC3 R Mux", "IF2 DAC45", "IF2 DAC5 Mux" },
	{ "DAC3 R Mux", "IF3 DAC", "IF3 DAC R" },
	{ "DAC3 R Mux", "IF4 DAC", "IF4 DAC R" },
	{ "DAC3 R Mux", "IF5 DAC", "IF5 DAC R" },
	{ "DAC3 R Mux", "IF1 DAC67", "IF1 DAC7 Mux" },
	{ "DAC3 R Mux", "IF2 DAC67", "IF2 DAC7 Mux" },
	{ "DAC3 R Mux", "SLB DAC45", "SLB DAC5" },
	{ "DAC3 R Mux", "SLB DAC67", "SLB DAC7" },
	{ "DAC3 R Mux", "OB23", "OutBound3" },
	{ "DAC3 R Mux", "OB45", "OutBound5" },
	{ "DAC3 R Mux", "STO3 ADC MIX", "Stereo3 ADC MIXR" },
	{ "DAC3 R Mux", NULL, "dac mono3 right filter" },
	{ "dac mono3 right filter", NULL, "PLL1", rt5679_is_sys_clk_from_pll },

	{ "Sidetone Mux", "DMIC1L", "DMIC1L" },
	{ "Sidetone Mux", "DMIC2L", "DMIC2L" },
	{ "Sidetone Mux", "DMIC3L", "DMIC3L" },
	{ "Sidetone Mux", "DMIC4L", "DMIC4L" },
	{ "Sidetone Mux", "ADC1", "ADC1" },
	{ "Sidetone Mux", "ADC2", "ADC2" },
	{ "Sidetone Mux", "ADC3", "ADC3" },
	{ "Sidetone Mux", "ADC4", "ADC4" },

	{ "Stereo DAC MIXL", "ST L Switch", "Sidetone Mux" },
	{ "Stereo DAC MIXL", "DAC1 L Switch", "DAC1 MIXL" },
	{ "Stereo DAC MIXL", "DAC1 R Switch", "DAC1 MIXR" },
	{ "Stereo DAC MIXL", "DAC2 L Switch", "DAC2 L Mux" },
	{ "Stereo DAC MIXL", "DAC2 R Switch", "DAC2 R Mux" },
	{ "Stereo DAC MIXL", "DAC3 L Switch", "DAC3 L Mux" },
	{ "Stereo DAC MIXR", "ST R Switch", "Sidetone Mux" },
	{ "Stereo DAC MIXR", "DAC1 R Switch", "DAC1 MIXR" },
	{ "Stereo DAC MIXR", "DAC1 L Switch", "DAC1 MIXL" },
	{ "Stereo DAC MIXR", "DAC2 R Switch", "DAC2 R Mux" },
	{ "Stereo DAC MIXR", "DAC2 L Switch", "DAC2 L Mux" },
	{ "Stereo DAC MIXR", "DAC3 R Switch", "DAC3 R Mux" },

	{ "Mono DAC MIXL", "ST L Switch", "Sidetone Mux" },
	{ "Mono DAC MIXL", "DAC2 L Switch", "DAC2 L Mux" },
	{ "Mono DAC MIXL", "DAC1 L Switch", "DAC1 MIXL" },
	{ "Mono DAC MIXL", "DAC2 R Switch", "DAC2 R Mux" },
	{ "Mono DAC MIXL", "DAC3 L Switch", "DAC3 L Mux" },
	{ "Mono DAC MIXL", "DAC3 R Switch", "DAC3 R Mux" },
	{ "Mono DAC MIXR", "ST R Switch", "Sidetone Mux" },
	{ "Mono DAC MIXR", "DAC2 R Switch", "DAC2 R Mux" },
	{ "Mono DAC MIXR", "DAC1 R Switch", "DAC1 MIXR" },
	{ "Mono DAC MIXR", "DAC2 L Switch", "DAC2 L Mux" },
	{ "Mono DAC MIXR", "DAC3 R Switch", "DAC3 R Mux" },
	{ "Mono DAC MIXR", "DAC3 L Switch", "DAC3 L Mux" },

	{ "DD MIXL", "Sto DAC Mix L Switch", "DAC1 L Mixer Source Mux" },
	{ "DD MIXL", "Mono DAC Mix L Switch", "Mono DAC MIXL" },
	{ "DD MIXL", "DAC3 L Switch", "DAC3 L Mux" },
	{ "DD MIXL", "DAC3 R Switch", "DAC3 R Mux" },
	{ "DD MIXL", "DAC1 L Switch", "DAC1 MIXL" },
	{ "DD MIXL", "DAC2 L Switch", "DAC2 L Mux" },
	{ "DD MIXR", "Sto DAC Mix R Switch", "DAC1 R Mixer Source Mux" },
	{ "DD MIXR", "Mono DAC Mix R Switch", "Mono DAC MIXR" },
	{ "DD MIXR", "DAC3 L Switch", "DAC3 L Mux" },
	{ "DD MIXR", "DAC3 R Switch", "DAC3 R Mux" },
	{ "DD MIXR", "DAC1 R Switch", "DAC1 MIXR" },
	{ "DD MIXR", "DAC2 R Switch", "DAC2 R Mux" },

	{ "DAC1 L Mixer Source Mux", "DAC1", "DAC1 MIXL"},
	{ "DAC1 L Mixer Source Mux", "Mixer", "Stereo DAC MIXL"},
	{ "DAC1 R Mixer Source Mux", "DAC1", "DAC1 MIXR"},
	{ "DAC1 R Mixer Source Mux", "Mixer", "Stereo DAC MIXR"},

	{ "Stereo DAC MIX", NULL, "DAC1 L Mixer Source Mux" },
	{ "Stereo DAC MIX", NULL, "DAC1 R Mixer Source Mux" },
	{ "Mono DAC MIX", NULL, "Mono DAC MIXL" },
	{ "Mono DAC MIX", NULL, "Mono DAC MIXR" },
	{ "DD MIX", NULL, "DD MIXL" },
	{ "DD MIX", NULL, "DD MIXR" },

	{ "DAC12 Source Mux", "STO1 DAC MIX", "Stereo DAC MIX" },
	{ "DAC12 Source Mux", "MONO DAC MIX", "Mono DAC MIX" },
	{ "DAC12 Source Mux", "DD MIX", "DD MIX" },

	{ "DAC3 Source Mux", "STO1 DAC MIX", "DAC1 L Mixer Source Mux" },
	{ "DAC3 Source Mux", "MONO DAC MIX", "Mono DAC MIXL" },
	{ "DAC3 Source Mux", "DD MIX", "DD MIXL" },

	{ "DAC4 Source Mux", "STO1 DAC MIX", "DAC1 R Mixer Source Mux" },
	{ "DAC4 Source Mux", "MONO DAC MIX", "Mono DAC MIXR" },
	{ "DAC4 Source Mux", "DD MIX", "DD MIXR" },

	{ "DAC5 Source Mux", "STO1 DAC MIXL", "DAC1 L Mixer Source Mux" },
	{ "DAC5 Source Mux", "MONO DAC MIXL", "Mono DAC MIXL" },
	{ "DAC5 Source Mux", "MONO DAC MIXR", "Mono DAC MIXR" },

	{ "DAC 1", NULL, "DAC12 Source Mux" },
	{ "DAC 1", NULL, "DAC 1 Clock" },
	{ "DAC 1", NULL, "Vref" },
	{ "DAC 1", NULL, "LDO DACREF1" },
	{ "DAC 2", NULL, "DAC12 Source Mux" },
	{ "DAC 2", NULL, "DAC 2 Clock" },
	{ "DAC 2", NULL, "Vref" },
	{ "DAC 2", NULL, "LDO DACREF2" },
	{ "DAC 3", NULL, "DAC3 Source Mux" },
	{ "DAC 3", NULL, "DAC 3 Clock" },
	{ "DAC 3", NULL, "Vref" },
	{ "DAC 3", NULL, "LDO ADDA" },
	{ "DAC 4", NULL, "DAC4 Source Mux" },
	{ "DAC 4", NULL, "DAC 4 Clock" },
	{ "DAC 4", NULL, "Vref" },
	{ "DAC 4", NULL, "LDO ADDA" },
	{ "DAC 5", NULL, "DAC5 Source Mux" },
	{ "DAC 5", NULL, "DAC 5 Clock" },
	{ "DAC 5", NULL, "Vref" },
	{ "DAC 5", NULL, "LDO ADDA" },

	{ "PDM1 L Mux", "STO1 DAC MIX", "DAC1 L Mixer Source Mux" },
	{ "PDM1 L Mux", "MONO DAC MIX", "Mono DAC MIXL" },
	{ "PDM1 L Mux", "DD MIX", "DD MIXL" },
	{ "PDM1 L Mux", NULL, "PDM1 Power" },
	{ "PDM1 R Mux", "STO1 DAC MIX", "DAC1 R Mixer Source Mux" },
	{ "PDM1 R Mux", "MONO DAC MIX", "Mono DAC MIXR" },
	{ "PDM1 R Mux", "DD MIX", "DD MIXR" },
	{ "PDM1 R Mux", NULL, "PDM1 Power" },
	{ "PDM2 L Mux", "STO1 DAC MIX", "DAC1 L Mixer Source Mux" },
	{ "PDM2 L Mux", "MONO DAC MIX", "Mono DAC MIXL" },
	{ "PDM2 L Mux", "DD MIX", "DD MIXL" },
	{ "PDM2 L Mux", NULL, "PDM2 Power" },
	{ "PDM2 R Mux", "STO1 DAC MIX", "DAC1 R Mixer Source Mux" },
	{ "PDM2 R Mux", "MONO DAC MIX", "Mono DAC MIXR" },
	{ "PDM2 R Mux", "DD MIX", "DD MIXR" },
	{ "PDM2 R Mux", NULL, "PDM2 Power" },

	{ "HPL Amp", NULL, "DAC 1" },
	{ "HPR Amp", NULL, "DAC 2" },
	{ "HPOL", NULL, "HPL Amp" },
	{ "HPOR", NULL, "HPR Amp" },

	{ "LOUT1 Amp", NULL, "DAC 3" },
	{ "LOUT2 Amp", NULL, "DAC 4" },
	{ "LOUT1 Amp", NULL, "LOUT1 Power" },
	{ "LOUT2 Amp", NULL, "LOUT2 Power" },

	{ "LOUT1 Playback", "Switch", "LOUT1 Amp" },
	{ "LOUT2 Playback", "Switch", "LOUT2 Amp" },
	{ "LOUT1", NULL, "LOUT1 Playback" },
	{ "LOUT2", NULL, "LOUT2 Playback" },

	{ "Mono Amp", NULL, "Mono Power" },
	{ "Mono Amp", NULL, "DAC 5" },
	{ "MONOOUT", NULL, "Mono Amp" },

	{ "PDM1 L Playback", "Switch", "PDM1 L Mux" },
	{ "PDM1 R Playback", "Switch", "PDM1 R Mux" },
	{ "PDM2 L Playback", "Switch", "PDM2 L Mux" },
	{ "PDM2 R Playback", "Switch", "PDM2 R Mux" },
	{ "PDM1L", NULL, "PDM1 L Playback" },
	{ "PDM1R", NULL, "PDM1 R Playback" },
	{ "PDM2L", NULL, "PDM2 L Playback" },
	{ "PDM2R", NULL, "PDM2 R Playback" },
};

static int get_clk_info(int sclk, int rate)
{
	int i, pd[] = {1, 2, 3, 4, 6, 8, 12, 16};

	if (sclk <= 0 || rate <= 0)
		return -EINVAL;

	rate = rate << 8;
	for (i = 0; i < ARRAY_SIZE(pd); i++)
		if (sclk == rate * pd[i])
			return i;

	return -EINVAL;
}

static int rt5679_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);
	unsigned int val_len = 0, sdp_reg;
	unsigned int val_clk, mask_clk, clk_reg = 0;
	unsigned int val_clk2, mask_clk2, clk_reg2 = 0;
	int pre_div, bclk_ms, frame_size;

	rt5679->lrck[dai->id] = params_rate(params);
	pre_div = get_clk_info(rt5679->sysclk, rt5679->lrck[dai->id]);
	if (pre_div < 0) {
		dev_err(codec->dev, "Unsupported clock setting\n");
		return -EINVAL;
	}

	frame_size = snd_soc_params_to_frame_size(params);
	if (frame_size < 0) {
		dev_err(codec->dev, "Unsupported frame size: %d\n", frame_size);
		return -EINVAL;
	}

	bclk_ms = frame_size > 32;
	rt5679->bclk[dai->id] = rt5679->lrck[dai->id] * (32 << bclk_ms);

	dev_dbg(dai->dev, "bclk is %dHz and lrck is %dHz\n",
		rt5679->bclk[dai->id], rt5679->lrck[dai->id]);
	dev_dbg(dai->dev, "bclk_ms is %d and pre_div is %d for iis %d\n",
				bclk_ms, pre_div, dai->id);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		break;

	case SNDRV_PCM_FORMAT_S20_3LE:
		val_len |= RT5679_I2S_DL_20;
		break;

	case SNDRV_PCM_FORMAT_S24_LE:
		val_len |= RT5679_I2S_DL_24;
		break;

	case SNDRV_PCM_FORMAT_S8:
		val_len |= RT5679_I2S_DL_8;
		break;

	default:
		return -EINVAL;
	}

	switch (dai->id) {
	case RT5679_AIF1:
		if (rt5679->master[dai->id]) {
			clk_reg = RT5679_I2S_MASTER_CLK_CTRL1;
			mask_clk = RT5679_MASTER_I2S_DIV1_MASK;
			val_clk = pre_div << RT5679_MASTER_I2S_DIV1_SFT;

			switch (rt5679->sysclk_src) {
			case RT5679_SCLK_S_PLL1:
				mask_clk |= RT5679_SEL_M_CLK_I2S1_MASK;
				val_clk |= RT5679_SEL_M_CLK_I2S1_PLL1;
				break;
			case RT5679_SCLK_S_RCCLK:
				mask_clk |= RT5679_SEL_M_CLK_I2S1_MASK;
				val_clk |= RT5679_SEL_M_CLK_I2S1_RCCLK;
				break;
			default:
				break;
			}
		} else {
			clk_reg = RT5679_CLK_TREE_CTRL1;
			mask_clk = RT5679_I2S_PD1_MASK;
			val_clk = pre_div << RT5679_I2S_PD1_SFT;
		}
		sdp_reg = RT5679_I2S1_SDP;
		break;

	case RT5679_AIF2:
		if (rt5679->master[dai->id]) {
			clk_reg = RT5679_I2S_MASTER_CLK_CTRL1;
			mask_clk = RT5679_MASTER_I2S_DIV2_MASK;
			val_clk = pre_div << RT5679_MASTER_I2S_DIV2_SFT;

			switch (rt5679->sysclk_src) {
			case RT5679_SCLK_S_PLL1:
				mask_clk |= RT5679_SEL_M_CLK_I2S2_MASK;
				val_clk |= RT5679_SEL_M_CLK_I2S2_PLL1;
				break;
			case RT5679_SCLK_S_RCCLK:
				mask_clk |= RT5679_SEL_M_CLK_I2S2_MASK;
				val_clk |= RT5679_SEL_M_CLK_I2S2_RCCLK;
				break;
			default:
				break;
			}
		} else {
			clk_reg = RT5679_CLK_TREE_CTRL1;
			mask_clk = RT5679_I2S_PD2_MASK;
			val_clk = pre_div << RT5679_I2S_PD2_SFT;
		}
		sdp_reg = RT5679_I2S2_SDP;
		break;

	case RT5679_AIF3:
		clk_reg = RT5679_CLK_TREE_CTRL1;
		mask_clk = RT5679_I2S_BCLK_MS3_MASK;
		val_clk = bclk_ms << RT5679_I2S_BCLK_MS3_SFT;
		if (rt5679->master[dai->id]) {
			clk_reg2 = RT5679_I2S_MASTER_CLK_CTRL2;
			mask_clk2 = RT5679_MASTER_I2S_DIV3_MASK;
			val_clk2 = pre_div << RT5679_MASTER_I2S_DIV3_SFT;

			switch (rt5679->sysclk_src) {
			case RT5679_SCLK_S_PLL1:
				mask_clk |= RT5679_SEL_M_CLK_I2S3_MASK;
				val_clk |= RT5679_SEL_M_CLK_I2S3_PLL1;
				break;
			case RT5679_SCLK_S_RCCLK:
				mask_clk |= RT5679_SEL_M_CLK_I2S3_MASK;
				val_clk |= RT5679_SEL_M_CLK_I2S3_RCCLK;
				break;
			default:
				break;
			}
		} else {
			mask_clk |= RT5679_I2S_PD3_MASK;
			val_clk |= pre_div << RT5679_I2S_PD3_SFT;
		}
		sdp_reg = RT5679_I2S3_SDP;
		break;

	case RT5679_AIF4:
		clk_reg = RT5679_CLK_TREE_CTRL1;
		mask_clk = RT5679_I2S_BCLK_MS4_MASK;
		val_clk = bclk_ms << RT5679_I2S_BCLK_MS4_SFT;
		if (rt5679->master[dai->id]) {
			clk_reg2 = RT5679_I2S_MASTER_CLK_CTRL2;
			mask_clk2 = RT5679_MASTER_I2S_DIV4_MASK;
			val_clk2 = pre_div << RT5679_MASTER_I2S_DIV4_SFT;

			switch (rt5679->sysclk_src) {
			case RT5679_SCLK_S_PLL1:
				mask_clk |= RT5679_SEL_M_CLK_I2S4_MASK;
				val_clk |= RT5679_SEL_M_CLK_I2S4_PLL1;
				break;
			case RT5679_SCLK_S_RCCLK:
				mask_clk |= RT5679_SEL_M_CLK_I2S4_MASK;
				val_clk |= RT5679_SEL_M_CLK_I2S4_RCCLK;
				break;
			default:
				break;
			}
		} else {
			mask_clk |= RT5679_I2S_PD4_MASK;
			val_clk |= pre_div << RT5679_I2S_PD4_SFT;
		}
		sdp_reg = RT5679_I2S4_SDP;
		break;

	case RT5679_AIF5:
		clk_reg = RT5679_CLK_TREE_CTRL2;
		mask_clk = RT5679_I2S_BCLK_MS5_MASK;
		val_clk = bclk_ms << RT5679_I2S_BCLK_MS5_SFT;
		if (rt5679->master[dai->id]) {
			clk_reg2 = RT5679_I2S_MASTER_CLK_CTRL3;
			mask_clk2 = RT5679_MASTER_I2S_DIV5_MASK;
			val_clk2 = pre_div << RT5679_MASTER_I2S_DIV5_SFT;

			switch (rt5679->sysclk_src) {
			case RT5679_SCLK_S_PLL1:
				mask_clk |= RT5679_SEL_M_CLK_I2S5_MASK;
				val_clk |= RT5679_SEL_M_CLK_I2S5_PLL1;
				break;
			case RT5679_SCLK_S_RCCLK:
				mask_clk |= RT5679_SEL_M_CLK_I2S5_MASK;
				val_clk |= RT5679_SEL_M_CLK_I2S5_RCCLK;
				break;
			default:
				break;
			}
		} else {
			mask_clk |= RT5679_I2S_PD5_MASK;
			val_clk |= pre_div << RT5679_I2S_PD5_SFT;
		}
		sdp_reg = RT5679_I2S5_SDP;
		break;

	default:
		break;
	}

	if (clk_reg) {
		regmap_update_bits(rt5679->regmap, sdp_reg, RT5679_I2S_DL_MASK,
			val_len);
		regmap_update_bits(rt5679->regmap, clk_reg, mask_clk, val_clk);
	}

	if (clk_reg2)
		regmap_update_bits(rt5679->regmap, clk_reg2, mask_clk2,
			val_clk2);

	return 0;
}

static int rt5679_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);

	rt5679->aif_pu = dai->id;

	return 0;
}

static int rt5679_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = dai->codec;
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);
	unsigned int reg_val = 0, sdp_reg = 0;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		rt5679->master[dai->id] = 1;
		break;

	case SND_SOC_DAIFMT_CBS_CFS:
		reg_val |= RT5679_I2S_MS_S;
		rt5679->master[dai->id] = 0;
		break;

	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;

	case SND_SOC_DAIFMT_IB_NF:
		reg_val |= RT5679_I2S_BP_INV;
		break;

	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		break;

	case SND_SOC_DAIFMT_LEFT_J:
		reg_val |= RT5679_I2S_DF_LEFT;
		break;

	case SND_SOC_DAIFMT_DSP_A:
		reg_val |= RT5679_I2S_DF_PCM_A;
		break;

	case SND_SOC_DAIFMT_DSP_B:
		reg_val |= RT5679_I2S_DF_PCM_B;
		break;

	default:
		return -EINVAL;
	}

	switch (dai->id) {
	case RT5679_AIF1:
		sdp_reg = RT5679_I2S1_SDP;
		break;

	case RT5679_AIF2:
		sdp_reg = RT5679_I2S2_SDP;
		break;

	case RT5679_AIF3:
		sdp_reg = RT5679_I2S3_SDP;
		break;

	case RT5679_AIF4:
		sdp_reg = RT5679_I2S4_SDP;
		break;

	case RT5679_AIF5:
		sdp_reg = RT5679_I2S5_SDP;
		break;

	default:
		break;
	}

	if (sdp_reg)
		regmap_update_bits(rt5679->regmap, sdp_reg,
			RT5679_I2S_MS_MASK | RT5679_I2S_BP_MASK |
			RT5679_I2S_DF_MASK, reg_val);

	return 0;
}

static int rt5679_set_dai_sysclk(struct snd_soc_dai *dai,
		int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = dai->codec;
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);
	unsigned int reg_val = 0;

	if (freq == rt5679->sysclk && clk_id == rt5679->sysclk_src)
		return 0;

	switch (clk_id) {
	case RT5679_SCLK_S_MCLK:
		reg_val |= RT5679_SCLK_SRC_MCLK1;
		break;

	case RT5679_SCLK_S_PLL1:
		reg_val |= RT5679_SCLK_SRC_PLL1;
		break;

	case RT5679_SCLK_S_RCCLK:
		reg_val |= RT5679_SCLK_SRC_RCCLK;
		break;

	default:
		dev_err(codec->dev, "Invalid clock id (%d)\n", clk_id);
		return -EINVAL;
	}
	regmap_update_bits(rt5679->regmap, RT5679_GLB_CLK1,
		RT5679_SCLK_SRC_MASK, reg_val);
	rt5679->sysclk = freq;
	rt5679->sysclk_src = clk_id;

	dev_dbg(dai->dev, "Sysclk is %dHz and clock id is %d\n", freq, clk_id);

	return 0;
}

/**
 * rt5679_pll_calc - Calcualte PLL M/N/K code.
 * @freq_in: external clock provided to codec.
 * @freq_out: target clock which codec works on.
 * @pll_code: Pointer to structure with M, N, K, bypass K and bypass M flag.
 *
 * Calcualte M/N/K code and bypass K/M flag to configure PLL for codec.
 *
 * Returns 0 for success or negative error code.
 */
static int rt5679_pll_calc(const unsigned int freq_in,
	const unsigned int freq_out, struct rt5679_pll_code *pll_code)
{
	int max_n = RT5679_PLL_N_MAX, max_m = RT5679_PLL_M_MAX;
	int k, n = 0, m = 0, red, n_t, m_t, pll_out, in_t;
	int out_t, red_t = abs(freq_out - freq_in);
	bool m_bypass = false, k_bypass = false;

	if (RT5679_PLL_INP_MAX < freq_in || RT5679_PLL_INP_MIN > freq_in)
		return -EINVAL;

	k = 100000000 / freq_out - 2;
	if (k > RT5679_PLL_K_MAX)
		k = RT5679_PLL_K_MAX;
	if (k < 0) {
		k = 0;
		k_bypass = true;
	}
	for (n_t = 0; n_t <= max_n; n_t++) {
		in_t = freq_in / (k_bypass ? 1 : (k + 2));
		pll_out = freq_out / (n_t + 2);
		if (in_t < 0)
			continue;
		if (in_t == pll_out) {
			m_bypass = true;
			n = n_t;
			goto code_find;
		}
		red = abs(in_t - pll_out);
		if (red < red_t) {
			m_bypass = true;
			n = n_t;
			m = m_t;
			if (red == 0)
				goto code_find;
			red_t = red;
		}
		for (m_t = 0; m_t <= max_m; m_t++) {
			out_t = in_t / (m_t + 2);
			red = abs(out_t - pll_out);
			if (red < red_t) {
				m_bypass = false;
				n = n_t;
				m = m_t;
				if (red == 0)
					goto code_find;
				red_t = red;
			}
		}
	}
	pr_debug("Only get approximation about PLL\n");

code_find:

	pll_code->m_bp = m_bypass;
	pll_code->k_bp = k_bypass;
	pll_code->m_code = m;
	pll_code->n_code = n;
	pll_code->k_code = k;

	return 0;
}

static int rt5679_set_dai_pll(struct snd_soc_dai *dai, int pll_id, int source,
			unsigned int freq_in, unsigned int freq_out)
{
	struct snd_soc_codec *codec = dai->codec;
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);
	struct rt5679_pll_code pll_code;
	int ret;

	if (!freq_in || !freq_out) {
		dev_dbg(codec->dev, "PLL disabled\n");

		rt5679->pll_in = 0;
		rt5679->pll_out = 0;
		regmap_update_bits(rt5679->regmap, RT5679_GLB_CLK1,
			RT5679_SCLK_SRC_MASK, RT5679_SCLK_SRC_MCLK1);
		return 0;
	}

	if (source == rt5679->pll_src && freq_in == rt5679->pll_in &&
	    freq_out == rt5679->pll_out)
		return 0;

	switch (source) {
	case RT5679_PLL1_S_MCLK:
		regmap_update_bits(rt5679->regmap, RT5679_GLB_CLK1,
			RT5679_PLL1_SRC_MASK, RT5679_PLL1_SRC_MCLK);
		break;

	case RT5679_PLL1_S_BCLK1:
	case RT5679_PLL1_S_BCLK2:
	case RT5679_PLL1_S_BCLK3:
	case RT5679_PLL1_S_BCLK4:
	case RT5679_PLL1_S_BCLK5:
		switch (dai->id) {
		case RT5679_AIF1:
			regmap_update_bits(rt5679->regmap, RT5679_GLB_CLK1,
				RT5679_PLL1_SRC_MASK, RT5679_PLL1_SRC_BCLK1);
			break;

		case RT5679_AIF2:
			regmap_update_bits(rt5679->regmap, RT5679_GLB_CLK1,
				RT5679_PLL1_SRC_MASK, RT5679_PLL1_SRC_BCLK2);
			break;

		case RT5679_AIF3:
			regmap_update_bits(rt5679->regmap, RT5679_GLB_CLK1,
				RT5679_PLL1_SRC_MASK, RT5679_PLL1_SRC_BCLK3);
			break;

		case RT5679_AIF4:
			regmap_update_bits(rt5679->regmap, RT5679_GLB_CLK1,
				RT5679_PLL1_SRC_MASK, RT5679_PLL1_SRC_BCLK4);
			break;

		case RT5679_AIF5:
			regmap_update_bits(rt5679->regmap, RT5679_GLB_CLK1,
				RT5679_PLL1_SRC_MASK, RT5679_PLL1_SRC_BCLK5);
			break;

		default:
			break;
		}
		break;

	default:
		dev_err(codec->dev, "Unknown PLL source %d\n", source);
		return -EINVAL;
	}

	ret = rt5679_pll_calc(freq_in, freq_out, &pll_code);
	if (ret < 0) {
		dev_err(codec->dev, "Unsupport input clock %d\n", freq_in);
		return ret;
	}

	dev_dbg(codec->dev, "m_bypass=%d k_bypass=%d m=%d n=%d k=%d\n",
		pll_code.m_bp, pll_code.k_bp,
		(pll_code.m_bp ? 0 : pll_code.m_code), pll_code.n_code,
		(pll_code.k_bp ? 0 : pll_code.k_code));

	regmap_write(rt5679->regmap, RT5679_PLL1_CTRL1,
		pll_code.n_code << RT5679_PLL_N_SFT |
		pll_code.k_bp << RT5679_PLL_K_BP_SFT |
		(pll_code.k_bp ? 0 : pll_code.k_code));
	regmap_write(rt5679->regmap, RT5679_PLL1_CTRL2,
		(pll_code.m_bp ? 0 : pll_code.m_code) << RT5679_PLL_M_SFT |
		pll_code.m_bp << RT5679_PLL_M_BP_SFT);

	rt5679->pll_in = freq_in;
	rt5679->pll_out = freq_out;
	rt5679->pll_src = source;

	return 0;
}

static int rt5679_set_tdm_slot(struct snd_soc_dai *dai, unsigned int tx_mask,
			unsigned int rx_mask, int slots, int slot_width)
{
	struct snd_soc_codec *codec = dai->codec;
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);
	unsigned int val = 0, reg = 0, mask;

	if (rx_mask || tx_mask) {
		switch (dai->id) {
		case RT5679_AIF1:
			regmap_update_bits(rt5679->regmap, RT5679_TDM1_CTRL1,
				RT5679_TDM_TDM_MODE, RT5679_TDM_TDM_MODE);
			break;

		case RT5679_AIF2:
			regmap_update_bits(rt5679->regmap, RT5679_TDM2_CTRL1,
				RT5679_TDM_TDM_MODE, RT5679_TDM_TDM_MODE);
			break;

		default:
			break;
		}

		if (rt5679->master[dai->id])
			val |= RT5679_TDM_M_TDM_MODE;
	}

	switch (slots) {
	case 4:
		if (rt5679->master[dai->id])
			val |= RT5679_TDM_M_SLOT_SEL_4CH;
		else
			val |= RT5679_TDM_IN_SLOT_SEL_4CH |
				RT5679_TDM_OUT_SLOT_SEL_4CH;
		break;

	case 6:
		if (rt5679->master[dai->id])
			val |= RT5679_TDM_M_SLOT_SEL_6CH;
		else
			val |= RT5679_TDM_IN_SLOT_SEL_6CH |
				RT5679_TDM_OUT_SLOT_SEL_6CH;
		break;

	case 8:
		if (rt5679->master[dai->id])
			val |= RT5679_TDM_M_SLOT_SEL_8CH;
		else
			val |= RT5679_TDM_IN_SLOT_SEL_8CH |
				RT5679_TDM_OUT_SLOT_SEL_8CH;
		break;

	case 2:
	default:
		break;
	}

	switch (slot_width) {
	case 20:
		if (rt5679->master[dai->id])
			val |= RT5679_TDM_M_LEN_20;
		else
			val |= RT5679_TDM_IN_LEN_20 | RT5679_TDM_OUT_LEN_20;
		break;

	case 24:
		if (rt5679->master[dai->id])
			val |= RT5679_TDM_M_LEN_24;
		else
			val |= RT5679_TDM_IN_LEN_24 | RT5679_TDM_OUT_LEN_24;
		break;

	case 32:
		if (rt5679->master[dai->id])
			val |= RT5679_TDM_M_LEN_32;
		else
			val |= RT5679_TDM_IN_LEN_32 | RT5679_TDM_OUT_LEN_32;
		break;

	case 16:
	default:
		break;
	}

	switch (dai->id) {
	case RT5679_AIF1:
		if (rt5679->master[RT5679_AIF1]) {
			reg = RT5679_TDM1_CTRL7;
			mask = RT5679_TDM_M_TDM_MODE |
				RT5679_TDM_M_SLOT_SEL_MASK |
				RT5679_TDM_M_LEN_MASK;
		} else {
			reg = RT5679_TDM1_CTRL1;
			mask = RT5679_TDM_IN_SLOT_SEL_MASK |
				RT5679_TDM_OUT_SLOT_SEL_MASK |
				RT5679_TDM_IN_LEN_MASK |
				RT5679_TDM_OUT_LEN_MASK;
		}
		break;

	case RT5679_AIF2:
		if (rt5679->master[RT5679_AIF2]) {
			reg = RT5679_TDM2_CTRL7;
			mask = RT5679_TDM_M_TDM_MODE |
				RT5679_TDM_M_SLOT_SEL_MASK |
				RT5679_TDM_M_LEN_MASK;
		} else {
			reg = RT5679_TDM1_CTRL2;
			mask = RT5679_TDM_IN_SLOT_SEL_MASK |
				RT5679_TDM_OUT_SLOT_SEL_MASK |
				RT5679_TDM_IN_LEN_MASK |
				RT5679_TDM_OUT_LEN_MASK;
		}
		break;

	default:
		break;
	}

	if (reg)
		regmap_update_bits(rt5679->regmap, reg, mask, val);

	return 0;
}

static ssize_t rt5679_codec_show_range(struct rt5679_priv *rt5679,
	char *buf, int start, int end)
{
	unsigned int val;
	int cnt = 0, i;

	for (i = start; i <= end; i++) {
		if (cnt + RT5679_REG_DISP_LEN >= PAGE_SIZE)
			break;

		if (rt5679_readable_register(NULL, i)) {
			regmap_read(rt5679->regmap, i, &val);

			cnt += snprintf(buf + cnt, RT5679_REG_DISP_LEN,
					"%04x: %04x\n", i, val);
		}
	}

	if (cnt >= PAGE_SIZE)
		cnt = PAGE_SIZE - 1;

	return cnt;
}

static ssize_t rt5679_codec_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct rt5679_priv *rt5679 = i2c_get_clientdata(client);

	return rt5679_codec_show_range(rt5679, buf, rt5679->reg_page << 8,
		(rt5679->reg_page << 8) | 0xff);
}

static ssize_t rt5679_codec_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct rt5679_priv *rt5679 = i2c_get_clientdata(client);
	unsigned int val = 0, addr = 0;
	int i;

	if (buf[0] == 'P' || buf[0] == 'p') {
		rt5679->reg_page = buf[1] - '0';
		return count;
	}

	pr_info("register \"%s\" count = %zu\n", buf, count);
	for (i = 0; i < count; i++) {
		if (*(buf + i) <= '9' && *(buf + i) >= '0')
			addr = (addr << 4) | (*(buf + i) - '0');
		else if (*(buf + i) <= 'f' && *(buf + i) >= 'a')
			addr = (addr << 4) | ((*(buf + i) - 'a') + 0xa);
		else if (*(buf + i) <= 'F' && *(buf + i) >= 'A')
			addr = (addr << 4) | ((*(buf + i)-'A') + 0xa);
		else
			break;
	}

	for (i = i + 1; i < count; i++) {
		if (*(buf + i) <= '9' && *(buf + i) >= '0')
			val = (val << 4) | (*(buf + i) - '0');
		else if (*(buf + i) <= 'f' && *(buf + i) >= 'a')
			val = (val << 4) | ((*(buf + i) - 'a') + 0xa);
		else if (*(buf + i) <= 'F' && *(buf + i) >= 'A')
			val = (val << 4) | ((*(buf + i) - 'A') + 0xa);
		else
			break;
	}

	pr_info("addr = 0x%04x val = 0x%04x\n", addr, val);
	if (addr > RT5679_DUMMY_REG_4 || val > 0xffff || val < 0)
		return count;

	if (i == count) {
		regmap_read(rt5679->regmap, addr, &val);
		pr_info("0x%04x = 0x%04x\n", addr, val);
	} else
		regmap_write(rt5679->regmap, addr, val);

	return count;
}
static DEVICE_ATTR(codec_reg, 0644, rt5679_codec_show, rt5679_codec_store);

static ssize_t rt5679_is_dsp_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct rt5679_priv *rt5679 = i2c_get_clientdata(client);

	return snprintf(buf, 3, "%c\n", rt5679->is_dsp_mode ? 'Y' : 'N');
}
static DEVICE_ATTR(is_dsp_mode, 0444, rt5679_is_dsp_mode_show, NULL);

static ssize_t rt5679_codec_adb_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct rt5679_priv *rt5679 = i2c_get_clientdata(client);
	struct snd_soc_codec *codec = rt5679->codec;
	unsigned int val;
	int cnt = 0, i;

	for (i = 0; i < rt5679->adb_reg_num; i++) {
		if (cnt + RT5679_REG_DISP_LEN >= PAGE_SIZE)
			break;
		val = snd_soc_read(codec, rt5679->adb_reg_addr[i]);
		cnt += snprintf(buf + cnt, RT5679_REG_DISP_LEN, "%04x: %04x\n",
			rt5679->adb_reg_addr[i], val);
	}

	return cnt;
}

static ssize_t rt5679_codec_adb_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct rt5679_priv *rt5679 = i2c_get_clientdata(client);
	struct snd_soc_codec *codec = rt5679->codec;
	unsigned int value = 0;
	int i = 2, j = 0;

	if (buf[0] == 'R' || buf[0] == 'r') {
		while (j <= 0x100 && i < count) {
			rt5679->adb_reg_addr[j] = 0;
			value = 0;
			for ( ; i < count; i++) {
				if (*(buf + i) <= '9' && *(buf + i) >= '0')
					value = (value << 4) | (*(buf + i) - '0');
				else if (*(buf + i) <= 'f' && *(buf + i) >= 'a')
					value = (value << 4) | ((*(buf + i) - 'a')+0xa);
				else if (*(buf + i) <= 'F' && *(buf + i) >= 'A')
					value = (value << 4) | ((*(buf + i) - 'A')+0xa);
				else
					break;
			}
			i++;

			rt5679->adb_reg_addr[j] = value;
			j++;
		}
		rt5679->adb_reg_num = j;
	} else if (buf[0] == 'W' || buf[0] == 'w') {
		while (j <= 0x100 && i < count) {
			/* Get address */
			rt5679->adb_reg_addr[j] = 0;
			value = 0;
			for ( ; i < count; i++) {
				if (*(buf + i) <= '9' && *(buf + i) >= '0')
					value = (value << 4) | (*(buf + i) - '0');
				else if (*(buf + i) <= 'f' && *(buf + i) >= 'a')
					value = (value << 4) | ((*(buf + i) - 'a')+0xa);
				else if (*(buf + i) <= 'F' && *(buf + i) >= 'A')
					value = (value << 4) | ((*(buf + i) - 'A')+0xa);
				else
					break;
			}
			i++;
			rt5679->adb_reg_addr[j] = value;

			/* Get value */
			rt5679->adb_reg_value[j] = 0;
			value = 0;
			for ( ; i < count; i++) {
				if (*(buf + i) <= '9' && *(buf + i) >= '0')
					value = (value << 4) | (*(buf + i) - '0');
				else if (*(buf + i) <= 'f' && *(buf + i) >= 'a')
					value = (value << 4) | ((*(buf + i) - 'a')+0xa);
				else if (*(buf + i) <= 'F' && *(buf + i) >= 'A')
					value = (value << 4) | ((*(buf + i) - 'A')+0xa);
				else
					break;
			}
			i++;
			rt5679->adb_reg_value[j] = value;

			j++;
		}

		rt5679->adb_reg_num = j;

		for (i = 0; i < rt5679->adb_reg_num; i++) {
			snd_soc_write(codec,
				rt5679->adb_reg_addr[i] & 0xffff,
				rt5679->adb_reg_value[i]);
		}

	}

	return count;
}

static DEVICE_ATTR(codec_reg_adb, 0664, rt5679_codec_adb_show,
			rt5679_codec_adb_store);

static int rt5679_set_bias_level(struct snd_soc_codec *codec,
			enum snd_soc_bias_level level)
{
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);

	switch (level) {
	case SND_SOC_BIAS_ON:
		break;

	case SND_SOC_BIAS_PREPARE:
		if (snd_soc_codec_get_bias_level(codec) == SND_SOC_BIAS_STANDBY) {
			regmap_update_bits(rt5679->regmap,
				RT5679_MCLK_GATING_CTRL, RT5679_MCLK_GATE_MASK,
				RT5679_MCLK_GATE_EN);
		}
		break;

	case SND_SOC_BIAS_STANDBY:
		break;

	case SND_SOC_BIAS_OFF:
		regmap_update_bits(rt5679->regmap, RT5679_MCLK_GATING_CTRL,
			RT5679_MCLK_GATE_MASK, RT5679_MCLK_GATE_DIS);
		break;

	default:
		break;
	}

	return 0;
}

static int rt5679_probe(struct snd_soc_codec *codec)
{
	struct rt5679_priv *rt5679 = snd_soc_codec_get_drvdata(codec);
	int ret;

	pr_info("Codec driver version %s\n", VERSION);

	rt5679->codec = codec;

	rt5679_reg_init(codec);

	ret = device_create_file(codec->dev, &dev_attr_codec_reg);
	if (ret != 0) {
		dev_err(codec->dev,
			"Failed to create codec_reg sysfs files: %d\n", ret);
		return ret;
	}

	ret = device_create_file(codec->dev, &dev_attr_codec_reg_adb);
	if (ret != 0) {
		dev_err(codec->dev,
			"Failed to create codec_reg_adb sysfs files: %d\n", ret);
		return ret;
	}

	ret = device_create_file(codec->dev, &dev_attr_is_dsp_mode);
	if (ret != 0) {
		dev_err(codec->dev,
			"Failed to create is_dsp_mode sysfs files: %d\n", ret);
		return ret;
	}

	rt5679_set_bias_level(codec, SND_SOC_BIAS_OFF);

	return 0;
}

static int rt5679_remove(struct snd_soc_codec *codec)
{
	rt5679_set_bias_level(codec, SND_SOC_BIAS_OFF);

	device_remove_file(codec->dev, &dev_attr_codec_reg);
	device_remove_file(codec->dev, &dev_attr_codec_reg_adb);
	device_remove_file(codec->dev, &dev_attr_is_dsp_mode);

	return 0;
}

#ifdef CONFIG_PM
static int rt5679_suspend(struct snd_soc_codec *codec)
{
	return 0;
}

static int rt5679_resume(struct snd_soc_codec *codec)
{
	return 0;
}
#else
#define rt5679_suspend NULL
#define rt5679_resume NULL
#endif

static int rt5679_i2c_read(void *context, unsigned int reg, unsigned int *val)
{
	struct i2c_client *client = context;
	struct rt5679_priv *rt5679 = i2c_get_clientdata(client);

	if (rt5679->is_dsp_mode)
#ifndef DSP_MODE_USE_SPI
		rt5679_dsp_mode_i2c_read(rt5679, reg, val);
#else
		rt5679_spi_read(0x1800c000 + reg * 2, val, 2);
#endif
	else
		regmap_read(rt5679->regmap_physical, reg, val);

	return 0;
}

static int rt5679_i2c_write(void *context, unsigned int reg, unsigned int val)
{
	struct i2c_client *client = context;
	struct rt5679_priv *rt5679 = i2c_get_clientdata(client);

	if (rt5679->is_dsp_mode)
#ifndef DSP_MODE_USE_SPI
		rt5679_dsp_mode_i2c_write(rt5679, reg, val);
#else
		rt5679_spi_write(0x1800c000 + reg * 2, val, 2);
#endif
	else
		regmap_write(rt5679->regmap_physical, reg, val);

	return 0;
}

#define RT5679_STEREO_RATES SNDRV_PCM_RATE_8000_192000
#define RT5679_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | \
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S8)

struct snd_soc_dai_ops rt5679_aif_dai_ops = {
	.hw_params = rt5679_hw_params,
	.prepare = rt5679_prepare,
	.set_fmt = rt5679_set_dai_fmt,
	.set_sysclk = rt5679_set_dai_sysclk,
	.set_pll = rt5679_set_dai_pll,
	.set_tdm_slot = rt5679_set_tdm_slot,
};

struct snd_soc_dai_driver rt5679_dai[] = {
	{
		.name = "rt5679-aif1",
		.id = RT5679_AIF1,
		.playback = {
			.stream_name = "AIF1 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = RT5679_STEREO_RATES,
			.formats = RT5679_FORMATS,
		},
		.capture = {
			.stream_name = "AIF1 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = RT5679_STEREO_RATES,
			.formats = RT5679_FORMATS,
		},
		.ops = &rt5679_aif_dai_ops,
	},
	{
		.name = "rt5679-aif2",
		.id = RT5679_AIF2,
		.playback = {
			.stream_name = "AIF2 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = RT5679_STEREO_RATES,
			.formats = RT5679_FORMATS,
		},
		.capture = {
			.stream_name = "AIF2 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = RT5679_STEREO_RATES,
			.formats = RT5679_FORMATS,
		},
		.ops = &rt5679_aif_dai_ops,
	},
	{
		.name = "rt5679-aif3",
		.id = RT5679_AIF3,
		.playback = {
			.stream_name = "AIF3 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RT5679_STEREO_RATES,
			.formats = RT5679_FORMATS,
		},
		.capture = {
			.stream_name = "AIF3 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RT5679_STEREO_RATES,
			.formats = RT5679_FORMATS,
		},
		.ops = &rt5679_aif_dai_ops,
	},
	{
		.name = "rt5679-aif4",
		.id = RT5679_AIF4,
		.playback = {
			.stream_name = "AIF4 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RT5679_STEREO_RATES,
			.formats = RT5679_FORMATS,
		},
		.capture = {
			.stream_name = "AIF4 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RT5679_STEREO_RATES,
			.formats = RT5679_FORMATS,
		},
		.ops = &rt5679_aif_dai_ops,
	},
	{
		.name = "rt5679-aif5",
		.id = RT5679_AIF5,
		.playback = {
			.stream_name = "AIF5 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RT5679_STEREO_RATES,
			.formats = RT5679_FORMATS,
		},
		.capture = {
			.stream_name = "AIF5 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RT5679_STEREO_RATES,
			.formats = RT5679_FORMATS,
		},
		.ops = &rt5679_aif_dai_ops,
	},
	{
		.name = "rt5679-slimbus",
		.id = RT5679_AIF6,
		.playback = {
			.stream_name = "SLIMBus Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RT5679_STEREO_RATES,
			.formats = RT5679_FORMATS,
		},
		.capture = {
			.stream_name = "SLIMBus Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RT5679_STEREO_RATES,
			.formats = RT5679_FORMATS,
		},
		.ops = &rt5679_aif_dai_ops,
	},
};

static struct snd_soc_codec_driver soc_codec_dev_rt5679 = {
	.probe = rt5679_probe,
	.remove = rt5679_remove,
	.suspend = rt5679_suspend,
	.resume = rt5679_resume,
	.set_bias_level = rt5679_set_bias_level,
	.idle_bias_off = true,
	.controls = rt5679_snd_controls,
	.num_controls = ARRAY_SIZE(rt5679_snd_controls),
	.dapm_widgets = rt5679_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(rt5679_dapm_widgets),
	.dapm_routes = rt5679_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(rt5679_dapm_routes),
};

static const struct regmap_config rt5679_regmap_physical = {
	.name = "physical",
	.reg_bits = 16,
	.val_bits = 16,

	.max_register = RT5679_DUMMY_REG_4,
	.readable_reg = rt5679_readable_register,

	.cache_type = REGCACHE_NONE,
};

static const struct regmap_config rt5679_regmap = {
	.reg_bits = 16,
	.val_bits = 16,

	.max_register = RT5679_DUMMY_REG_4,
	.volatile_reg = rt5679_volatile_register,
	.readable_reg = rt5679_readable_register,
	.reg_read = rt5679_i2c_read,
	.reg_write = rt5679_i2c_write,

	.cache_type = REGCACHE_RBTREE,
	.reg_defaults = rt5679_reg,
	.num_reg_defaults = ARRAY_SIZE(rt5679_reg),
};

static const struct i2c_device_id rt5679_i2c_id[] = {
	{ "rt5679", RT5679 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, rt5679_i2c_id);

#if defined(CONFIG_OF)
static const struct of_device_id rt5679_of_match[] = {
	{ .compatible = "realtek,rt5679", },
	{},
};
MODULE_DEVICE_TABLE(of, rt5679_of_match);
#endif

static void rt5679_calibrate(struct rt5679_priv *rt5679)
{
	int value, count;

	regmap_write(rt5679->regmap_physical, RT5679_RESET, 0x10ec);

	//HP performance fine tune
	regmap_write(rt5679->regmap_physical, 0x0609, 0x2474);
	regmap_write(rt5679->regmap_physical, 0x060a, 0x4477);
	regmap_write(rt5679->regmap_physical, 0x060b, 0x2474);
	regmap_write(rt5679->regmap_physical, 0x060c, 0x4477);
	regmap_write(rt5679->regmap_physical, 0x0671, 0xc0c0);
	regmap_write(rt5679->regmap_physical, 0x0603, 0x0622);
	regmap_write(rt5679->regmap_physical, 0x068f, 0x0014);
	regmap_write(rt5679->regmap_physical, 0x0690, 0x0014);
	//HP performance fine tune

	if (rt5679->pdata.use_ldo_avdd) {
		regmap_write(rt5679->regmap_physical, 0x068a, 0x0250);
		regmap_write(rt5679->regmap_physical, 0x0067, 0x0202);
		regmap_write(rt5679->regmap_physical, 0x0067, 0x1202);
	}

	regmap_write(rt5679->regmap_physical, 0x00fa, 0x0001);
	regmap_write(rt5679->regmap_physical, 0x0076, 0x0777);
	regmap_write(rt5679->regmap_physical, 0x0078, 0x0000);
	regmap_write(rt5679->regmap_physical, 0x0660, 0x1010);
	regmap_write(rt5679->regmap_physical, 0x0661, 0x1010);

	regmap_write(rt5679->regmap_physical, 0x0070, 0x8020); //Bit Length=24bits
	regmap_write(rt5679->regmap_physical, 0x0040, 0xf0aa);
	regmap_write(rt5679->regmap_physical, 0x0046, 0xb080);
	regmap_write(rt5679->regmap_physical, 0x0061, 0x8000);
	regmap_write(rt5679->regmap_physical, 0x0062, 0x0400);

	regmap_write(rt5679->regmap_physical, 0x061d, 0x04df); //Vref1 Fast Charge=0.45V
	//Verf1/2/5 Fast Charge
	regmap_write(rt5679->regmap_physical, 0x0063, 0xa240);
	regmap_write(rt5679->regmap_physical, 0x0066, 0x1680);
	msleep(20);
	//Vref1/2/5 Slow Charge
	regmap_write(rt5679->regmap_physical, 0x0063, 0xe340);
	regmap_write(rt5679->regmap_physical, 0x0066, 0x1f80);

	regmap_write(rt5679->regmap_physical, 0x0066, 0xdf80);


	//HP calibration setting-Power On
	regmap_write(rt5679->regmap_physical, 0x0080, 0x6000); //Swtich to RC Clock
	regmap_write(rt5679->regmap_physical, 0x0063, 0xe342); //Power on 25M RC
	regmap_write(rt5679->regmap_physical, 0x0076, 0x1777); //Div2
	regmap_write(rt5679->regmap_physical, 0x0066, 0xff80); //Powe On ADDAREF
	regmap_write(rt5679->regmap_physical, 0x000a, 0x5353); //pow_cal_buf
	regmap_write(rt5679->regmap_physical, 0x0614, 0xb490); //en_ckgen_adc6/7 & rst
	regmap_write(rt5679->regmap_physical, 0x0060, 0x0003); //pow_adc6/7
	regmap_write(rt5679->regmap_physical, 0x0401, 0x0630);
	regmap_write(rt5679->regmap_physical, 0x0403, 0x0267);
	regmap_write(rt5679->regmap_physical, 0x0404, 0x9ecd); //en_cal_clk_gatting

	regmap_write(rt5679->regmap_physical, 0x0400, 0x7f00); //rest on

	regmap_write(rt5679->regmap_physical, 0x0400, 0xff00); //porcedure en

	count = 0;
	while (true) {
		regmap_read(rt5679->regmap_physical, 0x0400, &value);
		if (value & 0x8000)
			usleep_range(10000, 10005);
		else
			break;

		if (count > 65) {
			pr_err("HP Calibration Failure\n");
			return;
		}

		count++;
	}

	regmap_write(rt5679->regmap_physical, RT5679_RESET, 0x10ec);

	//Fine Tune Performance
	regmap_write(rt5679->regmap_physical, 0x0600, 0x0a69); //MONOAMP_Bias=2uA

	if (rt5679->pdata.use_ldo_avdd) {
		regmap_write(rt5679->regmap_physical, 0x068a, 0x0250);
		regmap_write(rt5679->regmap_physical, 0x0067, 0x0202);
		regmap_write(rt5679->regmap_physical, 0x0067, 0x1202);
	}

	regmap_write(rt5679->regmap_physical, 0x00fa, 0x0001);
	regmap_write(rt5679->regmap_physical, 0x0076, 0x0777);
	regmap_write(rt5679->regmap_physical, 0x0078, 0x0000);
	regmap_write(rt5679->regmap_physical, 0x0664, 0x1010);
	regmap_write(rt5679->regmap_physical, 0x0070, 0x8020); //'Bit Length=24bits
	regmap_write(rt5679->regmap_physical, 0x061d, 0x04df); //'Vref1 Fast Charge=0.45V
	regmap_write(rt5679->regmap_physical, 0x0063, 0xa240); //'Vref1/2 Fast Charge
	msleep(20);
	regmap_write(rt5679->regmap_physical, 0x0063, 0xe340); //'Vref1/2 Slow Charge
	regmap_write(rt5679->regmap_physical, 0x0066, 0x2000); //'Power On ADDAREF

	if (rt5679->pdata.use_ldo_avdd)
		regmap_write(rt5679->regmap_physical, 0x0005, 0x1000);
	else
		regmap_write(rt5679->regmap_physical, 0x0005, 0x1800);

	regmap_write(rt5679->regmap_physical, 0x0040, 0xf0aa);
	regmap_write(rt5679->regmap_physical, 0x0049, 0x0000);
	regmap_write(rt5679->regmap_physical, 0x0061, 0x8000);
	regmap_write(rt5679->regmap_physical, 0x0062, 0x0400);

	if (rt5679->pdata.use_ldo_avdd)
		regmap_write(rt5679->regmap_physical, 0x0005, 0x5000);
	else
		regmap_write(rt5679->regmap_physical, 0x0005, 0x5800);

	//Switch To RC Clock For Calbration
	regmap_write(rt5679->regmap_physical, 0x0080, 0x6000); //'Swtich to RC Clock
	regmap_write(rt5679->regmap_physical, 0x0063, 0xe342); //'Power on 25M RC
	regmap_write(rt5679->regmap_physical, 0x0076, 0x1777); //'Div2

	//Power On Cal ADC and Clock
	regmap_write(rt5679->regmap_physical, 0x0614, 0xb490);
	regmap_write(rt5679->regmap_physical, 0x000a, 0x5255);
	regmap_write(rt5679->regmap_physical, 0x0060, 0x0002);
	regmap_write(rt5679->regmap_physical, 0x0404, 0x1c3d);
	regmap_write(rt5679->regmap_physical, 0x0422, 0x7000);
	regmap_write(rt5679->regmap_physical, 0x0420, 0xf914);

	count = 0;
	while (true) {
		regmap_read(rt5679->regmap_physical, 0x0420, &value);
		if (value & 0x8000)
			usleep_range(10000, 10005);
		else
			break;

		if (count > 55) {
			pr_err("MONO Calibration Failure\n");
			return;
		}

		count++;
	}
}

static int rt5679_parse_dt(struct rt5679_priv *rt5679, struct device_node *np)
{
	rt5679->pdata.gpio_ldo = of_get_named_gpio(np, "realtek,gpio_ldo", 0);
	rt5679->pdata.gpio_reset = of_get_named_gpio(np, "realtek,gpio_reset",
		0);

	rt5679->pdata.in1_diff = of_property_read_bool(np,
					"realtek,in1-differential");
	rt5679->pdata.in2_diff = of_property_read_bool(np,
					"realtek,in2-differential");
	rt5679->pdata.in3_diff = of_property_read_bool(np,
					"realtek,in3-differential");
	rt5679->pdata.in4_diff = of_property_read_bool(np,
					"realtek,in4-differential");
	rt5679->pdata.lout1_diff = of_property_read_bool(np,
					"realtek,lout1-differential");
	rt5679->pdata.lout2_diff = of_property_read_bool(np,
					"realtek,lout2-differential");

	of_property_read_u32(np, "realtek,dmic1_clk_pin",
		&rt5679->pdata.dmic1_clk_pin);
	of_property_read_u32(np, "realtek,dmic1_data_pin",
		&rt5679->pdata.dmic1_data_pin);
	of_property_read_u32(np, "realtek,dmic2_clk_pin",
		&rt5679->pdata.dmic2_clk_pin);
	of_property_read_u32(np, "realtek,dmic2_data_pin",
		&rt5679->pdata.dmic2_data_pin);
	of_property_read_u32(np, "realtek,dmic3_clk_pin",
		&rt5679->pdata.dmic3_clk_pin);
	of_property_read_u32(np, "realtek,dmic3_data_pin",
		&rt5679->pdata.dmic3_data_pin);
	of_property_read_u32(np, "realtek,dmic4_clk_pin",
		&rt5679->pdata.dmic4_clk_pin);
	of_property_read_u32(np, "realtek,dmic4_data_pin",
		&rt5679->pdata.dmic4_data_pin);

	rt5679->pdata.use_ldo_avdd= of_property_read_bool(np,
					"realtek,use_ldo_avdd");
	return 0;
}

static void rt5679_dmic1_config(struct rt5679_priv *rt5679)
{
	switch (rt5679->pdata.dmic1_clk_pin) {
	case RT5679_DMIC1_CLK_GPIO2:
		regmap_update_bits(rt5679->regmap, RT5679_MF_PIN_CTRL1,
			RT5679_GP2_TYPE_MASK, RT5679_GP2_TYPE_FUNC);
		regmap_update_bits(rt5679->regmap, RT5679_MF_PIN_CTRL2,
			RT5679_GP2_PRE_TYPE_MASK,
			RT5679_GP2_PRE_TYPE_DMIC1_SCL);
		break;

	case RT5679_DMIC1_CLK_GPIO21:
		regmap_update_bits(rt5679->regmap, RT5679_MF_PIN_CTRL3,
			RT5679_GP21_TYPE_MASK, RT5679_GP21_TYPE_DMIC1_SCL);
		break;

	default:
		pr_debug("no DMIC1 CLK\n");
		break;
	}

	switch (rt5679->pdata.dmic1_data_pin) {
	case RT5679_DMIC1_DATA_GPIO22:
		regmap_update_bits(rt5679->regmap, RT5679_DMIC_CTRL2,
			RT5679_DMIC1_DP_MASK, RT5679_DMIC1_DP_GPIO22);
		regmap_update_bits(rt5679->regmap, RT5679_MF_PIN_CTRL3,
			RT5679_GP22_TYPE_MASK, RT5679_GP22_TYPE_DMIC1_SDA);
		break;

	case RT5679_DMIC1_DATA_IN4N:
		regmap_update_bits(rt5679->regmap, RT5679_DMIC_CTRL2,
			RT5679_DMIC1_DP_MASK, RT5679_DMIC1_DP_IN4N);
		break;

	default:
		pr_debug("no DMIC1 DATA\n");
		break;
	}
}

static void rt5679_dmic2_config(struct rt5679_priv *rt5679)
{
	switch (rt5679->pdata.dmic2_clk_pin) {
	case RT5679_DMIC2_CLK_GPIO3:
		regmap_update_bits(rt5679->regmap, RT5679_MF_PIN_CTRL1,
			RT5679_GP3_TYPE_MASK, RT5679_GP3_TYPE_DMIC2_SCL);
		break;

	case RT5679_DMIC2_CLK_GPIO4:
		regmap_update_bits(rt5679->regmap, RT5679_MF_PIN_CTRL1,
			RT5679_GP4_TYPE_MASK | RT5679_GP4_PRE_TYPE_MASK,
			RT5679_GP4_TYPE_FUNC | RT5679_GP4_PRE_TYPE_DMIC2_SCL);
		break;

	case RT5679_DMIC2_CLK_MCLK2:
		regmap_update_bits(rt5679->regmap, RT5679_MF_PIN_CTRL2,
			RT5679_MCLK2_TYPE_MASK, RT5679_MCLK2_TYPE_DMIC2_SCL);
		break;

	default:
		pr_debug("no DMIC2 CLK\n");
		break;
	}

	switch (rt5679->pdata.dmic2_data_pin) {
	case RT5679_DMIC2_DATA_GPIO23:
		regmap_update_bits(rt5679->regmap, RT5679_DMIC_CTRL2,
			RT5679_DMIC2_DP_MASK, RT5679_DMIC2_DP_GPIO23);
		regmap_update_bits(rt5679->regmap, RT5679_MF_PIN_CTRL3,
			RT5679_GP23_TYPE_MASK, RT5679_GP23_TYPE_DMIC2_SDA);
		break;

	case RT5679_DMIC2_DATA_IN4P:
		regmap_update_bits(rt5679->regmap, RT5679_DMIC_CTRL2,
			RT5679_DMIC2_DP_MASK, RT5679_DMIC2_DP_IN4P);
		break;

	default:
		pr_debug("no DMIC2 DATA\n");
		break;
	}
}

static void rt5679_dmic3_config(struct rt5679_priv *rt5679)
{
	switch (rt5679->pdata.dmic3_clk_pin) {
	case RT5679_DMIC3_CLK_GPIO5:
		regmap_update_bits(rt5679->regmap, RT5679_MF_PIN_CTRL1,
			RT5679_GP5_TYPE_MASK, RT5679_GP5_TYPE_FUNC);
		regmap_update_bits(rt5679->regmap, RT5679_MF_PIN_CTRL2,
			RT5679_GP5_PRE_TYPE_MASK,
			RT5679_GP5_PRE_TYPE_DMIC3_SCL);
		break;

	case RT5679_DMIC3_CLK_GPIO16:
		regmap_update_bits(rt5679->regmap, RT5679_MF_PIN_CTRL1,
			RT5679_GP16_TYPE_MASK, RT5679_GP16_TYPE_DMIC3_SCL);
		break;

	default:
		pr_debug("no DMIC3 CLK\n");
		break;
	}

	switch (rt5679->pdata.dmic3_data_pin) {
	case RT5679_DMIC3_DATA_GPIO24:
		regmap_update_bits(rt5679->regmap, RT5679_DMIC_CTRL2,
			RT5679_DMIC3_DP_MASK, RT5679_DMIC3_DP_GPIO24);
		regmap_update_bits(rt5679->regmap, RT5679_MF_PIN_CTRL3,
			RT5679_GP24_TYPE_MASK, RT5679_GP24_TYPE_DMIC3_SDA);
		break;

	case RT5679_DMIC3_DATA_IN3N:
		regmap_update_bits(rt5679->regmap, RT5679_DMIC_CTRL2,
			RT5679_DMIC3_DP_MASK, RT5679_DMIC3_DP_IN3N);
		break;

	default:
		pr_debug("no DMIC3 DATA\n");
		break;
	}
}

static void rt5679_dmic4_config(struct rt5679_priv *rt5679)
{
	switch (rt5679->pdata.dmic4_clk_pin) {
	case RT5679_DMIC4_CLK_GPIO7:
		regmap_update_bits(rt5679->regmap, RT5679_MF_PIN_CTRL1,
			RT5679_GP7_TYPE_MASK, RT5679_GP7_TYPE_DMIC4_SCL);
		break;

	default:
		pr_debug("no DMIC4 CLK\n");
		break;
	}

	switch (rt5679->pdata.dmic4_data_pin) {
	case RT5679_DMIC4_DATA_GPIO6:
		regmap_update_bits(rt5679->regmap, RT5679_DMIC_CTRL2,
			RT5679_DMIC4_DP_MASK, RT5679_DMIC4_DP_GPIO6);
		regmap_update_bits(rt5679->regmap, RT5679_MF_PIN_CTRL2,
			RT5679_GP6_TYPE_MASK, RT5679_GP6_TYPE_DMIC4_SDA);
		break;

	case RT5679_DMIC4_DATA_IN3P:
		regmap_update_bits(rt5679->regmap, RT5679_DMIC_CTRL2,
			RT5679_DMIC4_DP_MASK, RT5679_DMIC4_DP_IN3P);
		break;

	default:
		pr_debug("no DMIC4 DATA\n");
		break;
	}
}

static int rt5679_i2c_probe(struct i2c_client *i2c,
		    const struct i2c_device_id *id)
{
	struct rt5679_platform_data *pdata = dev_get_platdata(&i2c->dev);
	struct rt5679_priv *rt5679;
	int ret;

	rt5679 = devm_kzalloc(&i2c->dev, sizeof(struct rt5679_priv),
				GFP_KERNEL);
	if (rt5679 == NULL)
		return -ENOMEM;

	i2c_set_clientdata(i2c, rt5679);

	if (pdata) {
		rt5679->pdata = *pdata;
	} else {
		if (i2c->dev.of_node) {
			ret = rt5679_parse_dt(rt5679, i2c->dev.of_node);
			if (ret) {
				dev_err(&i2c->dev,
					"Failed to parse device tree: %d\n",
					ret);
				return ret;
			}
		}
	}

	ret = devm_gpio_request_one(&i2c->dev, rt5679->pdata.gpio_ldo,
		GPIOF_OUT_INIT_HIGH, "rt5679");
	if (ret)
		dev_err(&i2c->dev, "Fail gpio_request_one gpio_ldo: %d\n", ret);

	ret = devm_gpio_request_one(&i2c->dev, rt5679->pdata.gpio_reset,
		GPIOF_OUT_INIT_HIGH, "rt5679");
	if (ret)
		dev_err(&i2c->dev, "Fail gpio_request_one gpio_reset: %d\n",
			ret);

	msleep(100);

	rt5679->regmap_physical = devm_regmap_init_i2c(i2c,
					&rt5679_regmap_physical);
	if (IS_ERR(rt5679->regmap_physical)) {
		ret = PTR_ERR(rt5679->regmap_physical);
		dev_err(&i2c->dev, "Failed to allocate register map: %d\n",
			ret);
		return ret;
	}

	rt5679->regmap = devm_regmap_init(&i2c->dev, NULL, i2c, &rt5679_regmap);
	if (IS_ERR(rt5679->regmap)) {
		ret = PTR_ERR(rt5679->regmap);
		dev_err(&i2c->dev, "Failed to allocate register map: %d\n",
			ret);
		return ret;
	}

	regmap_read(rt5679->regmap, RT5679_VENDOR_ID2, &ret);
	if (ret != RT5679_DEVICE_ID) {
		dev_err(&i2c->dev,
			"Device with ID register %x is not rt5679\n", ret);
		return -ENODEV;
	}

	mutex_init(&rt5679->dsp_lock);

	rt5679_calibrate(rt5679);

	regmap_write(rt5679->regmap_physical, RT5679_RESET, 0x10ec);

	/* IN/OUT diff mode*/
	if (rt5679->pdata.in1_diff)
		regmap_update_bits(rt5679->regmap, RT5679_BST12_CTRL,
			RT5679_IN1_DF, RT5679_IN1_DF);
	if (rt5679->pdata.in2_diff)
		regmap_update_bits(rt5679->regmap, RT5679_BST12_CTRL,
			RT5679_IN2_DF, RT5679_IN2_DF);
	if (rt5679->pdata.in3_diff)
		regmap_update_bits(rt5679->regmap, RT5679_BST34_CTRL,
			RT5679_IN3_DF, RT5679_IN3_DF);
	if (rt5679->pdata.in4_diff)
		regmap_update_bits(rt5679->regmap, RT5679_BST34_CTRL,
			RT5679_IN4_DF, RT5679_IN4_DF);
	if (rt5679->pdata.lout1_diff)
		regmap_update_bits(rt5679->regmap, RT5679_LOUT, RT5679_LOUT1_DF,
			RT5679_LOUT1_DF);
	if (rt5679->pdata.lout2_diff)
		regmap_update_bits(rt5679->regmap, RT5679_LOUT, RT5679_LOUT2_DF,
			RT5679_LOUT2_DF);

	/* DMIC pin*/
	if (rt5679->pdata.dmic1_clk_pin != RT5679_DMIC1_CLK_NULL &&
		rt5679->pdata.dmic1_data_pin != RT5679_DMIC1_DATA_NULL)
		rt5679_dmic1_config(rt5679);
	if (rt5679->pdata.dmic2_clk_pin != RT5679_DMIC2_CLK_NULL &&
		rt5679->pdata.dmic2_data_pin != RT5679_DMIC2_DATA_NULL)
		rt5679_dmic2_config(rt5679);
	if (rt5679->pdata.dmic3_clk_pin != RT5679_DMIC3_CLK_NULL &&
		rt5679->pdata.dmic3_data_pin != RT5679_DMIC3_DATA_NULL)
		rt5679_dmic3_config(rt5679);
	if (rt5679->pdata.dmic4_clk_pin != RT5679_DMIC4_CLK_NULL &&
		rt5679->pdata.dmic4_data_pin != RT5679_DMIC4_DATA_NULL)
		rt5679_dmic4_config(rt5679);

	if (rt5679->pdata.use_ldo_avdd) {
		regmap_update_bits(rt5679->regmap, RT5679_LDO_HV3_PR_CTRL,
			0x0040, 0x0040);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_LDO2, 0x0202,
			0x0202);
		regmap_update_bits(rt5679->regmap, RT5679_PWR_LDO2, 0x1000,
			0x1000);
	} else {
		regmap_update_bits(rt5679->regmap, RT5679_MONO_OUT, 0x0800,
		0x0800);
	}

	INIT_DELAYED_WORK(&rt5679->jack_detect_work, rt5679_jack_detect_work);

	if (i2c->irq) {
		ret = request_threaded_irq(i2c->irq, NULL, rt5679_irq,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING
			| IRQF_ONESHOT, "rt5679", rt5679);
		if (ret) {
			dev_err(&i2c->dev, "Failed to reguest IRQ: %d\n", ret);
			return ret;
		}
	}

	return snd_soc_register_codec(&i2c->dev, &soc_codec_dev_rt5679,
			rt5679_dai, ARRAY_SIZE(rt5679_dai));
}

static int rt5679_i2c_remove(struct i2c_client *i2c)
{
	snd_soc_unregister_codec(&i2c->dev);

#ifdef CONFIG_SWITCH
	switch_dev_unregister(&rt5679_headset_switch);
#endif

	return 0;
}

void rt5679_i2c_shutdown(struct i2c_client *client)
{
	struct rt5679_priv *rt5679 = i2c_get_clientdata(client);
	struct snd_soc_codec *codec = rt5679->codec;

	if (codec != NULL)
		rt5679_set_bias_level(codec, SND_SOC_BIAS_OFF);
}

struct i2c_driver rt5679_i2c_driver = {
	.driver = {
		.name = "rt5679",
		.of_match_table = rt5679_of_match,
	},
	.probe = rt5679_i2c_probe,
	.remove   = rt5679_i2c_remove,
	.shutdown = rt5679_i2c_shutdown,
	.id_table = rt5679_i2c_id,
};
module_i2c_driver(rt5679_i2c_driver);

MODULE_DESCRIPTION("ASoC ALC5679 driver");
MODULE_AUTHOR("Oder Chiou <oder_chiou@realtek.com>");
MODULE_LICENSE("GPL v2");
