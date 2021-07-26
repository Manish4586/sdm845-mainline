// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Intel Corporation

#include <asm/unaligned.h>
#include <linux/acpi.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>

#define IMX519_REG_MODE_SELECT		0x0100
#define IMX519_MODE_STANDBY		0x00
#define IMX519_MODE_STREAMING		0x01

/* Chip ID */
#define IMX519_REG_CHIP_ID		0x0016
#define IMX519_CHIP_ID			0x0519

/* V_TIMING internal */
#define IMX519_REG_FLL			0x0340
#define IMX519_FLL_MAX			0xffff

/* Exposure control */
#define IMX519_REG_EXPOSURE		0x0202
#define IMX519_EXPOSURE_MIN		1
#define IMX519_EXPOSURE_STEP		1
#define IMX519_EXPOSURE_DEFAULT		0x04f6

/*
 *  the digital control register for all color control looks like:
 *  +-----------------+------------------+
 *  |      [7:0]      |       [15:8]     |
 *  +-----------------+------------------+
 *  |	  0x020f      |       0x020e     |
 *  --------------------------------------
 *  it is used to calculate the digital gain times value(integral + fractional)
 *  the [15:8] bits is the fractional part and [7:0] bits is the integral
 *  calculation equation is:
 *      gain value (unit: times) = REG[15:8] + REG[7:0]/0x100
 *  Only value in 0x0100 ~ 0x0FFF range is allowed.
 *  Analog gain use 10 bits in the registers and allowed range is 0 ~ 960
 */
/* Analog gain control */
#define IMX519_REG_ANALOG_GAIN		0x0204
#define IMX519_ANA_GAIN_MIN		0
#define IMX519_ANA_GAIN_MAX		960
#define IMX519_ANA_GAIN_STEP		1
#define IMX519_ANA_GAIN_DEFAULT		0

/* Digital gain control */
#define IMX519_REG_DPGA_USE_GLOBAL_GAIN	0x3ff9
#define IMX519_REG_DIG_GAIN_GLOBAL	0x020e
#define IMX519_DGTL_GAIN_MIN		256
#define IMX519_DGTL_GAIN_MAX		4095
#define IMX519_DGTL_GAIN_STEP		1
#define IMX519_DGTL_GAIN_DEFAULT	256

/* Test Pattern Control */
#define IMX519_REG_TEST_PATTERN		0x0600
#define IMX519_TEST_PATTERN_DISABLED		0
#define IMX519_TEST_PATTERN_SOLID_COLOR		1
#define IMX519_TEST_PATTERN_COLOR_BARS		2
#define IMX519_TEST_PATTERN_GRAY_COLOR_BARS	3
#define IMX519_TEST_PATTERN_PN9			4

/* Flip Control */
#define IMX519_REG_ORIENTATION		0x0101

/* default link frequency and external clock */
#define IMX519_LINK_FREQ_DEFAULT	456000000
#define IMX519_EXT_CLK			24000000
#define IMX519_LINK_FREQ_INDEX		0

struct imx519_reg {
	u16 address;
	u8 val;
};

struct imx519_reg_list {
	u32 num_of_regs;
	const struct imx519_reg *regs;
};

/* Mode : resolution and related config&values */
struct imx519_mode {
	/* Frame width */
	u32 width;
	/* Frame height */
	u32 height;

	/* V-timing */
	u32 fll_def;
	u32 fll_min;

	/* H-timing */
	u32 llp;

	/* index of link frequency */
	u32 link_freq_index;

	/* Default register values */
	struct imx519_reg_list reg_list;
};

struct imx519_hwcfg {
	u32 ext_clk;			/* sensor external clk */
	s64 *link_freqs;		/* CSI-2 link frequencies */
	unsigned int nr_of_link_freqs;
};

struct imx519 {
	struct v4l2_subdev sd;
	struct media_pad pad;

	struct v4l2_ctrl_handler ctrl_handler;
	/* V4L2 Controls */
	struct v4l2_ctrl *link_freq;
	struct v4l2_ctrl *pixel_rate;
	struct v4l2_ctrl *vblank;
	struct v4l2_ctrl *hblank;
	struct v4l2_ctrl *exposure;
	struct v4l2_ctrl *vflip;
	struct v4l2_ctrl *hflip;

	/* Current mode */
	const struct imx519_mode *cur_mode;

	struct imx519_hwcfg *hwcfg;
	s64 link_def_freq;	/* CSI-2 link default frequency */

	/*
	 * Mutex for serialized access:
	 * Protect sensor set pad format and start/stop streaming safely.
	 * Protect access to sensor v4l2 controls.
	 */
	struct mutex mutex;

	/* Streaming on/off */
	bool streaming;
};

