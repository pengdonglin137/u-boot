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

#include <config.h>
#include <version.h>
#include <asm/arch/cpu.h>

#include "tiny4412_val.h"

_TEXT_BASE:
	.word	CONFIG_SYS_TEXT_BASE

	.globl lowlevel_init
lowlevel_init:

	/* use iROM stack in bl2 */
	ldr	sp, =0x02060000
	push	{lr}

	ldr	r1, =0x08
	ldr	r0, =0x110002E0
	str	r1, [r0, #0x04]

	/* set CP reset to low */
	ldr	r0, =0x11000C60
	ldr	r1, [r0]
	ldr	r2, =0xFFFFFF0F
	and	r1, r1, r2
	orr	r1, r1, #0x10
	str	r1, [r0]
	ldr	r0, =0x11000C68
	ldr	r1, [r0]
	ldr	r2, =0xFFFFFFF3
	and	r1, r1, r2
	orr	r1, r1, #0x4
	str	r1, [r0]
	ldr	r0, =0x11000C64
	ldr	r1, [r0]
	ldr	r2, =0xFFFFFFFD
	and	r1, r1, r2
	str	r1, [r0]

	/* During sleep/wakeup or AFTR mode, pmic_init function is not available
	 * and it causes delays. So except for sleep/wakeup and AFTR mode,
	 * the below function is needed
	 */

	bl	read_om

	/* when we already run in ram, we don't need to relocate U-Boot.
	 * and actually, memory controller must be configured before U-Boot
	 * is running in ram.
	 */
	ldr	r0, =0xff000fff
	bic	r1, pc, r0		/* r0 <- current base addr of code */
	ldr	r2, _TEXT_BASE	/* r1 <- original base addr in ram */
	bic	r2, r2, r0		/* r0 <- current base addr of code */
	cmp	r1, r2			/* compare r0, r1 */
	beq	after_copy		/* r0 == r1 then skip sdram init and u-boot.bin loading */

	/* init system clock */
	bl	system_clock_init

	/* Memory initialize */
	bl	mem_ctrl_asm_init

	/* init uart for debug */
	bl	uart_asm_init

	mov	r4, #0x4000
.L0:
	sub	r4, r4, #1
	cmp	r4, #0
	bne	.L0

	mov	r0, #'\r'
	bl	uart_asm_putc
	mov	r0, #'\n'
	bl	uart_asm_putc

	ldr	r1, =0x40000000
	ldr	r2, =0x87654321
	str	r2, [r1]
	str	r2, [r1, #0x04]
	str	r2, [r1, #0x08]
	ldr	r2, =0x55aaaa55
	str	r2, [r1, #0x10]
	nop

	mov	r4, #0xC0000
.L1:
	subs	r4, r4, #1
	bne	.L1

	ldr	r0, [r1]
	bl	uart_asm_putx
	mov	r0, #'.'
	bl	uart_asm_putc

	ldr	r0, [r1, #0x04]
	bl	uart_asm_putx
	mov	r0, #'.'
	bl	uart_asm_putc

	ldr	r0, [r1, #0x08]
	bl	uart_asm_putx
	mov	r0, #'.'
	bl	uart_asm_putc

	ldr	r0, [r1, #0x10]
	bl	uart_asm_putx
	mov	r0, #'>'
	bl	uart_asm_putc

	b	load_uboot

after_copy:
	/* set up C2C */
	ldr	r0, =EXYNOS4X12_SYSREG_BASE
	ldr	r2, =GENERAL_CTRL_C2C_OFFSET
	ldr	r1, [r0, r2]
	ldr	r3, =0x4000
	orr	r1, r1, r3
	str	r1, [r0, r2]

	/* Print 'K' */
	ldr	r0, =S5PV310_UART_CONSOLE_BASE
	ldr	r1, =0x4b4b4b4b
	str	r1, [r0, #UTXH_OFFSET]

	pop	{pc}

read_om:
	/* Read booting information */
	ldr	r0, =EXYNOS4X12_POWER_BASE
	ldr	r1, [r0,#0]
	bic	r2, r1, #0xffffffc1

	/* NAND BOOT */
@	cmp	r2, #0x0		@ 512B 4-cycle
@	moveq	r3, #BOOT_NAND

@	cmp	r2, #0x2		@ 2KB 5-cycle
@	moveq	r3, #BOOT_NAND

@	cmp	r2, #0x4		@ 4KB 5-cycle	8-bit ECC
@	moveq	r3, #BOOT_NAND

	cmp	r2, #0xA
	moveq	r3, #BOOT_ONENAND

	cmp	r2, #0x10		@ 2KB 5-cycle	16-bit ECC
	moveq	r3, #BOOT_NAND

	/* SD/MMC BOOT */
	cmp	r2, #0x4
	moveq	r3, #BOOT_MMCSD

	/* eMMC BOOT */
	cmp	r2, #0x6
	moveq	r3, #BOOT_EMMC

	/* eMMC 4.4 BOOT */
	cmp	r2, #0x8
	moveq	r3, #BOOT_EMMC_4_4
	cmp	r2, #0x28
	moveq	r3, #BOOT_EMMC_4_4

	ldr	r0, =0x10020800
	str	r3, [r0, #0x0c]

	mov	pc, lr


/*
 * uart_asm_init: Initialize UART in asm mode, 115200bps fixed.
 * void uart_asm_init(void)
 */
	.globl uart_asm_init
uart_asm_init:

	/* set GPIO to enable UART */
	@ GPIO setting for UART for UART0/1/2/3
	ldr	r0, =0x11400000
	ldr	r1, =0x22222222
	str	r1, [r0]
	ldr	r0, =0x11400020
	ldr	r1, =0x222222
	str	r1, [r0]

	ldr	r0, =EXYNOS4X12_CLOCK_BASE
	ldr	r1, =CLK_SRC_PERIL0_VAL
	ldr	r2, =CLK_SRC_PERIL0_OFFSET
	str	r1, [r0, r2]
	ldr	r1, =CLK_DIV_PERIL0_VAL
	ldr	r2, =CLK_DIV_PERIL0_OFFSET
	str	r1, [r0, r2]

	ldr	r0, =S5PV310_UART_CONSOLE_BASE
	ldr	r1, =0x111
	str	r1, [r0, #UFCON_OFFSET]

	mov	r1, #0x3
	str	r1, [r0, #ULCON_OFFSET]

	ldr	r1, =0x3c5
	str	r1, [r0, #UCON_OFFSET]

	ldr	r1, =UART_UBRDIV_VAL
	str	r1, [r0, #UBRDIV_OFFSET]

	ldr	r1, =UART_UDIVSLOT_VAL
	str	r1, [r0, #UDIVSLOT_OFFSET]

	ldr	r1, =0x4f4f4f4f
	str	r1, [r0, #UTXH_OFFSET]		@'O'

	mov	pc, lr


load_uboot:
	ldr	r0, =0x10020800
	ldr	r1, [r0, #0x0c]
	cmp	r1, #BOOT_NAND
	beq	nand_boot
	cmp	r1, #BOOT_ONENAND
	beq	onenand_boot
	cmp	r1, #BOOT_MMCSD
	beq	mmcsd_boot
	cmp	r1, #BOOT_EMMC
	beq	emmc_boot
	cmp	r1, #BOOT_EMMC_4_4
	beq	emmc_boot_4_4
	cmp	r1, #BOOT_NOR
	beq	nor_boot
	cmp	r1, #BOOT_SEC_DEV
	beq	mmcsd_boot

nand_boot:
	b nand_boot

onenand_boot:
	b onenand_boot

mmcsd_boot:
	ldr	r0, =EXYNOS4X12_CLOCK_BASE
	ldr	r2, =CLK_DIV_FSYS2_OFFSET
	ldr	r1, [r0, r2]
	orr	r1, r1, #0xf
	str	r1, [r0, r2]
	bl	movi_uboot_copy
	b	after_copy

emmc_boot:
	b emmc_boot

emmc_boot_4_4:
	b emmc_boot_4_4

nor_boot:
	b nor_boot

check_om_setting:
	b	check_om_setting

	.globl uart_asm_putc
uart_asm_putc:
	push	{r9}

	ldr	r9, =S5PV310_UART_CONSOLE_BASE
	str	r0, [r9, #UTXH_OFFSET]
	ldr	r9, =0x20000				@delay

.Luartputc:
	sub	r9, r9, #1
	cmp	r9, #0
	bne	.Luartputc

	pop	{r9}
	mov	pc, lr

	.globl uart_asm_putx
uart_asm_putx:
	stmfd sp!, {r3, r4, r5, lr}

	mov	r5, r0
	mov	r4, #28

.Luartputx:
	mov	r0, r5, asr r4
	and	r0, r0, #15
	cmp	r0, #9
	addle	r0, r0, #48
	addgt	r0, r0, #55
	bl	uart_asm_putc
	sub	r4, r4, #4
	cmn	r4, #4
	bne	.Luartputx

	ldmfd sp!, {r3, r4, r5, pc}

