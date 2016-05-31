/*
 * rt5668.h  --  RT5668 ALSA SoC audio driver
 *
 * Copyright 2015 Realtek Microelectronics
 * Author: Jack Yu <jack.yu@realtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __RT5668_H__
#define __RT5668_H__

/* Info */
#define RT5668_RESET				0x0000
#define RT5668_VENDOR_ID			0x00fd
#define RT5668_VENDOR_ID_1			0x00fe
#define RT5668_VENDOR_ID_2			0x00ff

#define RT5668_HP_AMP_1				0x0002
#define RT5668_HP_AMP_2				0x0003
#define RT5668_HPL_GAIN				0x0005
#define RT5668_HPR_GAIN				0x0006

#define RT5668_AEC_BST				0x000b
#define RT5668_IN1_IN2				0x000c
#define RT5668_CBJ_TYPE_1			0x0010
#define RT5668_CBJ_TYPE_2			0x0011
#define RT5668_CBJ_TYPE_3			0x0012

/* I/O - ADC/DAC/DMIC */
#define RT5668_DAC1_DIG_VOL			0x0019
#define RT5668_STO1_ADC_DIG_VOL			0x001c
#define RT5668_STO1_BOOST			0x001f
#define RT5668_HP_IMP_GAIN_1			0x0022
#define RT5668_HP_IMP_GAIN_2			0x0023
/* Mixer - D-D */
#define RT5668_STO1_ADC_MIXER			0x0026
#define RT5668_AD_DA_MIXER			0x0029
#define RT5668_STO_DAC_MIXER			0x002a

/*Mixer - Analog*/
#define RT5668_RECMIX1L				0x003c
#define RT5668_RECMIX1R				0x003e

/* Power */
#define RT5668_PWR_DIG_1			0x0061
#define RT5668_PWR_DIG_2			0x0062
#define RT5668_PWR_ANLG_1			0x0063
#define RT5668_PWR_ANLG_2			0x0064
#define RT5668_PWR_ANLG_3			0x0065
#define RT5668_PWR_MIXER			0x0066
#define RT5668_PWR_VOL				0x0067

#define RT5668_MCLK_DET				0x006b
/* Format - ADC/DAC */
#define RT5668_I2S1_SDP				0x0070
#define RT5668_ADDA_CLK_1			0x0073
/* Format - TDM Control */
#define RT5668_TDM_1				0x0078
#define RT5668_TDM_2				0x0079
#define RT5668_TDM_3				0x007a
#define RT5668_TDM_4				0x007b
#define RT5668_TDM_5				0x007c
#define RT5668_TDM_6				0x007d
#define RT5668_TDM_7				0x007e
#define RT5668_TDM_8				0x007f

/* Function - Analog */
#define RT5668_GLB_CLK				0x0080
#define RT5668_PLL_1				0x0081
#define RT5668_PLL_2				0x0082
#define RT5668_ASRC_1				0x0083
#define RT5668_ASRC_2				0x0084
#define RT5668_ASRC_3				0x0085
#define RT5668_ASRC_8				0x008a
#define RT5668_ASRC_9				0x008b
#define RT5668_ASRC_11				0x008c
#define RT5668_DEPOP_1				0x008e
#define RT5668_DEPOP_2				0x008f
#define RT5668_HP_CHARGE_PUMP_1			0x0091
#define RT5668_HP_CHARGE_PUMP_2			0x0092
#define RT5668_MICBIAS_2			0x0094
#define RT5668_AUTO_RC_CLK			0x009f

/* Function - Digital */
#define RT5668_ADC_EQ_1				0x00ae
#define RT5668_ADC_EQ_2				0x00af

#define RT5668_IRQ_1				0x00b6
#define RT5668_IRQ_2				0x00b7
#define RT5668_IRQ_3				0x00b8
#define RT5668_IRQ_4				0x00b9
#define RT5668_IRQ_5				0x00ba
#define RT5668_IRQ_6				0x00bb
#define RT5668_INT_ST_1				0x00be
#define RT5668_GPIO_1				0x00c0
#define RT5668_GPIO_2				0x00c1
#define RT5668_GPIO_3				0x00c2
#define RT5668_IL_CMD_1				0x00db
#define RT5668_IL_CMD_2				0x00dc
#define RT5668_IL_CMD_3				0x00dd
#define RT5668_IL_CMD_4				0x00de
#define RT5668_4BTN_IL_CMD_1			0x00df
#define RT5668_4BTN_IL_CMD_2			0x00e0
#define RT5668_4BTN_IL_CMD_3			0x00e1
/* General Control */
#define RT5668_DIG_MISC				0x00fa

#define RT5668_BIAS_CUR_8			0x0111
#define RT5668_CHOP_DAC				0x013a
#define RT5668_CHOP_ADC				0x013b
#define RT5668_CALIB_ADC			0x013c
#define RT5668_HP_LOGIC_1			0x01da
#define RT5668_HP_LOGIC_2			0x01db
#define RT5668_HP_CALIB_1			0x01de
#define RT5668_HP_CALIB_2			0x01df
#define RT5668_HP_CALIB_3			0x01e0
#define RT5668_HP_CALIB_4			0x01e1
#define RT5668_HP_CALIB_5			0x01e2
#define RT5668_HP_CALIB_6			0x01e3
#define RT5668_HP_CALIB_7			0x01e4
#define RT5668_HP_CALIB_8			0x01e5
#define RT5668_HP_CALIB_9			0x01e6
#define RT5668_HP_CALIB_10			0x01e7

