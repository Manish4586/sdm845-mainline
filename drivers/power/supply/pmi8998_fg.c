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
#include <linux/math64.h>

#include "pmi8998_fg.h"

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
		pr_err("base cannot be zero base=0x%02x\n", addr);
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
		pr_err("addr cannot be zero base=0x%02x\n", addr);
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

/*************************
 * Battery Status RW
 * ***********************/

static int pmi8998_fg_get_capacity(struct pmi8998_fg_chip *chip, int *val)
{
	u8 cap[2];
	int error = pmi8998_read(chip->regmap, cap, REG_BASE(chip) + BATT_MONOTONIC_SOC, 2);
	if (error)
		return error;
	//choose lesser of two
	if (cap[0] != cap[1]) {
		dev_warn(chip->dev, "cap not same\n");
		cap[0] = cap[0] < cap[1] ? cap[0] : cap[1];
	}
	*val = DIV_ROUND_CLOSEST((cap[0] - 1) * 98, 0xff - 2) + 1;
	return 0;
}

//BATT_MISSING_STS BIT(6)
static bool pmi8998_battery_missing(struct pmi8998_fg_chip *chip)
{
	int rc;
	u8 fg_batt_sts;

	rc = pmi8998_read(chip->regmap, &fg_batt_sts,
				 INT_RT_STS(REG_BATT(chip)), 1);
	if (rc) {
		pr_warn("read read failed: addr=%03X, rc=%d\n",
				INT_RT_STS(REG_BATT(chip)), rc);
		return false;
	}

	// Bit 6 is set if the battery is missing
	return (fg_batt_sts & BIT(6)) ? true : false;
}

static int fg_get_temperature(struct pmi8998_fg_chip *chip, int *val)
{
	int rc, temp;
	u8 *readval = 0;

	rc = pmi8998_read(chip->regmap, readval, REG_BATT(chip) + PARAM_ADDR_BATT_TEMP, 2);
	if (rc) {
		pr_err("failed to read temperature\n");
		return rc;
	}
	temp = ((readval[1] & BATT_TEMP_MSB_MASK) << 8) |
		(readval[0] & BATT_TEMP_LSB_MASK);
	temp = DIV_ROUND_CLOSEST(temp * 10, 4);

	*val = temp -2730;
	return 0;
}

static int pmi8998_fg_get_current(struct pmi8998_fg_chip *chip, int *val)
{
	int rc, temp;
	u8 *readval = 0;

	rc = pmi8998_read(chip->regmap, readval, REG_BATT(chip) + PARAM_ADDR_BATT_CURRENT, 2);
	if (rc) {
		pr_err("failed to read current\n");
		return rc;
	}

	temp = readval[1] << 8 | readval[0];
	temp = twos_compliment_extend(temp, 2);
	*val = div_s64((s64)temp * 488281,
			1000);
	return 0;
}

