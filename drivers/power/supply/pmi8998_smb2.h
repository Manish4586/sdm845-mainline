#define REG_BASE(chip)              (chip->base)
#define REG_OTG(chip)               (chip->base + 0x1100)
#define REG_BATIF(chip)             (chip->base + 0x1200)
#define REG_USBPTH(chip)            (chip->base + 0x1300) //qcom,usb-chgpth
#define REG_DCPTH(chip)             (chip->base + 0x1400)
#define REG_MISC(chip)              (chip->base + 0x1600)

#define TYPE_C_INTRPT_ENB_SOFTWARE_CTRL_REG	0x68

#define CC_ORIENTATION_BIT			BIT(1)
#define VCONN_EN_VALUE_BIT			BIT(3)
#define VCONN_EN_ORIENTATION_BIT		BIT(6)

static enum power_supply_property smb2_usb_main_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_PD_CURRENT_MAX,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_TYPE,
	POWER_SUPPLY_PROP_TYPEC_MODE,
	POWER_SUPPLY_PROP_TYPEC_POWER_ROLE,
	POWER_SUPPLY_PROP_TYPEC_CC_ORIENTATION,
	POWER_SUPPLY_PROP_CONNECTOR_TYPE,
};

struct pmi8998_smb2_chip {
	struct regmap regmap;
	struct device dev;

	struct 

	int base;
}