/* Headphone Amp Control 2 (0x0003) */
#define RT5668_EN_DAC_HPO_MASK			(0x1 << 14)
#define RT5668_EN_DAC_HPO_SHIFT			14
#define RT5668_EN_DAC_HPO_DIS			(0x0 << 14)
#define RT5668_EN_DAC_HPO_EN			(0x1 << 14)

/*Headphone Amp L/R Analog Gain and Digital NG2 Gain Control (0x0005 0x0006)*/
#define RT5668_GAIN_HP				(0x1f << 8)
#define RT5668_GAIN_HP_SHIFT			8

/* AEC BST Control (0x000b) */
#define RT5668_GAIN_CBJ_MASK			(0xf << 8)
#define RT5668_GAIN_CBJ_SHIFT			8

/* IN1 Control / MIC GND REF (0x000c) */
#define RT5668_IN1_DF_MASK			(0x1 << 15)
#define RT5668_IN1_DF_SHIFT			15

/* Combo Jack and Type Detection Control 1 (0x0010) */
#define RT5668_CBJ_DET_MASK			(0x1 << 15)
#define RT5668_CBJ_DET_SHIFT			15
#define RT5668_CBJ_DET_DIS			(0x0 << 15)
#define RT5668_CBJ_DET_EN			(0x1 << 15)
#define RT5668_DET_TYPE_MASK			(0x1 << 12)
#define RT5668_DET_TYPE_SHIFT			12
#define RT5668_DET_TYPE_WLCSP			(0x0 << 12)
#define RT5668_DET_TYPE_QFN			(0x1 << 12)
#define RT5668_VREF_BIAS_MASK			(0x1 << 6)
#define RT5668_VREF_BIAS_SHIFT			6
#define RT5668_VREF_BIAS_FSM			(0x0 << 6)
#define RT5668_VREF_BIAS_REG			(0x1 << 6)

/* REC Left Mixer Control 2 (0x003c) */
#define RT5668_RECMIX1L_BST1_CBJ		(0x1 << 7)
#define RT5668_RECMIX1L_BST1_CBJ_SHIFT		7
#define RT5668_RECMIX1L_BST2			(0x1 << 4)
#define RT5668_RECMIX1L_BST2_SHIFT		4

/* REC Right Mixer Control 2 (0x003e) */
#define RT5668_RECMIX1R_BST2			(0x1 << 4)
#define RT5668_RECMIX1R_BST2_SHIFT		4

/* DAC1 Digital Volume (0x0019) */
#define RT5668_DAC_L1_VOL_MASK			(0xff << 8)
#define RT5668_DAC_L1_VOL_SHIFT			8
#define RT5668_DAC_R1_VOL_MASK			(0xff)
#define RT5668_DAC_R1_VOL_SHIFT			0

/* ADC Digital Volume Control (0x001c) */
#define RT5668_ADC_L_MUTE_MASK			(0x1 << 15)
#define RT5668_ADC_L_MUTE_SHIFT			15
#define RT5668_ADC_L_VOL_MASK			(0x7f << 8)
#define RT5668_ADC_L_VOL_SHIFT			8
#define RT5668_ADC_R_MUTE_MASK			(0x1 << 7)
#define RT5668_ADC_R_MUTE_SHIFT			7
#define RT5668_ADC_R_VOL_MASK			(0x7f)
#define RT5668_ADC_R_VOL_SHIFT			0

/* Stereo ADC Mixer Control (0x0026) */
#define RT5668_M_STO1_ADC_L1			(0x1 << 15)
#define RT5668_M_STO1_ADC_L1_SHIFT		15
#define RT5668_M_STO1_ADC_L2			(0x1 << 14)
#define RT5668_M_STO1_ADC_L2_SHIFT		14
#define RT5668_STO1_ADC_L1_SRC			(0x1 << 13)
#define RT5668_STO1_ADC_L1_SRC_SHIFT		13
#define RT5668_STO1_ADC_L2_SRC			(0x1 << 12)
#define RT5668_STO1_ADC_L2_SRC_SHIFT		12
#define RT5668_STO1_ADC_L_SRC			(0x3 << 10)
#define RT5668_STO1_ADC_L_SRC_SHIFT		10
#define RT5668_M_STO1_ADC_R1			(0x1 << 7)
#define RT5668_M_STO1_ADC_R1_SHIFT		7
#define RT5668_M_STO1_ADC_R2			(0x1 << 6)
#define RT5668_M_STO1_ADC_R2_SHIFT		6
#define RT5668_STO1_ADC_R1_SRC			(0x1 << 5)
#define RT5668_STO1_ADC_R1_SRC_SHIFT		5
#define RT5668_STO1_ADC_R2_SRC			(0x1 << 4)
#define RT5668_STO1_ADC_R2_SRC_SHIFT		4
#define RT5668_STO1_ADC_R_SRC			(0x3 << 2)
#define RT5668_STO1_ADC_R_SRC_SHIFT		2

/* ADC Mixer to DAC Mixer Control (0x0029) */
#define RT5668_M_ADCMIX_L			(0x1 << 15)
#define RT5668_M_ADCMIX_L_SHIFT			15
#define RT5668_M_DAC1_L				(0x1 << 14)
#define RT5668_M_DAC1_L_SHIFT			14
#define RT5668_M_ADCMIX_R			(0x1 << 7)
#define RT5668_M_ADCMIX_R_SHIFT			7
#define RT5668_M_DAC1_R				(0x1 << 6)
#define RT5668_M_DAC1_R_SHIFT			6

/* Stereo DAC Mixer Control (0x002a) */
#define RT5668_M_DAC_L1_STO_L			(0x1 << 15)
#define RT5668_M_DAC_L1_STO_L_SHIFT		15
#define RT5668_M_DAC_R1_STO_L			(0x1 << 13)
#define RT5668_M_DAC_R1_STO_L_SHIFT		13
#define RT5668_M_DAC_L1_STO_R			(0x1 << 7)
#define RT5668_M_DAC_L1_STO_R_SHIFT		7
#define RT5668_M_DAC_R1_STO_R			(0x1 << 5)
#define RT5668_M_DAC_R1_STO_R_SHIFT		5

