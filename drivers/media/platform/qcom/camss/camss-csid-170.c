// SPDX-License-Identifier: GPL-2.0
/*
 * camss-csid-4-7.c
 *
 * Qualcomm MSM Camera Subsystem - CSID (CSI Decoder) Module
 *
 * Copyright (C) 2020 Linaro Ltd.
 */
#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/of.h>

#include "camss-csid.h"
#include "camss-csid-gen2.h"
#include "camss.h"

/* The CSID 2 IP-block is different from the others,
 * and is of a bare-bones Lite version, with no PIX
 * interface support. As a result of that it has an
 * alternate register layout.
 */
#define IS_LITE		(csid->id == 2 ? 1 : 0)

#define CSID_HW_VERSION		0x0
#define		HW_VERSION_STEPPING	0
#define		HW_VERSION_REVISION	16
#define		HW_VERSION_GENERATION	28

#define CSID_RST_STROBES	0x10
#define		RST_STROBES	0

#define CSID_CSI2_RX_IRQ_STATUS	0x20
#define	CSID_CSI2_RX_IRQ_MASK	0x24
#define CSID_CSI2_RX_IRQ_CLEAR	0x28

#define CSID_CSI2_RDIN_IRQ_STATUS(rdi)		((IS_LITE ? 0x30 : 0x40) \
						 + 0x10 * (rdi))
#define CSID_CSI2_RDIN_IRQ_MASK(rdi)		((IS_LITE ? 0x34 : 0x44) \
						 + 0x10 * (rdi))
#define CSID_CSI2_RDIN_IRQ_CLEAR(rdi)		((IS_LITE ? 0x38 : 0x48) \
						 + 0x10 * (rdi))
#define CSID_CSI2_RDIN_IRQ_SET(rdi)		((IS_LITE ? 0x3C : 0x4C) \
						 + 0x10 * (rdi))

#define CSID_TOP_IRQ_STATUS	0x70
#define		TOP_IRQ_STATUS_RESET_DONE 0
#define CSID_TOP_IRQ_MASK	0x74
#define CSID_TOP_IRQ_CLEAR	0x78
#define CSID_TOP_IRQ_SET	0x7C
#define CSID_IRQ_CMD		0x80
#define		IRQ_CMD_CLEAR	0
#define		IRQ_CMD_SET	4

#define CSID_CSI2_RX_CFG0	0x100
#define		CSI2_RX_CFG0_NUM_ACTIVE_LANES	0
#define		CSI2_RX_CFG0_DL0_INPUT_SEL	4
#define		CSI2_RX_CFG0_DL1_INPUT_SEL	8
#define		CSI2_RX_CFG0_DL2_INPUT_SEL	12
#define		CSI2_RX_CFG0_DL3_INPUT_SEL	16
#define		CSI2_RX_CFG0_PHY_NUM_SEL	20
#define		CSI2_RX_CFG0_PHY_TYPE_SEL	24

#define CSID_CSI2_RX_CFG1	0x104
#define		CSI2_RX_CFG1_PACKET_ECC_CORRECTION_EN		0
#define		CSI2_RX_CFG1_DE_SCRAMBLE_EN			1
#define		CSI2_RX_CFG1_VC_MODE				2
#define		CSI2_RX_CFG1_COMPLETE_STREAM_EN			4
#define		CSI2_RX_CFG1_COMPLETE_STREAM_FRAME_TIMING	5
#define		CSI2_RX_CFG1_MISR_EN				6
#define		CSI2_RX_CFG1_CGC_MODE				7
#define			CGC_MODE_DYNAMIC_GATING		0
#define			CGC_MODE_ALWAYS_ON		1

#define CSID_RDI_CFG0(rdi)			((IS_LITE ? 0x200 : 0x300) \
						 + 0x100 * (rdi))
#define		RDI_CFG0_BYTE_CNTR_EN		0
#define		RDI_CFG0_FORMAT_MEASURE_EN	1
#define		RDI_CFG0_TIMESTAMP_EN		2
#define		RDI_CFG0_DROP_H_EN		3
#define		RDI_CFG0_DROP_V_EN		4
#define		RDI_CFG0_CROP_H_EN		5
#define		RDI_CFG0_CROP_V_EN		6
#define		RDI_CFG0_MISR_EN		7
#define		RDI_CFG0_CGC_MODE		8
#define			CGC_MODE_DYNAMIC	0
#define			CGC_MODE_ALWAYS_ON	1
#define		RDI_CFG0_PLAIN_ALIGNMENT	9
#define			PLAIN_ALIGNMENT_LSB	0
#define			PLAIN_ALIGNMENT_MSB	1
#define		RDI_CFG0_PLAIN_FORMAT		10
#define		RDI_CFG0_DECODE_FORMAT		12
#define		RDI_CFG0_DATA_TYPE		16
#define		RDI_CFG0_VIRTUAL_CHANNEL	22
#define		RDI_CFG0_DT_ID			27
#define		RDI_CFG0_EARLY_EOF_EN		29
#define		RDI_CFG0_PACKING_FORMAT		30
#define		RDI_CFG0_ENABLE			31