static const struct imx519_reg imx519_global_regs[] = {
	{ 0x0136, 0x18 },
	{ 0x0137, 0x00 },
	{ 0x3c7e, 0x08 },
	{ 0x3c7f, 0x07 },
	{ 0x3020, 0x00 },
	{ 0x3e35, 0x01 },
	{ 0x3f7f, 0x01 },
	{ 0x5609, 0x57 },
	{ 0x5613, 0x51 },
	{ 0x561f, 0x5e },
	{ 0x5623, 0xd2 },
	{ 0x5637, 0x11 },
	{ 0x5657, 0x11 },
	{ 0x5659, 0x12 },
	{ 0x5733, 0x60 },
	{ 0x5905, 0x57 },
	{ 0x590f, 0x51 },
	{ 0x591b, 0x5e },
	{ 0x591f, 0xd2 },
	{ 0x5933, 0x11 },
	{ 0x5953, 0x11 },
	{ 0x5955, 0x12 },
	{ 0x5a2f, 0x60 },
	{ 0x5a85, 0x57 },
	{ 0x5a8f, 0x51 },
	{ 0x5a9b, 0x5e },
	{ 0x5a9f, 0xd2 },
	{ 0x5ab3, 0x11 },
	{ 0x5ad3, 0x11 },
	{ 0x5ad5, 0x12 },
	{ 0x5baf, 0x60 },
	{ 0x5c15, 0x2a },
	{ 0x5c17, 0x80 },
	{ 0x5c19, 0x31 },
	{ 0x5c1b, 0x87 },
	{ 0x5c25, 0x25 },
	{ 0x5c27, 0x7b },
	{ 0x5c29, 0x2a },
	{ 0x5c2b, 0x80 },
	{ 0x5c2d, 0x31 },
	{ 0x5c2f, 0x87 },
	{ 0x5c35, 0x2b },
	{ 0x5c37, 0x81 },
	{ 0x5c39, 0x31 },
	{ 0x5c3b, 0x87 },
	{ 0x5c45, 0x25 },
	{ 0x5c47, 0x7b },
	{ 0x5c49, 0x2a },
	{ 0x5c4b, 0x80 },
	{ 0x5c4d, 0x31 },
	{ 0x5c4f, 0x87 },
	{ 0x5c55, 0x2d },
	{ 0x5c57, 0x83 },
	{ 0x5c59, 0x32 },
	{ 0x5c5b, 0x88 },
	{ 0x5c65, 0x29 },
	{ 0x5c67, 0x7f },
	{ 0x5c69, 0x2e },
	{ 0x5c6b, 0x84 },
	{ 0x5c6d, 0x32 },
	{ 0x5c6f, 0x88 },
	{ 0x5e69, 0x04 },
	{ 0x5e9d, 0x00 },
	{ 0x5f18, 0x10 },
	{ 0x5f1a, 0x0e },
	{ 0x5f20, 0x12 },
	{ 0x5f22, 0x10 },
	{ 0x5f24, 0x0e },
	{ 0x5f28, 0x10 },
	{ 0x5f2a, 0x0e },
	{ 0x5f30, 0x12 },
	{ 0x5f32, 0x10 },
	{ 0x5f34, 0x0e },
	{ 0x5f38, 0x0f },
	{ 0x5f39, 0x0d },
	{ 0x5f3c, 0x11 },
	{ 0x5f3d, 0x0f },
	{ 0x5f3e, 0x0d },
	{ 0x5f61, 0x07 },
	{ 0x5f64, 0x05 },
	{ 0x5f67, 0x03 },
	{ 0x5f6a, 0x03 },
	{ 0x5f6d, 0x07 },
	{ 0x5f70, 0x07 },
	{ 0x5f73, 0x05 },
	{ 0x5f76, 0x02 },
	{ 0x5f79, 0x07 },
	{ 0x5f7c, 0x07 },
	{ 0x5f7f, 0x07 },
	{ 0x5f82, 0x07 },
	{ 0x5f85, 0x03 },
	{ 0x5f88, 0x02 },
	{ 0x5f8b, 0x01 },
	{ 0x5f8e, 0x01 },
	{ 0x5f91, 0x04 },
	{ 0x5f94, 0x05 },
	{ 0x5f97, 0x02 },
	{ 0x5f9d, 0x07 },
	{ 0x5fa0, 0x07 },
	{ 0x5fa3, 0x07 },
	{ 0x5fa6, 0x07 },
	{ 0x5fa9, 0x03 },
	{ 0x5fac, 0x01 },
	{ 0x5faf, 0x01 },
	{ 0x5fb5, 0x03 },
	{ 0x5fb8, 0x02 },
	{ 0x5fbb, 0x01 },
	{ 0x5fc1, 0x07 },
	{ 0x5fc4, 0x07 },
	{ 0x5fc7, 0x07 },
	{ 0x5fd1, 0x00 },
	{ 0x6302, 0x79 },
	{ 0x6305, 0x78 },
	{ 0x6306, 0xa5 },
	{ 0x6308, 0x03 },
	{ 0x6309, 0x20 },
	{ 0x630b, 0x0a },
	{ 0x630d, 0x48 },
	{ 0x630f, 0x06 },
	{ 0x6311, 0xa4 },
	{ 0x6313, 0x03 },
	{ 0x6314, 0x20 },
	{ 0x6316, 0x0a },
	{ 0x6317, 0x31 },
	{ 0x6318, 0x4a },
	{ 0x631a, 0x06 },
	{ 0x631b, 0x40 },
	{ 0x631c, 0xa4 },
	{ 0x631e, 0x03 },
	{ 0x631f, 0x20 },
	{ 0x6321, 0x0a },
	{ 0x6323, 0x4a },
	{ 0x6328, 0x80 },
	{ 0x6329, 0x01 },
	{ 0x632a, 0x30 },
	{ 0x632b, 0x02 },
	{ 0x632c, 0x20 },
	{ 0x632d, 0x02 },
	{ 0x632e, 0x30 },
	{ 0x6330, 0x60 },
	{ 0x6332, 0x90 },
	{ 0x6333, 0x01 },
	{ 0x6334, 0x30 },
	{ 0x6335, 0x02 },
	{ 0x6336, 0x20 },
	{ 0x6338, 0x80 },
	{ 0x633a, 0xa0 },
	{ 0x633b, 0x01 },
	{ 0x633c, 0x60 },
	{ 0x633d, 0x02 },
	{ 0x633e, 0x60 },
	{ 0x633f, 0x01 },
	{ 0x6340, 0x30 },
	{ 0x6341, 0x02 },
	{ 0x6342, 0x20 },
	{ 0x6343, 0x03 },
	{ 0x6344, 0x80 },
	{ 0x6345, 0x03 },
	{ 0x6346, 0x90 },
	{ 0x6348, 0xf0 },
	{ 0x6349, 0x01 },
	{ 0x634a, 0x20 },
	{ 0x634b, 0x02 },
	{ 0x634c, 0x10 },
	{ 0x634d, 0x03 },
	{ 0x634e, 0x60 },
	{ 0x6350, 0xa0 },
	{ 0x6351, 0x01 },
	{ 0x6352, 0x60 },
	{ 0x6353, 0x02 },
	{ 0x6354, 0x50 },
	{ 0x6355, 0x02 },
	{ 0x6356, 0x60 },
	{ 0x6357, 0x01 },
	{ 0x6358, 0x30 },
	{ 0x6359, 0x02 },
	{ 0x635a, 0x30 },
	{ 0x635b, 0x03 },
	{ 0x635c, 0x90 },
	{ 0x635f, 0x01 },
	{ 0x6360, 0x10 },
	{ 0x6361, 0x01 },
	{ 0x6362, 0x40 },
	{ 0x6363, 0x02 },
	{ 0x6364, 0x50 },
	{ 0x6368, 0x70 },
	{ 0x636a, 0xa0 },
	{ 0x636b, 0x01 },
	{ 0x636c, 0x50 },
	{ 0x637d, 0xe4 },
	{ 0x637e, 0xb4 },
	{ 0x638c, 0x8e },
	{ 0x638d, 0x38 },
	{ 0x638e, 0xe3 },
	{ 0x638f, 0x4c },
	{ 0x6390, 0x30 },
	{ 0x6391, 0xc3 },
	{ 0x6392, 0xae },
	{ 0x6393, 0xba },
	{ 0x6394, 0xeb },
	{ 0x6395, 0x6e },
	{ 0x6396, 0x34 },
	{ 0x6397, 0xe3 },
	{ 0x6398, 0xcf },
	{ 0x6399, 0x3c },
	{ 0x639a, 0xf3 },
	{ 0x639b, 0x0c },
	{ 0x639c, 0x30 },
	{ 0x639d, 0xc1 },
	{ 0x63b9, 0xa3 },
	{ 0x63ba, 0xfe },
	{ 0x7600, 0x01 },
	{ 0x79a0, 0x01 },
	{ 0x79a1, 0x01 },
	{ 0x79a2, 0x01 },
	{ 0x79a3, 0x01 },
	{ 0x79a4, 0x01 },
	{ 0x79a5, 0x20 },
	{ 0x79a9, 0x00 },
	{ 0x79aa, 0x01 },
	{ 0x79ad, 0x00 },
	{ 0x79af, 0x00 },
	{ 0x8173, 0x01 },
	{ 0x835c, 0x01 },
	{ 0x8a74, 0x01 },
	{ 0x8c1f, 0x00 },
	{ 0x8c27, 0x00 },
	{ 0x8c3b, 0x03 },
	{ 0x9004, 0x0b },
	{ 0x920c, 0x6a },
	{ 0x920d, 0x22 },
	{ 0x920e, 0x6a },
	{ 0x920f, 0x23 },
	{ 0x9214, 0x6a },
	{ 0x9215, 0x20 },
	{ 0x9216, 0x6a },
	{ 0x9217, 0x21 },
	{ 0x9385, 0x3e },
	{ 0x9387, 0x1b },
	{ 0x938d, 0x4d },
	{ 0x938f, 0x43 },
	{ 0x9391, 0x1b },
	{ 0x9395, 0x4d },
	{ 0x9397, 0x43 },
	{ 0x9399, 0x1b },
	{ 0x939d, 0x3e },
	{ 0x939f, 0x2f },
	{ 0x93a5, 0x43 },
	{ 0x93a7, 0x2f },
	{ 0x93a9, 0x2f },
	{ 0x93ad, 0x34 },
	{ 0x93af, 0x2f },
	{ 0x93b5, 0x3e },
	{ 0x93b7, 0x2f },
	{ 0x93bd, 0x4d },
	{ 0x93bf, 0x43 },
	{ 0x93c1, 0x2f },
	{ 0x93c5, 0x4d },
	{ 0x93c7, 0x43 },
	{ 0x93c9, 0x2f },
	{ 0x974b, 0x02 },
	{ 0x995c, 0x8c },
	{ 0x995d, 0x00 },
	{ 0x995e, 0x00 },
	{ 0x9963, 0x64 },
	{ 0x9964, 0x50 },
	{ 0xaa0a, 0x26 },
	{ 0xae03, 0x04 },
	{ 0xae04, 0x03 },
	{ 0xae05, 0x03 },
	{ 0xbc1c, 0x08 },
	{ 0xa9bb, 0x00 },
	{ 0xa9bd, 0x00 },
	{ 0xa9bf, 0x00 },
	{ 0x9883, 0x00 },
	{ 0x9886, 0x00 },
	{ 0x9889, 0x00 },
	{ 0x988c, 0x00 },
	{ 0x988f, 0x00 },
	{ 0x9892, 0x00 },
	{ 0x9895, 0x00 },
	{ 0x9898, 0x00 },
	{ 0x9899, 0x01 },
	{ 0xaa06, 0x3f },
	{ 0xaa07, 0x05 },
	{ 0xaa08, 0x04 },
	{ 0xaa12, 0x3f },
	{ 0xaa13, 0x04 },
	{ 0xaa14, 0x03 },
	{ 0xaa1e, 0x12 },
	{ 0xaa1f, 0x05 },
	{ 0xaa20, 0x04 },
	{ 0xaa2a, 0x0d },
	{ 0xaa2b, 0x04 },
	{ 0xaa2c, 0x03 },
	{ 0xac19, 0x02 },
	{ 0xac1b, 0x01 },
	{ 0xac1d, 0x01 },
	{ 0xac3c, 0x00 },
	{ 0xac3d, 0x01 },
	{ 0xac3e, 0x00 },
	{ 0xac3f, 0x01 },
	{ 0xac40, 0x00 },
	{ 0xac41, 0x01 },
	{ 0xac61, 0x02 },
	{ 0xac63, 0x01 },
	{ 0xac65, 0x01 },
	{ 0xac84, 0x00 },
	{ 0xac85, 0x01 },
	{ 0xac86, 0x00 },
	{ 0xac87, 0x01 },
	{ 0xac88, 0x00 },
	{ 0xac89, 0x01 },
};