/* Power Management for Digital 1 (0x0061) */
#define RT5668_PWR_I2S1				(0x1 << 15)
#define RT5668_PWR_I2S1_SHIFT			15
#define RT5668_PWR_DAC_L1			(0x1 << 11)
#define RT5668_PWR_DAC_L1_SHIFT			11
#define RT5668_PWR_DAC_R1			(0x1 << 10)
#define RT5668_PWR_DAC_R1_SHIFT			10
#define RT5668_PWR_LDO_DACREF_MASK		(0x1 << 8)
#define RT5668_PWR_LDO_DACREF_SHIFT		8
#define RT5668_PWR_LDO_DACREF_ON		(0x1 << 8)
#define RT5668_PWR_LDO_DACREF_DOWN		(0x0 << 8)
#define RT5668_PWR_LDO_SHIFT			8
#define RT5668_PWR_ADC_L1			(0x1 << 4)
#define RT5668_PWR_ADC_L1_SHIFT			4
#define RT5668_PWR_ADC_R1			(0x1 << 3)
#define RT5668_PWR_ADC_R1_SHIFT			3

/* Power Management for Digital 2 (0x0062) */
#define RT5668_PWR_ADC_S1F			(0x1 << 15)
#define RT5668_PWR_ADC_S1F_SHIFT		15
#define RT5668_PWR_DAC_S1F			(0x1 << 10)
#define RT5668_PWR_DAC_S1F_SHIFT		10

/* Power Management for Analog 1 (0x0063) */
#define RT5668_PWR_VREF1			(0x1 << 15)
#define RT5668_PWR_VREF1_MASK			(0x1 << 15)
#define RT5668_PWR_VREF1_SHIFT			15
#define RT5668_PWR_FV1				(0x1 << 14)
#define RT5668_PWR_FV1_MASK			(0x1 << 14)
#define RT5668_PWR_FV1_SHIFT			14
#define RT5668_PWR_VREF2			(0x1 << 13)
#define RT5668_PWR_VREF2_MASK			(0x1 << 13)
#define RT5668_PWR_VREF2_SHIFT			13
#define RT5668_PWR_FV2				(0x1 << 12)
#define RT5668_PWR_FV2_MASK			(0x1 << 12)
#define RT5668_PWR_FV2_SHIFT			12
#define RT5668_PWR_MB				(0x1 << 9)
#define RT5668_PWR_MB_MASK			(0x1 << 9)
#define RT5668_PWR_MB_SHIFT			9
#define RT5668_AMP_HP_MASK			(0x3 << 2)
#define RT5668_AMP_HP_SHIFT			2
#define RT5668_AMP_HP_1X			(0x0 << 2)
#define RT5668_AMP_HP_3X			(0x1 << 2)
#define RT5668_AMP_HP_5X			(0x3 << 2)
#define RT5668_LDO1_DVO_MASK			(0x3)
#define RT5668_LDO1_DVO_SHIFT			0
#define RT5668_LDO1_DVO_0_9V			(0x0)
#define RT5668_LDO1_DVO_1_0V			(0x1)
#define RT5668_LDO1_DVO_1_2V			(0x2)
#define RT5668_LDO1_DVO_1_4V			(0x3)

/* Power Management for Analog 2 (0x0064) */
#define RT5668_PWR_BST1				(0x1 << 15)
#define RT5668_PWR_BST1_MASK			(0x1 << 15)
#define RT5668_PWR_BST1_SHIFT			15
#define RT5668_PWR_BST1_OFF			(0x0 << 15)
#define RT5668_PWR_BST1_ON			(0x1 << 15)
#define RT5668_PWR_BST2				(0x1 << 14)
#define RT5668_PWR_BST2_MASK			(0x1 << 14)
#define RT5668_PWR_BST2_SHIFT			14
#define RT5668_PWR_MB1				(0x1 << 11)
#define RT5668_PWR_MB1_SHIFT			11
#define RT5668_PWR_MB2				(0x1 << 10)
#define RT5668_PWR_MB2_SHIFT			10
#define RT5668_PWR_BST2_OP			(0x1 << 6)
#define RT5668_PWR_BST2_OP_MASK			(0x1 << 6)
#define RT5668_PWR_BST2_OP_SHIFT		6
#define RT5668_PWR_JD1				(0x1 << 3)
#define RT5668_PWR_JD1_MASK			(0x1 << 3)
#define RT5668_PWR_JD1_SHIFT			3
#define RT5668_PWR_JD2				(0x1 << 2)
#define RT5668_PWR_JD2_MASK			(0x1 << 2)
#define RT5668_PWR_JD2_SHIFT			2
#define RT5668_PWR_RECMIX1			(0x1 << 1)
#define RT5668_PWR_RECMIX1_SHIFT		1
#define RT5668_PWR_RECMIX2			(0x1)
#define RT5668_PWR_RECMIX2_SHIFT		0

/* Power Management for Analog 3 (0x0065) */
#define RT5668_PWR_CBJ_MASK			(0x1 << 9)
#define RT5668_PWR_CBJ_SHIFT			9
#define RT5668_PWR_CBJ_OFF			(0x0 << 9)
#define RT5668_PWR_CBJ_ON			(0x1 << 9)
#define RT5668_PWR_PLL				(0x1 << 6)
#define RT5668_PWR_PLL_SHIFT			6
#define RT5668_PWR_LDO2				(0x1 << 2)
#define RT5668_PWR_LDO2_SHIFT			2

