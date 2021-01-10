// SPDX-License-Identifier: GPL-2.0
/*
 * FIXME copyright stuffs
 *
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-mediabus.h>
#include <asm/unaligned.h>

#define IMX363_REG_VALUE_08BIT		1
#define IMX363_REG_VALUE_16BIT		2

/* External clock frequency is 24.0M */
#define IMX363_XCLK_FREQ		24000000

/* Half of per-lane speed in Mbps (DDR) */
#define IMX363_DEFAULT_LINK_FREQ	456000000

/* currently only 2-lane operation is supported */
#define IMX363_NUM_LANES		2

/* no clue why this is 10, but it is */
#define IMX363_BITS_PER_SAMPLE		10

/* Pixel rate is fixed at 182.4M for all the modes */
#define IMX363_PIXEL_RATE		(IMX363_DEFAULT_LINK_FREQ * 2 * IMX363_NUM_LANES / IMX363_BITS_PER_SAMPLE)

/* Register map */

#define IMX363_REG_MODE_SELECT				0x0100

#define IMX363_REG_ORIENTATION				0x0101

#define IMX363_REG_EXCK_FREQ_H				0x0136 // integer part
#define IMX363_REG_EXCK_FREQ_L				0x0137 // fractional part
#define IMX363_REG_CSI_DT_FMT_H				0x0112
#define IMX363_REG_CSI_DT_FMT_L				0x0113
#define IMX363_REG_CSI_LANE_MODE			0x0114

#define IMX363_REG_HDR_MODE				0x0220
#define IMX363_REG_HDR_RESO_REDU_HHDR_RESO_REDU_V	0x0221

#define IMX363_REG_FRM_LENGTH_LINES			0x0340 // 16 bits
#define IMX363_REG_LINE_LENGTH_PCK			0x0342 // 16 bits

#define IMX363_REG_X_EVN_INC				0x0381
#define IMX363_REG_X_ODD_INC				0x0383
#define IMX363_REG_Y_EVN_INC				0x0385
#define IMX363_REG_Y_ODD_INC				0x0387

#define IMX363_REG_BINNING_MODE				0x0900
#define IMX363_REG_BINNING_TYPE				0x0901

#define IMX363_REG_X_ADDR_START				0x0344 // 16 bits
#define IMX363_REG_Y_ADDR_START				0x0346 // 16 bits
#define IMX363_REG_X_ADDR_END				0x0348 // 16 bits
#define IMX363_REG_Y_ADDR_END				0x034A // 16 bits
#define IMX363_REG_X_OUT_SIZE				0x034C // 16 bits
#define IMX363_REG_Y_OUT_SIZE				0x034E // 16 bits

#define IMX363_REG_DIG_CROP_X_OFFSET			0x0408 // 16 bits
#define IMX363_REG_DIG_CROP_Y_OFFSET			0x040A // 16 bits
#define IMX363_REG_DIG_CROP_IMAGE_WIDTH			0x040C // 16 bits
#define IMX363_REG_DIG_CROP_IMAGE_HEIGHT		0x040E // 16 bits

#define IMX363_REG_IVTPXCK_DIV				0x0301
#define IMX363_REG_IVTSYCK_DIV				0x0303
#define IMX363_REG_PREPLLCK_IVT_DIV			0x0305
#define IMX363_REG_PLL_IVT_MPY				0x0306 // 16 bits
#define IMX363_REG_IOPPXCK_DIV				0x0309
#define IMX363_REG_IOPSYCK_DIV				0x030B
#define IMX363_REG_PREPLLCK_IOP_DIV			0x030D
#define IMX363_REG_PLL_IOP_MPY				0x030E // 16 bits
#define IMX363_REG_PLL_MULT_DRIV			0x0310

#define IMX363_REG_COARSE_INTEG_TIME			0x0202 // 16 bits
#define IMX363_REG_ST_COARSE_INTEG_TIME			0x0224 // 16 bits
#define IMX363_REG_ANA_GAIN_GLOBAL			0x0204 // 16 bits
#define IMX363_REG_ST_ANA_GAIN_GLOBAL			0x0216 // 16 bits
#define IMX363_REG_DIG_GAIN_GLOBAL			0x020E // 16 bits
#define IMX363_REG_ST_DIG_GAIN_GLOBAL			0x0226 // 16 bits
	
#define IMX363_REG_DPHY_CTRL				0x0808

#define IMX363_REG_DUAL_PD_OUT_MODE			0x31A0
#define IMX363_REG_PREFER_DIG_BIN_H			0x31A6

/* register values */

/* DPHY control */
#define IMX363_DPHY_CTRL_AUTO		0x00
#define IMX363_DPHY_CTRL_MANUAL		0x01

/* Configuration happens in standby mode */
#define IMX363_MODE_STANDBY		0x00
#define IMX363_MODE_STREAMING		0x01

/* Chip ID */
/*???????? TODO*/ 
#define IMX363_CHIP_ID			0x0363

/* Number of CSI lines connected */
#define IMX363_CSI_LANE_NUM_2		0x1
#define IMX363_CSI_LANE_NUM_4		0x3

#define IMX363_VBLANK_MIN		4

/* HBLANK control - read only */
#define IMX363_PPL_DEFAULT		3448

/* Analog gain control */

#define IMX363_ANA_GAIN_MIN		0
#define IMX363_ANA_GAIN_MAX		448
#define IMX363_ANA_GAIN_STEP		1
#define IMX363_ANA_GAIN_DEFAULT		0x0


/* Exposure control */
#define IMX363_EXPOSURE_MIN		4
#define IMX363_EXPOSURE_STEP		1
#define IMX363_EXPOSURE_DEFAULT		0x640
#define IMX363_EXPOSURE_MAX		65535

/* Digital gain control */ // FIXME

#define IMX363_DGTL_GAIN_MIN		0x0100
#define IMX363_DGTL_GAIN_MAX		0x0fff
#define IMX363_DGTL_GAIN_DEFAULT	0x0100
#define IMX363_DGTL_GAIN_STEP		1

/* Binning types */

#define IMX363_BINNING_NONE		0x00
#define IMX363_BINNING_V2H2		0x22
#define IMX363_BINNING_V2H4		0x42

/* Data format to use for transmission */

#define IMX363_CSI_DATA_FORMAT_RAW10	0x0a0a

/* phase data */
#define DUAL_PD_OUT_DISABLE 0x02

/* IMX363 native and active pixel array size. */
#define IMX363_NATIVE_WIDTH		3296U
#define IMX363_NATIVE_HEIGHT		2480U
#define IMX363_PIXEL_ARRAY_LEFT		8U
#define IMX363_PIXEL_ARRAY_TOP		8U
#define IMX363_PIXEL_ARRAY_WIDTH	3280U
#define IMX363_PIXEL_ARRAY_HEIGHT	2464U

/* Calculate start and end address for simple centered crop */

#define IMX363_READOUT_CROP_CENTER_START(readout_len, total_len)	((total_len - readout_len)/2)
#define IMX363_READOUT_CROP_CENTER_END(readout_len, total_len)		((total_len - 1) - (total_len - readout_len)/2)

