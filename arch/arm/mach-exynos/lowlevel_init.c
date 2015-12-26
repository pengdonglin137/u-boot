/*
 * Lowlevel setup for EXYNOS5 based board
 *
 * Copyright (C) 2013 Samsung Electronics
 * Rajeshwari Shinde <rajeshwari.s@samsung.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <config.h>
#include <debug_uart.h>
#include <asm/arch/cpu.h>
#include <asm/arch/dmc.h>
#include <asm/arch/power.h>
#include <asm/arch/tzpc.h>
#include <asm/arch/periph.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/system.h>
#include <asm/armv7.h>
#include "common_setup.h"
#include "exynos5_setup.h"

/* These are the things we can do during low-level init */
enum {
	DO_WAKEUP	= 1 << 0,
	DO_CLOCKS	= 1 << 1,
	DO_MEM_RESET	= 1 << 2,
	DO_UART		= 1 << 3,
	DO_POWER	= 1 << 4,
};

#ifdef CONFIG_EXYNOS5420
/*
 * Power up secondary CPUs.
 */
static void secondary_cpu_start(void)
{
	v7_enable_smp(EXYNOS5420_INFORM_BASE);
	svc32_mode_en();
	branch_bx(CONFIG_EXYNOS_RELOCATE_CODE_BASE);
}

/*
 * This is the entry point of hotplug-in and
 * cluster switching.
 */
static void low_power_start(void)
{
	uint32_t val, reg_val;

	reg_val = readl(EXYNOS5420_SPARE_BASE);
	if (reg_val != CPU_RST_FLAG_VAL) {
		writel(0x0, CONFIG_LOWPOWER_FLAG);
		branch_bx(0x0);
	}

	reg_val = readl(CONFIG_PHY_IRAM_BASE + 0x4);
	if (reg_val != (uint32_t)&low_power_start) {
		/* Store jump address as low_power_start if not present */
		writel((uint32_t)&low_power_start, CONFIG_PHY_IRAM_BASE + 0x4);
		dsb();
		sev();
	}

	/* Set the CPU to SVC32 mode */
	svc32_mode_en();

#ifndef CONFIG_SYS_L2CACHE_OFF
	/* Read MIDR for Primary Part Number */
	mrc_midr(val);
	val = (val >> 4);
	val &= 0xf;

	if (val == 0xf) {
		configure_l2_ctlr();
		configure_l2_actlr();
		v7_enable_l2_hazard_detect();
	}
#endif

	/* Invalidate L1 & TLB */
	val = 0x0;
	mcr_tlb(val);
	mcr_icache(val);

	/* Disable MMU stuff and caches */
	mrc_sctlr(val);

	val &= ~((0x2 << 12) | 0x7);
	val |= ((0x1 << 12) | (0x8 << 8) | 0x2);
	mcr_sctlr(val);

	/* CPU state is hotplug or reset */
	secondary_cpu_start();

	/* Core should not enter into WFI here */
	wfi();
}

/*
 * Pointer to this function is stored in iRam which is used
 * for jump and power down of a specific core.
 */
static void power_down_core(void)
{
	uint32_t tmp, core_id, core_config;

	/* Get the unique core id */
	/*
	 * Multiprocessor Affinity Register
	 * [11:8]	Cluster ID
	 * [1:0]	CPU ID
	 */
	mrc_mpafr(core_id);
	tmp = core_id & 0x3;
	core_id = (core_id >> 6) & ~3;
	core_id |= tmp;
	core_id &= 0x3f;

	/* Set the status of the core to low */
	core_config = (core_id * CPU_CONFIG_STATUS_OFFSET);
	core_config += EXYNOS5420_CPU_CONFIG_BASE;
	writel(0x0, core_config);

	/* Core enter WFI */
	wfi();
}

/*
 * Configurations for secondary cores are inapt at this stage.
 * Reconfigure secondary cores. Shutdown and change the status
 * of all cores except the primary core.
 */
static void secondary_cores_configure(void)
{
	/* Clear secondary boot iRAM base */
	writel(0x0, (CONFIG_EXYNOS_RELOCATE_CODE_BASE + 0x1C));

	/* set lowpower flag and address */
	writel(CPU_RST_FLAG_VAL, CONFIG_LOWPOWER_FLAG);
	writel((uint32_t)&low_power_start, CONFIG_LOWPOWER_ADDR);
	writel(CPU_RST_FLAG_VAL, EXYNOS5420_SPARE_BASE);
	/* Store jump address for power down */
	writel((uint32_t)&power_down_core, CONFIG_PHY_IRAM_BASE + 0x4);

	/* Need all core power down check */
	dsb();
	sev();
}