/* Power Management for Volume (0x0067) */
#define RT5668_PWR_MIC_DET			(0x1 << 5)
#define RT5668_PWR_MIC_DET_SHIFT		5

/* MCLK and System Clock Detection Control (0x006b) */
#define RT5668_EN_ANA_CLK_DET_MASK		(0x1 << 15)
#define RT5668_EN_ANA_CLK_DET_SHIFT		15
#define RT5668_EN_ANA_CLK_DET_DIS		(0x0 << 15)
#define RT5668_EN_ANA_CLK_DET_AUTO		(0x1 << 15)
#define RT5668_PWR_CLK_DET_MASK			(0x1)
#define RT5668_PWR_CLK_DET_SHIFT		0
#define RT5668_PWR_CLK_DET_DIS			(0x0)
#define RT5668_PWR_CLK_DET_EN			(0x1)

/* I2S1 Audio Serial Data Port Control (0x0070) */
#define RT5668_I2S_MS_MASK			(0x1 << 15)
#define RT5668_I2S_MS_SHIFT			15
#define RT5668_I2S_MS_M				(0x0 << 15)
#define RT5668_I2S_MS_S				(0x1 << 15)
#define RT5668_I2S_BP_MASK			(0x1 << 8)
#define RT5668_I2S_BP_SHIFT			8
#define RT5668_I2S_BP_NOR			(0x0 << 8)
#define RT5668_I2S_BP_INV			(0x1 << 8)
#define RT5668_I2S_DL_MASK			(0x3 << 4)
#define RT5668_I2S_DL_SHIFT			4
#define RT5668_I2S_DL_16			(0x0 << 4)
#define RT5668_I2S_DL_20			(0x1 << 4)
#define RT5668_I2S_DL_24			(0x2 << 4)
#define RT5668_I2S_DL_8				(0x3 << 4)
#define RT5668_I2S_DF_MASK			(0x7)
#define RT5668_I2S_DF_SHIFT			0
#define RT5668_I2S_DF_I2S			(0x0)
#define RT5668_I2S_DF_LEFT			(0x1)
#define RT5668_I2S_DF_PCM_A			(0x2)
#define RT5668_I2S_DF_PCM_B			(0x3)
#define RT5668_I2S_DF_PCM_A_N			(0x6)
#define RT5668_I2S_DF_PCM_B_N			(0x7)

/* ADC/DAC Clock Control 1 (0x0073) */
#define RT5668_I2S_PD1_MASK			(0x7 << 12)
#define RT5668_I2S_PD1_SHIFT			12
#define RT5668_M_I2S_DIV_MASK			(0x7 << 8)
#define RT5668_M_I2S_DIV_SHIFT			8
#define RT5668_CLK_SRC_MASK			(0x3 << 4)
#define RT5668_CLK_SRC_MCLK			(0x0 << 4)
#define RT5668_CLK_SRC_PLL_OUT			(0x1 << 4)
#define RT5668_CLK_SRC_DIV			(0x2 << 4)
#define RT5668_CLK_SRC_RC			(0x3 << 4)
#define RT5668_DAC_OSR_MASK			(0x3 << 2)
#define RT5668_DAC_OSR_SHIFT			2
#define RT5668_DAC_OSR_128			(0x0 << 2)
#define RT5668_DAC_OSR_64			(0x1 << 2)
#define RT5668_DAC_OSR_32			(0x2 << 2)
#define RT5668_ADC_OSR_MASK			(0x3)
#define RT5668_ADC_OSR_SHIFT			0
#define RT5668_ADC_OSR_128			(0x0)
#define RT5668_ADC_OSR_64			(0x1)
#define RT5668_ADC_OSR_32			(0x2)

/* TDM1 control 1 (0x0078) */
#define RT5668_TDM_MODE_MASK			(0x1 << 15)
#define RT5668_TDM_MODE_SHIFT			15
#define RT5668_TDM_MODE_I2S			(0x0 << 15)
#define RT5668_TDM_MODE_TDM			(0x1 << 15)
#define RT5668_TDM_IN_CH_MASK			(0x3 << 10)
#define RT5668_TDM_IN_CH_SHIFT			10
#define RT5668_TDM_IN_CH_2			(0x0 << 10)
#define RT5668_TDM_IN_CH_4			(0x1 << 10)
#define RT5668_TDM_IN_CH_6			(0x2 << 10)
#define RT5668_TDM_IN_CH_8			(0x3 << 10)
#define RT5668_TDM_OUT_CH_MASK			(0x3 << 8)
#define RT5668_TDM_OUT_CH_SHIFT			8
#define RT5668_TDM_OUT_CH_2			(0x0 << 8)
#define RT5668_TDM_OUT_CH_4			(0x1 << 8)
#define RT5668_TDM_OUT_CH_6			(0x2 << 8)
#define RT5668_TDM_OUT_CH_8			(0x3 << 8)
#define RT5668_TDM_IN_LEN_MASK			(0x3 << 6)
#define RT5668_TDM_IN_LEN_SHIFT			6
#define RT5668_TDM_IN_LEN_16			(0x0 << 6)
#define RT5668_TDM_IN_LEN_20			(0x1 << 6)
#define RT5668_TDM_IN_LEN_24			(0x2 << 6)
#define RT5668_TDM_IN_LEN_32			(0x3 << 6)
#define RT5668_TDM_OUT_LEN_MASK			(0x3 << 4)
#define RT5668_TDM_OUT_LEN_SHIFT		4
#define RT5668_TDM_OUT_LEN_16			(0x0 << 4)
#define RT5668_TDM_OUT_LEN_20			(0x1 << 4)
#define RT5668_TDM_OUT_LEN_24			(0x2 << 4)
#define RT5668_TDM_OUT_LEN_32			(0x3 << 4)

