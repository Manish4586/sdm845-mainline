// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2020, The Linux Foundation. All rights reserved. 
 * Ported from downstream Qualcomm 4.9 kernel 
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/regmap.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/power_supply.h>
#include <linux/module.h>
#include <linux/regulator/driver.h>

#include "pmi8998_smb2.h"

static int smb2_probe(struct platform_device *pdev)
{
	struct pmi8998_smb2_chip *chip;
	int rc = 0;
	union power_supply_propval val;
	int usb_present, batt_present, batt_health, batt_charge_type;
	struct msm_bus_scale_pdata *pdata;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->regmap = dev_get_regmap(chip->dev->parent, NULL);
	if (!chip->regmap) {
		pr_err("parent regmap is missing\n");
		return -EINVAL;
	}

	// rc = smb2_chip_config_init(chip);
	// if (rc < 0) {
	// 	if (rc != -EPROBE_DEFER)
	// 		pr_err("Couldn't setup chip_config rc=%d\n", rc);
	// 	return rc;
	// }

	// rc = smb2_parse_dt(chip);
	// if (rc < 0) {
	// 	pr_err("Couldn't parse device tree rc=%d\n", rc);
	// 	goto cleanup;
	// }

	rc = smblib_init(chip);
	if (rc < 0) {
		pr_err("Smblib_init failed rc=%d\n", rc);
		goto cleanup;
	}

	/* set driver data before resources request it */
	platform_set_drvdata(pdev, chip);

	pdata = msm_bus_cl_get_pdata(pdev);
	if (!pdata)
		pr_err("Fail to get bus data\n");
	else
		chip->bus_client = msm_bus_scale_register_client(pdata);

	rc = smb2_init_vbus_regulator(chip);
	if (rc < 0) {
		pr_err("Couldn't initialize vbus regulator rc=%d\n",
			rc);
		goto cleanup;
	}

	rc = smb2_init_vconn_regulator(chip);
	if (rc < 0) {
		pr_err("Couldn't initialize vconn regulator rc=%d\n",
				rc);
		goto cleanup;
	}

	rc = devm_extcon_dev_register(chip->dev, chip->extcon);
	if (rc < 0) {
		dev_err(chip->dev, "failed to register extcon device rc=%d\n",
				rc);
		goto cleanup;
	}

	rc = smb2_init_hw(chip);
	if (rc < 0) {
		pr_err("Couldn't initialize hardware rc=%d\n", rc);
		goto cleanup;
	}

	rc = smb2_init_dc_psy(chip);
	if (rc < 0) {
		pr_err("Couldn't initialize dc psy rc=%d\n", rc);
		goto cleanup;
	}

	rc = smb2_init_usb_psy(chip);
	if (rc < 0) {
		pr_err("Couldn't initialize usb psy rc=%d\n", rc);
		goto cleanup;
	}

	rc = smb2_init_usb_main_psy(chip);
	if (rc < 0) {
		pr_err("Couldn't initialize usb main psy rc=%d\n", rc);
		goto cleanup;
	}

	rc = smb2_init_usb_port_psy(chip);
	if (rc < 0) {
		pr_err("Couldn't initialize usb pc_port psy rc=%d\n", rc);
		goto cleanup;
	}

	rc = smb2_init_batt_psy(chip);
	if (rc < 0) {
		pr_err("Couldn't initialize batt psy rc=%d\n", rc);
		goto cleanup;
	}

	rc = smb2_determine_initial_status(chip);
	if (rc < 0) {
		pr_err("Couldn't determine initial status rc=%d\n",
			rc);
		goto cleanup;
	}

	rc = smb2_request_interrupts(chip);
	if (rc < 0) {
		pr_err("Couldn't request interrupts rc=%d\n", rc);
		goto cleanup;
	}

	rc = smb2_post_init(chip);
	if (rc < 0) {
		pr_err("Failed in post init rc=%d\n", rc);
		goto cleanup;
	}

	smb2_create_debugfs(chip);

	rc = smblib_get_prop_usb_present(chip, &val);
	if (rc < 0) {
		pr_err("Couldn't get usb present rc=%d\n", rc);
		goto cleanup;
	}
	usb_present = val.intval;

	rc = smblib_get_prop_batt_present(chip, &val);
	if (rc < 0) {
		pr_err("Couldn't get batt present rc=%d\n", rc);
		goto cleanup;
	}
	batt_present = val.intval;

	rc = smblib_get_prop_batt_health(chip, &val);
	if (rc < 0) {
		pr_err("Couldn't get batt health rc=%d\n", rc);
		val.intval = POWER_SUPPLY_HEALTH_UNKNOWN;
	}
	batt_health = val.intval;

	rc = smblib_get_prop_batt_charge_type(chip, &val);
	if (rc < 0) {
		pr_err("Couldn't get batt charge type rc=%d\n", rc);
		goto cleanup;
	}
	batt_charge_type = val.intval;
/* david.liu@bsp, 20171023 Battery & Charging porting */
#ifdef CONFIG_PROC_FS
	if (!proc_create("ship_mode", 0644, NULL,
		 &proc_ship_mode_operations))
	pr_err("Failed to register proc interface\n");
#endif

	if (usb_present) {
		chip->boot_usb_present = true;
	}
	if (!usb_present && chip->vbus_present)
		op_handle_usb_plugin(chip);

	device_init_wakeup(chip->dev, true);
	chip->probe_done = true;
	request_plug_irq(chip);
	requset_vbus_ctrl_gpio(chip);
	pr_info("QPNP SMB2 probed successfully usb:present=%d type=%d batt:present = %d health = %d charge = %d\n",
		usb_present, chip->real_charger_type,
		batt_present, batt_health, batt_charge_type);
	return rc;

cleanup:
	smb2_free_interrupts(chip);
	if (chip->batt_psy)
		power_supply_unregister(chip->batt_psy);
	if (chip->usb_main_psy)
		power_supply_unregister(chip->usb_main_psy);
	if (chip->usb_psy)
		power_supply_unregister(chip->usb_psy);
	if (chip->usb_port_psy)
		power_supply_unregister(chip->usb_port_psy);
	if (chip->dc_psy)
		power_supply_unregister(chip->dc_psy);
	if (chip->vconn_vreg && chip->vconn_vreg->rdev)
		devm_regulator_unregister(chip->dev, chip->vconn_vreg->rdev);
	if (chip->vbus_vreg && chip->vbus_vreg->rdev)
		devm_regulator_unregister(chip->dev, chip->vbus_vreg->rdev);

	smblib_deinit(chip);

	platform_set_drvdata(pdev, NULL);
	return rc;
}

static const struct of_device_id match_table[] = {
	{ .compatible = "qcom,pmi8998-smb2", },
	{ },
};

static struct platform_driver smb2_driver = {
	.driver		= {
		.name		= "qcom,pmi8998-smb2",
		.owner		= THIS_MODULE,
		.of_match_table	= match_table,
	},
	.probe		= smb2_probe,
	.remove		= smb2_remove,
	.shutdown	= smb2_shutdown,
};
module_platform_driver(smb2_driver);

MODULE_DESCRIPTION("Qualcomm PMI8998 QPNP SMB2 Charger Driver");
MODULE_LICENSE("GPL v2");
