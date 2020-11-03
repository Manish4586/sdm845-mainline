/* SPDX-License-Identifier: GPL-2.0 */
/*
 * camss-video.h
 *
 * Qualcomm MSM Camera Subsystem - V4L2 device node
 *
 * Copyright (c) 2013-2015, The Linux Foundation. All rights reserved.
 * Copyright (C) 2015-2018 Linaro Ltd.
 */
#ifndef QC_MSM_CAMSS_VIDEO_H
#define QC_MSM_CAMSS_VIDEO_H

#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <media/media-entity.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-mediabus.h>
#include <media/videobuf2-v4l2.h>

#define DATA_TYPE_EMBEDDED_DATA_8BIT	0x12
#define DATA_TYPE_YUV420_8BIT		0x18
#define DATA_TYPE_YUV420_10BIT		0x19
#define DATA_TYPE_YUV420_8BIT_LEGACY	0x1a
#define DATA_TYPE_YUV420_8BIT_SHIFTED	0x1c /* Chroma Shifted Pixel Sampling */
#define DATA_TYPE_YUV420_10BIT_SHIFTED	0x1d /* Chroma Shifted Pixel Sampling */
#define DATA_TYPE_YUV422_8BIT		0x1e
#define DATA_TYPE_YUV422_10BIT		0x1f
#define DATA_TYPE_RGB444		0x20
#define DATA_TYPE_RGB555		0x21
#define DATA_TYPE_RGB565		0x22
#define DATA_TYPE_RGB666		0x23
#define DATA_TYPE_RGB888		0x24
#define DATA_TYPE_RAW_24BIT		0x27
#define DATA_TYPE_RAW_6BIT		0x28
#define DATA_TYPE_RAW_7BIT		0x29
#define DATA_TYPE_RAW_8BIT		0x2a
#define DATA_TYPE_RAW_10BIT		0x2b
#define DATA_TYPE_RAW_12BIT		0x2c
#define DATA_TYPE_RAW_14BIT		0x2d
#define DATA_TYPE_RAW_16BIT		0x2e
#define DATA_TYPE_RAW_20BIT		0x2f

#define DECODE_FORMAT_UNCOMPRESSED_6_BIT	0x0
#define DECODE_FORMAT_UNCOMPRESSED_8_BIT	0x1
#define DECODE_FORMAT_UNCOMPRESSED_10_BIT	0x2
#define DECODE_FORMAT_UNCOMPRESSED_12_BIT	0x3
#define DECODE_FORMAT_UNCOMPRESSED_14_BIT	0x4
#define DECODE_FORMAT_UNCOMPRESSED_16_BIT	0x5
#define DECODE_FORMAT_UNCOMPRESSED_20_BIT	0x6
#define DECODE_FORMAT_DPCM_10_6_10		0x7
#define DECODE_FORMAT_DPCM_10_8_10		0x8
#define DECODE_FORMAT_DPCM_12_6_12		0x9
#define DECODE_FORMAT_DPCM_12_8_12		0xA
#define DECODE_FORMAT_DPCM_14_8_14		0xB
#define DECODE_FORMAT_DPCM_14_10_14		0xC
#define DECODE_FORMAT_USER_DEFINED		0xE
#define DECODE_FORMAT_PAYLOAD_ONLY		0xF

#define ENCODE_FORMAT_RAW_8_BIT		0x1
#define ENCODE_FORMAT_RAW_10_BIT	0x2
#define ENCODE_FORMAT_RAW_12_BIT	0x3
#define ENCODE_FORMAT_RAW_14_BIT	0x4
#define ENCODE_FORMAT_RAW_16_BIT	0x5

#define PLAIN_FORMAT_PLAIN8	0x0 /* supports DPCM, UNCOMPRESSED_6/8_BIT */
#define PLAIN_FORMAT_PLAIN16	0x1 /* supports DPCM, UNCOMPRESSED_10/16_BIT */
#define PLAIN_FORMAT_PLAIN32	0x2 /* supports UNCOMPRESSED_20_BIT */

static const char* plain_format_str(unsigned int plain_fmt) {
	switch (plain_fmt) {
	case PLAIN_FORMAT_PLAIN8: return "PLAIN8 (for UNCOMPRESSED_6/8_BIT)";
	case PLAIN_FORMAT_PLAIN16: return "PLAIN16 (for DPCM & UNCOMPRESSED_10/16_BIT)";
	case PLAIN_FORMAT_PLAIN32: return "PLAIN32 (UNCOMPRESSED_20_BIT)";
	default: return "UKNOWN";
	}
}

static const char* data_type_str(unsigned int dt) {
	switch (dt) {
	case DATA_TYPE_EMBEDDED_DATA_8BIT: return "DATA_TYPE_EMBEDDED_DATA_8BIT";
	case DATA_TYPE_YUV422_8BIT: return "DATA_TYPE_YUV422_8BIT";
	case DATA_TYPE_RAW_6BIT: return "DATA_TYPE_RAW_6BIT";
	case DATA_TYPE_RAW_8BIT: return "DATA_TYPE_YUV422_8BIT";
	case DATA_TYPE_RAW_10BIT: return "DATA_TYPE_RAW_10BIT";
	case DATA_TYPE_RAW_12BIT: return "DATA_TYPE_RAW_12BIT";
	case DATA_TYPE_RAW_14BIT: return "DATA_TYPE_RAW_14BIT";
	default: return "UKNOWN";
	}
}

static const char* decode_format_str(unsigned int format) {
	switch (format) {
	case DECODE_FORMAT_UNCOMPRESSED_6_BIT: return "DECODE_FORMAT_UNCOMPRESSED_6_BIT";
	case DECODE_FORMAT_UNCOMPRESSED_8_BIT: return "DECODE_FORMAT_UNCOMPRESSED_8_BIT";
	case DECODE_FORMAT_UNCOMPRESSED_10_BIT: return "DECODE_FORMAT_UNCOMPRESSED_10_BIT";
	case DECODE_FORMAT_UNCOMPRESSED_12_BIT: return "DECODE_FORMAT_UNCOMPRESSED_12_BIT";
	case DECODE_FORMAT_UNCOMPRESSED_14_BIT: return "DECODE_FORMAT_UNCOMPRESSED_14_BIT";
	case DECODE_FORMAT_UNCOMPRESSED_16_BIT: return "DECODE_FORMAT_UNCOMPRESSED_16_BIT";
	case DECODE_FORMAT_UNCOMPRESSED_20_BIT: return "DECODE_FORMAT_UNCOMPRESSED_20_BIT";
	case DECODE_FORMAT_DPCM_10_6_10: return "DECODE_FORMAT_DPCM_10_6_10";
	case DECODE_FORMAT_DPCM_10_8_10: return "DECODE_FORMAT_DPCM_10_8_10";
	case DECODE_FORMAT_DPCM_12_6_12: return "DECODE_FORMAT_DPCM_12_6_12";
	case DECODE_FORMAT_DPCM_12_8_12: return "DECODE_FORMAT_DPCM_12_8_12";
	case DECODE_FORMAT_DPCM_14_8_14: return "DECODE_FORMAT_DPCM_14_8_14";
	case DECODE_FORMAT_DPCM_14_10_14: return "DECODE_FORMAT_DPCM_14_10_14";
	case DECODE_FORMAT_USER_DEFINED: return "DECODE_FORMAT_USER_DEFINED (jpeg, mpg4, arbitary data)";
	case DECODE_FORMAT_PAYLOAD_ONLY: return "DECODE_FORMAT_PAYLOAD_ONLY (no processing, data sent directly out)";
	default: return "UKNOWN";
	}
}