/* Global Clock Control (0x0080) */
#define RT5668_SCLK_SRC_MASK			(0x3 << 14)
#define RT5668_SCLK_SRC_SHIFT			14
#define RT5668_SCLK_SRC_MCLK			(0x0 << 14)
#define RT5668_SCLK_SRC_PLL1			(0x1 << 14)
#define RT5668_SCLK_SRC_RCCLK			(0x2 << 14)
#define RT5668_PLL1_SRC_MASK			(0x7 << 8)
#define RT5668_PLL1_SRC_SHIFT			8
#define RT5668_PLL1_SRC_MCLK			(0x0 << 8)
#define RT5668_PLL1_SRC_BCLK1			(0x1 << 8)
#define RT5668_PLL1_PD_MASK			(0x1 << 4)
#define RT5668_PLL1_PD_SHIFT			4

#define RT5668_PLL_INP_MAX			40000000
#define RT5668_PLL_INP_MIN			256000
/* PLL M/N/K Code Control 1 (0x0081) */
#define RT5668_PLL_N_MAX			0x001ff
#define RT5668_PLL_N_MASK			(RT5668_PLL_N_MAX << 7)
#define RT5668_PLL_N_SHIFT			7
#define RT5668_PLL_K_MAX			0x001f
#define RT5668_PLL_K_MASK			(RT5668_PLL_K_MAX)
#define RT5668_PLL_K_SHIFT			0

/* PLL M/N/K Code Control 2 (0x0082) */
#define RT5668_PLL_M_MAX			0x00f
#define RT5668_PLL_M_MASK			(RT5668_PLL_M_MAX << 12)
#define RT5668_PLL_M_SHIFT			12
#define RT5668_PLL_M_BP				(0x1 << 11)
#define RT5668_PLL_M_BP_SHIFT			11

/* PLL tracking mode 1 (0x0083) */
#define RT5668_I2S1_ASRC_MASK			(0x1 << 13)
#define RT5668_I2S1_ASRC_SHIFT			13
#define RT5668_DAC_STO1_ASRC_MASK		(0x1 << 12)
#define RT5668_DAC_STO1_ASRC_SHIFT		12
#define RT5668_ADC_STO1_ASRC_MASK		(0x1 << 4)
#define RT5668_ADC_STO1_ASRC_SHIFT		4

/* PLL tracking mode 2 (0x0084)*/
#define RT5668_DA_STO1_TRACK_MASK		(0x7 << 12)
#define RT5668_DA_STO1_TRACK_SHIFT		12
#define RT5668_DA_STO1_TRACK_SYSCLK		(0x0 << 12)
#define RT5668_DA_STO1_TRACK_I2S1		(0x1 << 12)

/* PLL tracking mode 3 (0x0085)*/
#define RT5668_AD_STO1_TRACK_MASK		(0x7 << 12)
#define RT5668_AD_STO1_TRACK_SHIFT		12
#define RT5668_AD_STO1_TRACK_SYSCLK		(0x0 << 12)
#define RT5668_AD_STO1_TRACK_I2S1		(0x1 << 12)

/* HPOUT Charge pump control 1 (0x0091) */
#define RT5668_OSW_HP_L_MASK			(0x1 << 11)
#define RT5668_OSW_HP_L_SHIFT			11
#define RT5668_OSW_HP_L_EN			(0x1 << 11)
#define RT5668_OSW_HP_L_DIS			(0x0 << 11)
#define RT5668_OSW_HP_R_MASK			(0x1 << 10)
#define RT5668_OSW_HP_R_SHIFT			10
#define RT5668_OSW_HP_R_EN			(0x1 << 10)
#define RT5668_OSW_HP_R_DIS			(0x0 << 10)
#define RT5668_SEL_PM_HP_MASK			(0x3 << 8)
#define RT5668_SEL_PM_HP_SHIFT			8
#define RT5668_SEL_PM_HP_0_6			(0x0 << 8)
#define RT5668_SEL_PM_HP_0_9			(0x1 << 8)
#define RT5668_SEL_PM_HP_1_8			(0x2 << 8)
#define RT5668_SEL_PM_HP_HIGH			(0x3 << 8)
#define RT5668_OVCD_HP_MASK			(0x1 << 2)
#define RT5668_OVCD_HP_SHIFT			2
#define RT5668_OVCD_HP_EN			(0x1 << 2)
#define RT5668_OVCD_HP_DIS			(0x0 << 2)

/* RC Clock Control (0x0094) */
#define RT5668_DIG_25M_CLK_MASK			(0x1 << 9)
#define RT5668_DIG_25M_CLK_SHIFT		9
#define RT5668_DIG_25M_CLK_DIS			(0x0 << 9)
#define RT5668_DIG_25M_CLK_EN			(0x1 << 9)
#define RT5668_DIG_1M_CLK_MASK			(0x1 << 8)
#define RT5668_DIG_1M_CLK_SHIFT			8
#define RT5668_DIG_1M_CLK_DIS			(0x0 << 8)
#define RT5668_DIG_1M_CLK_EN			(0x1 << 8)

/* Auto Turn On 1M RC CLK (0x009f) */
#define RT5668_IRQ_POW_SAV_MASK			(0x1 << 15)
#define RT5668_IRQ_POW_SAV_SHIFT		15
#define RT5668_IRQ_POW_SAV_DIS			(0x0 << 15)
#define RT5668_IRQ_POW_SAV_EN			(0x1 << 15)
#define RT5668_IRQ_POW_SAV_JD1_MASK		(0x1 << 14)
#define RT5668_IRQ_POW_SAV_JD1_SHIFT		14
#define RT5668_IRQ_POW_SAV_JD1_DIS		(0x0 << 14)
#define RT5668_IRQ_POW_SAV_JD1_EN		(0x1 << 14)

