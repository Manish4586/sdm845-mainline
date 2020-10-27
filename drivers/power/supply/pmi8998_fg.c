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

#include "pmi8998_fg.h"

struct pmi8998_fg_info {
	struct device *dev;
	struct regmap *regmap;
	struct mutex *lock;

	struct power_supply *supply;

	/* base addresses of components*/
	unsigned int soc_base;
	unsigned int batt_base;
	unsigned int mem_base;

	struct pmi8998_sram_param *sram;

	// Chip hardware info
	u8 revision[4];
	enum pmic pmic_version;
    
	int power_status;
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
static int pmi8998_read(struct regmap *map, u8 *val, u16 addr, int len)
{
	if ((addr & 0xff00) == 0) {
		dev_err(&chip->dev, "base cannot be zero base=0x%02x\n", addr);
		return -EINVAL;
	}

	return regmap_bulk_read(map, addr, val, len);
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
static int pmi8998_write(struct regmap *map, u8 *val, u16 addr, int len)
{
	int rc;
	bool sec_access = (addr & 0xff) > 0xd0;
	u8 sec_addr_val = 0xa5;

	if (sec_access) {
		rc = regmap_bulk_write(map,
				(addr & 0xff00) | 0xd0,
				&sec_addr_val, 1);
	}

	if ((addr & 0xff00) == 0) {
		dev_err(&chip->dev, "addr cannot be zero base=0x%02x\n", addr);
		return -EINVAL;
	}

	return regmap_bulk_write(map, addr, val, len);
}

static int pmi8998_masked_write(struct regmap *map, u16 addr,
		u8 mask, u8 val)
{
	int error;
	u8 reg;
	error = pmi8998_read(map, &reg, addr, 1);
	if (error)
		return error;

	reg &= ~mask;
	reg |= val & mask;

	error = pmi8998_write(map, &reg, addr, 1);
	return error;
}

/*************************
 * MISC
 * ***********************/

/**
 * @brief Checks and clears IMA status registers on error
 * 
 * @param chip 
 * @param check_hw_sts 
 * @return int 
 */
static int pmi8998_clear_ima(struct fg_chip *chip,
		bool check_hw_sts)
{
	int rc = 0, ret = 0;
	u8 err_sts = 0, exp_sts = 0, hw_sts = 0;
	bool run_err_clr_seq = false;

	rc = pmi8998_read(chip, &err_sts,
			chip->mem_base + MEM_INTF_IMA_ERR_STS, 1);
	if (rc) {
		dev_err(&chip->dev, "failed to read IMA_ERR_STS, rc=%d\n", rc);
		return rc;
	}

	rc = pmi8998_read(chip, &exp_sts,
			chip->mem_base + MEM_INTF_IMA_EXP_STS, 1);
	if (rc) {
		dev_err(&chip->dev, "Error in reading IMA_EXP_STS, rc=%d\n", rc);
		return rc;
	}


	if (check_hw_sts) {
		rc = pmi8998_read(chip, &hw_sts,
				chip->mem_base + MEM_INTF_IMA_HW_STS, 1);
		if (rc) {
			dev_err(&chip->dev, "Error in reading IMA_HW_STS, rc=%d\n", rc);
			return rc;
		}
		/*
		 * Lower nibble should be equal to upper nibble before SRAM
		 * transactions begins from SW side. If they are unequal, then
		 * the error clear sequence should be run irrespective of IMA
		 * exception errors.
		 */
		if ((hw_sts & 0x0f) != hw_sts >> 4) {
			dev_err(&chip->dev, "IMA HW not in correct state, hw_sts=%x\n",
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
		dev_warn(&chip->dev, "IMA exception bit set, exp_sts=%x\n", exp_sts);
		run_err_clr_seq = true;
	}

	if (run_err_clr_seq) {
		ret = fg_run_iacs_clear_sequence(chip);
		if (!ret)
			return -EAGAIN;
		else
			dev_err(&chip->dev, "Error clearing IMA exception ret=%d\n", ret);
	}

	return rc;
}

/********************
 * Init stuff
 * ******************/

static int pmi8998_battery_profile_init(struct pmi8998_fg_chip *chip)
{
	struct device_node *batt_node;
	struct device_node *node = chip->dev->of_node;
	int rc = 0, len = 0;
	const char *data;
}

static int pmi8998_fg_probe(struct platform_device *pdev)
{
	struct power_supply_config supply_config = {};
    struct pmi8998_fg_chip *chip;
	const __be32 *prop_addr;
	u8 dma_status;
	bool error_present;

    chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
    if (!chip) {
        return -ENOMEM;
    }

    chip->dev = &pdev->dev;
	mutex_init(&chip->lock);

	chip->regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!chip.regmap) {
		dev_err(&chip->dev, "failed to locat the regmap\n");
		return -ENODEV;
	}

	// Get base addresses

	prop_addr = of_get_address(pdev->dev.of_node, 0, NULL, NULL);
	if (!prop_addr) {
		dev_err(chip->dev, "Can't get address of pmi8998_fg\n");
		return -EINVAL;
	}
	chip->soc_base = be32_to_cpu(*prop_addr);

	prop_addr = of_get_address(pdev->dev.of_node, 1, NULL, NULL);
	if (!prop_addr) {
		dev_err(chip->dev, "invalid IO resources\n");
		return -EINVAL;
	}
	chip->batt_base = be32_to_cpu(*prop_addr);

	prop_addr = of_get_address(pdev->dev.of_node, 2, NULL, NULL);
	if (!prop_addr) {
		dev_err(chip->dev, "invalid IO resources\n");
		return -EINVAL;
	}
	chip->mem_base = be32_to_cpu(*prop_addr);

	chip->sram = pmi8998_sram_params_v2;

	chip->power_status = POWER_SUPPLY_STATUS_DISCHARGING;

	// Init memif (chip hardware info)
	rc = pmi8998_read(chip->regmap, chip->revision, chip->mem_base + DIG_MINOR, 4);
	if (rc) {
		dev_err(chip->dev, "Unable to read FG revision rc=%d\n", rc);
		return rc;
	}

	dev_info(&chip->dev, "pmi8998 revision DIG:%d.%d ANA:%d.%d\n",
		chip->revision[DIG_MAJOR], chip->revision[DIG_MINOR],
		chip->revision[ANA_MAJOR], chip->revision[ANA_MINOR]);
	
	/*
	 * Change the FG_MEM_INT interrupt to track IACS_READY
	 * condition instead of end-of-transaction. This makes sure
	 * that the next transaction starts only after the hw is ready.
	 * IACS_INTR_SRC_SLCT is BIT(3)
	 */
	rc = pmi8998_masked_write(chip,
		chip->mem_base + MEM_INTF_IMA_CFG, BIT(3),
		BIT(3));
	if (rc) {
		dev_err(chip->dev,
			"failed to configure interrupt source %d\n",
			rc);
		return rc;
	}

	rc = pmi8998_clear_ima(chip, true);
	if (rc && rc != -EAGAIN) {
		dev_err(&chip->dev, "Error clearing IMA, exception rc=%d", rc);
		return rc;
	}

	// Check and clear DMA errors
	rc = fg_read(chip, &dma_status, chip->mem_base + 0x70, 1);
	if (rc < 0) {
		pr_err("failed to dma_status, rc=%d\n", rc);
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
	
	pmi8998_battery_profile_init(&chip);

	return 0;
}

static int fg_remove(struct platform_device *pdev)
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

MODULE_DESCRIPTION("Qualcomm PMI8998 Fuel Guage Driver");
MODULE_LICENSE("GPL v2");