static const char* encode_format_str(unsigned int format) {
	switch (format) {
	case ENCODE_FORMAT_RAW_8_BIT: return "ENCODE_FORMAT_RAW_8_BIT";
	case ENCODE_FORMAT_RAW_10_BIT: return "ENCODE_FORMAT_RAW_10_BIT";
	case ENCODE_FORMAT_RAW_12_BIT: return "ENCODE_FORMAT_RAW_12_BIT";
	case ENCODE_FORMAT_RAW_14_BIT: return "ENCODE_FORMAT_RAW_14_BIT";
	case ENCODE_FORMAT_RAW_16_BIT: return "ENCODE_FORMAT_RAW_16_BIT";
	default: return "UKNOWN";
	}
}

static const char* pattern_str(unsigned int pattern) {
	switch (pattern) {
	case 0: return "INCREMENTING";
	case 1: return "ALTERNATING_55_AA";
	case 2: return "ALL_ZEROES";
	case 3: return "ALL_ONES";
	case 4: return "RANDOM";
	case 5: return "USER_SPECIFIED";
	case 6: return "COMPLEX_PATTERN";
	case 7: return "COLOR_BOX";
	case 8: return "COLOR_BARS";
	default: return "UKNOWN";
	}
}

static void dumpreg(const char *core, u32 reg, u32 val)
{
	printk("%s %s @ 0x%x: ??? <- 0x%08X\n", __func__, core, reg, val);
}


