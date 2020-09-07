/* SPDX-License-Identifier: GPL-2.0 */
/*
 * camss-csid.h
 *
 * Qualcomm MSM Camera Subsystem - CSID (CSI Decoder) Module
 *
 * Copyright (c) 2011-2014, The Linux Foundation. All rights reserved.
 * Copyright (C) 2015-2018 Linaro Ltd.
 */
#ifndef QC_MSM_CAMSS_CSID_H
#define QC_MSM_CAMSS_CSID_H

#include <linux/clk.h>
#include <media/media-entity.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>

#define MSM_CSID_PAD_SINK 0
#define MSM_CSID_PAD_SRC 1
#define MSM_CSID_PADS_NUM 2

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


enum csid_payload_mode {
	CSID_PAYLOAD_MODE_INCREMENTING = 0,
	CSID_PAYLOAD_MODE_ALTERNATING_55_AA = 1,
	CSID_PAYLOAD_MODE_ALL_ZEROES = 2,
	CSID_PAYLOAD_MODE_ALL_ONES = 3,
	CSID_PAYLOAD_MODE_RANDOM = 4,
	CSID_PAYLOAD_MODE_USER_SPECIFIED = 5,
};

struct csid_testgen_config {
	u8 enabled;
	enum csid_payload_mode payload_mode;
};

struct csid_phy_config {
	u8 csiphy_id;
	u8 lane_cnt;
	u32 lane_assign;
};

struct csid_device {
	struct camss *camss;
	u8 id;
	struct v4l2_subdev subdev;
	struct media_pad pads[MSM_CSID_PADS_NUM];
	void __iomem *base;
	u32 irq;
	char irq_name[30];
	struct camss_clock *clock;
	int nclocks;
	struct regulator *vdda;
	struct completion reset_complete;
	struct csid_testgen_config testgen;
	struct csid_phy_config phy;
	struct v4l2_mbus_framefmt fmt[MSM_CSID_PADS_NUM];
	struct v4l2_ctrl_handler ctrls;
	struct v4l2_ctrl *testgen_mode;
	const struct csid_format *formats;
	unsigned int nformats;
};

struct resources;

int msm_csid_subdev_init(struct camss *camss, struct csid_device *csid,
			 const struct resources *res, u8 id);

int msm_csid_register_entity(struct csid_device *csid,
			     struct v4l2_device *v4l2_dev);

void msm_csid_unregister_entity(struct csid_device *csid);

void msm_csid_get_csid_id(struct media_entity *entity, u8 *id);

#endif /* QC_MSM_CAMSS_CSID_H */
