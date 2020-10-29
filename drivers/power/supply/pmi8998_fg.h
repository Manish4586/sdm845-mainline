
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

enum wa_flags {
	PMI8998_V1_REV_WA = BIT(0),
	PMI8998_V2_REV_WA = BIT(1),
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
	void (*encode)(struct fg_sram_param sp, int val, u8 *buf);
	int (*decode)(struct fg_sram_param sp, u8* val);

	const char *name;
};

static struct pmi8998_sram_param pmi8998_sram_params_v2[FG_PARAM_MAX] = {
	[FG_DATA_BATT_TEMP] = {
		.address	= 0x50,
		.type		= BATT_BASE_PARAM,
		.length		= 2,
		.numrtr		= 4,
		.denmtr		= 10,		//Kelvin to DeciKelvin
		.val_offset	= -2730,	//DeciKelvin to DeciDegc
		.decode		= fg_decode_temperature
	},
	[FG_DATA_VOLTAGE] = {
		.address	= 0xa0,
		.type		= BATT_BASE_PARAM,
		.length		= 2,
		.numrtr		= 122070,
		.denmtr		= 1000,
		.wa_flags	= PMI8998_V2_REV_WA,
		.decode		= fg_decode_value_16b,
	},
	[FG_DATA_CURRENT] = {
		.address	= 0xa2,
		.length		= 2,
		.type		= BATT_BASE_PARAM,
		.wa_flags	= PMI8998_V2_REV_WA,
		.numrtr		= 1000,
		.denmtr		= 488281,
		.decode 	= fg_decode_current,
	},
};

struct pmi8998_fg_chip {
	struct device *dev;
	struct regmap *regmap;
	struct mutex lock;

	struct power_supply *bms_psy;

	u8 revision[4];
	enum pmic pmic_version;
	bool ima_supported;
	bool reset_on_lockup;

	/* base addresses of components*/
	unsigned int soc_base;
	unsigned int batt_base;
	unsigned int mem_base;

	struct pmi8998_sram_param *sram_params;
	struct battery_info batt_info;

	struct fg_learning_data learning_data;
	struct fg_rslow_data rslow_comp;
	int health;
	int status;
	int vbatt_est_diff;

	//board specific init fn
	int (*init_fn)(struct fg_chip *);
};