static const struct imx519_reg_list imx519_global_setting = {
	.num_of_regs = ARRAY_SIZE(imx519_global_regs),
	.regs = imx519_global_regs,
};

/* 1080p @ 29.932542fps */
static const struct imx519_reg mode_1920x1080_regs[] = {
	{ 0x0111, 0x03 },
	{ 0x0112, 0x0a },
	{ 0x0113, 0x0a },
	{ 0x0114, 0x02 },
	{ 0x0342, 0x19 },
	{ 0x0343, 0x00 },
	{ 0x0340, 0x0d },
	{ 0x0341, 0xe0 },
	{ 0x0344, 0x00 },
	{ 0x0345, 0x00 },
	{ 0x0346, 0x00 },
	{ 0x0347, 0x00 },
	{ 0x0348, 0x12 },
	{ 0x0349, 0x2f },
	{ 0x034a, 0x0d },
	{ 0x034b, 0xa7 },
	{ 0x0220, 0x01 },
	{ 0x0221, 0x11 },
	{ 0x0222, 0x01 },
	{ 0x0900, 0x00 },
	{ 0x0901, 0x11 },
	{ 0x0902, 0x0a },
	{ 0x3f4c, 0x01 },
	{ 0x3f4d, 0x01 },
	{ 0x4254, 0x7f },
	{ 0x0401, 0x00 },
	{ 0x0404, 0x00 },
	{ 0x0405, 0x10 },
	{ 0x0408, 0x00 },
	{ 0x0409, 0x00 },
	{ 0x040a, 0x00 },
	{ 0x040b, 0x00 },
	{ 0x040c, 0x12 },
	{ 0x040d, 0x30 },
	{ 0x040e, 0x0d },
	{ 0x040f, 0xa8 },
	{ 0x034c, 0x12 },
	{ 0x034d, 0x30 },
	{ 0x034e, 0x0d },
	{ 0x034f, 0xa8 },
	{ 0x0301, 0x06 },
	{ 0x0303, 0x04 },
	{ 0x0305, 0x04 },
	{ 0x0306, 0x01 },
	{ 0x0307, 0x55 },
	{ 0x0309, 0x0a },
	{ 0x030b, 0x02 },
	{ 0x030d, 0x03 },
	{ 0x030e, 0x01 },
	{ 0x030f, 0x1f },
	{ 0x0310, 0x01 },
	{ 0x0820, 0x0d },
	{ 0x0821, 0x74 },
	{ 0x0822, 0x00 },
	{ 0x0823, 0x00 },
	{ 0x3e20, 0x01 },
	{ 0x3e37, 0x01 },
	{ 0x38a3, 0x02 },
	{ 0x38b4, 0x06 },
	{ 0x38b5, 0x17 },
	{ 0x38b6, 0x04 },
	{ 0x38b7, 0x93 },
	{ 0x38b8, 0x0c },
	{ 0x38b9, 0x18 },
	{ 0x38ba, 0x09 },
	{ 0x38bb, 0x14 },
	{ 0x38ac, 0x01 },
	{ 0x38ad, 0x00 },
	{ 0x38ae, 0x00 },
	{ 0x38af, 0x00 },
	{ 0x38b0, 0x00 },
	{ 0x38b1, 0x00 },
	{ 0x38b2, 0x00 },
	{ 0x38b3, 0x00 },
	{ 0x38a4, 0x00 },
	{ 0x38a5, 0x5a },
	{ 0x38a6, 0x00 },
	{ 0x38a7, 0x50 },
	{ 0x38a8, 0x02 },
	{ 0x38a9, 0x30 },
	{ 0x38aa, 0x02 },
	{ 0x38ab, 0x28 },
	{ 0x3e3b, 0x00 },
	{ 0x0106, 0x00 },
	{ 0x0b00, 0x00 },
	{ 0x3230, 0x00 },
	{ 0x3f14, 0x00 },
	{ 0x3f3c, 0x03 },
	{ 0x3f0d, 0x0a },
	{ 0x3fbc, 0x00 },
	{ 0x3c06, 0x00 },
	{ 0x3c07, 0x80 },
	{ 0x3c0a, 0x00 },
	{ 0x3c0b, 0x00 },
	{ 0x3f78, 0x01 },
	{ 0x3f79, 0x54 },
	{ 0x3f7c, 0x00 },
	{ 0x3f7d, 0x00 },
	{ 0x0202, 0x03 },
	{ 0x0203, 0xe8 },
	{ 0x0224, 0x03 },
	{ 0x0225, 0xe8 },
	{ 0x0204, 0x00 },
	{ 0x0205, 0x00 },
	{ 0x0216, 0x00 },
	{ 0x0217, 0x00 },
	{ 0x020e, 0x01 },
	{ 0x020f, 0x00 },
	{ 0x0218, 0x01 },
	{ 0x0219, 0x00 },
};

