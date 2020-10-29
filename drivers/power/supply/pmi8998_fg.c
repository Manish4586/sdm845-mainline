// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2020, The Linux Foundation. All rights reserved. */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/regmap.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/power_supply.h>
#include <linux/module.h>

#include "pmi8998_fg.h"


static int64_t twos_compliment_extend(int64_t val, int nbytes)
{
	int i;
	int64_t mask;

	mask = 0x80LL << ((nbytes - 1) * 8);
	if (val & mask) {
		for (i = 8; i > nbytes; i--) {
			mask = 0xFFLL << ((i - 1) * 8);
			val |= mask;
		}
	}

	return val;
}

static int pmi8998_decode_temperature(struct pmi8998_sram_param sp, u8 *val)
{
	int temp;

	temp = ((val[1] & BATT_TEMP_MSB_MASK) << 8) |
		(val[0] & BATT_TEMP_LSB_MASK);
	temp = DIV_ROUND_CLOSEST(temp * sp.denmtr, sp.numrtr);

	return temp + sp.val_offset;
}

static int pmi8998_decode_value_16b(struct pmi8998_sram_param sp, u8 *value)
{
	int temp = 0, i;
	if (sp.wa_flags & PMI8998_V2_REV_WA)
		for (i = 0; i < sp.length; ++i)
			temp |= value[i] << (8 * (sp.length - i));
	else
		for (i = 0; i < sp.length; ++i)
			temp |= value[i] << (8 * i);
	return div_u64((u64)(u16)temp * sp.denmtr, sp.numrtr) + sp.val_offset;
}

static int pmi8998_decode_current(struct pmi8998_sram_param sp, u8 *val)
{
	int64_t temp;
	if (sp.wa_flags & PMI8998_V1_REV_WA)
		temp = val[0] << 8 | val[1];
	else
		temp = val[1] << 8 | val[0];
	temp = twos_compliment_extend(temp, 2);
	return div_s64((s64)temp * sp.denmtr,
			sp.numrtr);
}

static struct pmi8998_sram_param pmi8998_sram_params_v2[FG_PARAM_MAX] = {
	[FG_DATA_BATT_TEMP] = {
		.address	= 0x50,
		.type		= BATT_BASE_PARAM,
		.length		= 2,
		.numrtr		= 4,
		.denmtr		= 10,		//Kelvin to DeciKelvin
		.val_offset	= -2730,	//DeciKelvin to DeciDegc
		.decode		= pmi8998_decode_temperature
	},
	[FG_DATA_VOLTAGE] = {
		.address	= 0xa0,
		.type		= BATT_BASE_PARAM,
		.length		= 2,
		.numrtr		= 122070,
		.denmtr		= 1000,
		.wa_flags	= PMI8998_V2_REV_WA,
		.decode		= pmi8998_decode_value_16b,
	},
	[FG_DATA_CURRENT] = {
		.address	= 0xa2,
		.length		= 2,
		.type		= BATT_BASE_PARAM,
		.wa_flags	= PMI8998_V2_REV_WA,
		.numrtr		= 1000,
		.denmtr		= 488281,
		.decode 	= pmi8998_decode_current,
	},
};

/************************
 * IO FUNCTIONS
 * **********************/

/**
 * pmi8998_read() - Read multiple registers with regmap_bulk_read
 * 
 * @param regmap The regmap to read
 * @param val Pointer to read values into
 * @param addr Address to read from
 * @param len Number of registers (bytes) to read
 * @return int 0 on success, negative errno on error
 */
static int pmi8998_read(struct pmi8998_fg_chip *chip, u8 *val, u16 addr, int len)
{
	if ((addr & 0xff00) == 0) {
		dev_err(chip->dev, "base cannot be zero base=0x%02x\n", addr);
		return -EINVAL;
	}

	return regmap_bulk_read(chip->regmap, addr, val, len);
}

/**
 * @brief pmi8998_write() - Write multiple registers with regmap_bulk_write
 * 
 * @param map 
 * @param val 
 * @param addr 
 * @param len 
 * @return int 
 */
static int pmi8998_write(struct pmi8998_fg_chip *chip, u8 *val, u16 addr, int len)
{
	int rc;
	bool sec_access = (addr & 0xff) > 0xd0;
	u8 sec_addr_val = 0xa5;

	if (sec_access) {
		rc = regmap_bulk_write(chip->regmap,
				(addr & 0xff00) | 0xd0,
				&sec_addr_val, 1);
	}

	if ((addr & 0xff00) == 0) {
		dev_err(chip->dev, "addr cannot be zero base=0x%02x\n", addr);
		return -EINVAL;
	}

	return regmap_bulk_write(chip->regmap, addr, val, len);
}