extern void relocate_wait_code(void);
#endif

#if 0
void mem_test(void)
{
#define MEMORY_SIZE (10*1024*1024)
	volatile unsigned char *p = (volatile unsigned char *)(0x40000000);
	int i;
	for (i=0; i<MEMORY_SIZE; i++) {
		*p++ = 0x55;
	}

	p = (volatile unsigned char *)(0x40000000);
	for (i=0; i<MEMORY_SIZE; i++) {
		if (*p++ != 0x55)
		{
			printascii("mem test error: addr:");
			printhex8((uint)p);
			printascii("  value: ");
			printhex2((uint)(*p));
			printascii("\n\r");
			break;
		}
	}

	if (i >= MEMORY_SIZE)
		printascii("mem test ok.\n\r");
	else
		printascii("mem test error.\n\r");

}
#endif

int do_lowlevel_init(void)
{
	uint32_t reset_status;
	int actions = 0;

	arch_cpu_init();

#if !defined(CONFIG_SYS_L2CACHE_OFF) && defined(CONFIG_EXYNOS5420)
	/*
	 * Init L2 cache parameters here for use by boot and resume
	 *
	 * These are here instead of in v7_outer_cache_enable() so that the
	 * L2 cache settings get properly set even at resume time or if we're
	 * running U-Boot with the cache off.  The kernel still needs us to
	 * set these for it.
	 */
	configure_l2_ctlr();
	configure_l2_actlr();
	dsb();
	isb();

	relocate_wait_code();

	/* Reconfigure secondary cores */
	secondary_cores_configure();
#endif

	reset_status = get_reset_status();

	switch (reset_status) {
	case S5P_CHECK_SLEEP:
		actions = DO_CLOCKS | DO_WAKEUP;
		break;
	case S5P_CHECK_DIDLE:
	case S5P_CHECK_LPA:
		actions = DO_WAKEUP;
		break;
	default:
		/* This is a normal boot (not a wake from sleep) */
		actions = DO_CLOCKS | DO_MEM_RESET | DO_POWER;
	}

	if (actions & DO_POWER)
		set_ps_hold_ctrl();

	if (actions & DO_CLOCKS) {
#ifdef UBOOT_DEBUG_20151226
		{
			volatile unsigned int *p = (volatile unsigned int *)0x02050000;
			int i;

			for(i=0; i<0x2000; i++) {
				*p++ = 0x00000000;
			}

			 ((void(*)(u32, u32, u32*))(*((u32 *)0x02020030)))((u32)81, (u32)1, (u32 *)0x02050000);
		}
#endif
		system_clock_init();
#ifdef CONFIG_DEBUG_UART
		exynos_pinmux_config(PERIPH_ID_UART0, PINMUX_FLAG_NONE);
		debug_uart_init();
		printascii("UART OK.\n\r");
#endif
		mem_ctrl_init(actions & DO_MEM_RESET);
#if 0
		mem_test();
#endif
		tzpc_init();
#ifdef UBOOT_DEBUG_20151226
		{
			 ((void(*)(u32, u32, u32*))(*((u32 *)0x02020030)))((u32)81, (u32)1, (u32 *)0x02051000);

			 *(volatile unsigned int *)0x40000000 = 0xAAAA5555;
			 ((void(*)(u32, u32, u32*))(*((u32 *)0x02020030)))((u32)81, (u32)1, (u32 *)0x40000000);
		}

		{
			int i;
			volatile unsigned int *p = (volatile unsigned int *)0x02050000;
			printascii("0x02050000:\n\r");
			for(i=0; i<0x10; i++) {
				if (i%4 == 0)
					printascii("\n\r");
				printhex8((uint)(*p++));
				printch(' ');
			}

			p = (volatile unsigned int *)0x02051000;
			printascii("\n\r0x02051000:\n\r");
			for(i=0; i<0x10; i++) {
				if (i%4 == 0)
					printascii("\n\r");
				printhex8((uint)(*p++));
				printch(' ');
			}

			p = (volatile unsigned int *)0x40000000;
			printascii("\n\r0x40000000:\n\r");
			for(i=0; i<0x10; i++) {
				if (i%4 == 0)
					printascii("\n\r");
				printhex8((uint)(*p++));
				printch(' ');
			}

			printascii("\n\r");
		}
#endif
	}

	return actions & DO_WAKEUP;
}