static const struct imx519_reg mode_4608x3456_regs[] = {
	{ 0x0111, 0x03 },
	{ 0x0112, 0x0a },
	{ 0x0113, 0x0a },
	{ 0x0114, 0x02 },
	{ 0x0342, 0x32 },
	{ 0x0343, 0x00 },
	{ 0x0340, 0x0e },
	{ 0x0341, 0x3d },
	{ 0x0344, 0x00 },
	{ 0x0345, 0x00 },
	{ 0x0346, 0x00 },
	{ 0x0347, 0x00 },
	{ 0x0348, 0x12 },
	{ 0x0349, 0x2f },
	{ 0x034a, 0x0d },
	{ 0x034b, 0xa7 },
	{ 0x0220, 0x01 },
	{ 0x0221, 0x11 },
	{ 0x0222, 0x01 },
	{ 0x0900, 0x00 },
	{ 0x0901, 0x11 },
	{ 0x0902, 0x0a },
	{ 0x3f4c, 0x01 },
	{ 0x3f4d, 0x01 },
	{ 0x4254, 0x7f },
	{ 0x0401, 0x00 },
	{ 0x0404, 0x00 },
	{ 0x0405, 0x10 },
	{ 0x0408, 0x00 },
	{ 0x0409, 0x00 },
	{ 0x040a, 0x00 },
	{ 0x040b, 0x00 },
	{ 0x040c, 0x12 },
	{ 0x040d, 0x30 },
	{ 0x040e, 0x0d },
	{ 0x040f, 0xa8 },
	{ 0x034c, 0x12 },
	{ 0x034d, 0x30 },
	{ 0x034e, 0x0d },
	{ 0x034f, 0xa8 },
	{ 0x0301, 0x06 },
	{ 0x0303, 0x02 },
	{ 0x0305, 0x04 },
	{ 0x0306, 0x01 },
	{ 0x0307, 0x5e },
	{ 0x0309, 0x0a },
	{ 0x030b, 0x02 },
	{ 0x030d, 0x04 },
	{ 0x030e, 0x01 },
	{ 0x030f, 0x45 },
	{ 0x0310, 0x01 },
	{ 0x0820, 0x0b },
	{ 0x0821, 0x6d },
	{ 0x0822, 0x00 },
	{ 0x0823, 0x00 },
	{ 0x0106, 0x00 },
	{ 0x0b00, 0x00 },
	{ 0x3230, 0x00 },
	{ 0x3f14, 0x00 },
	{ 0x3f3c, 0x03 },
	{ 0x3f0d, 0x0a },
	{ 0x3c06, 0x01 },
	{ 0x3c07, 0x7a },
	{ 0x3c0a, 0x00 },
	{ 0x3c0b, 0x00 },
	{ 0x3f78, 0x00 },
	{ 0x3f79, 0x00 },
	{ 0x3f7c, 0x00 },
	{ 0x3f7d, 0x00 },
	{ 0x0202, 0x03 },
	{ 0x0203, 0xe8 },
	{ 0x0224, 0x03 },
	{ 0x0225, 0xe8 },
	{ 0x0204, 0x00 },
	{ 0x0205, 0x00 },
	{ 0x0216, 0x00 },
	{ 0x0217, 0x00 },
	{ 0x020e, 0x01 },
	{ 0x020f, 0x00 },
	{ 0x0218, 0x01 },
	{ 0x0219, 0x00 },
	{ 0x3e20, 0x02 },
	{ 0x3e3b, 0x01 },
	{ 0x4434, 0x02 },
	{ 0x4435, 0x30 },
	{ 0xe186, 0x34 },
	{ 0xe1a6, 0x34 },
};

static const char * const imx519_test_pattern_menu[] = {
	"Disabled",
	"Solid Colour",
	"Eight Vertical Colour Bars",
	"Colour Bars With Fade to Grey",
	"Pseudorandom Sequence (PN9)",
};

/* supported link frequencies */
static const s64 link_freq_menu_items[] = {
	IMX519_LINK_FREQ_DEFAULT,
};