#define CSID_RDI_CFG1(rdi)			((IS_LITE ? 0x204 : 0x304)\
						+ 0x100 * (rdi))
#define		RDI_CFG1_TIMESTAMP_STB_SEL	0

#define CSID_RDI_CTRL(rdi)			((IS_LITE ? 0x208 : 0x308)\
						+ 0x100 * (rdi))
#define		RDI_CTRL_HALT_CMD		0
#define			ALT_CMD_RESUME_AT_FRAME_BOUNDARY	1
#define		RDI_CTRL_HALT_MODE		2

#define CSID_RDI_FRM_DROP_PATTERN(rdi)			((IS_LITE ? 0x20C : 0x30C)\
							+ 0x100 * (rdi))
#define CSID_RDI_FRM_DROP_PERIOD(rdi)			((IS_LITE ? 0x210 : 0x310)\
							+ 0x100 * (rdi))
#define CSID_RDI_IRQ_SUBSAMPLE_PATTERN(rdi)		((IS_LITE ? 0x214 : 0x314)\
							+ 0x100 * (rdi))
#define CSID_RDI_IRQ_SUBSAMPLE_PERIOD(rdi)		((IS_LITE ? 0x218 : 0x318)\
							+ 0x100 * (rdi))
#define CSID_RDI_RPP_PIX_DROP_PATTERN(rdi)		((IS_LITE ? 0x224 : 0x324)\
							+ 0x100 * (rdi))
#define CSID_RDI_RPP_PIX_DROP_PERIOD(rdi)		((IS_LITE ? 0x228 : 0x328)\
							+ 0x100 * (rdi))
#define CSID_RDI_RPP_LINE_DROP_PATTERN(rdi)		((IS_LITE ? 0x22C : 0x32C)\
							+ 0x100 * (rdi))
#define CSID_RDI_RPP_LINE_DROP_PERIOD(rdi)		((IS_LITE ? 0x230 : 0x330)\
							+ 0x100 * (rdi))

#define CSID_TPG_CTRL		0x600
#define		TPG_CTRL_TEST_EN		0
#define		TPG_CTRL_FS_PKT_EN		1
#define		TPG_CTRL_FE_PKT_EN		2
#define		TPG_CTRL_NUM_ACTIVE_LANES	4
#define		TPG_CTRL_CYCLES_BETWEEN_PKTS	8
#define		TPG_CTRL_NUM_TRAIL_BYTES	20

#define CSID_TPG_VC_CFG0	0x604
#define		TPG_VC_CFG0_VC_NUM			0
#define		TPG_VC_CFG0_NUM_ACTIVE_SLOTS		8
#define			NUM_ACTIVE_SLOTS_0_ENABLED	0
#define			NUM_ACTIVE_SLOTS_0_1_ENABLED	1
#define			NUM_ACTIVE_SLOTS_0_1_2_ENABLED	2
#define			NUM_ACTIVE_SLOTS_0_1_3_ENABLED	3
#define		TPG_VC_CFG0_LINE_INTERLEAVING_MODE	10
#define			INTELEAVING_MODE_INTERLEAVED	0
#define			INTELEAVING_MODE_ONE_SHOT	1
#define		TPG_VC_CFG0_NUM_FRAMES			16

#define CSID_TPG_VC_CFG1	0x608
#define		TPG_VC_CFG1_H_BLANKING_COUNT		0
#define		TPG_VC_CFG1_V_BLANKING_COUNT		12
#define		TPG_VC_CFG1_V_BLANK_FRAME_WIDTH_SEL	24

#define CSID_TPG_LFSR_SEED	0x60C

#define CSID_TPG_DT_n_CFG_0(n)	(0x610 + (n) * 0xC)
#define		TPG_DT_n_CFG_0_FRAME_HEIGHT	0
#define		TPG_DT_n_CFG_0_FRAME_WIDTH	16

#define CSID_TPG_DT_n_CFG_1(n)	(0x614 + (n) * 0xC)
#define		TPG_DT_n_CFG_1_DATA_TYPE	0
#define		TPG_DT_n_CFG_1_ECC_XOR_MASK	8
#define		TPG_DT_n_CFG_1_CRC_XOR_MASK	16

#define CSID_TPG_DT_n_CFG_2(n)	(0x618 + (n) * 0xC)
#define		TPG_DT_n_CFG_2_PAYLOAD_MODE		0
#define		TPG_DT_n_CFG_2_USER_SPECIFIED_PAYLOAD	4
#define		TPG_DT_n_CFG_2_ENCODE_FORMAT		16

#define CSID_TPG_COLOR_BARS_CFG	0x640
#define		TPG_COLOR_BARS_CFG_UNICOLOR_BAR_EN	0
#define		TPG_COLOR_BARS_CFG_UNICOLOR_BAR_SEL	4
#define		TPG_COLOR_BARS_CFG_SPLIT_EN		5
#define		TPG_COLOR_BARS_CFG_ROTATE_PERIOD	8