/* IRQ Control 1 (0x00b6) */
#define RT5668_EN_CB_JD_MASK			(0x1 << 3)
#define RT5668_EN_CB_JD_SHIFT			3
#define RT5668_EN_CB_JD_EN			(0x1 << 3)
#define RT5668_EN_CB_JD_DIS			(0x0 << 3)

/* IRQ Control 3 (0x00b8) */
#define RT5668_EN_IRQ_INLINE_MASK		(0x1 << 6)
#define RT5668_EN_IRQ_INLINE_SHIFT		6
#define RT5668_EN_IRQ_INLINE_BYP		(0x0 << 6)
#define RT5668_EN_IRQ_INLINE_NOR		(0x1 << 6)

/* GPIO Control 1 (0x00c0) */
#define RT5668_GP1_PIN_MASK			(0x1 << 15)
#define RT5668_GP1_PIN_SHIFT			15
#define RT5668_GP1_PIN_GPIO1			(0x0 << 15)
#define RT5668_GP1_PIN_IRQ			(0x1 << 15)

/* GPIO Control 2 (0x00c1) */
#define RT5668_GP4_PIN_CONF_MASK		(0x1 << 5)
#define RT5668_GP4_PIN_CONF_SHIFT		5
#define RT5668_GP4_PIN_CONF_INPUT		(0x0 << 5)
#define RT5668_GP4_PIN_CONF_OUTPUT		(0x1 << 5)

/* GPIO Control 2 (0x00c2) */
#define RT5668_GP8_PIN_CONF_MASK		(0x1 << 13)
#define RT5668_GP8_PIN_CONF_SHIFT		13
#define RT5668_GP8_PIN_CONF_INPUT		(0x0 << 13)
#define RT5668_GP8_PIN_CONF_OUTPUT		(0x1 << 13)

/* 4 Buttons Inline Command Function 1 (0x00df) */
#define RT5668_4BTN_CLK_DEB_MASK		(0x3 << 2)
#define RT5668_4BTN_CLK_DEB_SHIFT		2
#define RT5668_4BTN_CLK_DEB_8MS			(0x0 << 2)
#define RT5668_4BTN_CLK_DEB_16MS		(0x1 << 2)
#define RT5668_4BTN_CLK_DEB_32MS		(0x2 << 2)
#define RT5668_4BTN_CLK_DEB_65MS		(0x3 << 2)

/* Inline Command Function 6 (0x00e0) */
#define RT5668_EN_4BTN_INL_MASK			(0x1 << 15)
#define RT5668_EN_4BTN_INL_SHIFT		15
#define RT5668_EN_4BTN_INL_DIS			(0x0 << 15)
#define RT5668_EN_4BTN_INL_EN			(0x1 << 15)
#define RT5668_RESET_4BTN_INL_MASK		(0x1 << 14)
#define RT5668_RESET_4BTN_INL_SHIFT		14
#define RT5668_RESET_4BTN_INL_RESET		(0x0 << 14)
#define RT5668_RESET_4BTN_INL_NOR		(0x1 << 14)

/* Digital Misc Control (0x00fa) */
#define RT5668_DIG_GATE_CTRL_MASK		0x1
#define RT5668_DIG_GATE_CTRL_SHIFT		(0)
#define RT5668_DIG_GATE_CTRL_DIS		0x0
#define RT5668_DIG_GATE_CTRL_EN			0x1

/* Chopper and Clock control for DAC L (0x013a)*/
#define RT5668_CKXEN_DAC1_MASK			(0x1 << 13)
#define RT5668_CKXEN_DAC1_SHIFT			13
#define RT5668_CKGEN_DAC1_MASK			(0x1 << 12)
#define RT5668_CKGEN_DAC1_SHIFT			12

/* Chopper and Clock control for ADC (0x013b)*/
#define RT5668_CKXEN_ADCC_MASK			(0x1 << 13)
#define RT5668_CKXEN_ADCC_SHIFT			13
#define RT5668_CKGEN_ADCC_MASK			(0x1 << 12)
#define RT5668_CKGEN_ADCC_SHIFT			12

/* HP Behavior Logic Control 2 (0x01db) */
#define RT5668_HP_SIG_SRC1_MASK			(0x3)
#define RT5668_HP_SIG_SRC1_SHIFT		0
#define RT5668_HP_SIG_SRC1_HP_DC		(0x0)
#define RT5668_HP_SIG_SRC1_HP_CALIB		(0x1)
#define RT5668_HP_SIG_SRC1_REG			(0x2)
#define RT5668_HP_SIG_SRC1_SILENCE		(0x3)

/* RT5663 specific register */
#define RT5663_RECMIX				0x0010
#define RT5663_BYPASS_STO_DAC			0x002d
#define RT5663_PRE_DIV_GATING_1			0x006e
#define RT5663_PRE_DIV_GATING_2			0x006f
#define RT5663_TDM_1				0x0077
#define RT5663_TDM_5				0x007b
#define RT5663_EM_JACK_TYPE_1			0x00e6
#define RT5663_EM_JACK_TYPE_2			0x00e7
#define RT5663_ANA_BIAS_CUR_1			0x0108
#define RT5663_DACREF_LDO			0x0112
#define RT5663_VREF_RECMIX			0x0118
#define RT5663_CHARGE_PUMP_1			0x0125
#define RT5663_CHARGE_PUMP_2			0x0128
#define RT5663_CHOP_DAC_R			0x013d
#define RT5663_STO_DRE_9			0x0168
#define RT5663_STO_DRE_10			0x0169
#define RT5663_HP_BIAS				0x0202
#define RT5663_CBJ_1				0x0250
#define RT5663_CBJ_2				0x0251
#define RT5663_DUMMY_1				0x02fa

