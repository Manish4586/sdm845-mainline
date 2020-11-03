
// #define FG_PARAM_MAX 49

/**** Registers *****/

// pmi8998 v2 specific
#define MEM_INTF_CFG                0x50
#define MEM_INTF_CTL                0x51
#define MEM_INTF_ADDR_LSB           0x61
#define MEM_INTF_RD_DATA0           0x67
#define MEM_INTF_WR_DATA0           0x63

// pm8950 / pm89988 common
#define MEM_INTF_IMA_CFG            0x52
#define MEM_INTF_IMA_OPR_STS        0x54
#define MEM_INTF_IMA_EXP_STS        0x55
#define MEM_INTF_IMA_HW_STS         0x56
#define MEM_INTF_BEAT_COUNT         0x57
#define MEM_INTF_IMA_ERR_STS        0x5f
#define MEM_INTF_IMA_BYTE_EN        0x60

#define BATT_INFO_THERM_C1          0x5c
#define BATT_INFO_VBATT_LSB         0xa0
#define BATT_INFO_VBATT_MSB         0xa1
#define BATT_INFO_IBATT_LSB         0xa2
#define BATT_INFO_IBATT_MSB         0xa3
#define BATT_INFO_BATT_TEMP_LSB     0x50
#define BATT_INFO_BATT_TEMP_MSB     0x51
#define BATT_MONOTONIC_SOC			0x09

#define BATT_TEMP_LSB_MASK			GENMASK(7, 0)
#define BATT_TEMP_MSB_MASK			GENMASK(2, 0)

#define REG_BASE(chip)				(chip->base)
#define REG_BATT(chip)				(chip->base + 0x100)
#define REG_MEM(chip)				(chip->base + 0x400)

/* Interrupt offsets */
#define INT_RT_STS(base)			(base + 0x10)
#define INT_EN_CLR(base)			(base + 0x16)

// Param addresses
#define PARAM_ADDR_BATT_TEMP		0x50
#define PARAM_ADDR_BATT_VOLTAGE		0xa0
#define PARAM_ADDR_BATT_CURRENT		0xa2

enum wa_flags {
	PMI8998_V1_REV_WA,
	PMI8998_V2_REV_WA,
};

enum pmi8998_rev_offsets {
	DIG_MINOR = 0x0,
	DIG_MAJOR = 0x1,
	ANA_MINOR = 0x2,
	ANA_MAJOR = 0x3,
};
enum pmi8998_rev {
	DIG_REV_1 = 0x1,
	DIG_REV_2 = 0x2,
	DIG_REV_3 = 0x3,
};

enum fg_sram_param_id {
	FG_DATA_BATT_TEMP = 0,
	FG_DATA_CHARGE,
	FG_DATA_OCV,
	FG_DATA_VOLTAGE,
	FG_DATA_CURRENT,
	FG_DATA_BATT_ESR,
	FG_DATA_BATT_ESR_COUNT,
	FG_DATA_BATT_ESR_REG,
	FG_DATA_BATT_SOC,
	FG_DATA_FULL_SOC,
	FG_DATA_CYCLE_COUNT,
	FG_DATA_VINT_ERR,
	FG_DATA_CPRED_VOLTAGE,
	FG_DATA_CHARGE_COUNTER,
	FG_DATA_BATT_ID,
	FG_DATA_BATT_ID_INFO,
	FG_DATA_NOM_CAP,
	FG_DATA_ACT_CAP,
	FG_PARAM_ACTUAL_CAP,
	FG_PARAM_ACTUAL_CHARGE,
	FG_PARAM_MAH_TO_SOC_CONV,

	FG_BATT_PROFILE,
	FG_BATT_PROFILE_INTEGRITY,
	FG_SETTING_ADC_CONF,
	FG_SETTING_RSLOW_CHG,
	FG_SETTING_RSLOW_DISCHG,
	FG_SETTING_ALG,
	FG_SETTING_EXTERNAL_SENSE,
	FG_SETTING_THERMAL_COEFFS,

	//Settings below this have corresponding dt entries
	FG_SETTING_TERM_CURRENT,
	FG_SETTING_SYS_TERM_CURRENT,
	FG_SETTING_CHG_TERM_CURRENT,
	FG_SETTING_CHG_TERM_BASE_CURRENT,
	FG_SETTING_CUTOFF_VOLT,
	FG_SETTING_EMPTY_VOLT,
	FG_SETTING_MAX_VOLT,
	FG_SETTING_BATT_LOW,
	FG_SETTING_CONST_CHARGE_VOLT_THR, //previsouly called FG_SETTING_BATT_FULL
	FG_SETTING_RECHARGE_THR,
	FG_SETTING_RECHARGE_VOLT_THR,
	FG_SETTING_DELTA_MSOC,
	FG_SETTING_DELTA_BSOC,
	FG_SETTING_BCL_LM_THR,
	FG_SETTING_BCL_MH_THR,
	FG_SETTING_BATT_COLD_TEMP,
	FG_SETTING_BATT_COOL_TEMP,
	FG_SETTING_BATT_WARM_TEMP,
	FG_SETTING_BATT_HOT_TEMP,
	FG_SETTING_THERM_DELAY,
	FG_PARAM_MAX
};

// enum sram_param_type {
// 	SRAM_PARAM,
// 	MEM_IF_PARAM,
// 	BATT_BASE_PARAM,
// 	SOC_BASE_PARAM,
// };

static enum power_supply_property fg_properties[] = {
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_OCV,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT,
	POWER_SUPPLY_PROP_CYCLE_COUNT,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_STATUS,
};

struct pmi8998_sram_param {
	u16	address;
	u8	offset;
	unsigned int length;
	int value;

	u8 type: 2;
	u8 wa_flags: 6;
	/* the conversion stuff */
	int numrtr;
	int denmtr;
	int val_offset;
	void (*encode)(struct pmi8998_sram_param sp, int val, u8 *buf);
	int (*decode)(struct pmi8998_sram_param sp, u8* val);

	const char *name;
};

struct fg_learning_data {
	struct mutex	learning_lock;
	bool		active;
	int64_t		cc_uah;
	int		learned_cc_uah;
	int		init_cc_pc_val;
	int		max_start_soc;
	int		max_increment;
	int		max_decrement;
	int		vbat_est_thr_uv;
	int		max_cap_limit;
	int		min_cap_limit;
	int		min_temp;
	int		max_temp;
};

struct battery_info {
	const char *manufacturer;
	const char *model;
	const char *serial_num;

	// u8 *batt_profile;
	// unsigned batt_profile_len;

	int nom_cap_uah;

	int batt_max_voltage_uv_design;
	int batt_max_voltage_uv;
};

struct pmi8998_fg_chip {
	struct device *dev;
	unsigned int base;
	struct regmap *regmap;
	struct mutex lock;

	struct power_supply *bms_psy;

	u8 revision[4];
	bool ima_supported;

	struct pmi8998_sram_param *sram_params;
	struct battery_info batt_info;

	struct fg_learning_data learning_data;

	int health;
	int status;
};
