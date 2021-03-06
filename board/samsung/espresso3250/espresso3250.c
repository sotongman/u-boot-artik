/*
 * Copyright (C) 2013 Samsung Electronics
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
#include <asm/io.h>
#include <netdev.h>
#include <asm/arch/cpu.h>
#include <asm/arch/clk.h>
#include <asm/arch/clock.h>
#include <asm/arch/power.h>
#include <asm/arch/gpio.h>
#include <asm/arch/mmc.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/sromc.h>
#include <asm/arch/sysreg.h>
#include <asm/unaligned.h>
#include <usb.h>
#include <usb/s3c_udc.h>
#include <mmc.h>
#include "pmic.h"

#define DEBOUNCE_DELAY	10000
#define OM_1stSDMMC_2ndUSB	0x2

DECLARE_GLOBAL_DATA_PTR;
unsigned int OmPin;

void pwm_early_init(unsigned int channel)
{
	struct exynos3_gpio_part1 *bank =
		(struct exynos3_gpio_part1 *)samsung_get_base_gpio_part1();

	if (channel > 4)
		return;

	pwm_enable(channel);
	pwm_disable(channel);
	s5p_gpio_cfg_pin(&bank->d0, channel, GPIO_FUNC(0x2));
}

int board_init(void)
{
	u8 read_vol_arm;
	u8 read_vol_int, read_vol_g3d;
	u8 read_vol_mif, read_rtc_ctrl;
	u8 read_buck;
	u8 temp;
	unsigned int reg;

	char bl1_version[9] = {0};

	/* display BL1 version */
#ifdef CONFIG_TRUSTZONE_ENABLE
	printf("\nTrustZone Enabled BSP");
	strncpy(&bl1_version[0], (char *)(CONFIG_PHY_IRAM_NS_BASE + 0x810), 8);
	printf("\nBL1 version: %s\n", &bl1_version[0]);
#endif

#if defined(CONFIG_PM)
#if defined (CONFIG_PMIC_S5M8767A)
#if 0
	IIC0_ERead(S5M8767A_RD_ADDR, BUCK2_DVS, &read_vol_arm);
	IIC0_ERead(S5M8767A_RD_ADDR, BUCK3_DVS, &read_vol_int);
	IIC0_ERead(S5M8767A_RD_ADDR, BUCK4_DVS, &read_vol_g3d);

	printf("\nPMIC: 8767 + FAN3555\n ");
	printf("MIF--FAN3555, fixed to: %dmV\t\n", 1100);

	//IIC0_ERead(S5M8767A_RD_ADDR, BUCK2_CTRL, &temp);
	//printf("ARM---BUCK2, BUCK2_CTRL value: %d\t\n", temp);
	printf("ARM---BUCK2: %dmV\t\n", RD_BUCK_VOLT((unsigned int)read_vol_arm));

	//IIC0_ERead(S5M8767A_RD_ADDR, BUCK3_CTRL, &temp);
	//printf("INT---BUCK3, BUCK3_CTRL value: %d\t\n", temp);
	printf("INT---BUCK3: %dmV\t\n", RD_BUCK_VOLT((unsigned int)read_vol_int));

	//IIC0_ERead(S5M8767A_RD_ADDR, BUCK4_CTRL, &temp);
	//printf("G3D---BUCK4, BUCK4_CTRL value: %d\t\n", temp);
	printf("G3D---BUCK4: %dmV\t\n", RD_BUCK_VOLT((unsigned int)read_vol_g3d));
#endif
#endif
#if defined (CONFIG_PMIC_S2MPU02)

	IIC0_ERead(S2MPU02_RD_ADDR, BUCK1_CTRL2, &read_vol_mif);
	IIC0_ERead(S2MPU02_RD_ADDR, BUCK2_CTRL2, &read_vol_arm);
	IIC0_ERead(S2MPU02_RD_ADDR, BUCK3_CTRL2, &read_vol_int);
	IIC0_ERead(S2MPU02_RD_ADDR, BUCK4_CTRL2, &read_vol_g3d);
	printf("MIF: %dmV\t", RD_BUCK_VOLT((unsigned int)read_vol_mif));
	printf("ARM: %dmV\t", RD_BUCK_VOLT((unsigned int)read_vol_arm));
	printf("INT: %dmV\t", RD_BUCK_VOLT((unsigned int)read_vol_int));
	printf("G3D: %dmV\t\n", RD_BUCK_VOLT((unsigned int)read_vol_g3d));