/* RT5663: RECMIX Control (0x0010) */
#define RT5663_RECMIX1_BST1_MASK		(0x1)
#define RT5663_RECMIX1_BST1_SHIFT		0
#define RT5663_RECMIX1_BST1_UNMUTE		(0x0)
#define RT5663_RECMIX1_BST1_MUTE		(0x1)

/* RT5663: Bypass Stereo1 DAC Mixer Control (0x002d) */
#define RT5663_DACL1_SRC_MASK			(0x1 << 3)
#define RT5663_DACL1_SRC_SHIFT			3
#define RT5663_DACR1_SRC_MASK			(0x1 << 2)
#define RT5663_DACR1_SRC_SHIFT			2

/* RT5663: TDM control 2 (0x0078) */
#define RT5663_DATA_SWAP_ADCDAT1_MASK		(0x3 << 14)
#define RT5663_DATA_SWAP_ADCDAT1_SHIFT		14
#define RT5663_DATA_SWAP_ADCDAT1_LR		(0x0 << 14)
#define RT5663_DATA_SWAP_ADCDAT1_RL		(0x1 << 14)
#define RT5663_DATA_SWAP_ADCDAT1_LL		(0x2 << 14)
#define RT5663_DATA_SWAP_ADCDAT1_RR		(0x3 << 14)

/* RT5663: TDM control 5 (0x007b) */
#define RT5663_TDM_LENGTN_MASK			(0x3)
#define RT5663_TDM_LENGTN_SHIFT			0
#define RT5663_TDM_LENGTN_16			(0x0)
#define RT5663_TDM_LENGTN_20			(0x1)
#define RT5663_TDM_LENGTN_24			(0x2)
#define RT5663_TDM_LENGTN_32			(0x3)

/* RT5663: Global Clock Control (0x0080) */
#define RT5663_PLL1_SRC_MASK			(0x7 << 11)
#define RT5663_PLL1_SRC_SHIFT			11
#define RT5663_PLL1_SRC_MCLK			(0x0 << 11)
#define RT5663_PLL1_SRC_BCLK1			(0x1 << 11)

/* PLL tracking mode 1 (0x0083) */
#define RT5663_I2S1_ASRC_MASK			(0x1 << 11)
#define RT5663_I2S1_ASRC_SHIFT			11
#define RT5663_DAC_STO1_ASRC_MASK		(0x1 << 10)
#define RT5663_DAC_STO1_ASRC_SHIFT		10
#define RT5663_ADC_STO1_ASRC_MASK		(0x1 << 3)
#define RT5663_ADC_STO1_ASRC_SHIFT		3

/* PLL tracking mode 2 (0x0084)*/
#define RT5663_AD_STO1_TRACK_MASK		(0x7)
#define RT5663_AD_STO1_TRACK_SHIFT		0
#define RT5663_AD_STO1_TRACK_SYSCLK		(0x0)
#define RT5663_AD_STO1_TRACK_I2S1		(0x1)

/* RT5663: HPOUT Charge pump control 1 (0x0091) */
#define RT5663_SI_HP_MASK			(0x1 << 12)
#define RT5663_SI_HP_SHIFT			12
#define RT5663_SI_HP_EN				(0x1 << 12)
#define RT5663_SI_HP_DIS			(0x0 << 12)

/* RT5663: GPIO Control 2 (0x00b6) */
#define RT5663_GP1_PIN_CONF_MASK		(0x1 << 2)
#define RT5663_GP1_PIN_CONF_SHIFT		2
#define RT5663_GP1_PIN_CONF_OUTPUT		(0x1 << 2)
#define RT5663_GP1_PIN_CONF_INPUT		(0x0 << 2)

/* RT5663: GPIO Control 2 (0x00b7) */
#define RT5663_EN_IRQ_INLINE_MASK		(0x1 << 3)
#define RT5663_EN_IRQ_INLINE_SHIFT		3
#define RT5663_EN_IRQ_INLINE_NOR		(0x1 << 3)
#define RT5663_EN_IRQ_INLINE_BYP		(0x0 << 3)

/* RT5663: IRQ Control 1 (0x00c1) */
#define RT5663_EN_IRQ_JD1_MASK			(0x1 << 6)
#define RT5663_EN_IRQ_JD1_SHIFT			6
#define RT5663_EN_IRQ_JD1_EN			(0x1 << 6)
#define RT5663_EN_IRQ_JD1_DIS			(0x0 << 6)

/* RT5663: Inline Command Function 2 (0x00dc) */
#define RT5663_PWR_MIC_DET_MASK			(0x1)
#define RT5663_PWR_MIC_DET_SHIFT		0
#define RT5663_PWR_MIC_DET_ON			(0x1)
#define RT5663_PWR_MIC_DET_OFF			(0x0)

/* RT5663: Embeeded Jack and Type Detection Control 1 (0x00e6)*/
#define RT5663_CBJ_DET_MASK			(0x1 << 15)
#define RT5663_CBJ_DET_SHIFT			15
#define RT5663_CBJ_DET_DIS			(0x0 << 15)
#define RT5663_CBJ_DET_EN			(0x1 << 15)
#define RT5663_EXT_JD_MASK			(0x1 << 11)
#define RT5663_EXT_JD_SHIFT			11
#define RT5663_EXT_JD_EN			(0x1 << 11)
#define RT5663_EXT_JD_DIS			(0x0 << 11)
#define RT5663_POL_EXT_JD_MASK			(0x1 << 10)
#define RT5663_POL_EXT_JD_SHIFT			10
#define RT5663_POL_EXT_JD_EN			(0x1 << 10)
#define RT5663_POL_EXT_JD_DIS			(0x0 << 10)