#define CSID_TPG_COLOR_BOX_CFG	0x644
#define		TPG_COLOR_BOX_CFG_MODE		0
#define		TPG_COLOR_BOX_PATTERN_SEL	2

static const struct csid_format csid_formats[] = {
	{
		MEDIA_BUS_FMT_UYVY8_2X8,
		DATA_TYPE_YUV422_8BIT,
		DECODE_FORMAT_UNCOMPRESSED_8_BIT,
		8,
		2,
	},
	{
		MEDIA_BUS_FMT_VYUY8_2X8,
		DATA_TYPE_YUV422_8BIT,
		DECODE_FORMAT_UNCOMPRESSED_8_BIT,
		8,
		2,
	},
	{
		MEDIA_BUS_FMT_YUYV8_2X8,
		DATA_TYPE_YUV422_8BIT,
		DECODE_FORMAT_UNCOMPRESSED_8_BIT,
		8,
		2,
	},
	{
		MEDIA_BUS_FMT_YVYU8_2X8,
		DATA_TYPE_YUV422_8BIT,
		DECODE_FORMAT_UNCOMPRESSED_8_BIT,
		8,
		2,
	},
	{
		MEDIA_BUS_FMT_SBGGR8_1X8,
		DATA_TYPE_RAW_8BIT,
		DECODE_FORMAT_UNCOMPRESSED_8_BIT,
		8,
		1,
	},
	{
		MEDIA_BUS_FMT_SGBRG8_1X8,
		DATA_TYPE_RAW_8BIT,
		DECODE_FORMAT_UNCOMPRESSED_8_BIT,
		8,
		1,
	},
	{
		MEDIA_BUS_FMT_SGRBG8_1X8,
		DATA_TYPE_RAW_8BIT,
		DECODE_FORMAT_UNCOMPRESSED_8_BIT,
		8,
		1,
	},
	{
		MEDIA_BUS_FMT_SRGGB8_1X8,
		DATA_TYPE_RAW_8BIT,
		DECODE_FORMAT_UNCOMPRESSED_8_BIT,
		8,
		1,
	},
	{
		MEDIA_BUS_FMT_SBGGR10_1X10,
		DATA_TYPE_RAW_10BIT,
		DECODE_FORMAT_UNCOMPRESSED_10_BIT,
		10,
		1,
	},
	{
		MEDIA_BUS_FMT_SGBRG10_1X10,
		DATA_TYPE_RAW_10BIT,
		DECODE_FORMAT_UNCOMPRESSED_10_BIT,
		10,
		1,
	},
	{
		MEDIA_BUS_FMT_SGRBG10_1X10,
		DATA_TYPE_RAW_10BIT,
		DECODE_FORMAT_UNCOMPRESSED_10_BIT,
		10,
		1,
	},
	{
		MEDIA_BUS_FMT_SRGGB10_1X10,
		DATA_TYPE_RAW_10BIT,
		DECODE_FORMAT_UNCOMPRESSED_10_BIT,
		10,
		1,
	},
	{
		MEDIA_BUS_FMT_Y10_1X10,
		DATA_TYPE_RAW_10BIT,
		DECODE_FORMAT_UNCOMPRESSED_10_BIT,
		10,
		1,
	},
	{
		MEDIA_BUS_FMT_SBGGR12_1X12,
		DATA_TYPE_RAW_12BIT,
		DECODE_FORMAT_UNCOMPRESSED_12_BIT,
		12,
		1,
	},
	{
		MEDIA_BUS_FMT_SGBRG12_1X12,
		DATA_TYPE_RAW_12BIT,
		DECODE_FORMAT_UNCOMPRESSED_12_BIT,
		12,
		1,
	},
	{
		MEDIA_BUS_FMT_SGRBG12_1X12,
		DATA_TYPE_RAW_12BIT,
		DECODE_FORMAT_UNCOMPRESSED_12_BIT,
		12,
		1,
	},
	{
		MEDIA_BUS_FMT_SRGGB12_1X12,
		DATA_TYPE_RAW_12BIT,
		DECODE_FORMAT_UNCOMPRESSED_12_BIT,
		12,
		1,
	},
	{
		MEDIA_BUS_FMT_SBGGR14_1X14,
		DATA_TYPE_RAW_14BIT,
		DECODE_FORMAT_UNCOMPRESSED_14_BIT,
		14,
		1,
	},
	{
		MEDIA_BUS_FMT_SGBRG14_1X14,
		DATA_TYPE_RAW_14BIT,
		DECODE_FORMAT_UNCOMPRESSED_14_BIT,
		14,
		1,
	},
	{
		MEDIA_BUS_FMT_SGRBG14_1X14,
		DATA_TYPE_RAW_14BIT,
		DECODE_FORMAT_UNCOMPRESSED_14_BIT,
		14,
		1,
	},
	{
		MEDIA_BUS_FMT_SRGGB14_1X14,
		DATA_TYPE_RAW_14BIT,
		DECODE_FORMAT_UNCOMPRESSED_14_BIT,
		14,
		1,
	},
};

