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
#include <asm/arch/cpu.h>

#include "tiny4412_val.h"

/*
 * system_clock_init: Initialize core clock and bus clock.
 * void system_clock_init(void)
 */
#define MEM_DLLl_ON

	.globl system_clock_init
system_clock_init:
	push	{lr}
	
	ldr	r0, =EXYNOS4X12_CLOCK_BASE	@0x1003_0000

@ CMU_CPU MUX / DIV
	ldr	r1, =0x0
	ldr	r2, =CLK_SRC_CPU_OFFSET
	str	r1, [r0, r2]

	/* wait ?us */
	mov	r1, #0x10000
1:	subs	r1, r1, #1
	bne	1b

	ldr	r1, =CLK_DIV_DMC0_VAL
	ldr	r2, =CLK_DIV_DMC0_OFFSET
	str	r1, [r0, r2]
	ldr	r1, =CLK_DIV_DMC1_VAL
	ldr	r2, =CLK_DIV_DMC1_OFFSET
	str	r1, [r0, r2]

@ CMU_TOP MUX / DIV
	ldr	r1, =CLK_SRC_TOP0_VAL
	ldr	r2, =CLK_SRC_TOP0_OFFSET
	str	r1, [r0, r2]
	ldr	r1, =CLK_SRC_TOP1_VAL
	ldr	r2, =CLK_SRC_TOP1_OFFSET
	str	r1, [r0, r2]

	/* wait ?us */
	mov	r1, #0x10000
3:	subs	r1, r1, #1
	bne	3b

	ldr	r1, =CLK_DIV_TOP_VAL
	ldr	r2, =CLK_DIV_TOP_OFFSET
	str	r1, [r0, r2]

@ CMU_LEFTBUS MUX / DIV
	ldr	r1, =CLK_SRC_LEFTBUS_VAL
	ldr	r2, =CLK_SRC_LEFTBUS_OFFSET
	str	r1, [r0, r2]

	/* wait ?us */
	mov	r1, #0x10000
4:	subs	r1, r1, #1
	bne	4b

	ldr	r1, =CLK_DIV_LEFRBUS_VAL
	ldr	r2, =CLK_DIV_LEFTBUS_OFFSET
	str	r1, [r0, r2]

@ CMU_RIGHTBUS MUX / DIV
	ldr	r1, =CLK_SRC_RIGHTBUS_VAL
	ldr	r2, =CLK_SRC_RIGHTBUS_OFFSET
	str	r1, [r0, r2]

	/* wait ?us */
	mov	r1, #0x10000
5:	subs	r1, r1, #1
	bne	5b

	ldr	r1, =CLK_DIV_RIGHTBUS_VAL
	ldr	r2, =CLK_DIV_RIGHTBUS_OFFSET
	str	r1, [r0, r2]

@ Set PLL locktime
	ldr	r1, =APLL_LOCK_VAL
	ldr	r2, =APLL_LOCK_OFFSET
	str	r1, [r0, r2]

	ldr	r1, =MPLL_LOCK_VAL
	ldr	r2, =MPLL_LOCK_OFFSET
	str	r1, [r0, r2]

	ldr	r1, =EPLL_LOCK_VAL
	ldr	r2, =EPLL_LOCK_OFFSET
	str	r1, [r0, r2]

	ldr	r1, =VPLL_LOCK_VAL
	ldr	r2, =VPLL_LOCK_OFFSET
	str	r1, [r0, r2]

	ldr	r1, =CLK_DIV_CPU0_VAL
	ldr	r2, =CLK_DIV_CPU0_OFFSET
	str	r1, [r0, r2]
	ldr	r1, =CLK_DIV_CPU1_VAL
	ldr	r2, =CLK_DIV_CPU1_OFFSET
	str	r1, [r0, r2]

@ Set APLL
	ldr	r1, =APLL_CON1_VAL
	ldr	r2, =APLL_CON1_OFFSET
	str	r1, [r0, r2]
	ldr	r1, =APLL_CON0_VAL
	ldr	r2, =APLL_CON0_OFFSET
	str	r1, [r0, r2]

	/* check MPLL and if MPLL is not 400 Mhz skip MPLL resetting for C2C operation */
	ldr	r2, =MPLL_CON0_OFFSET
	ldr r1, [r0, r2]
	ldr r3, =0xA0640301
	cmp r1, r3
	bne skip_mpll

@ Set MPLL
	ldr	r1, =MPLL_CON1_VAL
	ldr	r2, =MPLL_CON1_OFFSET
	str	r1, [r0, r2]
	ldr	r1, =MPLL_CON0_VAL
	ldr	r2, =MPLL_CON0_OFFSET
	str	r1, [r0, r2]
skip_mpll:

@ Set EPLL
	ldr     r1, =EPLL_CON2_VAL
	ldr     r2, =EPLL_CON2_OFFSET
	str     r1, [r0, r2]
	ldr     r1, =EPLL_CON1_VAL
	ldr     r2, =EPLL_CON1_OFFSET
	str     r1, [r0, r2]
	ldr     r1, =EPLL_CON0_VAL
	ldr     r2, =EPLL_CON0_OFFSET
	str     r1, [r0, r2]

@ Set VPLL
	ldr     r1, =VPLL_CON2_VAL
	ldr     r2, =VPLL_CON2_OFFSET
	str     r1, [r0, r2]
	ldr     r1, =VPLL_CON1_VAL
	ldr     r2, =VPLL_CON1_OFFSET
	str     r1, [r0, r2]
	ldr     r1, =VPLL_CON0_VAL
	ldr     r2, =VPLL_CON0_OFFSET
	str     r1, [r0, r2]

	/* wait ?us */
	mov	r1, #0x40000
6:	subs	r1, r1, #1
	bne	6b

	ldr	r1, =0x01000001
	ldr	r2, =CLK_SRC_CPU_OFFSET
	str	r1, [r0, r2]
	ldr	r1, =0x00011000
	ldr	r2, =CLK_SRC_DMC_OFFSET
	str	r1, [r0, r2]
	ldr	r1, =0x00000110
	ldr	r2, =CLK_SRC_TOP0_OFFSET
	str	r1, [r0, r2]
	ldr	r1, =0x01111000
	ldr	r2, =CLK_SRC_TOP1_OFFSET
	str	r1, [r0, r2]

	/* wait ?us */
	mov	r1, #0x10000
7:	subs	r1, r1, #1
	bne	7b

	pop	{pc}