#define IMX363_READOUT_CROP_CENTER_LEFT(readout_width)		IMX363_READOUT_CROP_CENTER_START(readout_width, IMX363_PIXEL_ARRAY_WIDTH)
#define IMX363_READOUT_CROP_CENTER_RIGHT(readout_width)		IMX363_READOUT_CROP_CENTER_END(readout_width, IMX363_PIXEL_ARRAY_WIDTH)
#define IMX363_READOUT_CROP_CENTER_TOP(readout_height)		IMX363_READOUT_CROP_CENTER_START(readout_height, IMX363_PIXEL_ARRAY_HEIGHT)
#define IMX363_READOUT_CROP_CENTER_BOTTOM(readout_height)	IMX363_READOUT_CROP_CENTER_END(readout_height, IMX363_PIXEL_ARRAY_HEIGHT)

/* A macro to generate a register list for changing the mode */

#define IMX363_MODE_REGS_GENERATE(readout_width, readout_height, pic_width, pic_height, binning)		\
	{IMX363_REG_X_ADDR_START, IMX363_REG_VALUE_16BIT, IMX363_READOUT_CROP_CENTER_LEFT(readout_width)},	\
	{IMX363_REG_X_ADDR_END, IMX363_REG_VALUE_16BIT, IMX363_READOUT_CROP_CENTER_RIGHT(readout_width)},	\
	{IMX363_REG_Y_ADDR_START, IMX363_REG_VALUE_16BIT, IMX363_READOUT_CROP_CENTER_TOP(readout_height)},	\
	{IMX363_REG_Y_ADDR_END, IMX363_REG_VALUE_16BIT, IMX363_READOUT_CROP_CENTER_BOTTOM(readout_height)},	\
	{IMX363_REG_X_OUT_SIZE, IMX363_REG_VALUE_16BIT, pic_width},						\
	{IMX363_REG_Y_OUT_SIZE, IMX363_REG_VALUE_16BIT, pic_height},						\
														\
	{IMX363_REG_DIG_CROP_X_OFFSET, IMX363_REG_VALUE_16BIT, 0},						\
	{IMX363_REG_DIG_CROP_Y_OFFSET, IMX363_REG_VALUE_16BIT, 0},						\
	{IMX363_REG_DIG_CROP_IMAGE_WIDTH, IMX363_REG_VALUE_16BIT, pic_width},					\
	{IMX363_REG_DIG_CROP_IMAGE_HEIGHT, IMX363_REG_VALUE_16BIT, pic_height},					\
														\
	{IMX363_REG_BINNING_TYPE, IMX363_REG_VALUE_08BIT, binning},						\

//	{IMX363_REG_TP_WINDOW_WIDTH_HIG, IMX363_REG_VALUE_16BIT, pic_width},					\
//	{IMX363_REG_TP_WINDOW_HEIGHT_HIG, IMX363_REG_VALUE_16BIT, pic_height},

struct imx363_reg {
	u16 address;
	u8 val_len;
	u16 val;
};

struct imx363_reg_list {
	unsigned int num_of_regs;
	const struct imx363_reg *regs;
};

/* Mode : resolution and related config&values */
struct imx363_mode {
	/* Frame width */
	unsigned int width;
	/* Frame height */
	unsigned int height;

	/* Analog crop rectangle. */
	struct v4l2_rect crop;

	/* V-timing */
	unsigned int vts_def;

	/* Default register values */
	struct imx363_reg_list reg_list;
};

static const struct imx363_reg setup_regs[] = {
	{IMX363_REG_MODE_SELECT, IMX363_REG_VALUE_08BIT, IMX363_MODE_STANDBY}, // standby

	{IMX363_REG_CSI_LANE_MODE, IMX363_REG_VALUE_08BIT, IMX363_CSI_LANE_NUM_4},

	{IMX363_REG_DPHY_CTRL, IMX363_REG_VALUE_08BIT, IMX363_DPHY_CTRL_AUTO},

	{IMX363_REG_EXCK_FREQ_H, IMX363_REG_VALUE_16BIT, 0x1800}, // 24.00 Mhz = 18.00h

//	{IMX363_REG_LINE_LENGTH_PCK, IMX363_REG_VALUE_16BIT, 0x2200},

//	{IMX363_REG_X_ODD_INC, IMX363_REG_VALUE_08BIT, 0x01},
//	{IMX363_REG_Y_ODD_INC, IMX363_REG_VALUE_08BIT, 0x01},

	// {IMX363_REG_HDR_MODE, IMX363_REG_VALUE_8BIT, 0x00}
	// {IMX363_REG_DUAL_PD_OUT_MODE, IMX363_REG_VALUE_8BIT, DUAL_PD_OUT_DISABLE}

	// {IMX363_REG_IVTPXCK_DIV, IMX363_REG_VALUE_8BIT, 3}
	// {IMX363_REG_IVTSYCK_DIV, IMX363_REG_VALUE_8BIT, 2}
	{IMX363_REG_PREPLLCK_IVT_DIV, IMX363_REG_VALUE_08BIT, 4},
	{IMX363_REG_PLL_IVT_MPY, IMX363_REG_VALUE_16BIT, 210},
	{IMX363_REG_IOPPXCK_DIV, IMX363_REG_VALUE_08BIT, 10},
	{IMX363_REG_IOPSYCK_DIV, IMX363_REG_VALUE_08BIT, 1},
	{IMX363_REG_PREPLLCK_IOP_DIV, IMX363_REG_VALUE_08BIT, 4},
	{IMX363_REG_PLL_IOP_MPY, IMX363_REG_VALUE_16BIT, 200},
	{IMX363_REG_PLL_MULT_DRIV, IMX363_REG_VALUE_08BIT, 1}, //true

	// magic default value updates - TODO: is this needed?
	{0x31A3, IMX363_REG_VALUE_08BIT, 0x00},
	{0x64D4, IMX363_REG_VALUE_08BIT, 0x01},
	{0x64D5, IMX363_REG_VALUE_08BIT, 0xAA},
	{0x64D6, IMX363_REG_VALUE_08BIT, 0x01},
	{0x64D7, IMX363_REG_VALUE_08BIT, 0xA9},
	{0x64D8, IMX363_REG_VALUE_08BIT, 0x01},
	{0x64D9, IMX363_REG_VALUE_08BIT, 0xA5},
	{0x64DA, IMX363_REG_VALUE_08BIT, 0x01},
	{0x64DB, IMX363_REG_VALUE_08BIT, 0xA1},
	{0x720A, IMX363_REG_VALUE_08BIT, 0x24},
	{0x720B, IMX363_REG_VALUE_08BIT, 0x89},
	{0x720C, IMX363_REG_VALUE_08BIT, 0x85},
	{0x720D, IMX363_REG_VALUE_08BIT, 0xA1},
	{0x720E, IMX363_REG_VALUE_08BIT, 0x6E},
	{0x729C, IMX363_REG_VALUE_08BIT, 0x59},
	{0x817C, IMX363_REG_VALUE_08BIT, 0xFF},
	{0x817D, IMX363_REG_VALUE_08BIT, 0x80},
	{0x9348, IMX363_REG_VALUE_08BIT, 0x96},
	{0x934B, IMX363_REG_VALUE_08BIT, 0x8C},
	{0x934C, IMX363_REG_VALUE_08BIT, 0x82},
	{0x9353, IMX363_REG_VALUE_08BIT, 0xAA},
	{0x9354, IMX363_REG_VALUE_08BIT, 0xAA},
};

