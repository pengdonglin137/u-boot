#include <common.h>
#include <lcd.h>

#include <asm/system.h>
#include <asm/gpio.h>
#include <asm/arch/s3c24x0_cpu.h>

DECLARE_GLOBAL_DATA_PTR;

#define U32 unsigned int
#define M5D(n)  ( ( n ) & 0x1fffff ) // To get lower 21bits
#define SCR_XSIZE_TFT LCD_XSIZE_TFT

#define HOZVAL_TFT		( LCD_XSIZE_TFT - 1 )
#define LINEVAL_TFT		( LCD_YSIZE_TFT - 1 )

vidinfo_t panel_info = {
	.vl_col = LCD_XSIZE_TFT,
	.vl_row = LCD_YSIZE_TFT,
	.vl_bpix = LCD_BPP,
};

//volatile unsigned short (*embedsky_LCD_BUFFER)[SCR_XSIZE_TFT] = (volatile unsigned short (*)[SCR_XSIZE_TFT])(gd->fb_base);
#define embedsky_LCD_BUFFER ((volatile unsigned short (*)[SCR_XSIZE_TFT])(gd->fb_base))
volatile char vbpd = 1, vfpd = 1, vspw = 1, hbpd = 1, hfpd = 1, hspw = 1, clkval_tft = 1 ;

void tq2440_lcd_ClearScr(U32 c)
{
	unsigned int x,y ;

	for( y = 0 ; y < LCD_YSIZE_TFT ; y++ )
	{
		for( x = 0 ; x < (SCR_XSIZE_TFT) ; x++ )
		{
			embedsky_LCD_BUFFER[y][x] = c;
		}
	}
	
}

void tq2440_lcd_PowerEnable(int invpwren , int pwren)
{
	struct s3c24x0_gpio * const gpio = s3c24x0_get_base_gpio() ;
	struct s3c24x0_lcd * const lcd = s3c24x0_get_base_lcd() ;

										//GPG4 is setted as LCD_PWREN
	gpio -> gpgup = gpio -> gpgup & (( ~( 1 << 4) ) | ( 1 << 4) );		// Pull-up disable
	gpio -> gpgcon = gpio -> gpgcon & (( ~( 3 << 8) ) | ( 3 << 8) );		//GPG4=LCD_PWREN
	gpio -> gpgdat = gpio -> gpgdat | (1 << 4 ) ;
										//invpwren=pwren;
										//Enable LCD POWER ENABLE Function
	lcd -> lcdcon5 = lcd -> lcdcon5 & (( ~( 1 << 3 ) ) | ( pwren << 3 ) );	// PWREN
	lcd -> lcdcon5 = lcd -> lcdcon5 & (( ~( 1 << 5 ) ) | ( invpwren << 5 ) );	// INVPWREN
}

void lcd_ctrl_init(void *lcdbase)
{
	struct s3c24x0_gpio * const gpio = s3c24x0_get_base_gpio() ;
	struct s3c24x0_lcd * const lcd = s3c24x0_get_base_lcd() ;
	char *s_lcd;

	lcd -> lcdsaddr1 = ( ( ( U32 ) embedsky_LCD_BUFFER >> 22 ) << 21 ) | M5D ( ( U32 ) embedsky_LCD_BUFFER >> 1 ) ;
	lcd -> lcdsaddr2 = M5D( ( ( U32) embedsky_LCD_BUFFER + ( SCR_XSIZE_TFT * LCD_YSIZE_TFT * 2 ) ) >> 1 ) ;
	lcd -> lcdsaddr3 = ( ( ( SCR_XSIZE_TFT - LCD_XSIZE_TFT ) / 1 ) << 11 ) | ( LCD_XSIZE_TFT /1 ) ;

	s_lcd = getenv ("dwVBPD");
	vbpd = s_lcd ? (int)simple_strtol(s_lcd, NULL, 10) : CONFIG_TQ2440_LCD_VBPD;

	s_lcd = getenv ("dwVFPD");
	vfpd = s_lcd ? (int)simple_strtol(s_lcd, NULL, 10) : CONFIG_TQ2440_LCD_VFPD;

	s_lcd = getenv ("dwVSPW");
	vspw = s_lcd ? (int)simple_strtol(s_lcd, NULL, 10) : CONFIG_TQ2440_LCD_VSPW;

	s_lcd = getenv ("dwHBPD");
	hbpd = s_lcd ? (int)simple_strtol(s_lcd, NULL, 10) : CONFIG_TQ2440_LCD_HBPD;

	s_lcd = getenv ("dwHFPD");
	hfpd = s_lcd ? (int)simple_strtol(s_lcd, NULL, 10) : CONFIG_TQ2440_LCD_HFPD;

	s_lcd = getenv ("dwHSPW");
	hspw = s_lcd ? (int)simple_strtol(s_lcd, NULL, 10) : CONFIG_TQ2440_LCD_HSPW;

	s_lcd = getenv ("dwCLKVAL");
	clkval_tft = s_lcd ? (int)simple_strtol(s_lcd, NULL, 10) : CONFIG_TQ2440_LCD_CLKVAL;

	tq2440_lcd_ClearScr( 0x0 ) ;

	gpio -> gpcup  = 0xffffffff ;
	gpio -> gpccon = 0xaaaaaaaa ;						//Initialize VD[0:7]	 
	 
	gpio -> gpdup  = 0xffffffff ;
	gpio -> gpdcon = 0xaaaaaaaa ;						//Initialize VD[15:8]

	lcd -> lcdcon1 = ( clkval_tft << 8 ) | ( MVAL_USED << 7 ) | (3 << 5 ) | ( 12 << 1 ) | 0 ;
										// TFT LCD panel,16bpp TFT,ENVID=off
	lcd -> lcdcon2 = ( vbpd << 24 ) | ( LINEVAL_TFT << 14 ) | ( vfpd << 6 ) | ( vspw ) ;
	lcd -> lcdcon3 = ( hbpd << 19 ) | ( HOZVAL_TFT << 8 ) | ( hfpd ) ;
	lcd -> lcdcon4 = ( MVAL << 8 ) | ( hspw ) ;
	lcd -> lcdcon5 = ( 1 << 11) | ( 0 << 10 ) | ( 1 << 9 ) | ( 1 << 8 ) | ( 0 << 7 ) | ( 0 << 6 ) | ( 1 << 3 ) | ( BSWP << 1 ) | ( HWSWP ) ;

	lcd -> lcdintmsk |= (3) ;						// MASK LCD Sub Interrupt
	lcd -> lpcsel &= ( ~7 ) ;						// Disable LPC3480
	lcd -> tpal = 0x0 ;							// Disable Temp Palette

	tq2440_lcd_PowerEnable( 0, 1 ) ;
}

void lcd_enable(void)
{
	struct s3c24x0_lcd * const lcd = s3c24x0_get_base_lcd() ;

	lcd -> lcdcon1 |= 1 ;						// ENVID=ON
}