/* Mode configs */
static const struct imx519_mode supported_modes[] = {
	{
		.width = 4608,
		.height = 3456,
		.fll_def = 3242,
		.fll_min = 3242,
		.llp = 3968,
		.link_freq_index = IMX519_LINK_FREQ_INDEX,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mode_1920x1080_regs),
			.regs = mode_1920x1080_regs,
		},
	},
	{
		.width = 4608,
		.height = 3456,
		.fll_def = 3242,
		.fll_min = 3242,
		.llp = 3968,
		.link_freq_index = IMX519_LINK_FREQ_INDEX,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mode_4608x3456_regs),
			.regs = mode_4608x3456_regs,
		},
	},
};

static inline struct imx519 *to_imx519(struct v4l2_subdev *_sd)
{
	return container_of(_sd, struct imx519, sd);
}

/* Get bayer order based on flip setting. */
static u32 imx519_get_format_code(struct imx519 *imx519)
{
	/*
	 * Only one bayer order is supported.
	 * It depends on the flip settings.
	 */
	u32 code;
	static const u32 codes[2][2] = {
		{ MEDIA_BUS_FMT_SRGGB10_1X10, MEDIA_BUS_FMT_SGRBG10_1X10, },
		{ MEDIA_BUS_FMT_SGBRG10_1X10, MEDIA_BUS_FMT_SBGGR10_1X10, },
	};

	lockdep_assert_held(&imx519->mutex);
	code = codes[imx519->vflip->val][imx519->hflip->val];

	return code;
}

/* Read registers up to 4 at a time */
static int imx519_read_reg(struct imx519 *imx519, u16 reg, u32 len, u32 *val)
{
	struct i2c_client *client = v4l2_get_subdevdata(&imx519->sd);
	struct i2c_msg msgs[2];
	u8 addr_buf[2];
	u8 data_buf[4] = { 0 };
	int ret;

	if (len > 4)
		return -EINVAL;

	put_unaligned_be16(reg, addr_buf);
	/* Write register address */
	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = ARRAY_SIZE(addr_buf);
	msgs[0].buf = addr_buf;

	/* Read data from register */
	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = &data_buf[4 - len];

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret != ARRAY_SIZE(msgs))
		return -EIO;

	*val = get_unaligned_be32(data_buf);

	return 0;
}

/* Write registers up to 4 at a time */
static int imx519_write_reg(struct imx519 *imx519, u16 reg, u32 len, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(&imx519->sd);
	u8 buf[6];

	if (len > 4)
		return -EINVAL;

	put_unaligned_be16(reg, buf);
	put_unaligned_be32(val << (8 * (4 - len)), buf + 2);
	if (i2c_master_send(client, buf, len + 2) != len + 2)
		return -EIO;

	return 0;
}

/* Write a list of registers */
static int imx519_write_regs(struct imx519 *imx519,
			     const struct imx519_reg *regs, u32 len)
{
	struct i2c_client *client = v4l2_get_subdevdata(&imx519->sd);
	int ret;
	u32 i;

	for (i = 0; i < len; i++) {
		ret = imx519_write_reg(imx519, regs[i].address, 1, regs[i].val);
		if (ret) {
			dev_err_ratelimited(&client->dev,
					    "write reg 0x%4.4x return err %d",
					    regs[i].address, ret);
			return ret;
		}
	}

	return 0;
}

/* Open sub-device */
static int imx519_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct imx519 *imx519 = to_imx519(sd);
	struct v4l2_mbus_framefmt *try_fmt =
		v4l2_subdev_get_try_format(sd, fh->state, 0);

	mutex_lock(&imx519->mutex);

	/* Initialize try_fmt */
	try_fmt->width = imx519->cur_mode->width;
	try_fmt->height = imx519->cur_mode->height;
	try_fmt->code = imx519_get_format_code(imx519);
	try_fmt->field = V4L2_FIELD_NONE;

	mutex_unlock(&imx519->mutex);

	return 0;
}

static int imx519_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct imx519 *imx519 = container_of(ctrl->handler,
					     struct imx519, ctrl_handler);
	struct i2c_client *client = v4l2_get_subdevdata(&imx519->sd);
	s64 max;
	int ret;

	/* Propagate change of current control to all related controls */
	switch (ctrl->id) {
	case V4L2_CID_VBLANK:
		/* Update max exposure while meeting expected vblanking */
		max = imx519->cur_mode->height + ctrl->val - 18;
		__v4l2_ctrl_modify_range(imx519->exposure,
					 imx519->exposure->minimum,
					 max, imx519->exposure->step, max);
		break;
	}

	/*
	 * Applying V4L2 control value only happens
	 * when power is up for streaming
	 */
	if (!pm_runtime_get_if_in_use(&client->dev))
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_ANALOGUE_GAIN:
		/* Analog gain = 1024/(1024 - ctrl->val) times */
		ret = imx519_write_reg(imx519, IMX519_REG_ANALOG_GAIN, 2,
				       ctrl->val);
		break;
	case V4L2_CID_DIGITAL_GAIN:
		ret = imx519_write_reg(imx519, IMX519_REG_DIG_GAIN_GLOBAL, 2,
				       ctrl->val);
		break;
	case V4L2_CID_EXPOSURE:
		ret = imx519_write_reg(imx519, IMX519_REG_EXPOSURE, 2,
				       ctrl->val);
		break;
	case V4L2_CID_VBLANK:
		/* Update FLL that meets expected vertical blanking */
		ret = imx519_write_reg(imx519, IMX519_REG_FLL, 2,
				       imx519->cur_mode->height + ctrl->val);
		break;
	case V4L2_CID_TEST_PATTERN:
		ret = imx519_write_reg(imx519, IMX519_REG_TEST_PATTERN,
				       2, ctrl->val);
		break;
	case V4L2_CID_HFLIP:
	case V4L2_CID_VFLIP:
		ret = imx519_write_reg(imx519, IMX519_REG_ORIENTATION, 1,
				       imx519->hflip->val |
				       imx519->vflip->val << 1);
		break;
	default:
		ret = -EINVAL;
		dev_info(&client->dev, "ctrl(id:0x%x,val:0x%x) is not handled",
			 ctrl->id, ctrl->val);
		break;
	}

	pm_runtime_put(&client->dev);

	return ret;
}

static const struct v4l2_ctrl_ops imx519_ctrl_ops = {
	.s_ctrl = imx519_set_ctrl,
};

static int imx519_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *sd_state,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	struct imx519 *imx519 = to_imx519(sd);

	if (code->index > 0)
		return -EINVAL;

	mutex_lock(&imx519->mutex);
	code->code = imx519_get_format_code(imx519);
	mutex_unlock(&imx519->mutex);

	return 0;
}