static void dumpreg_csid(const char *core, u32 reg, u32 val)
{
	switch (reg){

		case 0x0:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_HW_VERSION <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:15  GENERATION: %u\n", __func__, val & 0xFFFF);
			printk("%s 16:27 GENERATION: %u\n", __func__, (val >> 16) & 0xFFF);
			printk("%s 28:31 GENERATION: %u\n", __func__, (val >> 28) & 0xF);
			break;

		case 0x10:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RST_STROBES <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x24:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_CSI2_RX_IRQ_MASK <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x28:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_CSI2_RX_IRQ_CLEAR <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x34:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IPP_IRQ_MASK <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x38:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IPP_IRQ_CLEAR <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x44:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI#0_IRQ_MASK <- 0x%08X\n", __func__, core, reg, val);
			break;
		case 0x44 + 0x10:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI#1_IRQ_MASK <- 0x%08X\n", __func__, core, reg, val);
			break;
		case 0x44 + 0x20:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI#2_IRQ_MASK <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x48:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI#0_IRQ_CLEAR <- 0x%08X\n", __func__, core, reg, val);
			break;
		case 0x48 + 0x10:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI#1_IRQ_CLEAR <- 0x%08X\n", __func__, core, reg, val);
			break;
		case 0x48 + 0x20:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI#2_IRQ_CLEAR <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x70:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IRQ_STATUS <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x74:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IRQ_MASK <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x78:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IRQ_CLEAR_CMD <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x80:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IRQ_CMD <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0     CLEAR: %u\n", __func__, val & 0x1);
			printk("%s 4     SET: %u\n", __func__, val >> 4 & 0x1);
			break;

		case 0x100:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_CSI2_RX_CFG0 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:1   NUM_ACTIVE_LANES: %u\n", __func__, val & 0x3);
			printk("%s 4:5   DL0_INPUT_SEL: %u\n", __func__, (val >> 4) & 0x3);
			printk("%s 8:9   DL1_INPUT_SEL: %u\n", __func__, (val >> 8) & 0x3);
			printk("%s 12:13 DL2_INPUT_SEL: %u\n", __func__, (val >> 12) & 0x3);
			printk("%s 16:17 DL3_INPUT_SEL: %u\n", __func__, (val >> 16) & 0x3);
			printk("%s 20:22 PHY_NUM_SEL: %u\n", __func__, (val >> 20) & 0x7);
			printk("%s 24    PHY_TYPE_SEL: %s\n", __func__, (val >> 24 & 0x1) ? "CPHY":"DPHY");
			break;

		case 0x104:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_CSI2_RX_CFG1 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0     PACKET_ECC_CORRECTION_EN: %u\n", __func__, val & 0x1);
			printk("%s 1     DE_SCRAMBLE_EN: %u\n", __func__, (val >> 1) & 0x1);
			printk("%s 2     VC_MODE: %u\n", __func__, (val >> 2) & 0x1);
			printk("%s 4     COMPLETE_STREAM_EN: %u\n", __func__, (val >> 4) & 0x1);
			printk("%s 5     COMPLETE_STREAM_FRAME_TIMING: %u\n", __func__, (val >> 5) & 0x1);
			printk("%s 6     MISR_EN: %u\n", __func__, (val >> 6) & 0x1);
			printk("%s 7     CGC_MODE: %s\n", __func__, ((val >> 7) & 0x1) ? "ALWAYS_ON":"DYNAMIC_GATING" );
			break;

		case 0x108:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_CSI2_RX_CAPTURE_CTRL <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0     LONG_PKT_CAPTURE_EN: %u\n", __func__, val & 0x1);
			printk("%s 1     SHORT_PKT_CAPTURE_EN: %u\n", __func__, (val >> 1) & 0x1);
			printk("%s 2     CPHY_PKT_CAPTURE_EN: %u\n", __func__, (val >> 2) & 0x1);
			printk("%s 4:14  LONG_PKT_CAPTURE_VC_DT: 0x%X\n", __func__, (val >> 4) & 0x3F);
			printk("%s 15:19 SHORT_PKT_CAPTURE_VC_DT: 0x%X\n", __func__, (val >> 15) & 0x1F);
			printk("%s 20:30 CPHY_PKT_CAPTURE_VC_DT: 0x%X\n", __func__, (val >> 20) & 0x7FF);
			break;

		case 0x200:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IPP_CFG0 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0     FORMAT_MEASURE_EN: %u\n", __func__, val & 0x1);
			printk("%s 1     TIMESTAMP_EN: %u\n", __func__, (val & (0x1 << 1)) >> 1 );
			printk("%s 2     BIN_EN: %u\n", __func__, (val & (0x1 << 2)) >> 2 );
			printk("%s 3     DROP_H_EN: %u\n", __func__, (val & (0x1 << 3)) >> 3 );
			printk("%s 4     DROP_V_EN: %u\n", __func__, (val & (0x1 << 4)) >> 4 );
			printk("%s 5     CROP_H_EN: %u\n", __func__, (val & (0x1 << 5)) >> 5 );
			printk("%s 6     CROP_V_EN: %u\n", __func__, (val & (0x1 << 6)) >> 6 );
			printk("%s 7     PIX_STORE_EN: %u\n", __func__, (val & (0x1 << 7)) >> 7 );
			printk("%s 8     MISR_EN: %u\n", __func__, (val & (0x1 << 8)) >> 8 );
			printk("%s 9     CGC_MODE: %s\n", __func__, ((val & (0x1 << 9)) >> 9) ? "ALWAYS_ON":"DYNAMIC_GATING" );
			printk("%s 12:15 DECODE_FORMAT: %s (0x%X)\n", __func__, decode_format_str(val >> 12 & 0xF), val >> 12 & 0xF);
			printk("%s 16:21 DATA_TYPE: %s (0x%X)\n", __func__, data_type_str((val >> 16) & 0x3F), (val >> 16) & 0x3F);
			printk("%s 22:26 VIRTUAL_CHANNEL: %u\n", __func__, (val & (0x1F << 22)) >> 22 );
			printk("%s 27:28 DT_ID: %u\n", __func__, (val & (0x3 << 27)) >> 27 );
			printk("%s 31    ENABLE: %u\n", __func__, (val & (0x1 << 31)) >> 31 );
			break;

		case 0x204:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IPP_CFG1 <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x208:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IPP_CTRL <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:1   HALT_CMD: 0x%0X\n", __func__, val & 0x3);
			printk("%s 2:3   HALT_MODE: 0x%0X\n", __func__, val >> 2 & 0x3);
			printk("%s 4:5   HALT_MASTER_SEL: 0x%0X\n", __func__, val >> 4 & 0x3);
			break;

		case 0x20c:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IPP_FRAME_DROP_PATTERN <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x210:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IPP_FRAME_DROP_PERIOD <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x214:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IPP_IRQ_SUBSAMPLE_PATTERN <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x218:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IPP_IRQ_SUBSAMPLE_PERIOD <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x224:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IPP_PIX_DROP_PATTERN <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x228:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IPP_PIX_DROP_PERIOD <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x22c:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IPP_LINE_DROP_PATTERN <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x230:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IPP_LINE_DROP_PEDIOD <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x300:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_CFG0 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0     BYTE_CNTR_EN: %u\n", __func__, val & 0x1);
			printk("%s 1     FORMAT_MEASURE_EN: %u\n", __func__, (val & (0x1 << 1)) >> 1 );
			printk("%s 2     TIMESTAMP_EN: %u\n", __func__, (val & (0x1 << 2)) >> 2 );
			printk("%s 3     DROP_H_EN: %u\n", __func__, (val & (0x1 << 3)) >> 3 );
			printk("%s 4     DROP_V_EN: %u\n", __func__, (val & (0x1 << 4)) >> 4 );
			printk("%s 5     CROP_H_EN: %u\n", __func__, (val & (0x1 << 5)) >> 5 );
			printk("%s 6     CROP_V_EN: %u\n", __func__, (val & (0x1 << 6)) >> 6 );
			printk("%s 7     MISR_EN: %u\n", __func__, (val & (0x1 << 7)) >> 7 );
			printk("%s 8     CGC_MODE: %s\n", __func__, ((val & (0x1 << 8)) >> 8) ? "ALWAYS_ON":"DYNAMIC_GATING" );
			printk("%s 9     PLAIN_ALIGNMENT: %s\n", __func__, ((val & (0x1 << 9)) >> 9) ? "MSB":"LSB" );
			printk("%s 10:11 PLAIN_FORMAT: %s (0x%X)\n", __func__, plain_format_str((val >> 10) & 0x3), (val >> 10) & 0x3 );
			printk("%s 12:15 DECODE_FORMAT: %s (0x%X)\n", __func__, decode_format_str(val >> 12 & 0xF), val >> 12 & 0xF);
			printk("%s 16:21 DATA_TYPE: %s (0x%X)\n", __func__, data_type_str((val >> 16) & 0x3F), (val >> 16) & 0x3F);
			printk("%s 22:26 VIRTUAL_CHANNEL: %u\n", __func__, (val & (0x1F << 22)) >> 22 );
			printk("%s 27:28 DT_ID: %u\n", __func__, (val & (0x3 << 27)) >> 27 );
			printk("%s 29    EARLY_EOF_EN: %u\n", __func__, (val >> 29) & 0x1);
			printk("%s 30    PACKING_FORMAT: %s (0x%X)\n", __func__, (val >> 30 & 0x1) ? "MIPI" : "PLAIN", val >> 30 & 0x1);
			printk("%s 31    ENABLE: %u\n", __func__, (val & (0x1 << 31)) >> 31 );
			break;

		case 0x304:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_CFG1 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:1   TIMESTAMP_STB_SEL: %u\n", __func__, val & 0x3);
			break;

		case 0x308:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_CTRL <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:1    HALT_CMD: %u\n", __func__, val & 0x3);
			printk("%s 2      HALT_MODE: %u\n", __func__, (val >> 2) & 0x1);
			break;

		case 0x30c:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_FRM_DROP_PATTERN <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:31  PATTERN: %u\n", __func__, val);
			break;

		case 0x310:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_FRM_DROP_PERIOD <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:4   PERIOD: %u\n", __func__, val & 0x1F);
			break;

		case 0x314:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_IRQ_SUBSAMPLE_PATTERN <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:31  PATTERN: %u\n", __func__, val);
			break;

		case 0x318:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_IRQ_SUBSAMPLE_PERIOD <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:4   PERIOD: %u\n", __func__, val & 0x1F);
			break;

		case 0x324:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_RPP_PIX_DROP_PATTERN <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:31  PATTERN: %u\n", __func__, val);
			break;

		case 0x328:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_RPP_PIX_DROP_PERIOD <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:4   PERIOD: %u\n", __func__, val & 0x1F);
			break;

		case 0x32c:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_RPP_LINE_DROP_PATTERN <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:31  PATTERN: %u\n", __func__, val);
			break;

		case 0x330:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_RPP_LINE_DROP_PERIOD <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:4   PERIOD: %u\n", __func__, val & 0x1F);
			break;

		case 0x600:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_TG_CTRL <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0     TEST_EN: %u\n", __func__, val & 0x1);
			printk("%s 1     FS__PKT_EN: %u\n", __func__, (val >> 1) & 0x1);
			printk("%s 2     FE__PKT_EN: %u\n", __func__, (val >> 2) & 0x1);
			printk("%s 4:5   NUM_ACTIVE_LANES: %u\n", __func__, (val >> 4) & 0x3);
			printk("%s 8:17  CYCLES_BETWEEN_PKTS: %u\n", __func__, (val >> 8) & 0x3FF);
			printk("%s 20:29 NUM_TRAIL_BYTES: %u\n", __func__, (val >> 20) & 0x3FF);
			break;

		case 0x604:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_TG_VC_CFG0 <- 0x%08X\n", __func__, core, reg, val);
      		printk("%s 0:3   VC_NUM: %u\n", __func__, val & 0xF);
      		printk("%s 8:9   NUM_ACTIVE_DTS: %u\n", __func__, (val & (0x3 << 8)) >> 8);
			printk("%s 10    LINE_INTERLEAVING_MODE: %u\n", __func__, (val & (0x1 << 10)) >> 10);
			printk("%s 16:23 NUM_FRAMES: %u\n", __func__, (val & (0xFF << 16)) >> 16);
			break;

		case 0x608:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_TG_VC_CFG1 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:10  H_BLANKINK_COUNT: %u\n", __func__, val & 0x7FF);
			printk("%s 12:21 NUM_ACTIVE_DTS: %u\n", __func__, (val & (0x3FF << 12)) >> 12);
			printk("%s 24:25 LINE_INTERLEAVING_MODE: %u\n", __func__, (val & (0x3 << 24)) >> 24);
			break;

		case 0x60c:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_TG_LFSR_SEED <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x610:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_TG_DT_n_CFG_0 <- 0x%08X\n", __func__, core, reg, val);
      		printk("%s 0:14  FRAME_HEIGHT: %u\n", __func__, val & 0x3FFF);
      		printk("%s 16:31 FRAME_WIDTH: %u\n", __func__, (val & (0xFFFF << 16)) >> 16 );
			break;

		case 0x614:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_TG_DT_n_CFG_1 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:5   DATA_TYPE: %s (0x%x)\n", __func__, data_type_str(val & 0x3F), val & 0x3F );
			printk("%s 8:13  ECC_XOR_MASK: %u\n", __func__, (val & (0x3F << 8)) >> 8 );
			printk("%s 16:21 CRC_XOR_MASK: %u\n", __func__, (val & (0xFFFF << 16)) >> 16 );
			break;

		case 0x618:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_TG_DT_n_CFG_2 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:3   PAYLOAD_MODE: %s\n", __func__, pattern_str(val & 0x7) );
			printk("%s 4:11  USER_SPECIFIED_PAYLOAD: 0x%X\n", __func__, (val & (0xFF << 4)) >> 4 );
			printk("%s 16:19 ENCODE_FORMAT: %s\n", __func__, encode_format_str((val & (0xF << 16)) >> 16) );
			break;

		case 0x640:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_TG_COLOR_BARS_CFG <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x644:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_TG_COLOR_BOX_CFG <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x648:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_TG_COLOR_BOX_CFG <- 0x%08X\n", __func__, core, reg, val);
			break;

		default:
			printk("\n%s %s @ 0x%x: ??? <- 0x%08X\n", __func__, core, reg, val);
			break;
	}
}