static void csid_configure_stream(struct csid_device *csid, u8 enable)
{
	struct csid_testgen_config *tg = &csid->testgen;
	u32 val;
	u32 phy_sel = 0;
	u8 lane_cnt = csid->phy.lane_cnt;
	struct v4l2_mbus_framefmt *input_format = &csid->fmt[MSM_CSID_PAD_SRC];
	const struct csid_format *format = csid_get_fmt_entry(csid->formats, csid->nformats,
							      input_format->code);
	struct device *dev = csid->camss->dev;
	dev_info(dev, "%s:", __func__);

	if (!lane_cnt)
		lane_cnt = 4;

	if (!tg->enabled)
		phy_sel = csid->phy.csiphy_id;

	if (enable) {
		u8 vc = 0; /* Virtual Channel 0 */
		u8 dt_id = vc * 4;

		if (tg->enabled) {
			/* Config Test Generator */
			vc = 0xa;

			/* configure one DT, infinite frames */
			val = vc << TPG_VC_CFG0_VC_NUM;
			val |= INTELEAVING_MODE_ONE_SHOT << TPG_VC_CFG0_LINE_INTERLEAVING_MODE;
			val |= 0 << TPG_VC_CFG0_NUM_FRAMES;
			printk("\n%s CAMSS_CSID_TG_VC_CFG0\n", __func__);
			printk("%s 0:3   VC_NUM: %u\n", __func__, val & 0xF);
			printk("%s 8:9   NUM_ACTIVE_DTS: %u\n", __func__, (val & (0x3 << 8)) >> 8);
			printk("%s 10    LINE_INTERLEAVING_MODE: %u\n", __func__, (val & (0x1 << 10)) >> 10);
			printk("%s 16:23 NUM_FRAMES: %u\n", __func__, (val & (0xFF << 16)) >> 16);
			debug_writel(val, csid->base + CSID_TPG_VC_CFG0, csid->base_unmapped, csid->base);

			val = 0x740 << TPG_VC_CFG1_H_BLANKING_COUNT;
			val |= 0x3ff << TPG_VC_CFG1_V_BLANKING_COUNT;
			printk("\n%s CAMSS_CSID_TG_VC_CFG1\n", __func__);
			printk("%s 0:10  H_BLANKINK_COUNT: %u\n", __func__, val & 0x7FF);
			printk("%s 12:21 NUM_ACTIVE_DTS: %u\n", __func__, (val & (0x3FF << 12)) >> 12);
			printk("%s 24:25 LINE_INTERLEAVING_MODE: %u\n", __func__, (val & (0x3 << 24)) >> 24);
			debug_writel(val, csid->base + CSID_TPG_VC_CFG1, csid->base_unmapped, csid->base);

			// CAMSS_CSID_TG_LFSR_SEED @ 0x60C
			// 0:31  SEED
			debug_writel(0x12345678, csid->base + CSID_TPG_LFSR_SEED, csid->base_unmapped, csid->base);

			val = input_format->height & 0x1fff << TPG_DT_n_CFG_0_FRAME_HEIGHT;
			val |= input_format->width & 0x1fff << TPG_DT_n_CFG_0_FRAME_WIDTH;
			printk("\n%s CAMSS_CSID_TG_DT_n_CFG_0\n", __func__);
			printk("%s 0:14  FRAME_HEIGHT: %u\n", __func__, val & 0x3FFF);
			printk("%s 16:31 FRAME_WIDTH: %u\n", __func__, (val & (0xFFFF << 16)) >> 16);

			debug_writel(val, csid->base + CSID_TPG_DT_n_CFG_0(0), csid->base_unmapped, csid->base);

			val = DATA_TYPE_RAW_10BIT << TPG_DT_n_CFG_1_DATA_TYPE;
			printk("\n%s CAMSS_CSID_TG_DT_n_CFG_1\n", __func__);
			printk("%s 0:5   DATA_TYPE: %s (0x%x)\n", __func__, data_type_str(val & 0x3F), val & 0x3F);
			printk("%s 8:13  ECC_XOR_MASK: %u\n", __func__, (val & (0x3F << 8)) >> 8);
			printk("%s 16:21 CRC_XOR_MASK: %u\n", __func__, (val & (0xFFFF << 16)) >> 16);
			debug_writel(val, csid->base + CSID_TPG_DT_n_CFG_1(0), csid->base_unmapped, csid->base);

			val = tg->mode << TPG_DT_n_CFG_2_PAYLOAD_MODE;
			val |= 0xBE << TPG_DT_n_CFG_2_USER_SPECIFIED_PAYLOAD;
			val |= format->decode_format << TPG_DT_n_CFG_2_ENCODE_FORMAT;
			printk("\n%s CAMSS_CSID_TG_DT_n_CFG_2\n", __func__);
			printk("%s 0:3   PAYLOAD_MODE: %s\n", __func__, pattern_str(val & 0x7));
			printk("%s 4:11  USER_SPECIFIED_PAYLOAD: 0x%X\n", __func__, (val & (0xFF << 4)) >> 4);
			printk("%s 16:19 ENCODE_FORMAT: %s\n", __func__, encode_format_str((val & (0xF << 16)) >> 16));
			debug_writel(val, csid->base + CSID_TPG_DT_n_CFG_2(0), csid->base_unmapped, csid->base);

			debug_writel(0, csid->base + CSID_TPG_COLOR_BARS_CFG, csid->base_unmapped, csid->base);

			debug_writel(0, csid->base + CSID_TPG_COLOR_BOX_CFG, csid->base_unmapped, csid->base);
		}

		val = 1 << RDI_CFG0_BYTE_CNTR_EN;
		val |= 1 << RDI_CFG0_FORMAT_MEASURE_EN;
		val |= 1 << RDI_CFG0_TIMESTAMP_EN;
		val |= DECODE_FORMAT_PAYLOAD_ONLY << RDI_CFG0_DECODE_FORMAT;
		val |= DATA_TYPE_RAW_10BIT << RDI_CFG0_DATA_TYPE;
		val |= vc << RDI_CFG0_VIRTUAL_CHANNEL;
		val |= dt_id << RDI_CFG0_DT_ID;
		printk("\n%s CAMSS_CSID_RDI_CFG0\n", __func__);
		printk("%s 0     BYTE_CNTR_EN: %u\n", __func__, val & 0x1);
		printk("%s 1     FORMAT_MEASURE_EN: %u\n", __func__, (val & (0x1 << 1)) >> 1);
		printk("%s 2     TIMESTAMP_EN: %u\n", __func__, (val & (0x1 << 2)) >> 2);
		printk("%s 3     DROP_H_EN: %u\n", __func__, (val & (0x1 << 3)) >> 3);
		printk("%s 4     DROP_V_EN: %u\n", __func__, (val & (0x1 << 4)) >> 4);
		printk("%s 5     CROP_H_EN: %u\n", __func__, (val & (0x1 << 5)) >> 5);
		printk("%s 6     CROP_V_EN: %u\n", __func__, (val & (0x1 << 6)) >> 6);
		printk("%s 7     MISR_EN: %u\n", __func__, (val & (0x1 << 7)) >> 7);
		printk("%s 8     CGC_MODE: %s\n", __func__, ((val & (0x1 << 8)) >> 8) ? "ALWAYS_ON":"DYNAMIC_GATING");
		printk("%s 9     PLAIN_ALIGNMENT: %s\n", __func__, ((val & (0x1 << 9)) >> 9) ? "MSB":"LSB");
		printk("%s 10:11 PLAIN_FORMAT: %u\n", __func__, (val & (0x1 << 10)) >> 10);
		printk("%s 12:15 DECODE_FORMAT: %s\n", __func__, decode_format_str((val & (0xF << 12)) >> 12));
		printk("%s 16:21 DATA_TYPE: %s\n", __func__, data_type_str((val & (0x3F << 16)) >> 16));
		printk("%s 22:26 VIRTUAL_CHANNEL: %u\n", __func__, (val & (0x1F << 22)) >> 22);
		printk("%s 27:28 DT_ID: %u\n", __func__, (val & (0x3 << 27)) >> 27);
		printk("%s 31    ENABLE: %u\n", __func__, (val & (0x1 << 31)) >> 31);
		debug_writel(val, csid->base + CSID_RDI_CFG0(0), csid->base_unmapped, csid->base);

		/* CSID_TIMESTAMP_STB_POST_IRQ */
		val = 2 << RDI_CFG1_TIMESTAMP_STB_SEL;
		printk("\n%s CAMSS_CSID_RDI_CFG1\n", __func__);
		printk("%s 0:1   TIMESTAMP_STB_SEL: %u - %s\n", __func__, val & 0x3, val == 2 ? "TIMESTAMP_STB_POST_IRQ":"");
		debug_writel(val, csid->base + CSID_RDI_CFG1(0), csid->base_unmapped, csid->base);

		val = 1;
		printk("\n%s CAMSS_CSID_RDI_FRM_DROP_PERIOD\n", __func__);
		printk("%s 0:4   PERIOD: %u\n", __func__, val & 0x1F);
		debug_writel(val, csid->base + CSID_RDI_FRM_DROP_PERIOD(0), csid->base_unmapped, csid->base);

		val = 0;
		printk("\n%s CAMSS_CSID_RDI_FRM_DROP_PATTERN\n", __func__);
		printk("%s 0:31  PATTERN: %u\n", __func__, val);
		debug_writel(0, csid->base + CSID_RDI_FRM_DROP_PATTERN(0), csid->base_unmapped, csid->base);

		val = 1;
		printk("\n%s CAMSS_CSID_RDI_IRQ_SUBSAMPLE_PERIOD\n", __func__);
		printk("%s 0:4   PERIOD: %u\n", __func__, val  & 0x1F);
		debug_writel(val, csid->base + CSID_RDI_IRQ_SUBSAMPLE_PERIOD(0), csid->base_unmapped, csid->base);

		val = 0;
		printk("\n%s CAMSS_CSID_RDI_IRQ_SUBSAMPLE_PATTERN\n", __func__);
		printk("%s 0:31  PATTERN: %u\n", __func__, val);
		debug_writel(val, csid->base + CSID_RDI_IRQ_SUBSAMPLE_PATTERN(0), csid->base_unmapped, csid->base);

		val = 1;
		printk("\n%s CAMSS_CSID_RDI_RPP_PIX_DROP_PERIOD\n", __func__);
		printk("%s 0:4  PERIOD: %u\n", __func__, val & 0x1F);
		debug_writel(val, csid->base + CSID_RDI_RPP_PIX_DROP_PERIOD(0), csid->base_unmapped, csid->base);

		val = 0;
		printk("\n%s CAMSS_CSID_RDI_RPP_PIX_DROP_PATTERN\n", __func__);
		printk("%s 0:31  PATTERN: %u\n", __func__, val);
		debug_writel(val, csid->base + CSID_RDI_RPP_PIX_DROP_PATTERN(0), csid->base_unmapped, csid->base);

		val = 1;
		printk("\n%s CAMSS_CSID_RDI_RPP_LINE_DROP_PERIOD\n", __func__);
		printk("%s 0:4  PERIOD: %u\n", __func__, val & 0x1F);
		debug_writel(val, csid->base + CSID_RDI_RPP_LINE_DROP_PERIOD(0), csid->base_unmapped, csid->base);

		val = 0;
		printk("\n%s CAMSS_CSID_RDI_RPP_LINE_DROP_PATTERN\n", __func__);
		printk("%s 0:31  PATTERN: %u\n", __func__, val);
		debug_writel(val, csid->base + CSID_RDI_RPP_LINE_DROP_PATTERN(0), csid->base_unmapped, csid->base);

		val = 0;
		printk("\n%s CAMSS_CSID_RDI_CTRL\n", __func__);
		printk("%s 0:1    HALT_CMD: %u\n", __func__, val & 0x3);
		printk("%s 2      HALT_MODE: %u\n", __func__, (val >> 2) & 0x1);
		debug_writel(val, csid->base + CSID_RDI_CTRL(0), csid->base_unmapped, csid->base);

		val = readl_relaxed(csid->base + CSID_RDI_CFG0(0));
		val |=  1 << RDI_CFG0_ENABLE;
		printk("\n%s Read back CAMSS_CSID_RDI_CFG0\n", __func__);
		printk("%s 0     BYTE_CNTR_EN: %u\n", __func__, val & 0x1);
		printk("%s 1     FORMAT_MEASURE_EN: %u\n", __func__, (val & (0x1 << 1)) >> 1);
		printk("%s 2     TIMESTAMP_EN: %u\n", __func__, (val & (0x1 << 2)) >> 2);
		printk("%s 3     DROP_H_EN: %u\n", __func__, (val & (0x1 << 3)) >> 3);
		printk("%s 4     DROP_V_EN: %u\n", __func__, (val & (0x1 << 4)) >> 4);
		printk("%s 5     CROP_H_EN: %u\n", __func__, (val & (0x1 << 5)) >> 5);
		printk("%s 6     CROP_V_EN: %u\n", __func__, (val & (0x1 << 6)) >> 6);
		printk("%s 7     MISR_EN: %u\n", __func__, (val & (0x1 << 7)) >> 7);
		printk("%s 8     CGC_MODE: %s\n", __func__, ((val & (0x1 << 8)) >> 8) ? "ALWAYS_ON":"DYNAMIC_GATING");
		printk("%s 9     PLAIN_ALIGNMENT: %s\n", __func__, ((val & (0x1 << 9)) >> 9) ? "MSB":"LSB");
		printk("%s 10:11 PLAIN_FORMAT: %s (0x%X)\n", __func__, plain_format_str((val >> 10) & 0x3), val >> 10 & 0x3);
		printk("%s 12:15 DECODE_FORMAT: %s (0x%X)\n", __func__, decode_format_str(val >> 12 & 0xF), val >> 12 & 0xF);
		printk("%s 16:21 DATA_TYPE: %s (0x%X)\n", __func__, data_type_str((val >> 16) & 0x3F), (val >> 16) & 0x3F);
		printk("%s 22:26 VIRTUAL_CHANNEL: %u\n", __func__, (val & (0x1F << 22)) >> 22);
		printk("%s 27:28 DT_ID: %u\n", __func__, (val & (0x3 << 27)) >> 27);
		printk("%s 29    EARLY_EOF_EN: %u\n", __func__, (val >> 29) & 0x1);
		printk("%s 30    PACKING_FORMAT: %s (0x%X)\n", __func__, (val >> 30 & 0x1) ? "MIPI" : "PLAIN", val >> 30 & 0x1);
		printk("%s 31    ENABLE: %u\n", __func__, (val & (0x1 << 31)) >> 31);
		debug_writel(val, csid->base + CSID_RDI_CFG0(0), csid->base_unmapped, csid->base);
	}

	if (tg->enabled) {
		val = enable << TPG_CTRL_TEST_EN;
		val |= 1 << TPG_CTRL_FS_PKT_EN;
		val |= 1 << TPG_CTRL_FE_PKT_EN;
		val |= (lane_cnt - 1) << TPG_CTRL_NUM_ACTIVE_LANES;
		val |= 0x64 << TPG_CTRL_CYCLES_BETWEEN_PKTS;
		val |= 0xA << TPG_CTRL_NUM_TRAIL_BYTES;
		debug_writel(val, csid->base + CSID_TPG_CTRL, csid->base_unmapped, csid->base);
	}

	val = (lane_cnt - 1) << CSI2_RX_CFG0_NUM_ACTIVE_LANES;
	val |= csid->phy.lane_assign << CSI2_RX_CFG0_DL0_INPUT_SEL;
	val |= phy_sel << CSI2_RX_CFG0_PHY_NUM_SEL;
	printk("\n%s CAMSS_CSID_CSI2_RX_CFG0\n", __func__);
	printk("%s 0:1   NUM_ACTIVE_LANES: %u\n", __func__, val & 0x3);
	printk("%s 4:5   DL0_INPUT_SEL: %u\n", __func__, (val & (0x3 << 4)) >> 4);
	printk("%s 8:9   DL1_INPUT_SEL: %u\n", __func__, (val & (0x3 << 8)) >> 8);
	printk("%s 12:13 DL2_INPUT_SEL: %u\n", __func__, (val & (0x3 << 12)) >> 12);
	printk("%s 16:17 DL3_INPUT_SEL: %u\n", __func__, (val & (0x3 << 16)) >> 16);
	printk("%s 20:21 PHY_NUM_SEL: %u\n", __func__, (val & (0x3 << 20)) >> 20);
	printk("%s 24    PHY_TYPE_SEL: %s\n", __func__, ((val & (0x1 << 24)) >> 24) ? "CPHY":"DPHY");
	debug_writel(val, csid->base + CSID_CSI2_RX_CFG0, csid->base_unmapped, csid->base);

	val = 1 << CSI2_RX_CFG1_PACKET_ECC_CORRECTION_EN;
	val |= 1 << CSI2_RX_CFG1_MISR_EN;
	printk("\n%s CAMSS_CSID_CSI2_RX_CFG1\n", __func__);
	printk("%s 0     PACKET_ECC_CORRECTION_EN: %u\n", __func__, val & 0x1);
	printk("%s 1     DE_SCRAMBLE_EN: %u\n", __func__, (val & (0x1 << 1)) >> 1);
	printk("%s 2     VC_MODE: %u\n", __func__, (val & (0x1 << 2)) >> 2);
	printk("%s 4     COMPLETE_STREAM_EN: %u\n", __func__, (val & (0x1 << 4)) >> 4);
	printk("%s 5     COMPLETE_STREAM_FRAME_TIMING: %u\n", __func__, (val & (0x1 << 5)) >> 5);
	printk("%s 6     MISR_EN: %u\n", __func__, (val & (0x1 << 6)) >> 6);
	printk("%s 7     CGC_MODE: %s\n", __func__, ((val & (0x1 << 7)) >> 7) ? "ALWAYS_ON":"DYNAMIC_GATING");
	debug_writel(val, csid->base + CSID_CSI2_RX_CFG1, csid->base_unmapped, csid->base); // csi2_vc_mode_shift_val ?

	/* error irqs start at BIT(11) */
	debug_writel(~0u, csid->base + CSID_CSI2_RX_IRQ_MASK, csid->base_unmapped, csid->base);

	/* RDI irq */
	debug_writel(~0u, csid->base + CSID_TOP_IRQ_MASK, csid->base_unmapped, csid->base);

	val = 1 << RDI_CTRL_HALT_CMD;
	debug_writel(val, csid->base + CSID_RDI_CTRL(0), csid->base_unmapped, csid->base);
}

