/*
 * (C) Copyright 2011 Samsung Electronics Co. Ltd
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <common.h>

#define ISRAM_ADDRESS	0x02020000
#define EXTERNAL_FUNC_ADDRESS	(ISRAM_ADDRESS + 0x0030)

#define SDMMC_ReadBlocks(uStartBlk, uNumOfBlks, uDstAddr)	\
	(((void(*)(u32, u32, u32*))(*((u32 *)EXTERNAL_FUNC_ADDRESS)))(uStartBlk, uNumOfBlks, uDstAddr))

extern void uart_asm_putc(int c);
extern void uart_asm_putx(int x);

#define MT_START	0x40000000
#define MT_LEN		0x10000000

void uboot_mem_test(void)
{
	unsigned int *p;
	int i;

	/* show dram config */
	p = (unsigned int *) 0x10600000;
	for (i = 0; i < 0x100/4; i++, p++) {
		if (!(i & 0xf)) {
			uart_asm_putc('\r');
			uart_asm_putc('\n');
		}
		uart_asm_putx(*p);
		uart_asm_putc('.');
	}
	p = (unsigned int *) 0x10610000;
	for (i = 0; i < 0x100/4; i++, p++) {
		if (!(i & 0xf)) {
			uart_asm_putc('\r');
			uart_asm_putc('\n');
		}
		uart_asm_putx(*p);
		uart_asm_putc('.');
	}

	uart_asm_putc('\r');
	uart_asm_putc('\n');

	uart_asm_putc('L');
	uart_asm_putc('e');
	uart_asm_putc('n');
	uart_asm_putc(':');
	uart_asm_putc(' ');
	uart_asm_putx(MT_LEN);

	uart_asm_putc('\r');
	uart_asm_putc('\n');
	uart_asm_putc('W');

	/* writing */
	p = (unsigned int *) MT_START;
	for (i = 0; i < MT_LEN/4; i++, p++) {
		*p = i+0x5a000000;
		if (!(i & 0xfffff)) {
			uart_asm_putc('.');
		}
	}

	uart_asm_putc('\r');
	uart_asm_putc('\n');
	uart_asm_putc('R');

	/* read and verify */
	p = (unsigned int *) MT_START;
	for (i = 0; i < MT_LEN/4; i++, p++) {
		if (*p != (i+0x5a000000)) {
			uart_asm_putc('X');
			uart_asm_putx(i);
			uart_asm_putx((int)p);
			if (i > 4) {
				uart_asm_putx(*(p-4));
				uart_asm_putx(*(p-3));
				uart_asm_putx(*(p-2));
				uart_asm_putx(*(p-1));
			}
			uart_asm_putx(*p);
			uart_asm_putx(*(p+1));
			uart_asm_putx(*(p+2));
			uart_asm_putx(*(p+3));
			uart_asm_putx(*(p+4));
			break;
		}

		if (!(i & 0xfffff)) {
			uart_asm_putc('.');
		}
	}

	uart_asm_putc('\r');
	uart_asm_putc('\n');
	uart_asm_putc('>');

}

void movi_uboot_copy(void)
{
	//uboot_mem_test();
	SDMMC_ReadBlocks(MOVI_UBOOT_POS, MOVI_UBOOT_BLKCNT, (u32 *)(CONFIG_PHY_UBOOT_BASE));
}