#endif
#endif

#ifdef CONFIG_EXYNOS_SPI
	spi_init();
#endif

	//gd->bd->bi_arch_number = MACH_TYPE_SMDK3250;

	gd->bd->bi_boot_params = (PHYS_SDRAM_1+0x100);

	OmPin = __REG(EXYNOS4_POWER_BASE + INFORM3_OFFSET);
	#define SDMMC_DEV		0x4
	unsigned int om_status = readl(EXYNOS4_POWER_BASE + OM_STATUS_OFFSET);
	if(om_status == SDMMC_DEV) {
		OmPin = BOOT_EMMC_4_4;
	}

	printf("\nChecking Boot Mode ...");
	if (OmPin == BOOT_ONENAND) {
		printf(" OneNand\n");
	} else if (OmPin == BOOT_NAND) {
		printf(" NAND\n");
	} else if (OmPin == BOOT_MMCSD) {
		printf(" SDMMC\n");
	} else if (OmPin == BOOT_EMMC) {
		printf(" EMMC4.3\n");
	} else if (OmPin == BOOT_EMMC_4_4) {
		printf(" EMMC4.41\n");
	} else {
		printf(" Please check OM_pin\n");
	}

	/* Enable PWM to maintain the PWM output signal to low */
	pwm_early_init(0);
	pwm_early_init(1);

	return 0;
}

int dram_init(void)
{
	gd->ram_size	= get_ram_size((long *)PHYS_SDRAM_1, PHYS_SDRAM_1_SIZE)
			+ get_ram_size((long *)PHYS_SDRAM_2, PHYS_SDRAM_2_SIZE)
			+ get_ram_size((long *)PHYS_SDRAM_3, PHYS_SDRAM_3_SIZE)
			+ get_ram_size((long *)PHYS_SDRAM_4, PHYS_SDRAM_4_SIZE)
			+ get_ram_size((long *)PHYS_SDRAM_5, PHYS_SDRAM_7_SIZE)
			+ get_ram_size((long *)PHYS_SDRAM_6, PHYS_SDRAM_7_SIZE)
			+ get_ram_size((long *)PHYS_SDRAM_7, PHYS_SDRAM_7_SIZE)
			+ get_ram_size((long *)PHYS_SDRAM_8, PHYS_SDRAM_8_SIZE);

	return 0;
}

void dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;

	gd->bd->bi_dram[0].size = get_ram_size((long *)PHYS_SDRAM_1,
							PHYS_SDRAM_1_SIZE);
	gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
	gd->bd->bi_dram[1].size = get_ram_size((long *)PHYS_SDRAM_2,
							PHYS_SDRAM_2_SIZE);
	gd->bd->bi_dram[2].start = PHYS_SDRAM_3;
	gd->bd->bi_dram[2].size = get_ram_size((long *)PHYS_SDRAM_3,
							PHYS_SDRAM_3_SIZE);
	gd->bd->bi_dram[3].start = PHYS_SDRAM_4;
	gd->bd->bi_dram[3].size = get_ram_size((long *)PHYS_SDRAM_4,
							PHYS_SDRAM_4_SIZE);
	gd->bd->bi_dram[4].start = PHYS_SDRAM_5;
	gd->bd->bi_dram[4].size = get_ram_size((long *)PHYS_SDRAM_5,
							PHYS_SDRAM_5_SIZE);
	gd->bd->bi_dram[5].start = PHYS_SDRAM_6;
	gd->bd->bi_dram[5].size = get_ram_size((long *)PHYS_SDRAM_6,
							PHYS_SDRAM_6_SIZE);
	gd->bd->bi_dram[6].start = PHYS_SDRAM_7;
	gd->bd->bi_dram[6].size = get_ram_size((long *)PHYS_SDRAM_7,
							PHYS_SDRAM_7_SIZE);
	gd->bd->bi_dram[7].start = PHYS_SDRAM_8;
	gd->bd->bi_dram[7].size = get_ram_size((long *)PHYS_SDRAM_8,
							PHYS_SDRAM_8_SIZE);
}

#ifdef CONFIG_DRIVER_AX88796C_SPI
static int artik5_ax88796c_gpio_reset(void)
{
	struct exynos3_gpio_part2 *gpio2 =
		(struct exynos3_gpio_part2 *) samsung_get_base_gpio_part2();
	struct s5p_gpio_bank *bank;

	/* set reset */
	bank = &gpio2->e0;
	s5p_gpio_direction_output(bank, 2, 0);
	mdelay(10);
	s5p_gpio_direction_output(bank, 2, 1);
	mdelay(10);

	return 0;
}