static int csid_configure_testgen_pattern(struct csid_device *csid, s32 val)
{
	if (val > 0 && val <= csid->testgen.nmodes)
		csid->testgen.mode = val;

	return 0;
}

/*
 * csid_hw_version - CSID hardware version query
 * @csid: CSID device
 *
 * Return HW version or error
 */
static u32 csid_hw_version(struct csid_device *csid)
{
	u32 hw_version;
	u32 hw_gen;
	u32 hw_rev;
	u32 hw_step;

	hw_version = readl_relaxed(csid->base + CSID_HW_VERSION);
	hw_gen = (hw_version >> HW_VERSION_GENERATION) & 0xF;
	hw_rev = (hw_version >> HW_VERSION_REVISION) & 0xFFF;
	hw_step = (hw_version >> HW_VERSION_STEPPING) & 0xFFFF;
	dev_info(csid->camss->dev, "CSID HW Version = %u.%u.%u\n",
		hw_gen, hw_rev, hw_step);

	return hw_version;
}

/*
 * csid_isr - CSID module interrupt service routine
 * @irq: Interrupt line
 * @dev: CSID device
 *
 * Return IRQ_HANDLED on success
 */
static irqreturn_t csid_isr(int irq, void *dev)
{
	struct csid_device *csid = dev;
	u32 val;
	u8 reset_done;

	val = readl_relaxed(csid->base + CSID_TOP_IRQ_STATUS);
	debug_writel(val, csid->base + CSID_TOP_IRQ_CLEAR, csid->base_unmapped, csid->base);
	reset_done = val & BIT(TOP_IRQ_STATUS_RESET_DONE);

	dev_info(csid->camss->dev, "csid_isr(), irq_status = 0x%x, reset_done = %d", val, reset_done);
	reset_done = 1;

	val = readl_relaxed(csid->base + CSID_CSI2_RX_IRQ_STATUS);
	debug_writel(val, csid->base + CSID_CSI2_RX_IRQ_CLEAR, csid->base_unmapped, csid->base);

	val = readl_relaxed(csid->base + CSID_CSI2_RDIN_IRQ_STATUS(0));
	debug_writel(val, csid->base + CSID_CSI2_RDIN_IRQ_CLEAR(0), csid->base_unmapped, csid->base);

	val = 1 << IRQ_CMD_CLEAR;
	debug_writel(val, csid->base + CSID_IRQ_CMD, csid->base_unmapped, csid->base);

	if (reset_done)
		complete(&csid->reset_complete);

	return IRQ_HANDLED;
}