static const struct imx363_reg_list setup_reg_list = {
	.num_of_regs = ARRAY_SIZE(setup_regs),
	.regs = setup_regs,
};

static const struct imx363_reg mode_4032x3024_regs[] = {
	IMX363_MODE_REGS_GENERATE(4032, 3024, 4032, 3024, IMX363_BINNING_NONE)
};

static const struct imx363_reg raw10_framefmt_regs[] = {
	// {IMX363_REG_CSI_DATA_FORMAT_A_HIG, IMX363_REG_VALUE_16BIT, IMX363_CSI_DATA_FORMAT_RAW10},
	// {IMX363_REG_OPPXCK_DIV, IMX363_REG_VALUE_08BIT, 0x0a},
};

/* regulator supplies */
static const char * const imx363_supply_name[] = {
	/* Supplies can be enabled in any order */
	"VANA",  /* Analog (2.8V) supply */
	"VDIG",  /* Digital Core (1.8V) supply */
	"VDDL",  /* IF (1.2V) supply */
};

#define IMX363_NUM_SUPPLIES ARRAY_SIZE(imx363_supply_name)

/*
 * The supported formats.
 * This table MUST contain 4 entries per format, to cover the various flip
 * combinations in the order
 * - no flip
 * - h flip
 * - v flip
 * - h&v flips
 */
static const u32 codes[] = {
	MEDIA_BUS_FMT_SRGGB10_1X10,
	MEDIA_BUS_FMT_SGRBG10_1X10,
	MEDIA_BUS_FMT_SGBRG10_1X10,
	MEDIA_BUS_FMT_SBGGR10_1X10,

	MEDIA_BUS_FMT_SRGGB8_1X8,
	MEDIA_BUS_FMT_SGRBG8_1X8,
	MEDIA_BUS_FMT_SGBRG8_1X8,
	MEDIA_BUS_FMT_SBGGR8_1X8,
};

/*
 * Initialisation delay between XCLR low->high and the moment when the sensor
 * can start capture (i.e. can leave software stanby) must be not less than:
 *   t4 + max(t5, t6 + <time to initialize the sensor register over I2C>)
 * where
 *   t4 is fixed, and is max 200uS,
 *   t5 is fixed, and is 6000uS,
 *   t6 depends on the sensor external clock, and is max 32000 clock periods.
 * As per sensor datasheet, the external clock must be from 6MHz to 27MHz.
 * So for any acceptable external clock t6 is always within the range of
 * 1185 to 5333 uS, and is always less than t5.
 * For this reason this is always safe to wait (t4 + t5) = 6200 uS, then
 * initialize the sensor over I2C, and then exit the software standby.
 *
 * This start-up time can be optimized a bit more, if we start the writes
 * over I2C after (t4+t6), but before (t4+t5) expires. But then sensor
 * initialization over I2C may complete before (t4+t5) expires, and we must
 * ensure that capture is not started before (t4+t5).
 *
 * This delay doesn't account for the power supply startup time. If needed,
 * this should be taken care of via the regulator framework. E.g. in the
 * case of DT for regulator-fixed one should define the startup-delay-us
 * property.
 */
#define IMX363_XCLR_MIN_DELAY_US	6200
#define IMX363_XCLR_DELAY_RANGE_US	1000

/* Mode configs */
static const struct imx363_mode supported_modes[] = {
	{
		/* 8MPix 15fps mode */
		.width = 4032,
		.height = 3024,
		.crop = {
			.left = 0,
			.top = 0,
			.width = 4032,
			.height = 3024
		},
		// .vts_def = IMX363_VTS_15FPS,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mode_4032x3024_regs),
			.regs = mode_4032x3024_regs,
		},
	},
};

struct imx363 {
	struct v4l2_subdev sd;
	struct media_pad pad;

	struct v4l2_mbus_framefmt fmt;

	struct clk *xclk; /* system clock to IMX363 */
	u32 xclk_freq;

	struct gpio_desc *reset_gpio;
	struct regulator_bulk_data supplies[IMX363_NUM_SUPPLIES];

	struct v4l2_ctrl_handler ctrl_handler;
	/* V4L2 Controls */
	struct v4l2_ctrl *pixel_rate;
	struct v4l2_ctrl *exposure;
	struct v4l2_ctrl *vflip;
	struct v4l2_ctrl *hflip;
	struct v4l2_ctrl *vblank;
	struct v4l2_ctrl *hblank;

	/* Current mode */
	const struct imx363_mode *mode;

	/*
	 * Mutex for serialized access:
	 * Protect sensor module set pad format and start/stop streaming safely.
	 */
	struct mutex mutex;

	/* Streaming on/off */
	bool streaming;
};

static inline struct imx363 *to_imx363(struct v4l2_subdev *_sd)
{
	return container_of(_sd, struct imx363, sd);
}

/* Read registers up to 2 at a time */
// static int imx363_read_reg(struct imx363 *imx363, u16 reg, u32 len, u32 *val)
// {
// 	struct i2c_client *client = v4l2_get_subdevdata(&imx363->sd);
// 	struct i2c_msg msgs[2];
// 	u8 addr_buf[2] = { reg >> 8, reg & 0xff };
// 	u8 data_buf[4] = { 0, };
// 	int ret;

// 	if (len > 4)
// 		return -EINVAL;

// 	/* Write register address */
// 	msgs[0].addr = client->addr;
// 	msgs[0].flags = 0;
// 	msgs[0].len = ARRAY_SIZE(addr_buf);
// 	msgs[0].buf = addr_buf;

// 	/* Read data from register */
// 	msgs[1].addr = client->addr;
// 	msgs[1].flags = I2C_M_RD;
// 	msgs[1].len = len;
// 	msgs[1].buf = &data_buf[4 - len];

// 	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
// 	if (ret != ARRAY_SIZE(msgs))
// 		return -EIO;

// 	*val = get_unaligned_be32(data_buf);

// 	return 0;
// }

// /* Write registers up to 2 at a time */
// static int imx363_write_reg(struct imx363 *imx363, u16 reg, u32 len, u32 val)
// {
// 	struct i2c_client *client = v4l2_get_subdevdata(&imx363->sd);
// 	u8 buf[6];

// 	if (len > 4)
// 		return -EINVAL;

// 	put_unaligned_be16(reg, buf);
// 	put_unaligned_be32(val << (8 * (4 - len)), buf + 2);
// 	if (i2c_master_send(client, buf, len + 2) != len + 2)
// 		return -EIO;

// 	return 0;
// }

// /* Write a list of registers */
// static int imx363_write_regs(struct imx363 *imx363,
// 			     const struct imx363_reg *regs, u32 len)
// {
// 	struct i2c_client *client = v4l2_get_subdevdata(&imx363->sd);
// 	unsigned int i;
// 	int ret;