#define CS_HIGH (0) /* 0: low active, 1: high active */

int artik5_ax88796c_cs_activate(void)
{
	struct exynos3_gpio_part2 *gpio2 =
		(struct exynos3_gpio_part2 *) samsung_get_base_gpio_part2();
	struct s5p_gpio_bank *bank;

	bank = &gpio2->x3;
	s5p_gpio_direction_output(bank, 4, CS_HIGH ? 1 : 0);

	return 0;
}

int artik5_ax88796c_cs_deactivate(void)
{
	struct exynos3_gpio_part2 *gpio2 =
		(struct exynos3_gpio_part2 *) samsung_get_base_gpio_part2();
	struct s5p_gpio_bank *bank;

	bank = &gpio2->x3;
	s5p_gpio_direction_output(bank, 4, CS_HIGH ? 0 : 1);

	return 0;
}

int board_eth_init(bd_t *bis)
{
	int rc = 0;
	int num = 0;

	artik5_ax88796c_gpio_reset();

	rc = ax88796c_spi_initialize(AX88796C_SPI_BUS, AX88796C_SPI_CS,
		AX88796C_MAX_HZ, AX88796C_SPI_MODE);
	if (!rc)
		num++;

	return num;
}
#else	/* CONFIG_DRIVER_AX88796C_SPI */
int board_eth_init(bd_t *bis)
{
	return 0;
}
#endif	/* CONFIG_DRIVER_AX88796C_SPI */

#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
#ifdef CONFIG_MACH_ARTIK5
	printf("\nBoard: ARTIK5\n");
#else
	printf("\nBoard: ESPRESSO3250\n");
#endif
	return 0;
}
#endif

#ifdef CONFIG_GENERIC_MMC
int board_mmc_init(bd_t *bis)
{
	struct exynos4_power *pmu = (struct exynos4_power *)EXYNOS4_POWER_BASE;
	int err, OmPin;

	OmPin = readl(&pmu->inform3);

	err = exynos_pinmux_config(PERIPH_ID_SDMMC2, PINMUX_FLAG_NONE);
	if (err) {
		debug("SDMMC2 not configured\n");
		return err;
	}

	err = exynos_pinmux_config(PERIPH_ID_SDMMC0, PINMUX_FLAG_8BIT_MODE);
	if (err) {
		debug("MSHC0 not configured\n");
		return err;
	}

	if (OmPin == BOOT_EMMC_4_4 || OmPin == BOOT_EMMC) {
#ifdef USE_MMC0
		set_mmc_clk(PERIPH_ID_SDMMC0, 0);
		err = s5p_mmc_init(PERIPH_ID_SDMMC0, 8);
#endif
#ifdef USE_MMC2
		set_mmc_clk(PERIPH_ID_SDMMC2, 0);
		err = s5p_mmc_init(PERIPH_ID_SDMMC2, 4);
#endif
	}
	else {
#ifdef USE_MMC2
		set_mmc_clk(PERIPH_ID_SDMMC2, 0);
		err = s5p_mmc_init(PERIPH_ID_SDMMC2, 4);
#endif
#ifdef USE_MMC0
		set_mmc_clk(PERIPH_ID_SDMMC0, 0);
		err = s5p_mmc_init(PERIPH_ID_SDMMC0, 8);
#endif
	}

	return err;
}
#endif

static int board_uart_init(void)
{
	int err;

	err = exynos_pinmux_config(PERIPH_ID_UART0, PINMUX_FLAG_NONE);
	if (err) {
		debug("UART0 not configured\n");
		return err;
	}

	err = exynos_pinmux_config(PERIPH_ID_UART1, PINMUX_FLAG_NONE);
	if (err) {
		debug("UART1 not configured\n");
		return err;
	}

	err = exynos_pinmux_config(PERIPH_ID_UART2, PINMUX_FLAG_NONE);
	if (err) {
		debug("UART2 not configured\n");
		return err;
	}

	err = exynos_pinmux_config(PERIPH_ID_UART3, PINMUX_FLAG_NONE);
	if (err) {
		debug("UART3 not configured\n");
		return err;
	}

	return 0;
}

