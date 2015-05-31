/* NAND FLASH 控制器 */
#define NFCONF (*((volatile unsigned long *)0x4E000000))
#define NFCONT (*((volatile unsigned long *)0x4E000004))
#define NFCMMD (*((volatile unsigned char *)0x4E000008))
#define NFADDR (*((volatile unsigned char *)0x4E00000C))
#define NFDATA (*((volatile unsigned char *)0x4E000010))
#define NFSTAT (*((volatile unsigned char *)0x4E000020))
static void nand_read_ll(unsigned int addr, unsigned char *buf, unsigned int len);
static int isBootFromNorFlash(void)
{
	volatile int *p = (volatile int *)0;
	int val;
	val = *p;
	*p = 0x12345678;
	if(*p == 0x12345678)
	{
		/* 写成功, 是 nand 启动 */
		*p = val;
		return 0;
	}
	else
	{
		/* NOR 不能像内存一样写 */
		return 1;
	}
}
void nand_init_ll(void)
{
#define TACLS 0
#define TWRPH0 1
#define TWRPH1 0
	/* 设置时序 */
	NFCONF = (TACLS<<12)|(TWRPH0<<8)|(TWRPH1<<4);
	/* 使能 NAND Flash 控制器, 初始化 ECC, 禁止片选 */
	NFCONT = (1<<4)|(1<<1)|(1<<0);
}

int copy_code_to_sdram(unsigned char *src, unsigned char *dest, unsigned int len)
{
	int i = 0;
	/* 如果是 NOR 启动 */
	if(isBootFromNorFlash())
	{
		while (i < len)
		{
			dest[i] = src[i];
			i++;
		}
	}
	else
	{
		nand_init_ll();
		nand_read_ll((unsigned int)src, dest, len);
	}

	return 0;
}
void clear_bss(void)
{
	extern int __bss_start, __bss_end;
	int *p = &__bss_start;
	for (; p < &__bss_end; p++)
		*p = 0;
}
static void nand_select(void)
{
	NFCONT &= ~(1<<1);
}
static void nand_deselect(void)
{
	NFCONT |= (1<<1);
}
static void nand_cmd(unsigned char cmd)
{
	volatile int i;
	NFCMMD = cmd;
	for (i = 0; i < 10; i++);
}
static void nand_addr(unsigned int addr)
{
	unsigned int col = addr % 2048;
	unsigned int page = addr / 2048;
	volatile int i;
	NFADDR = col & 0xff;
	for (i = 0; i < 10; i++);
	NFADDR = (col >> 8) & 0xff;
	for (i = 0; i < 10; i++);
	NFADDR = page & 0xff;
	for (i = 0; i < 10; i++);
	NFADDR = (page >> 8) & 0xff;
	for (i = 0; i < 10; i++);
	NFADDR = (page >> 16) & 0xff;
	for (i = 0; i < 10; i++);
}
static void nand_wait_ready(void)
{
	while (!(NFSTAT & 1));
}
static unsigned char nand_data(void)
{
	return NFDATA;
}
static void nand_read_ll(unsigned int addr, unsigned char *buf, unsigned int len)
{
	int col = addr % 2048;
	int i = 0;
	/* 1. 选中 */
	nand_select();
	while (i < len)
	{
		/* 2. 发出读命令 00h */
		nand_cmd(0x00);
		/* 3. 发出地址(分 5 步发出) */
		nand_addr(addr);
		/* 4. 发出读命令 30h */
		nand_cmd(0x30);
		/* 5. 判断状态 */
		nand_wait_ready();
		/* 6. 读数据 */
		for (; (col < 2048) && (i < len); col++)
		{
			buf[i] = nand_data();
			i++;
			addr++;
		}
		col = 0;
	}
	/* 7. 取消选中 */
	nand_deselect();
}