// 	for (i = 0; i < len; i++) {
// 		ret = imx363_write_reg(imx363, regs[i].address, regs[i].val_len, regs[i].val);
// 		if (ret) {
// 			dev_err_ratelimited(&client->dev,
// 					    "Failed to write reg 0x%4.4x. error = %d\n",
// 					    regs[i].address, ret);

// 			return ret;
// 		}
// 	}

// 	return 0;
// }

// /* Get bayer order based on flip setting. */
// static u32 imx363_get_format_code(struct imx363 *imx363, u32 code)
// {
// 	unsigned int i;

// 	lockdep_assert_held(&imx363->mutex);

// 	for (i = 0; i < ARRAY_SIZE(codes); i++)
// 		if (codes[i] == code)
// 			break;

// 	if (i >= ARRAY_SIZE(codes))
// 		i = 0;

// 	i = (i & ~3) | (imx363->vflip->val ? 2 : 0) |
// 	    (imx363->hflip->val ? 1 : 0);

// 	return codes[i];
// }

// static void imx363_set_default_format(struct imx363 *imx363)
// {
// 	struct v4l2_mbus_framefmt *fmt;

// 	fmt = &imx363->fmt;
// 	fmt->code = MEDIA_BUS_FMT_SRGGB10_1X10;
// 	fmt->colorspace = V4L2_COLORSPACE_SRGB;
// 	fmt->ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(fmt->colorspace);
// 	fmt->quantization = V4L2_MAP_QUANTIZATION_DEFAULT(true,
// 							  fmt->colorspace,
// 							  fmt->ycbcr_enc);
// 	fmt->xfer_func = V4L2_MAP_XFER_FUNC_DEFAULT(fmt->colorspace);
// 	fmt->width = supported_modes[0].width;
// 	fmt->height = supported_modes[0].height;
// 	fmt->field = V4L2_FIELD_NONE;
// }

// static int imx363_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
// {
// 	struct imx363 *imx363 = to_imx363(sd);
// 	struct v4l2_mbus_framefmt *try_fmt =
// 		v4l2_subdev_get_try_format(sd, fh->pad, 0);
// 	struct v4l2_rect *try_crop;

// 	mutex_lock(&imx363->mutex);

// 	/* Initialize try_fmt */
// 	try_fmt->width = supported_modes[0].width;
// 	try_fmt->height = supported_modes[0].height;
// 	try_fmt->code = imx363_get_format_code(imx363,
// 					       MEDIA_BUS_FMT_SRGGB10_1X10);
// 	try_fmt->field = V4L2_FIELD_NONE;

// 	/* Initialize try_crop rectangle. */
// 	try_crop = v4l2_subdev_get_try_crop(sd, fh->pad, 0);
// 	try_crop->top = IMX363_PIXEL_ARRAY_TOP;
// 	try_crop->left = IMX363_PIXEL_ARRAY_LEFT;
// 	try_crop->width = IMX363_PIXEL_ARRAY_WIDTH;
// 	try_crop->height = IMX363_PIXEL_ARRAY_HEIGHT;

// 	mutex_unlock(&imx363->mutex);

// 	return 0;
// }

// static int imx363_set_ctrl(struct v4l2_ctrl *ctrl)
// {
// 	struct imx363 *imx363 =
// 		container_of(ctrl->handler, struct imx363, ctrl_handler);
// 	struct i2c_client *client = v4l2_get_subdevdata(&imx363->sd);
// 	int ret;

// 	if (ctrl->id == V4L2_CID_VBLANK) {
// 		int exposure_max, exposure_def;

// 		/* Update max exposure while meeting expected vblanking */
// 		exposure_max = imx363->mode->height + ctrl->val - 4;
// 		exposure_def = (exposure_max < IMX363_EXPOSURE_DEFAULT) ?
// 			exposure_max : IMX363_EXPOSURE_DEFAULT;
// 		__v4l2_ctrl_modify_range(imx363->exposure,
// 					 imx363->exposure->minimum,
// 					 exposure_max, imx363->exposure->step,
// 					 exposure_def);
// 	}

// 	/*
// 	 * Applying V4L2 control value only happens
// 	 * when power is up for streaming
// 	 */
// 	if (pm_runtime_get_if_in_use(&client->dev) == 0)
// 		return 0;

// 	switch (ctrl->id) {
// 	case V4L2_CID_ANALOGUE_GAIN:
// 		ret = imx363_write_reg(imx363, IMX363_REG_ANA_GAIN_GLOBAL,
// 				       IMX363_REG_VALUE_08BIT, ctrl->val);
// 		break;
// 	case V4L2_CID_EXPOSURE:
// 		ret = imx363_write_reg(imx363, IMX363_REG_COARSE_INTEG_TIME,
// 				       IMX363_REG_VALUE_16BIT, ctrl->val);
// 		break;
// 	case V4L2_CID_DIGITAL_GAIN:
// 		ret = imx363_write_reg(imx363, IMX363_REG_DIG_GAIN_GLOBAL,
// 				       IMX363_REG_VALUE_16BIT, ctrl->val);
// 		break;
// 	case V4L2_CID_HFLIP:
// 	case V4L2_CID_VFLIP:
// 		ret = imx363_write_reg(imx363, IMX363_REG_ORIENTATION, 1,
// 				       imx363->hflip->val |
// 				       imx363->vflip->val << 1);
// 		break;
// 	case V4L2_CID_VBLANK:
// 		ret = imx363_write_reg(imx363, IMX363_REG_FRM_LENGTH_LINES,
// 				       IMX363_REG_VALUE_16BIT,
// 				       imx363->mode->height + ctrl->val);
// 		break;
// 	default:
// 		dev_info(&client->dev,
// 			 "ctrl(id:0x%x,val:0x%x) is not handled\n",
// 			 ctrl->id, ctrl->val);
// 		ret = -EINVAL;
// 		break;
// 	}

// 	pm_runtime_put(&client->dev);

// 	return ret;
// }

// static const struct v4l2_ctrl_ops imx363_ctrl_ops = {
// 	.s_ctrl = imx363_set_ctrl,
// };

// static int imx363_enum_mbus_code(struct v4l2_subdev *sd,
// 				 struct v4l2_subdev_pad_config *cfg,
// 				 struct v4l2_subdev_mbus_code_enum *code)
// {
// 	struct imx363 *imx363 = to_imx363(sd);

// 	if (code->index >= (ARRAY_SIZE(codes) / 4))
// 		return -EINVAL;

// 	code->code = imx363_get_format_code(imx363, codes[code->index * 4]);

// 	return 0;
// }

// static int imx363_enum_frame_size(struct v4l2_subdev *sd,
// 				  struct v4l2_subdev_pad_config *cfg,
// 				  struct v4l2_subdev_frame_size_enum *fse)
// {
// 	struct imx363 *imx363 = to_imx363(sd);

// 	if (fse->index >= ARRAY_SIZE(supported_modes))
// 		return -EINVAL;

// 	if (fse->code != imx363_get_format_code(imx363, fse->code))
// 		return -EINVAL;

// 	fse->min_width = supported_modes[fse->index].width;
// 	fse->max_width = fse->min_width;
// 	fse->min_height = supported_modes[fse->index].height;
// 	fse->max_height = fse->min_height;

// 	return 0;
// }