static int pmi8998_fg_get_voltage(struct pmi8998_fg_chip *chip, int *val)
{
	int rc, temp;
	u8 *readval = 0;

	rc = pmi8998_read(chip->regmap, readval, REG_BATT(chip) + PARAM_ADDR_BATT_CURRENT, 2);
	if (rc) {
		pr_err("failed to read current\n");
		return rc;
	}

	temp = readval[1] << 8 | readval[0];
	temp = twos_compliment_extend(temp, 2);
	return div_s64((s64)temp * 488281,
			1000);
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
	rc = pmi8998_masked_write(chip->regmap, REG_MEM(chip) + MEM_INTF_IMA_CFG,
				BIT(2), BIT(2));
	if (rc) {
		pr_err("Error writing to IMA_CFG, rc=%d\n", rc);
		return rc;
	}

	temp = 0x4;
	rc = pmi8998_write(chip->regmap, &temp, REG_MEM(chip) + MEM_INTF_ADDR_LSB + 1, 1);
	if (rc) {
		pr_err("Error writing to MEM_INTF_ADDR_MSB, rc=%d\n", rc);
		return rc;
	}

	temp = 0x0;
	rc = pmi8998_write(chip->regmap, &temp, REG_MEM(chip) + MEM_INTF_WR_DATA0 + 3, 1);
	if (rc) {
		pr_err("Error writing to WR_DATA3, rc=%d\n", rc);
		return rc;
	}

	rc = pmi8998_read(chip->regmap, &temp, REG_MEM(chip) + MEM_INTF_RD_DATA0 + 3, 1);
	if (rc) {
		pr_err("Error writing to RD_DATA3, rc=%d\n", rc);
		return rc;
	}

	rc = pmi8998_masked_write(chip->regmap, REG_MEM(chip) + MEM_INTF_IMA_CFG,
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

	rc = pmi8998_read(chip->regmap, &err_sts,
			REG_MEM(chip) + MEM_INTF_IMA_ERR_STS, 1);
	if (rc) {
		dev_err(chip->dev, "failed to read IMA_ERR_STS, rc=%d\n", rc);
		return rc;
	}

	rc = pmi8998_read(chip->regmap, &exp_sts,
			REG_MEM(chip) + MEM_INTF_IMA_EXP_STS, 1);
	if (rc) {
		dev_err(chip->dev, "Error in reading IMA_EXP_STS, rc=%d\n", rc);
		return rc;
	}


	if (check_hw_sts) {
		rc = pmi8998_read(chip->regmap, &hw_sts,
				REG_MEM(chip) + MEM_INTF_IMA_HW_STS, 1);
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

static int fg_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct pmi8998_fg_chip *chip = power_supply_get_drvdata(psy);
	int error = 0, temp;

	dev_info(chip->dev, "Getting property: %d", psp);

	switch (psp) {
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = "QCOM";
		break;
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = "PMI8998 Battery";
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		//error = pmi8998_fg_get_capacity(chip, &val->intval);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		//error = pmi8998_fg_get_current(chip, &val->intval);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		//error = pmi8998_fg_get_voltage(chip, &val->intval);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_OCV:
		//error = fg_get_param(chip, FG_DATA_OCV, &val->intval);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = chip->batt_info.batt_max_voltage_uv;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		error = fg_get_temperature(chip, &val->intval);
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		error = 10;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN:
		val->intval = 3370000; /* single-cell li-ion low end */
		break;
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:
		//error = fg_get_param(chip, FG_DATA_CHARGE_COUNTER, &val->intval);
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		val->intval = chip->batt_info.nom_cap_uah;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		val->intval = chip->learning_data.learned_cc_uah;
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		val->intval = chip->learning_data.cc_uah;
		break;
	// case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
	// 	temp = chip->param[FG_SETTING_CHG_TERM_CURRENT].value * 1000;
	// 	val->intval = temp;
	// 	break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = chip->status;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	default:
		pr_err("invalid property: %d\n", psp);
		return -EINVAL;
	}
	return error;
}

static const struct power_supply_desc bms_psy_desc = {
	.name = "pmi8998-bms",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = fg_properties,
	.num_properties = ARRAY_SIZE(fg_properties),
	.get_property = fg_get_property,
	//.set_property = fg_set_property,
	//.property_is_writeable = fg_writable_property,
};


// static int pmi8998_fg_of_battery_init(struct pmbms_psy_desci8998_fg_chip *chip){
// 	struct device_node *batt_node;
// 	struct device_node *node = chip->dev->of_node;
// 	int rc = 0, len = 0;
// 	const char *data;
//
// 	batt_node = of_find_node_by_name(node, "qcom,battery-data");
// 	if (!batt_node) {
// 		pr_err("No available batterydata\n");
// 		return rc;
// 	}
//
// 	rc = of_property_read_u32(batt_node, "qcom,max-voltage-uv",
// 					&chip->batt_info.batt_max_voltage_uv_design);
//
	// data = of_get_property(chip->dev->of_node,
	// 		"qcom,thermal-coefficients", &len);
	// if (data && ((len == 6 && chip->pmic_version == PMI8950) ||
	// 		(len == 3 && chip->pmic_version != PMI8950))) {
	// 	memcpy(chip->batt_info.thermal_coeffs, data, len);
	// 	chip->param[FG_SETTING_THERMAL_COEFFS].length = len;
	// }

	// data = of_get_property(batt_node, "qcom,fg-profile-data", &len);
	// if (!data) {
	// 	pr_err("no battery profile loaded\n");
	// 	return 0;
	// }

	// if ((chip->pmic_version == PMI8950 && len != FG_PROFILE_LEN_PMI8950) ||
	// 	(chip->pmic_version == PMI8998_V1 && len != FG_PROFILE_LEN_PMI8998) ||
	// 	(chip->pmic_version == PMI8998_V2 && len != FG_PROFILE_LEN_PMI8998)) {
	// 	pr_err("battery profile incorrect size: %d\n", len);
	// 	return -EINVAL;
	// }

	// chip->batt_info.batt_profile = devm_kzalloc(chip->dev,
	// 		sizeof(char) * len, GFP_KERNEL);

	// if (!chip->batt_info.batt_profile) {
	// 	pr_err("coulnt't allocate mem for battery profile\n");
	// 	rc = -ENOMEM;
	// 	return rc;
	// }

	// if (!is_profile_load_required(chip))
	// 	goto done;
	// else
	// 	pr_warn("profile load needs to be done, but not done!\n");
//
// done:
// 	rc = fg_get_param(chip, FG_DATA_NOM_CAP, &chip->batt_info.nom_cap_uah);
// 	if (rc) {
// 		pr_err("Failed to read nominal capacitance: %d\n", rc);
// 		return -EINVAL;
// 	}
//
// 	return rc;
// }

static int pmi8998_fg_probe(struct platform_device *pdev)
{
	struct power_supply_config supply_config = {};
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

	// Get base address
	prop_addr = of_get_address(pdev->dev.of_node, 0, NULL, NULL);
	if (!prop_addr) {
		dev_err(chip->dev, "Couldn't read SOC base address from dt\n");
		return -EINVAL;
	}
	chip->base = be32_to_cpu(*prop_addr);

	//chip->sram_params = pmi8998_sram_params_v2;

	// Init memif fn inlined here (chip hardware info)
	rc = pmi8998_read(chip->regmap, chip->revision, REG_MEM(chip) + DIG_MINOR, 4);
	if (rc) {
		dev_err(chip->dev, "Unable to read FG revision rc=%d\n", rc);
		return rc;
	}

	// dev_info(chip->dev, "pmi8998 revision DIG:%d.%d ANA:%d.%d\n",
	// 	chip->revision[DIG_MAJOR], chip->revision[DIG_MINOR],
	// 	chip->revision[ANA_MAJOR], chip->revision[ANA_MINOR]);
	
	/*
	 * Change the FG_MEM_INT interrupt to track IACS_READY
	 * condition instead of end-of-transaction. This makes sure
	 * that the next transaction starts only after the hw is ready.
	 * IACS_INTR_SRC_SLCT is BIT(3)
	 */
	rc = pmi8998_masked_write(chip->regmap,
		REG_MEM(chip) + MEM_INTF_IMA_CFG, BIT(3), BIT(3));
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
	rc = pmi8998_read(chip->regmap, &dma_status, REG_MEM(chip) + 0x70, 1);
	if (rc < 0) {
		pr_err("failed to read dma_status, rc=%d\n", rc);
		return rc;
	}

	error_present = dma_status & (BIT(1) | BIT(2));
	rc = pmi8998_masked_write(chip->regmap, REG_MEM(chip) + 0x71, BIT(0),
			error_present ? BIT(0) : 0);
	if (rc < 0) {
		pr_err("failed to write dma_ctl, rc=%d\n", rc);
		return rc;
	}

	// TODO: Init properties
	dev_info(chip->dev, "probed revision DIG:%d.%d ANA:%d.%d\n",
		chip->revision[DIG_MAJOR], chip->revision[DIG_MINOR],
		chip->revision[ANA_MAJOR], chip->revision[ANA_MINOR]);
	//pmi8998_battery_profile_init(&chip);
	//pmi8998_hw_init();

	supply_config.drv_data = chip;
	supply_config.of_node = pdev->dev.of_node;

	chip->bms_psy = devm_power_supply_register(chip->dev,
			&bms_psy_desc, &supply_config);
	if (IS_ERR(chip->bms_psy)) {
		dev_err(&pdev->dev, "failed to register battery\n");
		return PTR_ERR(chip->bms_psy);
	}

	platform_set_drvdata(pdev, chip);
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