/*
 * csid_reset - Trigger reset on CSID module and wait to complete
 * @csid: CSID device
 *
 * Return 0 on success or a negative error code otherwise
 */
static int csid_reset(struct csid_device *csid)
{
	unsigned long time;
	u32 val;

	dev_info(csid->camss->dev, "%s", __func__);

	reinit_completion(&csid->reset_complete);

	debug_writel(1, csid->base + CSID_TOP_IRQ_CLEAR, csid->base_unmapped, csid->base);
	debug_writel(1, csid->base + CSID_IRQ_CMD, csid->base_unmapped, csid->base);
	debug_writel(1, csid->base + CSID_TOP_IRQ_MASK, csid->base_unmapped, csid->base);
	debug_writel(1, csid->base + CSID_IRQ_CMD, csid->base_unmapped, csid->base);

	/* preserve registers */
	val = 0x1e << RST_STROBES;
	debug_writel(val, csid->base + CSID_RST_STROBES, csid->base_unmapped, csid->base);

	time = wait_for_completion_timeout(&csid->reset_complete,
					   msecs_to_jiffies(CSID_RESET_TIMEOUT_MS));
	if (!time) {
		dev_err(csid->camss->dev, "CSID reset timeout\n");
		return -EIO;
	}

	return 0;
}