// static void imx363_reset_colorspace(struct v4l2_mbus_framefmt *fmt)
// {
// 	fmt->colorspace = V4L2_COLORSPACE_SRGB;
// 	fmt->ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(fmt->colorspace);
// 	fmt->quantization = V4L2_MAP_QUANTIZATION_DEFAULT(true,
// 							  fmt->colorspace,
// 							  fmt->ycbcr_enc);
// 	fmt->xfer_func = V4L2_MAP_XFER_FUNC_DEFAULT(fmt->colorspace);
// }

// static void imx363_update_pad_format(struct imx363 *imx363,
// 				     const struct imx363_mode *mode,
// 				     struct v4l2_subdev_format *fmt)
// {
// 	fmt->format.width = mode->width;
// 	fmt->format.height = mode->height;
// 	fmt->format.field = V4L2_FIELD_NONE;
// 	imx363_reset_colorspace(&fmt->format);
// }

// static int __imx363_get_pad_format(struct imx363 *imx363,
// 				   struct v4l2_subdev_pad_config *cfg,
// 				   struct v4l2_subdev_format *fmt)
// {
// 	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
// 		struct v4l2_mbus_framefmt *try_fmt =
// 			v4l2_subdev_get_try_format(&imx363->sd, cfg, fmt->pad);
// 		/* update the code which could change due to vflip or hflip: */
// 		try_fmt->code = imx363_get_format_code(imx363, try_fmt->code);
// 		fmt->format = *try_fmt;
// 	} else {
// 		imx363_update_pad_format(imx363, imx363->mode, fmt);
// 		fmt->format.code = imx363_get_format_code(imx363,
// 							  imx363->fmt.code);
// 	}

// 	return 0;
// }

// static int imx363_get_pad_format(struct v4l2_subdev *sd,
// 				 struct v4l2_subdev_pad_config *cfg,
// 				 struct v4l2_subdev_format *fmt)
// {
// 	struct imx363 *imx363 = to_imx363(sd);
// 	int ret;

// 	mutex_lock(&imx363->mutex);
// 	ret = __imx363_get_pad_format(imx363, cfg, fmt);
// 	mutex_unlock(&imx363->mutex);

// 	return ret;
// }

// static int imx363_set_pad_format(struct v4l2_subdev *sd,
// 				 struct v4l2_subdev_pad_config *cfg,
// 				 struct v4l2_subdev_format *fmt)
// {
// 	struct imx363 *imx363 = to_imx363(sd);
// 	const struct imx363_mode *mode;
// 	struct v4l2_mbus_framefmt *framefmt;
// 	int exposure_max, exposure_def, hblank;
// 	unsigned int i;

// 	mutex_lock(&imx363->mutex);

// 	for (i = 0; i < ARRAY_SIZE(codes); i++)
// 		if (codes[i] == fmt->format.code)
// 			break;
// 	if (i >= ARRAY_SIZE(codes))
// 		i = 0;

// 	/* Bayer order varies with flips */
// 	fmt->format.code = imx363_get_format_code(imx363, codes[i]);

// 	mode = v4l2_find_nearest_size(supported_modes,
// 				      ARRAY_SIZE(supported_modes),
// 				      width, height,
// 				      fmt->format.width, fmt->format.height);
// 	imx363_update_pad_format(imx363, mode, fmt);
// 	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
// 		framefmt = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
// 		*framefmt = fmt->format;
// 	} else if (imx363->mode != mode ||
// 		   imx363->fmt.code != fmt->format.code) {
// 		imx363->fmt = fmt->format;
// 		imx363->mode = mode;
// 		/* Update limits and set FPS to default */
// 		// __v4l2_ctrl_modify_range(imx363->vblank, IMX363_VBLANK_MIN,
// 		// 			 IMX363_VTS_MAX - mode->height, 1,
// 		// 			 mode->vts_def - mode->height);
// 		__v4l2_ctrl_s_ctrl(imx363->vblank,
// 				   mode->vts_def - mode->height);
// 		/* Update max exposure while meeting expected vblanking */
// 		exposure_max = mode->vts_def - 4;
// 		exposure_def = (exposure_max < IMX363_EXPOSURE_DEFAULT) ?
// 			exposure_max : IMX363_EXPOSURE_DEFAULT;
// 		__v4l2_ctrl_modify_range(imx363->exposure,
// 					 imx363->exposure->minimum,
// 					 exposure_max, imx363->exposure->step,
// 					 exposure_def);
// 		/*
// 		 * Currently PPL is fixed to IMX363_PPL_DEFAULT, so hblank
// 		 * depends on mode->width only, and is not changeble in any
// 		 * way other than changing the mode.
// 		 */
// 		hblank = IMX363_PPL_DEFAULT - mode->width;
// 		__v4l2_ctrl_modify_range(imx363->hblank, hblank, hblank, 1,
// 					 hblank);
// 	}

// 	mutex_unlock(&imx363->mutex);

// 	return 0;
// }

// static int imx363_set_framefmt(struct imx363 *imx363)
// {
// 	switch (imx363->fmt.code) {
// 	case MEDIA_BUS_FMT_SRGGB10_1X10:
// 	case MEDIA_BUS_FMT_SGRBG10_1X10:
// 	case MEDIA_BUS_FMT_SGBRG10_1X10:
// 	case MEDIA_BUS_FMT_SBGGR10_1X10:
// 		return imx363_write_regs(imx363, raw10_framefmt_regs,
// 					ARRAY_SIZE(raw10_framefmt_regs));
// 	}

// 	return -EINVAL;
// }

// static const struct v4l2_rect *
// __imx363_get_pad_crop(struct imx363 *imx363, struct v4l2_subdev_pad_config *cfg,
// 		      unsigned int pad, enum v4l2_subdev_format_whence which)
// {
// 	switch (which) {
// 	case V4L2_SUBDEV_FORMAT_TRY:
// 		return v4l2_subdev_get_try_crop(&imx363->sd, cfg, pad);
// 	case V4L2_SUBDEV_FORMAT_ACTIVE:
// 		return &imx363->mode->crop;
// 	}

// 	return NULL;
// }

// static int imx363_get_selection(struct v4l2_subdev *sd,
// 				struct v4l2_subdev_pad_config *cfg,
// 				struct v4l2_subdev_selection *sel)
// {
// 	switch (sel->target) {
// 	case V4L2_SEL_TGT_CROP: {
// 		struct imx363 *imx363 = to_imx363(sd);

// 		mutex_lock(&imx363->mutex);
// 		sel->r = *__imx363_get_pad_crop(imx363, cfg, sel->pad,
// 						sel->which);
// 		mutex_unlock(&imx363->mutex);

// 		return 0;
// 	}

// 	case V4L2_SEL_TGT_NATIVE_SIZE:
// 		sel->r.top = 0;
// 		sel->r.left = 0;
// 		sel->r.width = IMX363_NATIVE_WIDTH;
// 		sel->r.height = IMX363_NATIVE_HEIGHT;

// 		return 0;

// 	case V4L2_SEL_TGT_CROP_DEFAULT:
// 		sel->r.top = IMX363_PIXEL_ARRAY_TOP;
// 		sel->r.left = IMX363_PIXEL_ARRAY_LEFT;
// 		sel->r.width = IMX363_PIXEL_ARRAY_WIDTH;
// 		sel->r.height = IMX363_PIXEL_ARRAY_HEIGHT;