static void dumpreg_csid_ifelite(const char *core, u32 reg, u32 val)
{
	switch (reg){

		case 0x0:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_HW_VERSION <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:15  GENERATION: %u\n", __func__, val & 0xFFFF);
			printk("%s 16:27 GENERATION: %u\n", __func__, (val >> 16) & 0xFFF);
			printk("%s 28:31 GENERATION: %u\n", __func__, (val >> 28) & 0xF);
			break;

		case 0x10:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RST_STROBES <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x24:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_CSI2_RX_IRQ_MASK <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x28:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_CSI2_RX_IRQ_CLEAR <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x34:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI#0_IRQ_MASK <- 0x%08X\n", __func__, core, reg, val);
			break;
		case 0x34 + 0x10:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI#1_IRQ_MASK <- 0x%08X\n", __func__, core, reg, val);
			break;
		case 0x34 + 0x20:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI#2_IRQ_MASK <- 0x%08X\n", __func__, core, reg, val);
			break;
		case 0x34 + 0x30:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI#3_IRQ_MASK <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x38:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI#0_IRQ_CLEAR <- 0x%08X\n", __func__, core, reg, val);
			break;
		case 0x38 + 0x10:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI#1_IRQ_CLEAR <- 0x%08X\n", __func__, core, reg, val);
			break;
		case 0x38 + 0x20:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI#2_IRQ_CLEAR <- 0x%08X\n", __func__, core, reg, val);
			break;
		case 0x38 + 0x30:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI#3_IRQ_CLEAR <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x3C:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI#0_IRQ_SET <- 0x%08X\n", __func__, core, reg, val);
			break;
		case 0x3C + 0x10:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI#1_IRQ_SET <- 0x%08X\n", __func__, core, reg, val);
			break;
		case 0x3C + 0x20:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI#2_IRQ_SET <- 0x%08X\n", __func__, core, reg, val);
			break;
		case 0x3C + 0x30:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI#3_IRQ_SET <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x70:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IRQ_STATUS <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x74:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IRQ_MASK <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x78:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IRQ_CLEAR_CMD <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x80:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_IRQ_CMD <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0     CLEAR: %u\n", __func__, val & 0x1);
			printk("%s 4     SET: %u\n", __func__, val >> 4 & 0x1);
			break;

		case 0x100:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_CSI2_RX_CFG0 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:1   NUM_ACTIVE_LANES: %u\n", __func__, val & 0x3);
			printk("%s 4:5   DL0_INPUT_SEL: %u\n", __func__, (val >> 4) & 0x3);
			printk("%s 8:9   DL1_INPUT_SEL: %u\n", __func__, (val >> 8) & 0x3);
			printk("%s 12:13 DL2_INPUT_SEL: %u\n", __func__, (val >> 12) & 0x3);
			printk("%s 16:17 DL3_INPUT_SEL: %u\n", __func__, (val >> 16) & 0x3);
			printk("%s 20:22 PHY_NUM_SEL: %u\n", __func__, (val >> 20) & 0x7);
			printk("%s 24    PHY_TYPE_SEL: %s\n", __func__, (val >> 24 & 0x1) ? "CPHY":"DPHY");
			break;

		case 0x104:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_CSI2_RX_CFG1 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0     PACKET_ECC_CORRECTION_EN: %u\n", __func__, val & 0x1);
			printk("%s 1     DE_SCRAMBLE_EN: %u\n", __func__, (val >> 1) & 0x1);
			printk("%s 2     VC_MODE: %u\n", __func__, (val >> 2) & 0x1);
			printk("%s 4     COMPLETE_STREAM_EN: %u\n", __func__, (val >> 4) & 0x1);
			printk("%s 5     COMPLETE_STREAM_FRAME_TIMING: %u\n", __func__, (val >> 5) & 0x1);
			printk("%s 6     MISR_EN: %u\n", __func__, (val >> 6) & 0x1);
			printk("%s 7     CGC_MODE: %s\n", __func__, ((val >> 7) & 0x1) ? "ALWAYS_ON":"DYNAMIC_GATING" );
			break;

		case 0x108:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_CSI2_RX_CAPTURE_CTRL <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0     LONG_PKT_CAPTURE_EN: %u\n", __func__, val & 0x1);
			printk("%s 1     SHORT_PKT_CAPTURE_EN: %u\n", __func__, (val >> 1) & 0x1);
			printk("%s 2     CPHY_PKT_CAPTURE_EN: %u\n", __func__, (val >> 2) & 0x1);
			printk("%s 4:14  LONG_PKT_CAPTURE_VC_DT: 0x%X\n", __func__, (val >> 4) & 0x3F);
			printk("%s 15:19 SHORT_PKT_CAPTURE_VC_DT: 0x%X\n", __func__, (val >> 15) & 0x1F);
			printk("%s 20:30 CPHY_PKT_CAPTURE_VC_DT: 0x%X\n", __func__, (val >> 20) & 0x7FF);
			break;

		case 0x200:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_CFG0 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0     BYTE_CNTR_EN: %u\n", __func__, val & 0x1);
			printk("%s 1     FORMAT_MEASURE_EN: %u\n", __func__, (val & (0x1 << 1)) >> 1 );
			printk("%s 2     TIMESTAMP_EN: %u\n", __func__, (val & (0x1 << 2)) >> 2 );
			printk("%s 3     DROP_H_EN: %u\n", __func__, (val & (0x1 << 3)) >> 3 );
			printk("%s 4     DROP_V_EN: %u\n", __func__, (val & (0x1 << 4)) >> 4 );
			printk("%s 5     CROP_H_EN: %u\n", __func__, (val & (0x1 << 5)) >> 5 );
			printk("%s 6     CROP_V_EN: %u\n", __func__, (val & (0x1 << 6)) >> 6 );
			printk("%s 7     MISR_EN: %u\n", __func__, (val & (0x1 << 7)) >> 7 );
			printk("%s 8     CGC_MODE: %s\n", __func__, ((val & (0x1 << 8)) >> 8) ? "ALWAYS_ON":"DYNAMIC_GATING" );
			printk("%s 9     PLAIN_ALIGNMENT: %s\n", __func__, ((val & (0x1 << 9)) >> 9) ? "MSB":"LSB" );
			printk("%s 10:11 PLAIN_FORMAT: %s (0x%X)\n", __func__, plain_format_str((val >> 10) & 0x3), (val >> 10) & 0x3 );
			printk("%s 12:15 DECODE_FORMAT: %s (0x%X)\n", __func__, decode_format_str(val >> 12 & 0xF), val >> 12 & 0xF);
			printk("%s 16:21 DATA_TYPE: %s (0x%X)\n", __func__, data_type_str((val >> 16) & 0x3F), (val >> 16) & 0x3F);
			printk("%s 22:26 VIRTUAL_CHANNEL: %u\n", __func__, (val & (0x1F << 22)) >> 22 );
			printk("%s 27:28 DT_ID: %u\n", __func__, (val & (0x3 << 27)) >> 27 );
			printk("%s 29    EARLY_EOF_EN: %u\n", __func__, (val >> 29) & 0x1);
			printk("%s 30    PACKING_FORMAT: %s (0x%X)\n", __func__, (val >> 30 & 0x1) ? "MIPI" : "PLAIN", val >> 30 & 0x1);
			printk("%s 31    ENABLE: %u\n", __func__, (val & (0x1 << 31)) >> 31 );
			break;

		case 0x204:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_CFG1 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:1   TIMESTAMP_STB_SEL: %u\n", __func__, val & 0x3);
			break;

		case 0x208:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_CTRL <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:1    HALT_CMD: %u\n", __func__, val & 0x3);
			printk("%s 2      HALT_MODE: %u\n", __func__, (val >> 2) & 0x1);
			break;

		case 0x20c:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_FRAME_DROP_PATTERN <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:31  PATTERN: %u\n", __func__, val);
			break;

		case 0x210:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_FRAME_DROP_PERIOD <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:4   PERIOD: %u\n", __func__, val & 0x1F);
			break;

		case 0x214:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_IRQ_SUBSAMPLE_PATTERN <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:31  PATTERN: %u\n", __func__, val);
			break;

		case 0x218:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_IRQ_SUBSAMPLE_PERIOD <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:4   PERIOD: %u\n", __func__, val & 0x1F);
			break;

		case 0x224:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_RPP_PIX_DROP_PATTERN <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:31  PATTERN: %u\n", __func__, val);
			break;

		case 0x228:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_RPP_PIX_DROP_PERIOD <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:4   PERIOD: %u\n", __func__, val & 0x1F);
			break;

		case 0x22c:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_RPP_LINE_DROP_PATTERN <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:31  PATTERN: %u\n", __func__, val);
			break;

		case 0x230:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_RPP_LINE_DROP_PERIOD <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:4   PERIOD: %u\n", __func__, val & 0x1F);
			break;

		case 0x240:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_REST_STROBES <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0     CSID_CLK_RST_STB: %u\n", __func__, val & 0x1);
			printk("%s 1     IFE_CLK_RST_STB: %u\n", __func__, val >> 1 & 0x1);
			printk("%s 2     MISR_RST_STB: %u\n", __func__, val >> 2 & 0x1);
			printk("%s 3     FORMAT_MEASURE_RST_STB: %u\n", __func__, val >> 3 & 0x1);
			printk("%s 4     TIMESTAMP_RST_STB: %u\n", __func__, val >> 4 & 0x1);
			printk("%s 5     FRAMEDROP_RST_STB: %u\n", __func__, val >> 5 & 0x1);
			printk("%s 6     IRQ_SUBSAMPLE_RST_STB: %u\n", __func__, val >> 6 & 0x1);
			printk("%s 7     BYTE_CNTR_RST_STB: %u\n", __func__, val >> 7 & 0x1);
			break;

		case 0x250:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_RDI_STATUS <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0     HALT: %u\n", __func__, val & 0x1);
			break;

		case 0x600:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_TG_CTRL <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0     TEST_EN: %u\n", __func__, val & 0x1);
			printk("%s 1     FS__PKT_EN: %u\n", __func__, (val >> 1) & 0x1);
			printk("%s 2     FE__PKT_EN: %u\n", __func__, (val >> 2) & 0x1);
			printk("%s 4:5   NUM_ACTIVE_LANES: %u\n", __func__, (val >> 4) & 0x3);
			printk("%s 8:17  CYCLES_BETWEEN_PKTS: %u\n", __func__, (val >> 8) & 0x3FF);
			printk("%s 20:29 NUM_TRAIL_BYTES: %u\n", __func__, (val >> 20) & 0x3FF);
			break;

		case 0x604:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_TG_VC_CFG0 <- 0x%08X\n", __func__, core, reg, val);
      		printk("%s 0:3   VC_NUM: %u\n", __func__, val & 0xF);
      		printk("%s 8:9   NUM_ACTIVE_DTS: %u\n", __func__, (val & (0x3 << 8)) >> 8);
			printk("%s 10    LINE_INTERLEAVING_MODE: %u\n", __func__, (val & (0x1 << 10)) >> 10);
			printk("%s 16:23 NUM_FRAMES: %u\n", __func__, (val & (0xFF << 16)) >> 16);
			break;

		case 0x608:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_TG_VC_CFG1 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:10  H_BLANKINK_COUNT: %u\n", __func__, val & 0x7FF);
			printk("%s 12:21 NUM_ACTIVE_DTS: %u\n", __func__, (val & (0x3FF << 12)) >> 12);
			printk("%s 24:25 LINE_INTERLEAVING_MODE: %u\n", __func__, (val & (0x3 << 24)) >> 24);
			break;

		case 0x60c:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_TG_LFSR_SEED <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x610:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_TG_DT_n_CFG_0 <- 0x%08X\n", __func__, core, reg, val);
      		printk("%s 0:14  FRAME_HEIGHT: %u\n", __func__, val & 0x3FFF);
      		printk("%s 16:31 FRAME_WIDTH: %u\n", __func__, (val & (0xFFFF << 16)) >> 16 );
			break;

		case 0x614:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_TG_DT_n_CFG_1 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:5   DATA_TYPE: %s (0x%x)\n", __func__, data_type_str(val & 0x3F), val & 0x3F );
			printk("%s 8:13  ECC_XOR_MASK: %u\n", __func__, (val & (0x3F << 8)) >> 8 );
			printk("%s 16:21 CRC_XOR_MASK: %u\n", __func__, (val & (0xFFFF << 16)) >> 16 );
			break;

		case 0x618:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_TG_DT_n_CFG_2 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:3   PAYLOAD_MODE: %s\n", __func__, pattern_str(val & 0x7) );
			printk("%s 4:11  USER_SPECIFIED_PAYLOAD: 0x%X\n", __func__, (val & (0xFF << 4)) >> 4 );
			printk("%s 16:19 ENCODE_FORMAT: %s\n", __func__, encode_format_str((val & (0xF << 16)) >> 16) );
			break;

		case 0x640:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_TG_COLOR_BARS_CFG <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x644:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_TG_COLOR_BOX_CFG <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x648:
			printk("\n%s %s @ 0x%x: CAMSS_CSID_TG_COLOR_BOX_CFG <- 0x%08X\n", __func__, core, reg, val);
			break;

		default:
			printk("\n%s %s @ 0x%x: ??? <- 0x%08X\n", __func__, core, reg, val);
			break;
	}
}