#ifdef CONFIG_BOARD_EARLY_INIT_F
int board_early_init_f(void)
{
	return board_uart_init();
}
#endif

#define SCAN_KEY_CNT	10
unsigned int check_key(void)
{
	unsigned int scan_cnt = 0;
	unsigned int cur_key_val = 0;
	unsigned int old_key_val = 0;
	unsigned int total_scan_cnt = 0;
	int err;
	struct exynos3_gpio_part2 *gpio2 = (struct exynos3_gpio_part2 *)samsung_get_base_gpio_part2();

	unsigned int pwr, home, back;

	/* GPX1_6 : Back Key */
	s5p_gpio_direction_input(&gpio2->x1, 6);
	s5p_gpio_set_pull(&gpio2->x1, 6, GPIO_PULL_UP);

	/* GPX2_7 : Power Key */
	s5p_gpio_direction_input(&gpio2->x2, 7);
	s5p_gpio_set_pull(&gpio2->x2, 7, GPIO_PULL_UP);

	/* GPX0_3 : Home Key */
	s5p_gpio_direction_input(&gpio2->x0, 3);
	s5p_gpio_set_pull(&gpio2->x0, 3, GPIO_PULL_UP);

	/* Fix me : Add the code to scan key */

	mdelay(10);

	old_key_val = cur_key_val = s5p_gpio_get_value(&gpio2->x2, 7) << 2 |
				s5p_gpio_get_value(&gpio2->x0, 3) << 1 |
				s5p_gpio_get_value(&gpio2->x1, 6);

	while (1) {
		if (total_scan_cnt >= SCAN_KEY_CNT) {
			cur_key_val = 7;
			break;
		}

		mdelay(10);

		cur_key_val = s5p_gpio_get_value(&gpio2->x2, 7) << 2 |
					s5p_gpio_get_value(&gpio2->x0, 3) << 1 |
					s5p_gpio_get_value(&gpio2->x1, 6);

		if (cur_key_val == old_key_val) {
			scan_cnt++;
		} else {
			scan_cnt = 0;
			old_key_val = cur_key_val;
			total_scan_cnt++;
		}

		if (scan_cnt >= SCAN_KEY_CNT) {
			break;
		}
	}

	return cur_key_val;
}

#define RECOVERY_MENU_CNT	3
char func_name[RECOVERY_MENU_CNT][30] = {"Reset", "Wipe", "Update"};

int menu_select(int i)
{
	switch(i) {
	case 0:
		setenv("bootcmd", "reset");
		break;
	case 1:
#ifdef CONFIG_CMD_LCD
		run_command("lcd 0", 0);
	        run_command("lcd 2", 0);
#endif
#ifdef CONFIG_CMD_LCDTEXT
		run_command("lcdtext 0 30 \"Formatting ...\"", 0);
#endif
#ifdef CONFIG_FACTORY_RESET_COMMAND
		run_command(CONFIG_FACTORY_RESET_COMMAND, 0);
#endif
		break;
	case 2:
#ifdef CONFIG_BOOTCOMMAND_VIA_SCRIPT
		setenv("bootcmd", CONFIG_BOOTCOMMAND_VIA_SCRIPT);
#endif
		break;
	}
	return 0;
}

int menu_up(int i)
{
	int ret;

	if (i <= 0)
		ret = RECOVERY_MENU_CNT - 1;
	else
		ret = i - 1;

	return ret;
}

int menu_down(int i)
{
	int ret;

	if (i >= RECOVERY_MENU_CNT - 1)
		ret = 0;
	else
		ret = i + 1;

	return ret;
}

int recovery_mode(void)
{
	int i = 0, init = 1;
	unsigned int cur_key_val = 0, old_key_val = 7, temp_key_val = 0;

#ifdef CONFIG_CMD_LCD
	run_command("lcd 0", 0);
	run_command("lcd 2", 0);
#endif
	while (1) {
		temp_key_val = check_key();
		if (temp_key_val != 0x7 || init) {
			init = 0;
			old_key_val = cur_key_val;
			cur_key_val = temp_key_val;
#ifdef CONFIG_CMD_LCD
			run_command("lcd 2", 0);
#endif
			switch(cur_key_val) {
			case 0x3:		// Power Key Pressed : Select
				menu_select(i);
				return 0;
			case 0x5:		// Home Key Pressed : Up
				i = menu_up(i);
				break;
			case 0x6:		// Back Key Pressed : Down
				i = menu_down(i);
				break;
			}
#ifdef CONFIG_CMD_LCDTEXT
			if (i == 0)
				run_command("lcdtext 0 30 \"v Reset\"", 0);
			else
				run_command("lcdtext 0 30 \"  Reset\"", 0);

			if (i == 1)
				run_command("lcdtext \"v Wipe the partition\"", 0);
			else
				run_command("lcdtext \"  Wipe the partition\"", 0);

			if (i == 2)
				run_command("lcdtext \"v Update from SDcard\"", 0);
			else
				run_command("lcdtext \"  Update from SDcard\"", 0);
#endif
			printf("%s \n", func_name[i]);
		}
	}
	return 0;
}