static int pmi8998_masked_write(struct pmi8998_fg_chip *chip, u16 addr,
		u8 mask, u8 val)
{
	int error;
	u8 reg;
	error = pmi8998_read(chip, &reg, addr, 1);
	if (error)
		return error;

	reg &= ~mask;
	reg |= val & mask;

	error = pmi8998_write(chip, &reg, addr, 1);
	return error;
}

/*************************
 * MISC
 * ***********************/

/* IMA_IACS_CLR			BIT(2)
   IMA_IACS_RDY			BIT(1) */
static int pmi8998_iacs_clear_sequence(struct pmi8998_fg_chip *chip)
{
	int rc = 0;
	u8 temp;

	/* clear the error */
	rc = pmi8998_masked_write(chip, chip->mem_base + MEM_INTF_IMA_CFG,
				BIT(2), BIT(2));
	if (rc) {
		pr_err("Error writing to IMA_CFG, rc=%d\n", rc);
		return rc;
	}

	temp = 0x4;
	rc = pmi8998_write(chip, &temp, chip->mem_base + MEM_INTF_ADDR_LSB + 1, 1);
	if (rc) {
		pr_err("Error writing to MEM_INTF_ADDR_MSB, rc=%d\n", rc);
		return rc;
	}

	temp = 0x0;
	rc = pmi8998_write(chip, &temp, chip->mem_base + MEM_INTF_WR_DATA0 + 3, 1);
	if (rc) {
		pr_err("Error writing to WR_DATA3, rc=%d\n", rc);
		return rc;
	}

	rc = pmi8998_read(chip, &temp, chip->mem_base + MEM_INTF_RD_DATA0 + 3, 1);
	if (rc) {
		pr_err("Error writing to RD_DATA3, rc=%d\n", rc);
		return rc;
	}

	rc = pmi8998_masked_write(chip, chip->mem_base + MEM_INTF_IMA_CFG,
				BIT(2), 0);
	if (rc) {
		pr_err("Error writing to IMA_CFG, rc=%d\n", rc);
		return rc;
	}
	return rc;
}

/**
 * @brief Checks and clears IMA status registers on error
 * 
 * @param chip 
 * @param check_hw_sts 
 * @return int 
 */
static int pmi8998_clear_ima(struct pmi8998_fg_chip *chip,
		bool check_hw_sts)
{
	int rc = 0, ret = 0;
	u8 err_sts = 0, exp_sts = 0, hw_sts = 0;
	bool run_err_clr_seq = false;

	rc = pmi8998_read(chip, &err_sts,
			chip->mem_base + MEM_INTF_IMA_ERR_STS, 1);
	if (rc) {
		dev_err(chip->dev, "failed to read IMA_ERR_STS, rc=%d\n", rc);
		return rc;
	}

	rc = pmi8998_read(chip, &exp_sts,
			chip->mem_base + MEM_INTF_IMA_EXP_STS, 1);
	if (rc) {
		dev_err(chip->dev, "Error in reading IMA_EXP_STS, rc=%d\n", rc);
		return rc;
	}


	if (check_hw_sts) {
		rc = pmi8998_read(chip, &hw_sts,
				chip->mem_base + MEM_INTF_IMA_HW_STS, 1);
		if (rc) {
			dev_err(chip->dev, "Error in reading IMA_HW_STS, rc=%d\n", rc);
			return rc;
		}
		/*
		 * Lower nibble should be equal to upper nibble before SRAM
		 * transactions begins from SW side. If they are unequal, then
		 * the error clear sequence should be run irrespective of IMA
		 * exception errors.
		 */
		if ((hw_sts & 0x0f) != hw_sts >> 4) {
			dev_err(chip->dev, "IMA HW not in correct state, hw_sts=%x\n",
					hw_sts);
			run_err_clr_seq = true;
		}
	}

	/*
		IACS_ERR_BIT		BIT(0)
		XCT_ERR_BIT			BIT(1)
		DATA_RD_ERR_BIT		BIT(3)
		DATA_WR_ERR_BIT		BIT(4)
		ADDR_BURST_WRAP_BIT	BIT(5)
		ADDR_RNG_ERR_BIT	BIT(6)
		ADDR_SRC_ERR_BIT	BIT(7)
	*/
	if (exp_sts & (BIT(0) | BIT(1) | BIT(3) |
		BIT(4) | BIT(5) | BIT(6) |
		BIT(7))) {
		dev_warn(chip->dev, "IMA exception bit set, exp_sts=%x\n", exp_sts);
		run_err_clr_seq = true;
	}

	if (run_err_clr_seq) {
		ret = pmi8998_iacs_clear_sequence(chip);
		if (!ret)
			return -EAGAIN;
		else
			dev_err(chip->dev, "Error clearing IMA exception ret=%d\n", ret);
	}

	return rc;
}