static int imx519_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *sd_state,
				  struct v4l2_subdev_frame_size_enum *fse)
{
	struct imx519 *imx519 = to_imx519(sd);

	if (fse->index >= ARRAY_SIZE(supported_modes))
		return -EINVAL;

	mutex_lock(&imx519->mutex);
	if (fse->code != imx519_get_format_code(imx519)) {
		mutex_unlock(&imx519->mutex);
		return -EINVAL;
	}
	mutex_unlock(&imx519->mutex);

	fse->min_width = supported_modes[fse->index].width;
	fse->max_width = fse->min_width;
	fse->min_height = supported_modes[fse->index].height;
	fse->max_height = fse->min_height;

	return 0;
}

static void imx519_update_pad_format(struct imx519 *imx519,
				     const struct imx519_mode *mode,
				     struct v4l2_subdev_format *fmt)
{
	fmt->format.width = mode->width;
	fmt->format.height = mode->height;
	fmt->format.code = imx519_get_format_code(imx519);
	fmt->format.field = V4L2_FIELD_NONE;
}

static int imx519_do_get_pad_format(struct imx519 *imx519,
				    struct v4l2_subdev_state *sd_state,
				    struct v4l2_subdev_format *fmt)
{
	struct v4l2_mbus_framefmt *framefmt;
	struct v4l2_subdev *sd = &imx519->sd;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		framefmt = v4l2_subdev_get_try_format(sd, sd_state, fmt->pad);
		fmt->format = *framefmt;
	} else {
		imx519_update_pad_format(imx519, imx519->cur_mode, fmt);
	}

	return 0;
}

static int imx519_get_pad_format(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *sd_state,
				 struct v4l2_subdev_format *fmt)
{
	struct imx519 *imx519 = to_imx519(sd);
	int ret;

	mutex_lock(&imx519->mutex);
	ret = imx519_do_get_pad_format(imx519, sd_state, fmt);
	mutex_unlock(&imx519->mutex);

	return ret;
}

static int
imx519_set_pad_format(struct v4l2_subdev *sd,
		      struct v4l2_subdev_state *sd_state,
		      struct v4l2_subdev_format *fmt)
{
	struct imx519 *imx519 = to_imx519(sd);
	const struct imx519_mode *mode;
	struct v4l2_mbus_framefmt *framefmt;
	s32 vblank_def;
	s32 vblank_min;
	s64 h_blank;
	u64 pixel_rate;
	u32 height;

	mutex_lock(&imx519->mutex);

	/*
	 * Only one bayer order is supported.
	 * It depends on the flip settings.
	 */
	fmt->format.code = imx519_get_format_code(imx519);

	mode = v4l2_find_nearest_size(supported_modes,
				      ARRAY_SIZE(supported_modes),
				      width, height,
				      fmt->format.width, fmt->format.height);
	imx519_update_pad_format(imx519, mode, fmt);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		framefmt = v4l2_subdev_get_try_format(sd, sd_state, fmt->pad);
		*framefmt = fmt->format;
	} else {
		imx519->cur_mode = mode;
		pixel_rate = imx519->link_def_freq * 2 * 4;
		do_div(pixel_rate, 10);
		__v4l2_ctrl_s_ctrl_int64(imx519->pixel_rate, pixel_rate);
		/* Update limits and set FPS to default */
		height = imx519->cur_mode->height;
		vblank_def = imx519->cur_mode->fll_def - height;
		vblank_min = imx519->cur_mode->fll_min - height;
		height = IMX519_FLL_MAX - height;
		__v4l2_ctrl_modify_range(imx519->vblank, vblank_min, height, 1,
					 vblank_def);
		__v4l2_ctrl_s_ctrl(imx519->vblank, vblank_def);
		h_blank = mode->llp - imx519->cur_mode->width;
		/*
		 * Currently hblank is not changeable.
		 * So FPS control is done only by vblank.
		 */
		__v4l2_ctrl_modify_range(imx519->hblank, h_blank,
					 h_blank, 1, h_blank);
	}

	mutex_unlock(&imx519->mutex);

	return 0;
}

/* Start streaming */
static int imx519_start_streaming(struct imx519 *imx519)
{
	struct i2c_client *client = v4l2_get_subdevdata(&imx519->sd);
	const struct imx519_reg_list *reg_list;
	int ret;

	/* Global Setting */
	reg_list = &imx519_global_setting;
	ret = imx519_write_regs(imx519, reg_list->regs, reg_list->num_of_regs);
	if (ret) {
		dev_err(&client->dev, "failed to set global settings");
		return ret;
	}

	/* Apply default values of current mode */
	reg_list = &imx519->cur_mode->reg_list;
	ret = imx519_write_regs(imx519, reg_list->regs, reg_list->num_of_regs);
	if (ret) {
		dev_err(&client->dev, "failed to set mode");
		return ret;
	}

	/* set digital gain control to all color mode */
	ret = imx519_write_reg(imx519, IMX519_REG_DPGA_USE_GLOBAL_GAIN, 1, 1);
	if (ret)
		return ret;

	/* Apply customized values from user */
	ret =  __v4l2_ctrl_handler_setup(imx519->sd.ctrl_handler);
	if (ret)
		return ret;

	return imx519_write_reg(imx519, IMX519_REG_MODE_SELECT,
				1, IMX519_MODE_STREAMING);
}

/* Stop streaming */
static int imx519_stop_streaming(struct imx519 *imx519)
{
	return imx519_write_reg(imx519, IMX519_REG_MODE_SELECT,
				1, IMX519_MODE_STANDBY);
}