int board_late_init(void)
{
	struct exynos4_power *pmu = (struct exynos4_power *)EXYNOS4_POWER_BASE;
	uint rst_stat;
	unsigned int pressed_key = ~0;
	unsigned om_status = readl(&pmu->om_stat) & 0x3f;

	rst_stat = readl(&pmu->rst_stat);
	printf("rst_stat : 0x%x\n", rst_stat);

	pmic_enable_wtsr();

#if 0
	pressed_key = check_key();
#endif
	switch(pressed_key) {
	case 0x1:
		if ((om_status >> 1) == OM_1stSDMMC_2ndUSB) {
			printf("RecoveryMode from microSD \n");
#ifdef CONFIG_CMD_LCD
			run_command("lcd 0", 0);
			run_command("lcd 2", 0);
#endif
#ifdef CONFIG_CMD_LCDTEXT
			run_command("lcdtext \"Copying Bootloaders\"", 0);
			run_command("lcdtext \"   from microSD to eMMC...\"", 0);
#endif
#ifdef CONFIG_RECOVERYCOMMAND_1st_SDMMC
			run_command(CONFIG_RECOVERYCOMMAND_1st_SDMMC, 0);
#endif
#ifdef CONFIG_CMD_LCDTEXT
			run_command("lcdtext \"Done...\"", 0);
#endif
			mdelay(1000);
			setenv("bootcmd", " ");
			setenv("bootdelay", "0");
#ifdef CONFIG_CMD_LCDTEXT
			run_command("lcd 1", 0);
#endif
		} else {
			printf("Recovery Mode \n");
			setenv("bootdelay", "0");
			recovery_mode();
		}
		break;
	case 0x2:
		printf("Download Mode \n");
#ifdef CONFIG_CMD_LCD
		run_command("lcd 0", 0);
#endif
#ifdef CONFIG_CMD_LCDTEXT
		run_command("lcdtext \"Downlaod Mode\"", 0);
#endif
		run_command("fastboot", 0);
		break;
	}

#ifdef CONFIG_RECOVERY_MODE
	u32 second_boot_info = readl(CONFIG_SECONDARY_BOOT_INFORM_BASE);

	if (second_boot_info == 1) {
		printf("###Recovery Mode(SDMMC)###\n");
		writel(0x0, CONFIG_SECONDARY_BOOT_INFORM_BASE);
#ifdef CONFIG_RECOVERYCOMMAND_SDMMC
		setenv("bootcmd", CONFIG_RECOVERYCOMMAND_SDMMC);
#endif
	}
#ifdef CONFIG_EXYNOS_DA
	second_boot_info = readl(CONFIG_SECONDARY_BOOT_INFORM_BASE);
	if (second_boot_info == 2) {
		printf("###USB Download### om:%d\n", readl(&pmu->inform3));
		writel(0x0, CONFIG_SECONDARY_BOOT_INFORM_BASE);
		writel(BOOT_EMMC_4_4, &pmu->inform3);
		run_command(CFG_DNW_AUTO_CFG_PARTITION, 0); // make partition
		setenv("bootcmd", "dnw v05");
		printf("###USB Download### om:%d\n", readl(&pmu->inform3));
	}
#endif
#endif

#ifdef CONFIG_FACTORY_RESET_COMMAND
	if ((readl(&pmu->inform4)) == CONFIG_FACTORY_RESET_MODE) {
		writel(0x0, &pmu->inform4);
		setenv("bootcmd", CONFIG_FACTORY_RESET_BOOTCOMMAND);
	}
#endif

#ifdef CONFIG_FASTBOOT_AUTO_REBOOT
	if (readl(&pmu->inform4) == CONFIG_FASTBOOT_AUTO_REBOOT_MODE) {
		writel(0x0, &pmu->inform4);
		run_command("fastboot", 0);
	}
#endif

#ifdef CONFIG_RAMDUMP_MODE
	/*   check reset status for ramdump */
	if ((rst_stat & (WRESET | SYS_WDTRESET | ISP_ARM_WDTRESET))
		|| (readl(CONFIG_RAMDUMP_SCRATCH) == CONFIG_RAMDUMP_MODE)) {
		/*   run reset instead of fastboot */
		run_command("reset", 0);
	}
#endif

#ifdef CONFIG_FACTORY_INFO
	if (readl(&pmu->inform3) == BOOT_MMCSD)
		setenv("emmc_dev", "1");
#endif
	return 0;
}