/********************
 * Init stuff
 * ******************/

// static int pmi8998_battery_profile_init(struct pmi8998_fg_chip *chip)
// {
// 	struct device_node *batt_node;
// 	struct device_node *node = chip->dev->of_node;
// 	int rc = 0, len = 0;
// 	const char *data;
// }

static int pmi8998_fg_probe(struct platform_device *pdev)
{
	// struct power_supply_config supply_config = {};
    struct pmi8998_fg_chip *chip;
	const __be32 *prop_addr;
	int rc = 0;
	u8 dma_status;
	bool error_present;

    chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
    if (!chip) {
        return -ENOMEM;
    }

    chip->dev = &pdev->dev;
	mutex_init(&chip->lock);

	chip->regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!chip->regmap) {
		dev_err(chip->dev, "failed to locate the regmap\n");
		return -ENODEV;
	}

	// Get base addresses

	prop_addr = of_get_address(pdev->dev.of_node, 0, NULL, NULL);
	if (!prop_addr) {
		dev_err(chip->dev, "Couldn't read SOC base address from dt\n");
		return -EINVAL;
	}
	chip->soc_base = be32_to_cpu(*prop_addr);

	prop_addr = of_get_address(pdev->dev.of_node, 1, NULL, NULL);
	if (!prop_addr) {
		dev_err(chip->dev, "Couldn't read BATT base address from dt\n");
		return -EINVAL;
	}
	chip->batt_base = be32_to_cpu(*prop_addr);

	prop_addr = of_get_address(pdev->dev.of_node, 2, NULL, NULL);
	if (!prop_addr) {
		dev_err(chip->dev, "Couldn't read MEM base address from dts\n");
		return -EINVAL;
	}
	chip->mem_base = be32_to_cpu(*prop_addr);

	chip->sram_params = pmi8998_sram_params_v2;

	// Init memif (chip hardware info)
	rc = pmi8998_read(chip, chip->revision, chip->mem_base + DIG_MINOR, 4);
	if (rc) {
		dev_err(chip->dev, "Unable to read FG revision rc=%d\n", rc);
		return rc;
	}

	dev_info(chip->dev, "pmi8998 revision DIG:%d.%d ANA:%d.%d\n",
		chip->revision[DIG_MAJOR], chip->revision[DIG_MINOR],
		chip->revision[ANA_MAJOR], chip->revision[ANA_MINOR]);
	
	/*
	 * Change the FG_MEM_INT interrupt to track IACS_READY
	 * condition instead of end-of-transaction. This makes sure
	 * that the next transaction starts only after the hw is ready.
	 * IACS_INTR_SRC_SLCT is BIT(3)
	 */
	rc = pmi8998_masked_write(chip,
		chip->mem_base + MEM_INTF_IMA_CFG, BIT(3), BIT(3));
	if (rc) {
		dev_err(chip->dev,
			"failed to configure interrupt source %d\n",
			rc);
		return rc;
	}

	rc = pmi8998_clear_ima(chip, true);
	if (rc && rc != -EAGAIN) {
		dev_err(chip->dev, "Error clearing IMA, exception rc=%d", rc);
		return rc;
	}

	// Check and clear DMA errors
	rc = pmi8998_read(chip, &dma_status, chip->mem_base + 0x70, 1);
	if (rc < 0) {
		pr_err("failed to read dma_status, rc=%d\n", rc);
		return rc;
	}

	error_present = dma_status & (BIT(1) | BIT(2));
	rc = pmi8998_masked_write(chip, chip->mem_base + 0x71, BIT(0),
			error_present ? BIT(0) : 0);
	if (rc < 0) {
		pr_err("failed to write dma_ctl, rc=%d\n", rc);
		return rc;
	}

	// TODO: Init properties
	dev_info(chip->dev, "NO ERRORS, reaches till here\n");
	//pmi8998_battery_profile_init(&chip);
	//pmi8998_hw_init();
	return 0;
}

static int pmi8998_fg_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id fg_match_id_table[] = {
	{ .compatible = "qcom,pmi8998-fg" },
	{ /* sentinal */ }
};
MODULE_DEVICE_TABLE(of, fg_match_id_table);

static struct platform_driver qcom_fg_driver = {
	.probe = pmi8998_fg_probe,
	.remove = pmi8998_fg_remove,
	.driver = {
		.name = "pmi8998-fg",
		.of_match_table = fg_match_id_table,
	},
};

module_platform_driver(qcom_fg_driver);

MODULE_AUTHOR("Caleb Connolly <caleb@connolly.tech>");
MODULE_AUTHOR("Joel Selvaraj <jo@jsfamily.in>");
MODULE_DESCRIPTION("Qualcomm PMI8998 Fuel Guage Driver");
MODULE_LICENSE("GPL v2");