// 		return 0;
// 	}

// 	return -EINVAL;
// }

// static int imx363_start_streaming(struct imx363 *imx363)
// {
// 	struct i2c_client *client = v4l2_get_subdevdata(&imx363->sd);
// 	const struct imx363_reg_list *reg_list;
// 	int ret;

// 	/* Apply default configuration */
// 	reg_list = &setup_reg_list;
// 	ret = imx363_write_regs(imx363, reg_list->regs, reg_list->num_of_regs);
// 	if (ret) {
// 		dev_err(&client->dev, "%s failed to set mode\n", __func__);
// 		return ret;
// 	}

// 	/* Set up registers according to current mode */
// 	reg_list = &imx363->mode->reg_list;
// 	ret = imx363_write_regs(imx363, reg_list->regs, reg_list->num_of_regs);
// 	if (ret) {
// 		dev_err(&client->dev, "%s failed to set mode\n", __func__);
// 		return ret;
// 	}

// 	ret = imx363_set_framefmt(imx363);
// 	if (ret) {
// 		dev_err(&client->dev, "%s failed to set frame format: %d\n",
// 			__func__, ret);
// 		return ret;
// 	}

// 	/* Apply customized values from user */
// 	ret =  __v4l2_ctrl_handler_setup(imx363->sd.ctrl_handler);
// 	if (ret)
// 		return ret;

// 	/* set stream on register */
// 	return imx363_write_reg(imx363, IMX363_REG_MODE_SELECT,
// 				IMX363_REG_VALUE_08BIT, IMX363_MODE_STREAMING);
// }

// static void imx363_stop_streaming(struct imx363 *imx363)
// {
// 	struct i2c_client *client = v4l2_get_subdevdata(&imx363->sd);
// 	int ret;

// 	/* set stream off register */
// 	ret = imx363_write_reg(imx363, IMX363_REG_MODE_SELECT,
// 			       IMX363_REG_VALUE_08BIT, IMX363_MODE_STANDBY);
// 	if (ret)
// 		dev_err(&client->dev, "%s failed to set stream\n", __func__);
// }

// static int imx363_set_stream(struct v4l2_subdev *sd, int enable)
// {
// 	struct imx363 *imx363 = to_imx363(sd);
// 	struct i2c_client *client = v4l2_get_subdevdata(sd);
// 	int ret = 0;

// 	mutex_lock(&imx363->mutex);
// 	if (imx363->streaming == enable) {
// 		mutex_unlock(&imx363->mutex);
// 		return 0;
// 	}

// 	if (enable) {
// 		ret = pm_runtime_get_sync(&client->dev);
// 		if (ret < 0) {
// 			pm_runtime_put_noidle(&client->dev);
// 			goto err_unlock;
// 		}

// 		/*
// 		 * Apply default & customized values
// 		 * and then start streaming.
// 		 */
// 		ret = imx363_start_streaming(imx363);
// 		if (ret)
// 			goto err_rpm_put;
// 	} else {
// 		imx363_stop_streaming(imx363);
// 		pm_runtime_put(&client->dev);
// 	}

// 	imx363->streaming = enable;

// 	/* vflip and hflip cannot change during streaming */
// 	__v4l2_ctrl_grab(imx363->vflip, enable);
// 	__v4l2_ctrl_grab(imx363->hflip, enable);

// 	mutex_unlock(&imx363->mutex);

// 	return ret;

// err_rpm_put:
// 	pm_runtime_put(&client->dev);
// err_unlock:
// 	mutex_unlock(&imx363->mutex);

// 	return ret;
// }

/* Power/clock management functions */
// static int imx363_power_on(struct device *dev)
// {
// 	struct i2c_client *client = to_i2c_client(dev);
// 	struct v4l2_subdev *sd = i2c_get_clientdata(client);
// 	struct imx363 *imx363 = to_imx363(sd);
// 	int ret;

// 	ret = regulator_bulk_enable(IMX363_NUM_SUPPLIES,
// 				    imx363->supplies);
// 	if (ret) {
// 		dev_err(&client->dev, "%s: failed to enable regulators\n",
// 			__func__);
// 		return ret;
// 	}

// 	ret = clk_prepare_enable(imx363->xclk);
// 	if (ret) {
// 		dev_err(&client->dev, "%s: failed to enable clock\n",
// 			__func__);
// 		goto reg_off;
// 	}

// 	gpiod_set_value_cansleep(imx363->reset_gpio, 1);
// 	usleep_range(IMX363_XCLR_MIN_DELAY_US,
// 		     IMX363_XCLR_MIN_DELAY_US + IMX363_XCLR_DELAY_RANGE_US);

// 	return 0;

// reg_off:
// 	regulator_bulk_disable(IMX363_NUM_SUPPLIES, imx363->supplies);

// 	return ret;
// }

// static int imx363_power_off(struct device *dev)
// {
// 	struct i2c_client *client = to_i2c_client(dev);
// 	struct v4l2_subdev *sd = i2c_get_clientdata(client);
// 	struct imx363 *imx363 = to_imx363(sd);

// 	gpiod_set_value_cansleep(imx363->reset_gpio, 0);
// 	regulator_bulk_disable(IMX363_NUM_SUPPLIES, imx363->supplies);
// 	clk_disable_unprepare(imx363->xclk);

// 	return 0;
// }

// static int __maybe_unused imx363_suspend(struct device *dev)
// {
// 	struct i2c_client *client = to_i2c_client(dev);
// 	struct v4l2_subdev *sd = i2c_get_clientdata(client);
// 	struct imx363 *imx363 = to_imx363(sd);

// 	if (imx363->streaming)
// 		imx363_stop_streaming(imx363);

// 	return 0;
// }

// static int __maybe_unused imx363_resume(struct device *dev)
// {
// 	struct i2c_client *client = to_i2c_client(dev);
// 	struct v4l2_subdev *sd = i2c_get_clientdata(client);
// 	struct imx363 *imx363 = to_imx363(sd);
// 	int ret;

// 	if (imx363->streaming) {
// 		ret = imx363_start_streaming(imx363);
// 		if (ret)
// 			goto error;
// 	}

// 	return 0;

// error:
// 	imx363_stop_streaming(imx363);
// 	imx363->streaming = 0;

// 	return ret;
// }

// static int imx363_get_regulators(struct imx363 *imx363)
// {
// 	struct i2c_client *client = v4l2_get_subdevdata(&imx363->sd);
// 	unsigned int i;

// 	for (i = 0; i < IMX363_NUM_SUPPLIES; i++)
// 		imx363->supplies[i].supply = imx363_supply_name[i];

// 	return devm_regulator_bulk_get(&client->dev,
// 				       IMX363_NUM_SUPPLIES,
// 				       imx363->supplies);
// }

// /* Verify chip ID */
// static int imx363_identify_module(struct imx363 *imx363)
// {
// 	struct i2c_client *client = v4l2_get_subdevdata(&imx363->sd);
// 	int ret;
// 	u32 val;