#ifdef CONFIG_USB_GADGET
static int s5p_phy_control(int on)
{
	return 0;
}

struct s3c_plat_otg_data s5p_otg_data = {
	.phy_control	= s5p_phy_control,
	.regs_phy	= EXYNOS4_USBPHY_BASE,
	.regs_otg	= EXYNOS4_USBOTG_BASE,
	.usb_phy_ctrl	= EXYNOS4_USBPHY_CONTROL,
	.usb_flags	= PHY0_SLEEP,
};

int board_usb_init(int index, enum usb_init_type init)
{
	return s3c_udc_probe(&s5p_otg_data);
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	return 0;
}
#endif

#ifdef CONFIG_USBDOWNLOAD_GADGET
int g_dnl_bind_fixup(struct usb_device_descriptor *dev, const char *name)
{
	if (!strcmp(name, "usb_dnl_thor")) {
		put_unaligned(CONFIG_G_DNL_THOR_VENDOR_NUM, &dev->idVendor);
		put_unaligned(CONFIG_G_DNL_THOR_PRODUCT_NUM, &dev->idProduct);
	} else if (!strcmp(name, "usb_dnl_ums")) {
		put_unaligned(CONFIG_G_DNL_UMS_VENDOR_NUM, &dev->idVendor);
		put_unaligned(CONFIG_G_DNL_UMS_PRODUCT_NUM, &dev->idProduct);
	} else {
		put_unaligned(CONFIG_G_DNL_VENDOR_NUM, &dev->idVendor);
		put_unaligned(CONFIG_G_DNL_PRODUCT_NUM, &dev->idProduct);
	}
	return 0;
}
#endif

#ifdef CONFIG_SET_DFU_ALT_INFO
char *get_dfu_alt_system(char *interface, char *devstr)
{
	char *rootdev = getenv("rootdev");

	if (rootdev != NULL && rootdev[0] == '1')
		setenv("dfu_alt_system", CONFIG_DFU_ALT_SYSTEM_SD);

	return getenv("dfu_alt_system");
}

char *get_dfu_alt_boot(char *interface, char *devstr)
{
	struct mmc *mmc;
	char *alt_boot;
	int dev_num;

	dev_num = simple_strtoul(devstr, NULL, 10);

	mmc = find_mmc_device(dev_num);
	if (!mmc)
		return NULL;

	if (mmc_init(mmc))
		return NULL;

	if (IS_SD(mmc))
		alt_boot = CONFIG_DFU_ALT_BOOT_SD;
	else
		alt_boot = CONFIG_DFU_ALT_BOOT_EMMC;

	return alt_boot;
}

void set_dfu_alt_info(char *interface, char *devstr)
{
	size_t buf_size = CONFIG_SET_DFU_ALT_BUF_LEN;
	ALLOC_CACHE_ALIGN_BUFFER(char, buf, buf_size);
	char *alt_info = "Settings not found!";
	char *status = "error!\n";
	char *alt_setting;
	char *alt_sep;
	int offset = 0;

	puts("DFU alt info setting: ");

	alt_setting = get_dfu_alt_boot(interface, devstr);
	if (alt_setting) {
		setenv("dfu_alt_boot", alt_setting);
		offset = snprintf(buf, buf_size, "%s", alt_setting);
	}

	alt_setting = get_dfu_alt_system(interface, devstr);
	if (alt_setting) {
		if (offset)
			alt_sep = ";";
		else
			alt_sep = "";

		offset += snprintf(buf + offset, buf_size - offset,
				    "%s%s", alt_sep, alt_setting);
	}

	if (offset) {
		alt_info = buf;
		status = "done\n";
	}

	setenv("dfu_alt_info", alt_info);
	puts(status);
}
#endif