static u32 csid_src_pad_code(struct csid_device *csid, u32 sink_code,
			     unsigned int match_format_idx, u32 match_code)
{
	switch (sink_code) {
	case MEDIA_BUS_FMT_SBGGR10_1X10:
	{
		u32 src_code[] = {
			MEDIA_BUS_FMT_SBGGR10_1X10,
			MEDIA_BUS_FMT_SBGGR10_2X8_PADHI_LE,
		};

		return csid_find_code(src_code, ARRAY_SIZE(src_code),
				      match_format_idx, match_code);
	}
	case MEDIA_BUS_FMT_Y10_1X10:
	{
		u32 src_code[] = {
			MEDIA_BUS_FMT_Y10_1X10,
			MEDIA_BUS_FMT_Y10_2X8_PADHI_LE,
		};

		return csid_find_code(src_code, ARRAY_SIZE(src_code),
				      match_format_idx, match_code);
	}
	default:
		if (match_format_idx > 0)
			return 0;

		return sink_code;
	}
}

static void csid_subdev_init(struct csid_device *csid)
{
	csid->formats = csid_formats;
	csid->nformats = ARRAY_SIZE(csid_formats);
	csid->testgen.modes = csid_testgen_modes;
	csid->testgen.nmodes = CSID_PAYLOAD_MODE_NUM_SUPPORTED_GEN2;
}

const struct csid_hw_ops csid_ops_170 = {
	.configure_stream = csid_configure_stream,
	.configure_testgen_pattern = csid_configure_testgen_pattern,
	.hw_version = csid_hw_version,
	.isr = csid_isr,
	.reset = csid_reset,
	.src_pad_code = csid_src_pad_code,
	.subdev_init = csid_subdev_init,
};