// 	ret = imx363_read_reg(imx363, IMX363_REG_CHIP_ID,
// 			      IMX363_REG_VALUE_16BIT, &val);
// 	if (ret) {
// 		dev_err(&client->dev, "failed to read chip id %x\n",
// 			IMX363_CHIP_ID);
// 		return ret;
// 	}

// 	if (val != IMX363_CHIP_ID) {
// 		dev_err(&client->dev, "chip id mismatch: %x!=%x\n",
// 			IMX363_CHIP_ID, val);
// 		return -EIO;
// 	}

// 	return 0;
// }

// static const struct v4l2_subdev_core_ops imx363_core_ops = {
// 	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
// 	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
// };

// static const struct v4l2_subdev_video_ops imx363_video_ops = {
// 	.s_stream = imx363_set_stream,
// };

// static const struct v4l2_subdev_pad_ops imx363_pad_ops = {
// 	.enum_mbus_code = imx363_enum_mbus_code,
// 	.get_fmt = imx363_get_pad_format,
// 	.set_fmt = imx363_set_pad_format,
// 	.get_selection = imx363_get_selection,
// 	.enum_frame_size = imx363_enum_frame_size,
// };

// static const struct v4l2_subdev_ops imx363_subdev_ops = {
// 	.core = &imx363_core_ops,
// 	.video = &imx363_video_ops,
// 	.pad = &imx363_pad_ops,
// };

// static const struct v4l2_subdev_internal_ops imx363_internal_ops = {
// 	.open = imx363_open,
// };

/* Initialize control handlers */
// static int imx363_init_controls(struct imx363 *imx363)
// {
// 	struct i2c_client *client = v4l2_get_subdevdata(&imx363->sd);
// 	struct v4l2_ctrl_handler *ctrl_hdlr;
// 	unsigned int height = imx363->mode->height;
// 	struct v4l2_fwnode_device_properties props;
// 	int exposure_max, exposure_def, hblank;
// 	int i, ret;

// 	ctrl_hdlr = &imx363->ctrl_handler;
// 	ret = v4l2_ctrl_handler_init(ctrl_hdlr, 11);
// 	if (ret)
// 		return ret;

// 	mutex_init(&imx363->mutex);
// 	ctrl_hdlr->lock = &imx363->mutex;

// 	/* By default, PIXEL_RATE is read only */
// 	imx363->pixel_rate = v4l2_ctrl_new_std(ctrl_hdlr, &imx363_ctrl_ops,
// 					       V4L2_CID_PIXEL_RATE,
// 					       IMX363_PIXEL_RATE,
// 					       IMX363_PIXEL_RATE, 1,
// 					       IMX363_PIXEL_RATE);

// 	/* Initial vblank/hblank/exposure parameters based on current mode */
// 	imx363->vblank = v4l2_ctrl_new_std(ctrl_hdlr, &imx363_ctrl_ops,
// 					   V4L2_CID_VBLANK, IMX363_VBLANK_MIN,
// 					   IMX363_VTS_MAX - height, 1,
// 					   imx363->mode->vts_def - height);
// 	hblank = IMX363_PPL_DEFAULT - imx363->mode->width;
// 	imx363->hblank = v4l2_ctrl_new_std(ctrl_hdlr, &imx363_ctrl_ops,
// 					   V4L2_CID_HBLANK, hblank, hblank,
// 					   1, hblank);
// 	if (imx363->hblank)
// 		imx363->hblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;
// 	exposure_max = imx363->mode->vts_def - 4;
// 	exposure_def = (exposure_max < IMX363_EXPOSURE_DEFAULT) ?
// 		exposure_max : IMX363_EXPOSURE_DEFAULT;
// 	imx363->exposure = v4l2_ctrl_new_std(ctrl_hdlr, &imx363_ctrl_ops,
// 					     V4L2_CID_EXPOSURE,
// 					     IMX363_EXPOSURE_MIN, exposure_max,
// 					     IMX363_EXPOSURE_STEP,
// 					     exposure_def);

// 	v4l2_ctrl_new_std(ctrl_hdlr, &imx363_ctrl_ops, V4L2_CID_ANALOGUE_GAIN,
// 			  IMX363_ANA_GAIN_MIN, IMX363_ANA_GAIN_MAX,
// 			  IMX363_ANA_GAIN_STEP, IMX363_ANA_GAIN_DEFAULT);

// 	v4l2_ctrl_new_std(ctrl_hdlr, &imx363_ctrl_ops, V4L2_CID_DIGITAL_GAIN,
// 			  IMX363_DGTL_GAIN_MIN, IMX363_DGTL_GAIN_MAX,
// 			  IMX363_DGTL_GAIN_STEP, IMX363_DGTL_GAIN_DEFAULT);

// 	imx363->hflip = v4l2_ctrl_new_std(ctrl_hdlr, &imx363_ctrl_ops,
// 					  V4L2_CID_HFLIP, 0, 1, 1, 0);
// 	if (imx363->hflip)
// 		imx363->hflip->flags |= V4L2_CTRL_FLAG_MODIFY_LAYOUT;

// 	imx363->vflip = v4l2_ctrl_new_std(ctrl_hdlr, &imx363_ctrl_ops,
// 					  V4L2_CID_VFLIP, 0, 1, 1, 0);
// 	if (imx363->vflip)
// 		imx363->vflip->flags |= V4L2_CTRL_FLAG_MODIFY_LAYOUT;

// 	if (ctrl_hdlr->error) {
// 		ret = ctrl_hdlr->error;
// 		dev_err(&client->dev, "%s control init failed (%d)\n",
// 			__func__, ret);
// 		goto error;
// 	}

// 	ret = v4l2_fwnode_device_parse(&client->dev, &props);
// 	if (ret)
// 		goto error;

// 	ret = v4l2_ctrl_new_fwnode_properties(ctrl_hdlr, &imx363_ctrl_ops,
// 					      &props);
// 	if (ret)
// 		goto error;

// 	imx363->sd.ctrl_handler = ctrl_hdlr;

// 	return 0;

// error:
// 	v4l2_ctrl_handler_free(ctrl_hdlr);
// 	mutex_destroy(&imx363->mutex);

// 	return ret;
// }

// static void imx363_free_controls(struct imx363 *imx363)
// {
// 	v4l2_ctrl_handler_free(imx363->sd.ctrl_handler);
// 	mutex_destroy(&imx363->mutex);
// }

// static int imx363_check_hwcfg(struct device *dev)
// {
// 	struct fwnode_handle *endpoint;
// 	struct v4l2_fwnode_endpoint ep_cfg = {
// 		.bus_type = V4L2_MBUS_CSI2_DPHY
// 	};
// 	int ret = -EINVAL;

// 	endpoint = fwnode_graph_get_next_endpoint(dev_fwnode(dev), NULL);
// 	if (!endpoint) {
// 		dev_err(dev, "endpoint node not found\n");
// 		return -EINVAL;
// 	}

// 	if (v4l2_fwnode_endpoint_alloc_parse(endpoint, &ep_cfg)) {
// 		dev_err(dev, "could not parse endpoint\n");
// 		goto error_out;
// 	}

// 	/* Check the number of MIPI CSI2 data lanes */
// 	if (ep_cfg.bus.mipi_csi2.num_data_lanes != 2) {
// 		dev_err(dev, "only 2 data lanes are supported\n");
// 		goto error_out;
// 	}

