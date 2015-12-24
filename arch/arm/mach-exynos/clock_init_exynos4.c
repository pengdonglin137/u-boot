/*
 * Clock Initialization for board based on EXYNOS4210
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
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/clk.h>
#include <asm/arch/clock.h>
#include "common_setup.h"
#ifdef CONFIG_TINY4412
#include "tiny4412_setup.h"
#else
#include "exynos4_setup.h"
#endif

/*
 * system_clock_init: Initialize core clock and bus clock.
 * void system_clock_init(void)
 */
void system_clock_init(void)
{
#ifdef CONFIG_TINY4412
	unsigned int set, clr, clr_src_cpu, clr_pll_con0, clr_src_dmc;
	struct exynos4x12_clock *clk = (struct exynos4x12_clock *)
						samsung_get_base_clock();

	/*
	 * CMU_CPU clocks src to MPLL
	 * Bit values:                 0  ; 1
	 * MUX_APLL_SEL:        FIN_PLL   ; FOUT_APLL
	 * MUX_CORE_SEL:        MOUT_APLL ; SCLK_MPLL
	 * MUX_HPM_SEL:         MOUT_APLL ; SCLK_MPLL_USER_C
	 * MUX_MPLL_USER_SEL_C: FIN_PLL   ; SCLK_MPLL
	*/
	clr_src_cpu = MUX_APLL_SEL(1) | MUX_CORE_SEL(1) |
		      MUX_HPM_SEL(1) | MUX_MPLL_USER_SEL_C(1);
	set = MUX_APLL_SEL(0) | MUX_CORE_SEL(1) | MUX_HPM_SEL(1) |
	      MUX_MPLL_USER_SEL_C(1);

	clrsetbits_le32(&clk->src_cpu, clr_src_cpu, set);

	/* Wait for mux change */
	while (readl(&clk->mux_stat_cpu) & MUX_STAT_CPU_CHANGING)
		continue;

	/* Set APLL to 1400MHz */
	clr_pll_con0 = SDIV(7) | PDIV(63) | MDIV(1023) | FSEL(1);
	set = SDIV(0x0) | PDIV(0x3) | MDIV(0xAF) | FSEL(1);

	clrsetbits_le32(&clk->apll_con0, clr_pll_con0, set);

	/* Wait for PLL to be locked */
	while (!(readl(&clk->apll_con0) & PLL_LOCKED_BIT))
		continue;

	/* Set CMU_CPU clocks src to APLL */
	set = MUX_APLL_SEL(1) | MUX_CORE_SEL(0) | MUX_HPM_SEL(0) |
	      MUX_MPLL_USER_SEL_C(1);
	clrsetbits_le32(&clk->src_cpu, clr_src_cpu, set);

	/* Wait for mux change */
	while (readl(&clk->mux_stat_cpu) & MUX_STAT_CPU_CHANGING)
		continue;

	set = CORE_RATIO(0) | COREM0_RATIO(2) | COREM1_RATIO(5) |
	      PERIPH_RATIO(0) | ATB_RATIO(4) | PCLK_DBG_RATIO(1) |
	      APLL_RATIO(0) | CORE2_RATIO(0);
	/*
	 * Set dividers for MOUTcore = 1400 MHz
	 * coreout =      MOUT / (ratio + 1) = 1400 MHz (0)
	 * corem0 =     armclk / (ratio + 1) = 466 MHz (2)
	 * corem1 =     armclk / (ratio + 1) = 233 MHz (5)
	 * periph =     armclk / (ratio + 1) = 1400 MHz (0)
	 * atbout =       MOUT / (ratio + 1) = 280 MHz (4)
	 * pclkdbgout = atbout / (ratio + 1) = 140 MHz (1)
	 * sclkapll = MOUTapll / (ratio + 1) = 1400 MHz (0)
	 * core2out = core_out / (ratio + 1) = 1400 MHz (0) (armclk)
	*/
	clr = CORE_RATIO(7) | COREM0_RATIO(7) | COREM1_RATIO(7) |
	      PERIPH_RATIO(7) | ATB_RATIO(7) | PCLK_DBG_RATIO(7) |
	      APLL_RATIO(7) | CORE2_RATIO(7);

	clrsetbits_le32(&clk->div_cpu0, clr, set);

	/* Wait for divider ready status */
	while (readl(&clk->div_stat_cpu0) & DIV_STAT_CPU0_CHANGING)
		continue;

	/*
	 * For MOUThpm = 1400 MHz (MOUTapll)
	 * doutcopy = MOUThpm / (ratio + 1) = 280 (4)
	 * sclkhpm = doutcopy / (ratio + 1) = 280 (4)
	 * cores_out = armclk / (ratio + 1) = 280 (4)
	 */
	clr = COPY_RATIO(7) | HPM_RATIO(7) | CORES_RATIO(7);
	set = COPY_RATIO(4) | HPM_RATIO(4) | CORES_RATIO(4);

	clrsetbits_le32(&clk->div_cpu1, clr, set);

	/* Wait for divider ready status */
	while (readl(&clk->div_stat_cpu1) & DIV_STAT_CPU1_CHANGING)
		continue;

	/*
	 * Set CMU_DMC clocks src to APLL
	 * Bit values:             0  ; 1
	 * MUX_C2C_SEL:      SCLKMPLL ; SCLKAPLL
	 * MUX_DMC_BUS_SEL:  SCLKMPLL ; SCLKAPLL
	 * MUX_DPHY_SEL:     SCLKMPLL ; SCLKAPLL
	 * MUX_MPLL_SEL:     FINPLL   ; MOUT_MPLL_FOUT
	 * MUX_PWI_SEL:      0110 (MPLL); 0111 (EPLL); 1000 (VPLL); 0(XXTI)
	 * MUX_G2D_ACP0_SEL: SCLKMPLL ; SCLKAPLL
	 * MUX_G2D_ACP1_SEL: SCLKEPLL ; SCLKVPLL
	 * MUX_G2D_ACP_SEL:  OUT_ACP0 ; OUT_ACP1
	*/
	clr_src_dmc = MUX_C2C_SEL(1) | MUX_DMC_BUS_SEL(1) |
		      MUX_DPHY_SEL(1) | MUX_MPLL_SEL(1) |
		      MUX_PWI_SEL(15) | MUX_G2D_ACP0_SEL(1) |
		      MUX_G2D_ACP1_SEL(1) | MUX_G2D_ACP_SEL(1);
	set = MUX_C2C_SEL(1) | MUX_DMC_BUS_SEL(1) | MUX_DPHY_SEL(1) |
	      MUX_MPLL_SEL(0) | MUX_PWI_SEL(0) | MUX_G2D_ACP0_SEL(1) |
	      MUX_G2D_ACP1_SEL(1) | MUX_G2D_ACP_SEL(1);

	clrsetbits_le32(&clk->src_dmc, clr_src_dmc, set);

	/* Wait for mux change */
	while (readl(&clk->mux_stat_dmc) & MUX_STAT_DMC_CHANGING)
		continue;

	/* Set MPLL to 800MHz */
	set = SDIV(0) | PDIV(3) | MDIV(100) | FSEL(0) | PLL_ENABLE(1);

	clrsetbits_le32(&clk->mpll_con0, clr_pll_con0, set);

	/* Wait for PLL to be locked */
	while (!(readl(&clk->mpll_con0) & PLL_LOCKED_BIT))
		continue;

	/* Switch back CMU_DMC mux */
	set = MUX_C2C_SEL(0) | MUX_DMC_BUS_SEL(0) | MUX_DPHY_SEL(0) |
	      MUX_MPLL_SEL(1) | MUX_PWI_SEL(6) | MUX_G2D_ACP0_SEL(0) |
	      MUX_G2D_ACP1_SEL(0) | MUX_G2D_ACP_SEL(0);

	clrsetbits_le32(&clk->src_dmc, clr_src_dmc, set);

	/* Wait for mux change */
	while (readl(&clk->mux_stat_dmc) & MUX_STAT_DMC_CHANGING)
		continue;

	/* CLK_DIV_DMC0 */
	clr = ACP_RATIO(7) | ACP_PCLK_RATIO(7) | DPHY_RATIO(7) |
	      DMC_RATIO(7) | DMCD_RATIO(7) | DMCP_RATIO(7);
	/*
	 * For:
	 * MOUTdmc = 800 MHz
	 * MOUTdphy = 800 MHz
	 *
	 * aclk_acp = MOUTdmc / (ratio + 1) = 200 (3)
	 * pclk_acp = aclk_acp / (ratio + 1) = 100 (1)
	 * sclk_dphy = MOUTdphy / (ratio + 1) = 400 (1)
	 * sclk_dmc = MOUTdmc / (ratio + 1) = 400 (1)
	 * aclk_dmcd = sclk_dmc / (ratio + 1) = 200 (1)
	 * aclk_dmcp = aclk_dmcd / (ratio + 1) = 100 (1)
	 */
	set = ACP_RATIO(3) | ACP_PCLK_RATIO(1) | DPHY_RATIO(1) |
	      DMC_RATIO(1) | DMCD_RATIO(1) | DMCP_RATIO(1);

	clrsetbits_le32(&clk->div_dmc0, clr, set);

	/* Wait for divider ready status */
	while (readl(&clk->div_stat_dmc0) & DIV_STAT_DMC0_CHANGING)
		continue;

	/* CLK_DIV_DMC1 */
	clr = G2D_ACP_RATIO(15) | C2C_RATIO(7) | PWI_RATIO(15) |
	      C2C_ACLK_RATIO(7) | DVSEM_RATIO(127) | DPM_RATIO(127);
	/*
	 * For:
	 * MOUTg2d = 800 MHz
	 * MOUTc2c = 800 Mhz
	 * MOUTpwi = 800 MHz
	 *
	 * sclk_g2d_acp = MOUTg2d / (ratio + 1) = 200 (3)
	 * sclk_c2c = MOUTc2c / (ratio + 1) = 400 (1)
	 * aclk_c2c = sclk_c2c / (ratio + 1) = 200 (1)
	 * sclk_pwi = MOUTpwi / (ratio + 1) = 100 (7)
	 */
	set = G2D_ACP_RATIO(3) | C2C_RATIO(1) | PWI_RATIO(7) |
	      C2C_ACLK_RATIO(1) | DVSEM_RATIO(1) | DPM_RATIO(1);

	clrsetbits_le32(&clk->div_dmc1, clr, set);

	/* Wait for divider ready status */
	while (readl(&clk->div_stat_dmc1) & DIV_STAT_DMC1_CHANGING)
		continue;

	/* CLK_SRC_PERIL0 */
	clr = UART0_SEL(15) | UART1_SEL(15) | UART2_SEL(15) |
	      UART3_SEL(15) | UART4_SEL(15);
	/*
	 * Set CLK_SRC_PERIL0 clocks src to MPLL
	 * src values: 0(XXTI); 1(XusbXTI); 2(SCLK_HDMI24M); 3(SCLK_USBPHY0);
	 *             5(SCLK_HDMIPHY); 6(SCLK_MPLL_USER_T); 7(SCLK_EPLL);
	 *             8(SCLK_VPLL)
	 *
	 * Set all to SCLK_MPLL_USER_T
	 */
	set = UART0_SEL(6) | UART1_SEL(6) | UART2_SEL(6) | UART3_SEL(6) |
	      UART4_SEL(6);

	clrsetbits_le32(&clk->src_peril0, clr, set);

	/* CLK_DIV_PERIL0 */
	clr = UART0_RATIO(15) | UART1_RATIO(15) | UART2_RATIO(15) |
	      UART3_RATIO(15) | UART4_RATIO(15);
	/*
	 * For MOUTuart0-4: 800MHz
	 *
	 * SCLK_UARTx = MOUTuartX / (ratio + 1) = 100 (7)
	*/
	set = UART0_RATIO(7) | UART1_RATIO(7) | UART2_RATIO(7) |
	      UART3_RATIO(7) | UART4_RATIO(7);

	clrsetbits_le32(&clk->div_peril0, clr, set);

	while (readl(&clk->div_stat_peril0) & DIV_STAT_PERIL0_CHANGING)
		continue;

	/* CLK_DIV_FSYS1 */
	clr = MMC0_RATIO(15) | MMC0_PRE_RATIO(255) | MMC1_RATIO(15) |
	      MMC1_PRE_RATIO(255);
	/*
	 * For MOUTmmc0-3 = 800 MHz (MPLL)
	 *
	 * DOUTmmc1 = MOUTmmc1 / (ratio + 1) = 100 (7)
	 * sclk_mmc1 = DOUTmmc1 / (ratio + 1) = 50 (1)
	 * DOUTmmc0 = MOUTmmc0 / (ratio + 1) = 100 (7)
	 * sclk_mmc0 = DOUTmmc0 / (ratio + 1) = 50 (1)
	*/
	set = MMC0_RATIO(7) | MMC0_PRE_RATIO(1) | MMC1_RATIO(7) |
	      MMC1_PRE_RATIO(1);

	clrsetbits_le32(&clk->div_fsys1, clr, set);

	/* Wait for divider ready status */
	while (readl(&clk->div_stat_fsys1) & DIV_STAT_FSYS1_CHANGING)
		continue;

	/* CLK_DIV_FSYS2 */
	clr = MMC2_RATIO(15) | MMC2_PRE_RATIO(255) | MMC3_RATIO(15) |
	      MMC3_PRE_RATIO(255);
	/*
	 * For MOUTmmc0-3 = 800 MHz (MPLL)
	 *
	 * DOUTmmc3 = MOUTmmc3 / (ratio + 1) = 100 (7)
	 * sclk_mmc3 = DOUTmmc3 / (ratio + 1) = 50 (1)
	 * DOUTmmc2 = MOUTmmc2 / (ratio + 1) = 100 (7)
	 * sclk_mmc2 = DOUTmmc2 / (ratio + 1) = 50 (1)
	*/
	set = MMC2_RATIO(7) | MMC2_PRE_RATIO(1) | MMC3_RATIO(7) |
	      MMC3_PRE_RATIO(1);

	clrsetbits_le32(&clk->div_fsys2, clr, set);

	/* Wait for divider ready status */
	while (readl(&clk->div_stat_fsys2) & DIV_STAT_FSYS2_CHANGING)
		continue;

	/* CLK_DIV_FSYS3 */
	clr = MMC4_RATIO(15) | MMC4_PRE_RATIO(255);
	/*
	 * For MOUTmmc4 = 800 MHz (MPLL)
	 *
	 * DOUTmmc4 = MOUTmmc4 / (ratio + 1) = 100 (7)
	 * sclk_mmc4 = DOUTmmc4 / (ratio + 1) = 100 (0)
	*/
	set = MMC4_RATIO(7) | MMC4_PRE_RATIO(0);

	clrsetbits_le32(&clk->div_fsys3, clr, set);

	/* Wait for divider ready status */
	while (readl(&clk->div_stat_fsys3) & DIV_STAT_FSYS3_CHANGING)
		continue;
#else
	struct exynos4_clock *clk =
			(struct exynos4_clock *)samsung_get_base_clock();

	writel(CLK_SRC_CPU_VAL, &clk->src_cpu);

	sdelay(0x10000);

	writel(CLK_SRC_TOP0_VAL, &clk->src_top0);
	writel(CLK_SRC_TOP1_VAL, &clk->src_top1);
	writel(CLK_SRC_DMC_VAL, &clk->src_dmc);
	writel(CLK_SRC_LEFTBUS_VAL, &clk->src_leftbus);
	writel(CLK_SRC_RIGHTBUS_VAL, &clk->src_rightbus);
	writel(CLK_SRC_FSYS_VAL, &clk->src_fsys);
	writel(CLK_SRC_PERIL0_VAL, &clk->src_peril0);
	writel(CLK_SRC_CAM_VAL, &clk->src_cam);
	writel(CLK_SRC_MFC_VAL, &clk->src_mfc);
	writel(CLK_SRC_G3D_VAL, &clk->src_g3d);
	writel(CLK_SRC_LCD0_VAL, &clk->src_lcd0);

	sdelay(0x10000);

	writel(CLK_DIV_CPU0_VAL, &clk->div_cpu0);
	writel(CLK_DIV_CPU1_VAL, &clk->div_cpu1);
	writel(CLK_DIV_DMC0_VAL, &clk->div_dmc0);
	writel(CLK_DIV_DMC1_VAL, &clk->div_dmc1);
	writel(CLK_DIV_LEFTBUS_VAL, &clk->div_leftbus);
	writel(CLK_DIV_RIGHTBUS_VAL, &clk->div_rightbus);
	writel(CLK_DIV_TOP_VAL, &clk->div_top);
	writel(CLK_DIV_FSYS1_VAL, &clk->div_fsys1);
	writel(CLK_DIV_FSYS2_VAL, &clk->div_fsys2);
	writel(CLK_DIV_FSYS3_VAL, &clk->div_fsys3);
	writel(CLK_DIV_PERIL0_VAL, &clk->div_peril0);
	writel(CLK_DIV_CAM_VAL, &clk->div_cam);
	writel(CLK_DIV_MFC_VAL, &clk->div_mfc);
	writel(CLK_DIV_G3D_VAL, &clk->div_g3d);
	writel(CLK_DIV_LCD0_VAL, &clk->div_lcd0);

	/* Set PLL locktime */
	writel(PLL_LOCKTIME, &clk->apll_lock);
	writel(PLL_LOCKTIME, &clk->mpll_lock);
	writel(PLL_LOCKTIME, &clk->epll_lock);
	writel(PLL_LOCKTIME, &clk->vpll_lock);

	writel(APLL_CON1_VAL, &clk->apll_con1);
	writel(APLL_CON0_VAL, &clk->apll_con0);
	writel(MPLL_CON1_VAL, &clk->mpll_con1);
	writel(MPLL_CON0_VAL, &clk->mpll_con0);
	writel(EPLL_CON1_VAL, &clk->epll_con1);
	writel(EPLL_CON0_VAL, &clk->epll_con0);
	writel(VPLL_CON1_VAL, &clk->vpll_con1);
	writel(VPLL_CON0_VAL, &clk->vpll_con0);

	sdelay(0x30000);
#endif
}
