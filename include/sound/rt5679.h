/*
 * linux/sound/rt5679.h -- Platform data for ALC5679
 *
 * Copyright 2015 Realtek Semiconductor Corp.
 * Author: Oder Chiou <oder_chiou@realtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LINUX_SND_RT5679_H
#define __LINUX_SND_RT5679_H

enum rt5679_dmic1_clk_pin {
	RT5679_DMIC1_CLK_NULL,
	RT5679_DMIC1_CLK_GPIO2,
	RT5679_DMIC1_CLK_GPIO21,
};

enum rt5679_dmic1_data_pin {
	RT5679_DMIC1_DATA_NULL,
	RT5679_DMIC1_DATA_GPIO22,
	RT5679_DMIC1_DATA_IN4N,
};

enum rt5679_dmic2_clk_pin {
	RT5679_DMIC2_CLK_NULL,
	RT5679_DMIC2_CLK_GPIO3,
	RT5679_DMIC2_CLK_GPIO4,
	RT5679_DMIC2_CLK_MCLK2,
};

enum rt5679_dmic2_data_pin {
	RT5679_DMIC2_DATA_NULL,
	RT5679_DMIC2_DATA_GPIO23,
	RT5679_DMIC2_DATA_IN4P,
};

enum rt5679_dmic3_clk_pin {
	RT5679_DMIC3_CLK_NULL,
	RT5679_DMIC3_CLK_GPIO5,
	RT5679_DMIC3_CLK_GPIO16,
};

enum rt5679_dmic3_data_pin {
	RT5679_DMIC3_DATA_NULL,
	RT5679_DMIC3_DATA_GPIO24,
	RT5679_DMIC3_DATA_IN3N,
};

enum rt5679_dmic4_clk_pin {
	RT5679_DMIC4_CLK_NULL,
	RT5679_DMIC4_CLK_GPIO7,
};

enum rt5679_dmic4_data_pin {
	RT5679_DMIC4_DATA_NULL,
	RT5679_DMIC4_DATA_GPIO6,
	RT5679_DMIC4_DATA_IN3P,
};

struct rt5679_platform_data {
	int gpio_ldo;
	int gpio_reset;

	bool in1_diff;
	bool in2_diff;
	bool in3_diff;
	bool in4_diff;
	bool lout1_diff;
	bool lout2_diff;

	enum rt5679_dmic1_clk_pin dmic1_clk_pin;
	enum rt5679_dmic1_data_pin dmic1_data_pin;
	enum rt5679_dmic2_clk_pin dmic2_clk_pin;
	enum rt5679_dmic2_data_pin dmic2_data_pin;
	enum rt5679_dmic3_clk_pin dmic3_clk_pin;
	enum rt5679_dmic3_data_pin dmic3_data_pin;
	enum rt5679_dmic4_clk_pin dmic4_clk_pin;
	enum rt5679_dmic4_data_pin dmic4_data_pin;

	bool use_ldo_avdd;
};

#endif