// 	/* Check the link frequency set in device tree */
// 	if (!ep_cfg.nr_of_link_frequencies) {
// 		dev_err(dev, "link-frequency property not found in DT\n");
// 		goto error_out;
// 	}

// 	if (ep_cfg.nr_of_link_frequencies != 1 ||
// 	    ep_cfg.link_frequencies[0] != IMX363_DEFAULT_LINK_FREQ) {
// 		dev_err(dev, "Link frequency not supported: %lld\n",
// 			ep_cfg.link_frequencies[0]);
// 		goto error_out;
// 	}

// 	ret = 0;

// error_out:
// 	v4l2_fwnode_endpoint_free(&ep_cfg);
// 	fwnode_handle_put(endpoint);

// 	return ret;
// }

static int imx363_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct imx363 *imx363;
	int ret;

	imx363 = devm_kzalloc(&client->dev, sizeof(*imx363), GFP_KERNEL);
	if (!imx363)
    {
        dev_err(dev, "no mem\n");
        // return -ENOMEM;
    }
	
	// v4l2_i2c_subdev_init(&imx363->sd, client, &imx363_subdev_ops);

	/* Check the hardware configuration in device tree */
	// if (imx363_check_hwcfg(dev))
	// 	return -EINVAL;

	/* Get system clock (xclk) */
	imx363->xclk = devm_clk_get(dev, NULL);
	if (IS_ERR(imx363->xclk)) {
		dev_err(dev, "failed to get xclk\n");
		// return PTR_ERR(imx363->xclk);
	}

	ret = fwnode_property_read_u32(dev_fwnode(dev), "clock-frequency", &imx363->xclk_freq);
	if (ret) {
		dev_err(dev, "could not get xclk frequency\n");
		// return ret;
	}

	/* this driver currently expects 24MHz; allow 1% tolerance */
	if (imx363->xclk_freq < 23760000 || imx363->xclk_freq > 24240000) {
		dev_err(dev, "xclk frequency not supported: %d Hz\n",
			imx363->xclk_freq);
		// return -EINVAL;
	}

	ret = clk_set_rate(imx363->xclk, imx363->xclk_freq);
	if (ret) {
		dev_err(dev, "could not set xclk frequency\n");
		// return ret;
	}


	// ret = imx363_get_regulators(imx363);
	// if (ret) {
	// 	dev_err(dev, "failed to get regulators\n");
	// 	// return ret;
	// }

	/* Request optional enable pin */
	imx363->reset_gpio = devm_gpiod_get_optional(dev, "reset",
						     GPIOD_OUT_HIGH);

	/*
	 * The sensor must be powered for imx363_identify_module()
	 * to be able to read the CHIP_ID register
	 */
	// ret = imx363_power_on(dev);
	// if (ret){
    //     dev_err(dev, "failed to power on\n");
    //     // return ret;
    // }

    ret = clk_prepare_enable(imx363->xclk);
	if (ret) {
		dev_err(dev, "%s: failed to enable clock\n",
			__func__);
	}

	gpiod_set_value_cansleep(imx363->reset_gpio, 1);
	usleep_range(IMX363_XCLR_MIN_DELAY_US,
		     IMX363_XCLR_MIN_DELAY_US + IMX363_XCLR_DELAY_RANGE_US);

    dev_info(dev, "%s: probed successfully\n");

	// ret = imx363_identify_module(imx363);
	// if (ret)
	// 	goto error_power_off;

	// /* Set default mode to max resolution */
	// imx363->mode = &supported_modes[0];

	// /* sensor doesn't enter LP-11 state upon power up until and unless
	//  * streaming is started, so upon power up switch the modes to:
	//  * streaming -> standby
	//  */
	// ret = imx363_write_reg(imx363, IMX363_REG_MODE_SELECT,
	// 		       IMX363_REG_VALUE_08BIT, IMX363_MODE_STREAMING);
	// if (ret < 0)
	// 	goto error_power_off;
	// usleep_range(100, 110);

	// /* put sensor back to standby mode */
	// ret = imx363_write_reg(imx363, IMX363_REG_MODE_SELECT,
	// 		       IMX363_REG_VALUE_08BIT, IMX363_MODE_STANDBY);
	// if (ret < 0)
	// 	goto error_power_off;
	// usleep_range(100, 110);

	// ret = imx363_init_controls(imx363);
	// if (ret)
	// 	goto error_power_off;

	// /* Initialize subdev */
	// imx363->sd.internal_ops = &imx363_internal_ops;
	// imx363->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	// imx363->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;

	// /* Initialize source pad */
	// imx363->pad.flags = MEDIA_PAD_FL_SOURCE;

	// /* Initialize default format */
	// imx363_set_default_format(imx363);

	// ret = media_entity_pads_init(&imx363->sd.entity, 1, &imx363->pad);
	// if (ret) {
	// 	dev_err(dev, "failed to init entity pads: %d\n", ret);
	// 	goto error_handler_free;
	// }

	// ret = v4l2_async_register_subdev_sensor_common(&imx363->sd);
	// if (ret < 0) {
	// 	dev_err(dev, "failed to register sensor sub-device: %d\n", ret);
	// 	goto error_media_entity;
	// }

	// /* Enable runtime PM and turn off the device */
	// pm_runtime_set_active(dev);
	// pm_runtime_enable(dev);
	// pm_runtime_idle(dev);

	return 0;

// error_media_entity:
// 	media_entity_cleanup(&imx363->sd.entity);

// error_handler_free:
// 	imx363_free_controls(imx363);

// error_power_off:
// 	imx363_power_off(dev);

	// return ret;
}

static int imx363_remove(struct i2c_client *client)
{
	// struct v4l2_subdev *sd = i2c_get_clientdata(client);
	// struct imx363 *imx363 = to_imx363(sd);

	// v4l2_async_unregister_subdev(sd);
	// media_entity_cleanup(&sd->entity);
	// imx363_free_controls(imx363);

	// pm_runtime_disable(&client->dev);
	// if (!pm_runtime_status_suspended(&client->dev))
	// 	imx363_power_off(&client->dev);
	// pm_runtime_set_suspended(&client->dev);

	return 0;
}

static const struct of_device_id imx363_dt_ids[] = {
	{ .compatible = "sony,imx363" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx363_dt_ids);

// static const struct dev_pm_ops imx363_pm_ops = {
// 	SET_SYSTEM_SLEEP_PM_OPS(imx363_suspend, imx363_resume)
// 	SET_RUNTIME_PM_OPS(imx363_power_off, imx363_power_on, NULL)
// };

static struct i2c_driver imx363_i2c_driver = {
	.driver = {
		.name = "imx363",
		.of_match_table	= imx363_dt_ids,
		// .pm = &imx363_pm_ops,
	},
	.probe_new = imx363_probe,
	.remove = imx363_remove,
};

module_i2c_driver(imx363_i2c_driver);

MODULE_AUTHOR("Dave Stevenson <dave.stevenson@raspberrypi.com");
MODULE_DESCRIPTION("Sony IMX363 sensor driver");
MODULE_LICENSE("GPL v2");