/* RT5663: DACREF LDO Control (0x0112)*/
#define RT5663_PWR_LDO_DACREFL_MASK		(0x1 << 9)
#define RT5663_PWR_LDO_DACREFL_SHIFT		9
#define RT5663_PWR_LDO_DACREFR_MASK		(0x1 << 1)
#define RT5663_PWR_LDO_DACREFR_SHIFT		1

/* RT5663: Stereo Dynamic Range Enhancement Control 9 (0x0168, 0x0169)*/
#define RT5663_DRE_GAIN_HP_MASK			(0x1f)
#define RT5663_DRE_GAIN_HP_SHIFT		0

/* RT5663: Combo Jack Control (0x0250) */
#define RT5663_INBUF_CBJ_BST1_MASK		(0x1 << 11)
#define RT5663_INBUF_CBJ_BST1_SHIFT		11
#define RT5663_INBUF_CBJ_BST1_ON		(0x1 << 11)
#define RT5663_INBUF_CBJ_BST1_OFF		(0x0 << 11)
#define RT5663_CBJ_SENSE_BST1_MASK		(0x1 << 10)
#define RT5663_CBJ_SENSE_BST1_SHIFT		10
#define RT5663_CBJ_SENSE_BST1_L			(0x1 << 10)
#define RT5663_CBJ_SENSE_BST1_R			(0x0 << 10)

/* RT5663: Combo Jack Control (0x0251) */
#define RT5663_GAIN_BST1_MASK			(0xf)
#define RT5663_GAIN_BST1_SHIFT			0

/* RT5663: Dummy register 1 (0x02fa) */
#define RT5663_EMB_CLK_MASK			(0x1 << 9)
#define RT5663_EMB_CLK_SHIFT			9
#define RT5663_EMB_CLK_EN			(0x1 << 9)
#define RT5663_EMB_CLK_DIS			(0x0 << 9)
#define RT5663_HPA_CPL_BIAS_MASK		(0x7 << 6)
#define RT5663_HPA_CPL_BIAS_SHIFT		6
#define RT5663_HPA_CPL_BIAS_0_5			(0x0 << 6)
#define RT5663_HPA_CPL_BIAS_1			(0x1 << 6)
#define RT5663_HPA_CPL_BIAS_2			(0x2 << 6)
#define RT5663_HPA_CPL_BIAS_3			(0x3 << 6)
#define RT5663_HPA_CPL_BIAS_4_1			(0x4 << 6)
#define RT5663_HPA_CPL_BIAS_4_2			(0x5 << 6)
#define RT5663_HPA_CPL_BIAS_6			(0x6 << 6)
#define RT5663_HPA_CPL_BIAS_8			(0x7 << 6)
#define RT5663_HPA_CPR_BIAS_MASK		(0x7 << 3)
#define RT5663_HPA_CPR_BIAS_SHIFT		3
#define RT5663_HPA_CPR_BIAS_0_5			(0x0 << 3)
#define RT5663_HPA_CPR_BIAS_1			(0x1 << 3)
#define RT5663_HPA_CPR_BIAS_2			(0x2 << 3)
#define RT5663_HPA_CPR_BIAS_3			(0x3 << 3)
#define RT5663_HPA_CPR_BIAS_4_1			(0x4 << 3)
#define RT5663_HPA_CPR_BIAS_4_2			(0x5 << 3)
#define RT5663_HPA_CPR_BIAS_6			(0x6 << 3)
#define RT5663_HPA_CPR_BIAS_8			(0x7 << 3)
#define RT5663_DUMMY_BIAS_MASK			(0x7)
#define RT5663_DUMMY_BIAS_SHIFT			0
#define RT5663_DUMMY_BIAS_0_5			(0x0)
#define RT5663_DUMMY_BIAS_1			(0x1)
#define RT5663_DUMMY_BIAS_2			(0x2)
#define RT5663_DUMMY_BIAS_3			(0x3)
#define RT5663_DUMMY_BIAS_4_1			(0x4)
#define RT5663_DUMMY_BIAS_4_2			(0x5)
#define RT5663_DUMMY_BIAS_6			(0x6)
#define RT5663_DUMMY_BIAS_8			(0x7)


/* System Clock Source */
enum {
	RT5668_SCLK_S_MCLK,
	RT5668_SCLK_S_PLL1,
	RT5668_SCLK_S_RCCLK,
};

/* PLL1 Source */
enum {
	RT5668_PLL1_S_MCLK,
	RT5668_PLL1_S_BCLK1,
};

enum {
	RT5668_AIF,
	RT5668_AIFS,
};

/* asrc clock source */
enum {
	RT5668_CLK_SEL_SYS = 0x0,
	RT5668_CLK_SEL_I2S1_ASRC = 0x1,
};

/* filter mask */
enum {
	RT5668_DA_STEREO_FILTER = 0x1,
	RT5668_AD_STEREO_FILTER = 0x2,
};

int rt5668_headset_detect(struct snd_soc_codec *codec, int jack_insert);
int rt5668_button_detect(struct snd_soc_codec *codec);
int rt5668_set_jack_detect(struct snd_soc_codec *codec,
	struct snd_soc_jack *hs_jack);
int rt5668_sel_asrc_clk_src(struct snd_soc_codec *codec,
	unsigned int filter_mask, unsigned int clk_src);

#endif /* __RT5668_H__ */
