/*
 * Copyright (C) 2014 Samsung Electronics
 * Przemyslaw Marczak <p.marczak@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/power.h>
#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>
#include <asm/gpio.h>
#include <asm/arch/cpu.h>
#include <dm.h>
#include <power/pmic.h>
#include <power/regulator.h>
#include <power/max77686_pmic.h>
#include <errno.h>
#include <mmc.h>
#include <usb.h>
#include <usb/s3c_udc.h>
#include <samsung/misc.h>
#include "setup.h"

DECLARE_GLOBAL_DATA_PTR;

int exynos_early_init_f(void)
{
	return 0;
}

int exynos_init(void)
{
	return 0;
}

int exynos_power_init(void)
{
	return 0;
}

int board_usb_init(int index, enum usb_init_type init)
{
	return 0;
}
