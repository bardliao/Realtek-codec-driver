/*
 * rt5679-spi.h  --  ALC5679 ALSA SoC audio codec driver
 *
 * Copyright 2015 Realtek Semiconductor Corp.
 * Author: Oder Chiou <oder_chiou@realtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __RT5679_SPI_H__
#define __RT5679_SPI_H__

#define RT5679_SPI_BUF_LEN 240

/* SPI Command */
enum {
	RT5679_SPI_CMD_16_READ = 0,
	RT5679_SPI_CMD_16_WRITE,
	RT5679_SPI_CMD_32_READ,
	RT5679_SPI_CMD_32_WRITE,
	RT5679_SPI_CMD_BURST_READ,
	RT5679_SPI_CMD_BURST_WRITE,
};

int rt5679_spi_read(u32 addr, unsigned int *val, size_t len);
int rt5679_spi_write(u32 addr, unsigned int val, size_t len);
int rt5679_spi_burst_read(u32 addr, u8 *rxbuf, size_t len);
int rt5679_spi_burst_write(u32 addr, const u8 *txbuf, size_t len);

#endif /* __RT5679_SPI_H__ */