static int imx519_set_stream(struct v4l2_subdev *sd, int enable)
{
	struct imx519 *imx519 = to_imx519(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	mutex_lock(&imx519->mutex);
	if (imx519->streaming == enable) {
		mutex_unlock(&imx519->mutex);
		return 0;
	}

	if (enable) {
		ret = pm_runtime_resume_and_get(&client->dev);
		if (ret < 0)
			goto err_unlock;

		/*
		 * Apply default & customized values
		 * and then start streaming.
		 */
		ret = imx519_start_streaming(imx519);
		if (ret)
			goto err_rpm_put;
	} else {
		imx519_stop_streaming(imx519);
		pm_runtime_put(&client->dev);
	}

	imx519->streaming = enable;

	/* vflip and hflip cannot change during streaming */
	__v4l2_ctrl_grab(imx519->vflip, enable);
	__v4l2_ctrl_grab(imx519->hflip, enable);

	mutex_unlock(&imx519->mutex);

	return ret;

err_rpm_put:
	pm_runtime_put(&client->dev);
err_unlock:
	mutex_unlock(&imx519->mutex);

	return ret;
}

static int __maybe_unused imx519_suspend(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct imx519 *imx519 = to_imx519(sd);

	if (imx519->streaming)
		imx519_stop_streaming(imx519);

	return 0;
}

static int __maybe_unused imx519_resume(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct imx519 *imx519 = to_imx519(sd);
	int ret;

	if (imx519->streaming) {
		ret = imx519_start_streaming(imx519);
		if (ret)
			goto error;
	}

	return 0;

error:
	imx519_stop_streaming(imx519);
	imx519->streaming = 0;
	return ret;
}

/* Verify chip ID */
static int imx519_identify_module(struct imx519 *imx519)
{
	struct i2c_client *client = v4l2_get_subdevdata(&imx519->sd);
	int ret;
	u32 val;

	ret = imx519_read_reg(imx519, IMX519_REG_CHIP_ID, 2, &val);
	if (ret)
		return ret;

	if (val != IMX519_CHIP_ID) {
		dev_err(&client->dev, "chip id mismatch: %x!=%x",
			IMX519_CHIP_ID, val);
		return -EIO;
	}

	dev_info(&client->dev, "IMX519 SENSOR ID MATCHES!!!! ID = 0X%X", val);

	return 0;
}

static const struct v4l2_subdev_core_ops imx519_subdev_core_ops = {
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_video_ops imx519_video_ops = {
	.s_stream = imx519_set_stream,
};

static const struct v4l2_subdev_pad_ops imx519_pad_ops = {
	.enum_mbus_code = imx519_enum_mbus_code,
	.get_fmt = imx519_get_pad_format,
	.set_fmt = imx519_set_pad_format,
	.enum_frame_size = imx519_enum_frame_size,
};

static const struct v4l2_subdev_ops imx519_subdev_ops = {
	.core = &imx519_subdev_core_ops,
	.video = &imx519_video_ops,
	.pad = &imx519_pad_ops,
};

static const struct media_entity_operations imx519_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static const struct v4l2_subdev_internal_ops imx519_internal_ops = {
	.open = imx519_open,
};

/* Initialize control handlers */
static int imx519_init_controls(struct imx519 *imx519)
{
	struct i2c_client *client = v4l2_get_subdevdata(&imx519->sd);
	struct v4l2_ctrl_handler *ctrl_hdlr;
	s64 exposure_max;
	s64 vblank_def;
	s64 vblank_min;
	s64 hblank;
	u64 pixel_rate;
	const struct imx519_mode *mode;
	u32 max;
	int ret;

	ctrl_hdlr = &imx519->ctrl_handler;
	ret = v4l2_ctrl_handler_init(ctrl_hdlr, 10);
	if (ret)
		return ret;

	ctrl_hdlr->lock = &imx519->mutex;
	max = ARRAY_SIZE(link_freq_menu_items) - 1;
	imx519->link_freq = v4l2_ctrl_new_int_menu(ctrl_hdlr, &imx519_ctrl_ops,
						   V4L2_CID_LINK_FREQ, max, 0,
						   link_freq_menu_items);
	if (imx519->link_freq)
		imx519->link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	/* pixel_rate = link_freq * 2 * nr_of_lanes / bits_per_sample */
	pixel_rate = imx519->link_def_freq * 2 * 4;
	do_div(pixel_rate, 10);
	/* By default, PIXEL_RATE is read only */
	imx519->pixel_rate = v4l2_ctrl_new_std(ctrl_hdlr, &imx519_ctrl_ops,
					       V4L2_CID_PIXEL_RATE, pixel_rate,
					       pixel_rate, 1, pixel_rate);

	/* Initial vblank/hblank/exposure parameters based on current mode */
	mode = imx519->cur_mode;
	vblank_def = mode->fll_def - mode->height;
	vblank_min = mode->fll_min - mode->height;
	imx519->vblank = v4l2_ctrl_new_std(ctrl_hdlr, &imx519_ctrl_ops,
					   V4L2_CID_VBLANK, vblank_min,
					   IMX519_FLL_MAX - mode->height,
					   1, vblank_def);

	hblank = mode->llp - mode->width;
	imx519->hblank = v4l2_ctrl_new_std(ctrl_hdlr, &imx519_ctrl_ops,
					   V4L2_CID_HBLANK, hblank, hblank,
					   1, hblank);
	if (imx519->hblank)
		imx519->hblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	/* fll >= exposure time + adjust parameter (default value is 18) */
	exposure_max = mode->fll_def - 18;
	imx519->exposure = v4l2_ctrl_new_std(ctrl_hdlr, &imx519_ctrl_ops,
					     V4L2_CID_EXPOSURE,
					     IMX519_EXPOSURE_MIN, exposure_max,
					     IMX519_EXPOSURE_STEP,
					     IMX519_EXPOSURE_DEFAULT);

	imx519->hflip = v4l2_ctrl_new_std(ctrl_hdlr, &imx519_ctrl_ops,
					  V4L2_CID_HFLIP, 0, 1, 1, 0);
	imx519->vflip = v4l2_ctrl_new_std(ctrl_hdlr, &imx519_ctrl_ops,
					  V4L2_CID_VFLIP, 0, 1, 1, 0);

	v4l2_ctrl_new_std(ctrl_hdlr, &imx519_ctrl_ops, V4L2_CID_ANALOGUE_GAIN,
			  IMX519_ANA_GAIN_MIN, IMX519_ANA_GAIN_MAX,
			  IMX519_ANA_GAIN_STEP, IMX519_ANA_GAIN_DEFAULT);

	/* Digital gain */
	v4l2_ctrl_new_std(ctrl_hdlr, &imx519_ctrl_ops, V4L2_CID_DIGITAL_GAIN,
			  IMX519_DGTL_GAIN_MIN, IMX519_DGTL_GAIN_MAX,
			  IMX519_DGTL_GAIN_STEP, IMX519_DGTL_GAIN_DEFAULT);

	v4l2_ctrl_new_std_menu_items(ctrl_hdlr, &imx519_ctrl_ops,
				     V4L2_CID_TEST_PATTERN,
				     ARRAY_SIZE(imx519_test_pattern_menu) - 1,
				     0, 0, imx519_test_pattern_menu);
	if (ctrl_hdlr->error) {
		ret = ctrl_hdlr->error;
		dev_err(&client->dev, "control init failed: %d", ret);
		goto error;
	}

	imx519->sd.ctrl_handler = ctrl_hdlr;

	return 0;

error:
	v4l2_ctrl_handler_free(ctrl_hdlr);

	return ret;
}

static struct imx519_hwcfg *imx519_get_hwcfg(struct device *dev)
{
	struct imx519_hwcfg *cfg;
	struct v4l2_fwnode_endpoint bus_cfg = {
		.bus_type = V4L2_MBUS_CSI2_DPHY
	};
	struct fwnode_handle *ep;
	struct fwnode_handle *fwnode = dev_fwnode(dev);
	unsigned int i;
	int ret;

	if (!fwnode)
		return NULL;

	ep = fwnode_graph_get_next_endpoint(fwnode, NULL);
	if (!ep)
		return NULL;

	ret = v4l2_fwnode_endpoint_alloc_parse(ep, &bus_cfg);
	if (ret)
		goto out_err;

	cfg = devm_kzalloc(dev, sizeof(*cfg), GFP_KERNEL);
	if (!cfg)
		goto out_err;

	ret = fwnode_property_read_u32(dev_fwnode(dev), "clock-frequency",
				       &cfg->ext_clk);
	if (ret) {
		dev_err(dev, "can't get clock frequency");
		goto out_err;
	}

	dev_dbg(dev, "ext clk: %d", cfg->ext_clk);
	if (cfg->ext_clk != IMX519_EXT_CLK) {
		dev_err(dev, "external clock %d is not supported",
			cfg->ext_clk);
		goto out_err;
	}

	dev_dbg(dev, "num of link freqs: %d", bus_cfg.nr_of_link_frequencies);
	if (!bus_cfg.nr_of_link_frequencies) {
		dev_warn(dev, "no link frequencies defined");
		goto out_err;
	}

	cfg->nr_of_link_freqs = bus_cfg.nr_of_link_frequencies;
	cfg->link_freqs = devm_kcalloc(dev,
				       bus_cfg.nr_of_link_frequencies + 1,
				       sizeof(*cfg->link_freqs), GFP_KERNEL);
	if (!cfg->link_freqs)
		goto out_err;

	for (i = 0; i < bus_cfg.nr_of_link_frequencies; i++) {
		cfg->link_freqs[i] = bus_cfg.link_frequencies[i];
		dev_dbg(dev, "link_freq[%d] = %lld", i, cfg->link_freqs[i]);
	}

	v4l2_fwnode_endpoint_free(&bus_cfg);
	fwnode_handle_put(ep);
	return cfg;

out_err:
	v4l2_fwnode_endpoint_free(&bus_cfg);
	fwnode_handle_put(ep);
	return NULL;
}

static int imx519_probe(struct i2c_client *client)
{
	struct imx519 *imx519;
	int ret;
	u32 i;

	imx519 = devm_kzalloc(&client->dev, sizeof(*imx519), GFP_KERNEL);
	if (!imx519)
		return -ENOMEM;

	mutex_init(&imx519->mutex);

	/* Initialize subdev */
	v4l2_i2c_subdev_init(&imx519->sd, client, &imx519_subdev_ops);

	/* Check module identity */
	ret = imx519_identify_module(imx519);
	if (ret) {
		dev_err(&client->dev, "failed to find sensor: %d", ret);
		goto error_probe;
	}

	imx519->hwcfg = imx519_get_hwcfg(&client->dev);
	if (!imx519->hwcfg) {
		dev_err(&client->dev, "failed to get hwcfg");
		ret = -ENODEV;
		goto error_probe;
	}

	imx519->link_def_freq = link_freq_menu_items[IMX519_LINK_FREQ_INDEX];
	for (i = 0; i < imx519->hwcfg->nr_of_link_freqs; i++) {
		if (imx519->hwcfg->link_freqs[i] == imx519->link_def_freq) {
			dev_dbg(&client->dev, "link freq index %d matched", i);
			break;
		}
	}

	if (i == imx519->hwcfg->nr_of_link_freqs) {
		dev_err(&client->dev, "no link frequency supported");
		ret = -EINVAL;
		goto error_probe;
	}

	/* Set default mode to max resolution */
	imx519->cur_mode = &supported_modes[0];

	ret = imx519_init_controls(imx519);
	if (ret) {
		dev_err(&client->dev, "failed to init controls: %d", ret);
		goto error_probe;
	}

	/* Initialize subdev */
	imx519->sd.internal_ops = &imx519_internal_ops;
	imx519->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
		V4L2_SUBDEV_FL_HAS_EVENTS;
	imx519->sd.entity.ops = &imx519_subdev_entity_ops;
	imx519->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;

	/* Initialize source pad */
	imx519->pad.flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&imx519->sd.entity, 1, &imx519->pad);
	if (ret) {
		dev_err(&client->dev, "failed to init entity pads: %d", ret);
		goto error_handler_free;
	}

	ret = v4l2_async_register_subdev_sensor(&imx519->sd);
	if (ret < 0)
		goto error_media_entity;

	/*
	 * Device is already turned on by i2c-core with ACPI domain PM.
	 * Enable runtime PM and turn off the device.
	 */
	pm_runtime_set_active(&client->dev);
	pm_runtime_enable(&client->dev);
	pm_runtime_idle(&client->dev);

	return 0;

error_media_entity:
	media_entity_cleanup(&imx519->sd.entity);

error_handler_free:
	v4l2_ctrl_handler_free(imx519->sd.ctrl_handler);

error_probe:
	mutex_destroy(&imx519->mutex);

	return ret;
}

static int imx519_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx519 *imx519 = to_imx519(sd);

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	v4l2_ctrl_handler_free(sd->ctrl_handler);

	pm_runtime_disable(&client->dev);
	pm_runtime_set_suspended(&client->dev);

	mutex_destroy(&imx519->mutex);

	return 0;
}

static const struct dev_pm_ops imx519_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(imx519_suspend, imx519_resume)
};

static const struct acpi_device_id imx519_acpi_ids[] __maybe_unused = {
	{ "SONY319A" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(acpi, imx519_acpi_ids);

static struct i2c_driver imx519_i2c_driver = {
	.driver = {
		.name = "imx519",
		.pm = &imx519_pm_ops,
		.acpi_match_table = ACPI_PTR(imx519_acpi_ids),
	},
	.probe_new = imx519_probe,
	.remove = imx519_remove,
};
module_i2c_driver(imx519_i2c_driver);

MODULE_AUTHOR("Qiu, Tianshu <tian.shu.qiu@intel.com>");
MODULE_AUTHOR("Rapolu, Chiranjeevi <chiranjeevi.rapolu@intel.com>");
MODULE_AUTHOR("Bingbu Cao <bingbu.cao@intel.com>");
MODULE_AUTHOR("Yang, Hyungwoo <hyungwoo.yang@intel.com>");
MODULE_DESCRIPTION("Sony imx519 sensor driver");
MODULE_LICENSE("GPL v2");
