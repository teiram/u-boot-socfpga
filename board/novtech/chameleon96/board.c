// SPDX-License-Identifier: GPL-2.0

#ifdef CONFIG_BOARD_LATE_INIT

#include <stdio.h>
#include <asm/io.h>
#include <i2c.h>
#include <command.h>

static int hdmi_init(int mode) {
	u8 value;
	printf("> Initializing HDMI Transmitter for mode %s\n", mode ? "1080p" : "720p");
	struct udevice *dev;
	int ret;

	ret = i2c_get_chip_for_busnum(2, 0x37, 1, &dev);
	if (ret) {
		puts("Cannot find TDA19988 I2C device\n");
		return -1;
	}

	//i2c mw 37 FF.1 02 1    
	value = 0x02;
    dm_i2c_write(dev, 0xFF, &value, 1);

    ret = i2c_get_chip_for_busnum(2, 0x73, 1, &dev);
	if (ret) {
		puts("Cannot find TDA19988 I2C HDMI device\n");
		return -1;
	}

	// i2c mw 73 FF.1 00 1                                         
	value = 0x00;
    dm_i2c_write(dev, 0xFF, &value, 1);

	//i2c mw 73 A0.1 06/02 1                                        
	value = mode ? 0x06 : 0x02;
    dm_i2c_write(dev, 0xA0, &value, 1);

	//i2c mw 73 CB.1 00 1                                         
	value = 0x00;
    dm_i2c_write(dev, 0xCB, &value, 1);

	//i2c mw 73 F0.1 00 1                                         
	value = 0x00;
    dm_i2c_write(dev, 0xF0, &value, 1);

	//i2c mw 73 18.1 FF 1                                         
	value = 0xFF;
    dm_i2c_write(dev, 0x18, &value, 1);

	//i2c mw 73 19.1 FF 1                                         
	value = 0xFF;
    dm_i2c_write(dev, 0x19, &value, 1);

	//i2c mw 73 1A.1 FF 1                                         
	value = 0xFF;
    dm_i2c_write(dev, 0x1A, &value, 1);

	//i2c mw 73 20.1 45 1                                         
	value = 0x45;
    dm_i2c_write(dev, 0x20, &value, 1);

	//i2c mw 73 21.1 23 1                                         
	value = 0x23;
    dm_i2c_write(dev, 0x21, &value, 1);

	//i2c mw 73 22.1 01 1                                         
	value = 0x01;
    dm_i2c_write(dev, 0x22, &value, 1);

    ret = i2c_get_chip_for_busnum(2, 0x37, 1, &dev);
	if (ret) {
		puts("Cannot find TDA19988 I2C device\n");
		return -1;
	}

	//i2c mw 73 23.1 20 1                                        
	value = 0x20;
    dm_i2c_write(dev, 0x23, &value, 1);

    return 0;
}

int board_late_init(void) {
    int reg;

    printf("> Resetting USB-OTG PHY\n");  // Reset on RGMII0_TX_CTL => GPIO09 => bit 9 of GPIO0
   	reg = readl(0xFF708004);
	reg |= 0x00000200;                    // Set bit 9 => output
	writel(reg, 0xFF708004);

	reg = readl(0xFF708000);
	reg |= 0x00000200;                    // Set bit 9. USB PHY is reset high
	writel(reg, 0xFF708000);
	
    printf("> Resetting USB Hub\n");      // nReset on RGMII0_TX_CLK => GPIO00 => bit 0 of GPIO0
    reg = readl(0xFF708004);
	reg |= 0x00000001;                    // Set bit 0 => output
	writel(reg, 0xFF708004);

	reg = readl(0xFF708000);
	reg &= 0xFFFFFFFE;                    // Reset bit 0. USB HUB is reset low
	writel(reg, 0xFF708000);

	    
	printf("> Loading hps-ch96.rbf from mmc 0:1\n");
	int rc = run_command("load mmc 0:1 $loadaddr hps-ch96.rbf", 0);
	printf("  Exit code: %d\n", rc);

    int tries = 1;

    while (tries <= 5) {
        printf("> Programming FPGA\n");
        rc = run_command("fpga load 0 $loadaddr $filesize", 0);
        if (rc >= 0) {
            break;
        } else {
            tries++;
        }
    }
	printf("  Exit code: %d after %d tries\n", rc, tries);
    printf("> Enabling FPGA-HPS bridges\n");
	rc = run_command("bridge enable", 0);			
	printf("  Exit code %d\n", rc);
	reg = readl(0xFF200000);
	printf("* FPGA HDL ID = 0x%x.\n", reg);	
	
	
    printf("> Enabling USB HUB\n");      // Reset on RGMII0_TX_CLK => GPIO00 => bit 0 of GPIO0
	reg = readl(0xFF708000);
	reg |= 0x00000001;                   // Set bit 0 (reset low)
	writel(reg, 0xFF708000);

    printf("> Enabling USB-OTG PHY\n");  // Reset on RMGII0_TX_CTL => GPIO09 => bit 9 of GPIO0
	reg = readl(0xFF708000);
	reg &= 0xFFFFFDFF;                   // Reset bit 9 (reset high)
	writel(reg, 0xFF708000);


    hdmi_init(0);

    return 0;
}

#endif /* CONFIG_BOARD_LATE_INIT */