static void dumpreg_ife(const char *core, u32 reg, u32 val)
{
	switch (reg){
		case 0x0:
			printk("\n%s %s @ 0x%x: IFE_HW_VERSION <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:15  INCR_VERSION: %u\n", __func__, val & 0xFFFF);
			printk("%s 16:27 MINOR_VERSION: %u\n", __func__, val >> 16 & 0xFFF);
			printk("%s 28:31 MAJOR_VERSION: %u\n", __func__, val >> 28 & 0xF);
			break;

		case 0x018:
			printk("\n%s %s @ 0x%x: IFE_GLOBAL_RESET_CMD <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0     CORE_RESET: %u\n", __func__, val & 0x1);
			printk("%s 2     BUS_HW_RESET: %u\n", __func__, (val >> 2) & 0x1);
			printk("%s 3     BUS_SW_RESET: %u\n", __func__, (val >> 3) & 0x1);
			printk("%s 4     REGISTER_RESET: %u\n", __func__, (val >> 4) & 0x1);
			printk("%s 9     IDLE_CGC_RESET: %u\n", __func__, (val >> 9) & 0x1);
			printk("%s 10    RDI_0_RESET: %u\n", __func__, (val >> 10) & 0x1);
			printk("%s 11    RDI_1_RESET: %u\n", __func__, (val >> 11) & 0x1);
			printk("%s 12    RDI_2_RESET: %u\n", __func__, (val >> 12) & 0x1);
			printk("%s 13    RDI_3_RESET: %u\n", __func__, (val >> 13) & 0x1);
			printk("%s 30    VFE_DOMAIN_RESET: %u\n", __func__, (val >> 30) & 0x1);
			printk("%s 31    RESET_BYPASS: %u\n", __func__, (val >> 31) & 0x1);
			break;

		case 0x058:
			printk("\n%s %s @ 0x%x: IFE_IRQ_CMD <- 0x%08X\n", __func__, core, reg, val);
    		printk("%s 0     GLOBAL_CLEAR: %u\n", __func__, val & 0x1);
			break;

		case 0x05c:
			printk("\n%s %s @ 0x%x: IFE_IRQ_MASK_0 <- 0x%08X (bad reg details?)\n", __func__, core, reg, val);
			printk("%s 0  CAMIF SOF: %u\n", __func__, val & 0x1);
			printk("%s 1  CAMIF EOF: %u\n", __func__, (val >> 1) & 0x1);
			printk("%s 4  LINE PIX REG UPDATE: %u\n", __func__, (val >> 5) & 0x1);
			printk("%s 5  RDI#0 REG UPDATE: %u\n", __func__, (val >> 5) & 0x1);
			printk("%s 6  RDI#1 REG UPDATE: %u\n", __func__, (val >> 6) & 0x1);
			printk("%s 7  RDI#2 REG UPDATE: %u\n", __func__, (val >> 7) & 0x1);
			printk("%s 8  MASTER#0 PING PONG: %u\n", __func__, (val >> 8) & 0x1);
			printk("%s 9  MASTER#1 PING PONG: %u\n", __func__, (val >> 9) & 0x1);
			printk("%s 10 MASTER#2 PING PONG: %u\n", __func__, (val >> 10) & 0x1);
			printk("%s 11 MASTER#3 PING PONG: %u\n", __func__, (val >> 11) & 0x1);
			printk("%s 25 IMAGE COMPOSITE DONE #0: %u\n", __func__, (val >> 25) & 0x1);
			printk("%s 26 IMAGE COMPOSITE DONE #1: %u\n", __func__, (val >> 26) & 0x1);
			printk("%s 27 IMAGE COMPOSITE DONE #2: %u\n", __func__, (val >> 27) & 0x1);
			printk("%s 31 RESET ACK: %u\n", __func__, (val >> 31) & 0x1);
			break;

		case 0x060:
			printk("\n%s %s @ 0x%x: IFE_IRQ_MASK_1 <- 0x%08X (bad reg details?)\n", __func__, core, reg, val);
			printk("%s 0  CAMIF ERROR: %u\n", __func__, val & 0x1);
			printk("%s 7  VIOLATION: %u\n", __func__, (val >> 7) & 0x1);
			printk("%s 8  BUS DBG HALT ACK: %u\n", __func__, (val >> 8) & 0x1);
			printk("%s 9  MASTER#0 BUS OVERFLOW: %u\n", __func__, (val >> 9) & 0x1);
			printk("%s 10 MASTER#1 BUS OVERFLOW: %u\n", __func__, (val >> 10) & 0x1);
			printk("%s 11 MASTER#2 BUS OVERFLOW: %u\n", __func__, (val >> 11) & 0x1);
			printk("%s 12 MASTER#3 BUS OVERFLOW: %u\n", __func__, (val >> 12) & 0x1);
			printk("%s 29 RDI#0 SOF: %u\n", __func__, (val >> 29) & 0x1);
			printk("%s 30 RDI#1 SOF: %u\n", __func__, (val >> 30) & 0x1);
			printk("%s 31 RDI#2 SOF: %u\n", __func__, (val >> 31) & 0x1);
			break;

		case 0x064:
			printk("\n%s %s @ 0x%x: IFE_IRQ_CLEAR_0 <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x068:
			printk("\n%s %s @ 0x%x: IFE_IRQ_CLEAR_1 <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x06c:
			printk("\n%s %s @ 0x%x: IFE_IRQ_STATUS_0 <- 0x%08X (bad reg details?)\n", __func__, core, reg, val);
			printk("%s 0  CAMIF SOF: %u\n", __func__, val & 0x1);
			printk("%s 5  PIX LINE UPDATE: %u\n", __func__, (val >> 4) & 0x1);
			printk("%s 5  RDI#0 REG UPDATE: %u\n", __func__, (val >> 5) & 0x1);
			printk("%s 6  RDI#1 REG UPDATE: %u\n", __func__, (val >> 6) & 0x1);
			printk("%s 7  RDI#2 REG UPDATE: %u\n", __func__, (val >> 7) & 0x1);
			printk("%s 8  MASTER#0 PING PONG: %u\n", __func__, (val >> 8) & 0x1);
			printk("%s 9  MASTER#1 PING PONG: %u\n", __func__, (val >> 9) & 0x1);
			printk("%s 10 MASTER#2 PING PONG: %u\n", __func__, (val >> 10) & 0x1);
			printk("%s 11 MASTER#3 PING PONG: %u\n", __func__, (val >> 11) & 0x1);
			printk("%s 31 RESET ACK: %u\n", __func__, (val >> 31) & 0x1);
			break;

		case 0x070:
			printk("\n%s %s @ 0x%x: IFE_IRQ_STATUS_1 <- 0x%08X (bad reg details?)\n", __func__, core, reg, val);
			printk("%s 7     VIOLATION: %u\n", __func__, (val >> 7) & 0x1);
			printk("%s 8     BUS DBG HALT: %u\n", __func__, (val >> 8) & 0x1);
			printk("%s 27    RDI#0 SOF: %u\n", __func__, (val >> 27) & 0x1);
			printk("%s 28    RDI#1 SOF: %u\n", __func__, (val >> 28)& 0x1);
			printk("%s 29    RDI#2 SOF: %u\n", __func__, (val >> 29) & 0x1);
			printk("%s 30    RDI#3 SOF: %u\n", __func__, (val >> 30) & 0x1);
			break;

		case 0x200c:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM_CGC_OVERRIDE <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x2044:
			printk("\n%s %s @ 0x%x: IFE_BUS_IRQ_MASK_0 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0     COMP_RESET_DONE: %u\n", __func__, val & 0x1);
			printk("%s 1     COMP_REG_UPDATE0_DONE: %u\n", __func__, val >> 1 & 0x1);
			printk("%s 2     COMP_REG_UPDATE1_DONE: %u\n", __func__, val >> 2 & 0x1);
			printk("%s 3     COMP_REG_UPDATE2_DONE: %u\n", __func__, val >> 3 & 0x1);
			printk("%s 4     COMP_REG_UPDATE3_DONE: %u\n", __func__, val >> 4 & 0x1);
			printk("%s 5     COMP0_BUF_DONE: %u\n", __func__, val >> 5 & 0x1);
			printk("%s 6     COMP1_BUF_DONE: %u\n", __func__, val >> 6 & 0x1);
			printk("%s 7     COMP2_BUF_DONE: %u\n", __func__, val >> 7 & 0x1);
			printk("%s 8     COMP3_BUF_DONE: %u\n", __func__, val >> 8 & 0x1);
			printk("%s 9     COMP4_BUF_DONE: %u\n", __func__, val >> 9 & 0x1);
			printk("%s 10    COMP5_BUF_DONE: %u\n", __func__, val >> 10 & 0x1);
			printk("%s 11    COMP_ERROR: %u\n", __func__, val >> 11 & 0x1);
			printk("%s 12    COMP_OVERWRITE: %u\n", __func__, val >> 12 & 0x1);
			printk("%s 13    OVERFLOW: %u\n", __func__, val >> 13 & 0x1);
			printk("%s 14    VIOLATION: %u\n", __func__, val >> 14 & 0x1);
			break;

		case 0x2048:
			printk("\n%s %s @ 0x%x: IFE_BUS_IRQ_MASK_1 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:3   WR_CLIENT_BUF_DONE: %u\n", __func__, val & 0xF);
			printk("%s 24    EARLY_DONE: %u\n", __func__, val >> 24 & 0x1);
			break;

		case 0x204C:
			printk("\n%s %s @ 0x%x: IFE_BUS_IRQ_MASK_2 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0     DUAL_COMP0_DONE: %u\n", __func__, val & 0x1);
			printk("%s 1     DUAL_COMP1_DONE: %u\n", __func__, val >> 1 & 0x1);
			printk("%s 2     DUAL_COMP2_DONE: %u\n", __func__, val >> 2 & 0x1);
			printk("%s 3     DUAL_COMP3_DONE: %u\n", __func__, val >> 3 & 0x1);
			printk("%s 4     DUAL_COMP4_DONE: %u\n", __func__, val >> 4 & 0x1);
			printk("%s 5     DUAL_COMP5_DONE: %u\n", __func__, val >> 5 & 0x1);
			printk("%s 6     DUAL_COMP_ERROR: %u\n", __func__, val >> 6 & 0x1);
			printk("%s 7     DUAL_COMP_OVERWRITE: %u\n", __func__, val >> 7 & 0x1);
			break;

		case 0x2050:
			printk("\n%s %s @ 0x%x: IFE_BUS_IRQ_CLEAR_0 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0     COMP_RESET_DONE: %u\n", __func__, val & 0x1);
			printk("%s 1     COMP_REG_UPDATE0_DONE: %u\n", __func__, val >> 1 & 0x1);
			printk("%s 2     COMP_REG_UPDATE1_DONE: %u\n", __func__, val >> 2 & 0x1);
			printk("%s 3     COMP_REG_UPDATE2_DONE: %u\n", __func__, val >> 3 & 0x1);
			printk("%s 4     COMP_REG_UPDATE3_DONE: %u\n", __func__, val >> 4 & 0x1);
			printk("%s 5     COMP0_BUF_DONE: %u\n", __func__, val >> 5 & 0x1);
			printk("%s 6     COMP1_BUF_DONE: %u\n", __func__, val >> 6 & 0x1);
			printk("%s 7     COMP2_BUF_DONE: %u\n", __func__, val >> 7 & 0x1);
			printk("%s 8     COMP3_BUF_DONE: %u\n", __func__, val >> 8 & 0x1);
			printk("%s 9     COMP4_BUF_DONE: %u\n", __func__, val >> 9 & 0x1);
			printk("%s 10    COMP5_BUF_DONE: %u\n", __func__, val >> 10 & 0x1);
			printk("%s 11    COMP_ERROR: %u\n", __func__, val >> 11 & 0x1);
			printk("%s 12    COMP_OVERWRITE: %u\n", __func__, val >> 12 & 0x1);
			printk("%s 13    OVERFLOW: %u\n", __func__, val >> 13 & 0x1);
			printk("%s 14    VIOLATION: %u\n", __func__, val >> 14 & 0x1);
			break;

		case 0x2054:
			printk("\n%s %s @ 0x%x: IFE_BUS_IRQ_CLEAR_1 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:3   WR_CLIENT_BUF_DONE: %u\n", __func__, val & 0xF);
			printk("%s 24    EARLY_DONE: %u\n", __func__, val >> 24 & 0x1);
			break;

		case 0x2058:
			printk("\n%s %s @ 0x%x: IFE_BUS_IRQ_CLEAR_2 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0     DUAL_COMP0_DONE: %u\n", __func__, val & 0x1);
			printk("%s 1     DUAL_COMP1_DONE: %u\n", __func__, val >> 1 & 0x1);
			printk("%s 2     DUAL_COMP2_DONE: %u\n", __func__, val >> 2 & 0x1);
			printk("%s 3     DUAL_COMP3_DONE: %u\n", __func__, val >> 3 & 0x1);
			printk("%s 4     DUAL_COMP4_DONE: %u\n", __func__, val >> 4 & 0x1);
			printk("%s 5     DUAL_COMP5_DONE: %u\n", __func__, val >> 5 & 0x1);
			printk("%s 6     DUAL_COMP_ERROR: %u\n", __func__, val >> 6 & 0x1);
			printk("%s 7     DUAL_COMP_OVERWRITE: %u\n", __func__, val >> 7 & 0x1);
			break;

		case 0x205c:
			printk("\n%s %s @ 0x%x: IFE_BUS_IRQ_STATUS_0 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0     COMP_RESET_DONE: %u\n", __func__, val & 0x1);
			printk("%s 1     COMP_REG_UPDATE0_DONE: %u\n", __func__, val >> 1 & 0x1);
			printk("%s 2     COMP_REG_UPDATE1_DONE: %u\n", __func__, val >> 2 & 0x1);
			printk("%s 3     COMP_REG_UPDATE2_DONE: %u\n", __func__, val >> 3 & 0x1);
			printk("%s 4     COMP_REG_UPDATE3_DONE: %u\n", __func__, val >> 4 & 0x1);
			printk("%s 5     COMP0_BUF_DONE: %u\n", __func__, val >> 5 & 0x1);
			printk("%s 6     COMP1_BUF_DONE: %u\n", __func__, val >> 6 & 0x1);
			printk("%s 7     COMP2_BUF_DONE: %u\n", __func__, val >> 7 & 0x1);
			printk("%s 8     COMP3_BUF_DONE: %u\n", __func__, val >> 8 & 0x1);
			printk("%s 9     COMP4_BUF_DONE: %u\n", __func__, val >> 9 & 0x1);
			printk("%s 10    COMP5_BUF_DONE: %u\n", __func__, val >> 10 & 0x1);
			printk("%s 11    COMP_ERROR: %u\n", __func__, val >> 11 & 0x1);
			printk("%s 12    COMP_OVERWRITE: %u\n", __func__, val >> 12 & 0x1);
			printk("%s 13    OVERFLOW: %u\n", __func__, val >> 13 & 0x1);
			printk("%s 14    VIOLATION: %u\n", __func__, val >> 14 & 0x1);
			break;

		case 0x2060:
			printk("\n%s %s @ 0x%x: IFE_BUS_IRQ_STATUS_1 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:3   WR_CLIENT_BUF_DONE: %u\n", __func__, val & 0xF);
			printk("%s 24    EARLY_DONE: %u\n", __func__, val >> 24 & 0x1);
			break;

		case 0x2064:
			printk("\n%s %s @ 0x%x: IFE_BUS_IRQ_STATUS_2 <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0     DUAL_COMP0_DONE: %u\n", __func__, val & 0x1);
			printk("%s 1     DUAL_COMP1_DONE: %u\n", __func__, val >> 1 & 0x1);
			printk("%s 2     DUAL_COMP2_DONE: %u\n", __func__, val >> 2 & 0x1);
			printk("%s 3     DUAL_COMP3_DONE: %u\n", __func__, val >> 3 & 0x1);
			printk("%s 4     DUAL_COMP4_DONE: %u\n", __func__, val >> 4 & 0x1);
			printk("%s 5     DUAL_COMP5_DONE: %u\n", __func__, val >> 5 & 0x1);
			printk("%s 6     DUAL_COMP_ERROR: %u\n", __func__, val >> 6 & 0x1);
			printk("%s 7     DUAL_COMP_OVERWRITE: %u\n", __func__, val >> 7 & 0x1);
			break;

		case 0x2068:
			printk("\n%s %s @ 0x%x: IFE_BUS_WR_IRQ_CMD <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x2080:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM_ADDR_SYNC_FRAME_HEADER <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x2084:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM_ADDR_SYNC_NO_SYNC <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x211c:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM_TEST_BUS_CTRL <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x2200:
		case 0x2200 + 0x100:
		case 0x2200 + 0x200:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM_STATUS0 <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x2204:
		case 0x2204 + 0x100:
		case 0x2204 + 0x200:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM_STATUS0 <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x2208:
		case 0x2208 + 0x100:
		case 0x2208 + 0x200:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM_CFG <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0  WM_CFG_EN: %u\n", __func__, val & 0x1);
			printk("%s 1  WM_CFG_MODE: %u\n", __func__, (val >> 1) & 0x1);
			printk("%s 2  WM_CFG_MODE: %u\n", __func__, (val >> 2) & 0x1);
			break;

		case 0x220c:
		case 0x220c + 0x100:
		case 0x220c + 0x200:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM_HEADER_ADDR <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x2210:
		case 0x2210 + 0x100:
		case 0x2210 + 0x200:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM_HEADER_CFG <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x2214:
		case 0x2214 + 0x100:
		case 0x2214 + 0x200:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM_IMAGE_ADDR <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x2218:
		case 0x2218 + 0x100:
		case 0x2218 + 0x200:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM_IMAGE_ADDR_OFFSET <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x221c:
		case 0x221c + 0x100:
		case 0x221c + 0x200:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM_BUFFER_WIDTH_CFG <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:31 WIDTH: %u\n", __func__, val);
			break;

		case 0x2220:
		case 0x2220 + 0x100:
		case 0x2220 + 0x200:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM_BUFFER_HEIGHT_CFG <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:15 HEIGHT: %u\n", __func__, val & 0xFFFF);
			break;

		case 0x2224:
		case 0x2224 + 0x100:
		case 0x2224 + 0x200:
		    printk("\n%s %s @ 0x%x: IFE_BUS_WM_PACKER_CFG <- 0x%08X\n", __func__, core, reg, val);
			printk("%s 0:3  PACKER_CFG_MODE: %u\n", __func__, val & 0xF);
			printk("%s 4    PACKER_CFG_ALIGNMENT: %u\n", __func__, (val >> 4) & 0x1);
			break;

		case 0x2228:
		case 0x2228 + 0x100:
		case 0x2228 + 0x200:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM_STRIDE <- 0x%08X\n", __func__, core, reg, val);
    		printk("%s 0:20 HEIGHT: %u\n", __func__, val & 0xFFFFF);
			break;

		case 0x2248:
		case 0x2248 + 0x100:
		case 0x2248 + 0x200:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM_IRQ_SUBSAMPLE_PERIOD <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x224c:
		case 0x224c + 0x100:
		case 0x224c + 0x200:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM_IRQ_SUBSAMPLE_PATTERN <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x2250:
		case 0x2250 + 0x100:
		case 0x2250 + 0x200:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM_FRAMEDROP_PERIOD <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x2254:
		case 0x2254 + 0x100:
		case 0x2254 + 0x200:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM_FRAMEDROP_PATTERN <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x2258:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM#0_FRAME_INC <- 0x%08X\n", __func__, core, reg, val);
			break;
		case 0x2258 + 0x100:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM#1_FRAME_INC <- 0x%08X\n", __func__, core, reg, val);
			break;
		case 0x2258 + 0x200:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM#2_FRAME_INC <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x225c:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM#0_BURST_LIMIT <- 0x%08X\n", __func__, core, reg, val);
			break;
		case 0x225c + 0x100:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM#1_BURST_LIMIT <- 0x%08X\n", __func__, core, reg, val);
			break;
		case 0x225c + 0x200:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM#2_BURST_LIMIT <- 0x%08X\n", __func__, core, reg, val);
			break;

		case 0x226c:
			printk("\n%s %s @ 0x%x: IFE_BUS_WM_DEBUG_STATUS_CFG <- 0x%08X\n", __func__, core, reg, val);
			break;

		default:
			printk("\n%s %s @ 0x%x: ??? <- 0x%08X\n", __func__, core, reg, val);
			break;
	}
}


static void reg_addr_decode(u64 reg_base_unmapped, u32 reg, u32 val) {
	if (reg_base_unmapped >= 0xac65000 && reg_base_unmapped <= (0xac65000 + 0x1000)) {
		dumpreg("csiphy0", reg, val);
	} else if (reg_base_unmapped >= 0xac66000 && reg_base_unmapped <= (0xac66000 + 0x1000)) {
		dumpreg("csiphy1", reg, val);
	} else if (reg_base_unmapped >= 0xac67000 && reg_base_unmapped <= (0xac67000 + 0x1000)) {
		dumpreg("csiphy2", reg, val);
	} else if (reg_base_unmapped >= 0xac68000 && reg_base_unmapped <= (0xac68000 + 0x1000)) {
		dumpreg("csiphy3", reg, val);
	} else if (reg_base_unmapped >= 0xacb3000 && reg_base_unmapped <= (0xacb3000 + 0x1000)) {
		dumpreg_csid("csid0", reg, val);
	} else if (reg_base_unmapped >= 0xacba000 && reg_base_unmapped <= (0xacba000 + 0x1000)) {
		dumpreg_csid("csid1", reg, val);
	} else if (reg_base_unmapped >= 0xacc8000 && reg_base_unmapped <= (0xacc8000 + 0x1000)) {
		dumpreg_csid_ifelite("csid2-ifelite", reg, val);
	} else if (reg_base_unmapped >= 0xacaf000 && reg_base_unmapped <= (0xacaf000 + 0x4000)) {
		dumpreg_ife("ife0", reg, val);
	} else if (reg_base_unmapped >= 0xacb6000 && reg_base_unmapped <= (0xacb6000 + 0x4000)) {
		dumpreg_ife("ife1", reg, val);
	} else if (reg_base_unmapped >= 0xacc4000 && reg_base_unmapped <= (0xacc4000 + 0x4000)) {
		dumpreg_ife("ife-lite", reg, val);
	} else {
		dumpreg("???", reg, val);
	}
}

#if 0
#define debug_writel(val, reg_addr, reg_base_unmapped, reg_base) {\
	u32 reg = reg_addr - reg_base; \
	reg_addr_decode(reg_base_unmapped, reg, val); \
	writel_relaxed(val, reg_addr);\
}
#else
#define debug_writel(val, reg_addr, reg_base_unmapped, reg_base) {\
	writel_relaxed(val, reg_addr);\
}
#endif


struct camss_buffer {
	struct vb2_v4l2_buffer vb;
	dma_addr_t addr[3];
	struct list_head queue;
};

struct camss_video;

struct camss_video_ops {
	int (*queue_buffer)(struct camss_video *vid, struct camss_buffer *buf);
	int (*flush_buffers)(struct camss_video *vid,
			     enum vb2_buffer_state state);
};

struct camss_format_info;

struct camss_video {
	struct camss *camss;
	struct vb2_queue vb2_q;
	struct video_device vdev;
	struct media_pad pad;
	struct v4l2_format active_fmt;
	enum v4l2_buf_type type;
	struct media_pipeline pipe;
	const struct camss_video_ops *ops;
	struct mutex lock;
	struct mutex q_lock;
	unsigned int bpl_alignment;
	unsigned int line_based;
	const struct camss_format_info *formats;
	unsigned int nformats;
};

int msm_video_register(struct camss_video *video, struct v4l2_device *v4l2_dev,
		       const char *name, int is_pix);

void msm_video_unregister(struct camss_video *video);

#endif /* QC_MSM_CAMSS_VIDEO_H */
