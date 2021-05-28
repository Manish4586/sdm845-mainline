// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2020 Caleb Connolly <caleb@connolly.tech>
 * Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 */

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/swab.h>
#include <linux/backlight.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

#define MAX_DATA_LEN 16

#define dsi_dcs_write_seq(dsi, seq...) do {				\
		static const u8 d[] = { seq };				\
		int ret;						\
		ret = mipi_dsi_dcs_write_buffer(dsi, d, ARRAY_SIZE(d));	\
		if (ret < 0)						\
			return ret;					\
	} while (0)

#define sofef00_gen_cmd(seq...) \
	{ .data = { seq }, .len = ((int)(sizeof((char[]){ seq })/sizeof(char))) }

struct sofef00_panel_cmd {
	char data[MAX_DATA_LEN];
	unsigned long len;
};

struct sofef00_panel_desc {
	struct drm_display_mode mode;

	struct sofef00_panel_cmd *on_cmds;
	unsigned long on_cmds_len;
	struct sofef00_panel_cmd *prepare_cmds;
	unsigned long prepare_cmds_len;
	struct sofef00_panel_cmd *enable_cmds;
	unsigned long enable_cmds_len;
	struct sofef00_panel_cmd *off_cmds;
	unsigned long off_cmds_len;
};

struct sofef00_panel {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator *supply;
	struct gpio_desc *reset_gpio;
	struct sofef00_panel_desc *desc;
	bool prepared;
};

static const struct sofef00_panel_desc enchilada_panel = {
	.mode = {
		.clock = (1080 + 112 + 16 + 36) * (2280 + 36 + 8 + 12) * 60 / 1000,
		.hdisplay = 1080,
		.hsync_start = 1080 + 112,
		.hsync_end = 1080 + 112 + 16,
		.htotal = 1080 + 112 + 16 + 36,
		.vdisplay = 2280,
		.vsync_start = 2280 + 36,
		.vsync_end = 2280 + 36 + 8,
		.vtotal = 2280 + 36 + 8 + 12,
		.width_mm = 68,
		.height_mm = 145,
	},
	.on_cmds = NULL,
	.on_cmds_len = 0,
	.prepare_cmds = NULL,
	.prepare_cmds_len = 0,
	.enable_cmds = NULL,
	.enable_cmds_len = 0,
	.off_cmds = NULL,
	.off_cmds_len = 0,
};

static struct sofef00_panel_cmd fajita_on_cmds[] = {
	sofef00_gen_cmd(0x9F, 0xA5, 0xA5),
	sofef00_gen_cmd(0x11, 0x00), /* MIPI_DCS_EXIT_SLEEP_MODE */
	sofef00_gen_cmd(0x9F, 0x5A, 0x5A),
	/* FD Setting */
	sofef00_gen_cmd(0xF0, 0x5A, 0x5A),
	sofef00_gen_cmd(0xB0, 0x01),
	sofef00_gen_cmd(0xCD, 0x01),
	sofef00_gen_cmd(0xF0, 0xA5, 0xA5),
	sofef00_gen_cmd(),
	/* TE ON */
	sofef00_gen_cmd(0x9F, 0xA5, 0xA5),
	sofef00_gen_cmd(0x35, 0x00), /* MIPI_DCS_SET_TEAR_ON */
	sofef00_gen_cmd(0x9F, 0x5A, 0x5A),
	/* MIC Setting */
	sofef00_gen_cmd(0xF0, 0x5A, 0x5A),
	sofef00_gen_cmd(0xEB, 0x17, 0x41, 0x92, 0x0E, 0x10, 0x82, 0x5A),
	sofef00_gen_cmd(0xF0, 0xA5, 0xA5),
	/* CASET/PASET */
	sofef00_gen_cmd(0x2A, 0x00, 0x00, 0x04, 0x37),
	sofef00_gen_cmd(0x2B, 0x00, 0x00, 0x09, 0x23),
	/* TSP H_sync Setting */
	sofef00_gen_cmd(0xF0, 0x5A, 0x5A),
	sofef00_gen_cmd(0xB0, 0x09),
	sofef00_gen_cmd(0xE8, 0x10, 0x30),
	sofef00_gen_cmd(0xF0, 0xA5, 0xA5),
	/* Dimming Setting */
	sofef00_gen_cmd(0xF0, 0x5A, 0x5A),
	sofef00_gen_cmd(0xB0, 0x07),
	sofef00_gen_cmd(0xB7, 0x01),
	sofef00_gen_cmd(0xB0, 0x08),
	sofef00_gen_cmd(0xB7, 0x12),
	sofef00_gen_cmd(0xF0, 0xA5, 0xA5),
	/* ESD improvement Setting */
	sofef00_gen_cmd(0xFC, 0x5A, 0x5A),
	sofef00_gen_cmd(0xB0, 0x01),
	sofef00_gen_cmd(0xE3, 0x88),
	sofef00_gen_cmd(0xB0, 0x07),
	sofef00_gen_cmd(0xED, 0x67),
	sofef00_gen_cmd(0xFC, 0xA5, 0xA5),
	sofef00_gen_cmd(0xF0, 0x5A, 0x5A),
	sofef00_gen_cmd(0x53, 0x20), /* MIPI_DCS_WRITE_CONTROL_DISPLAY */
	sofef00_gen_cmd(0xF0, 0xA5, 0xA5),
	/* ACL off */
	sofef00_gen_cmd(0x55, 0x00),
	/* SEED OFF */
	// sofef00_gen_cmd(0xF0, 0x5A, 0x5A),
	// sofef00_gen_cmd(0xB1, 0x00, 0x01),
	// sofef00_gen_cmd(0xF0, 0xA5, 0xA5),
	/* SEED TCS OFF */
	sofef00_gen_cmd(0xF0, 0x5A, 0x5A),
	sofef00_gen_cmd(0xB3, 0x00, 0xC1),
	sofef00_gen_cmd(0xF0, 0xA5, 0xA5),
	/* Display on */
	sofef00_gen_cmd(0x9F, 0xA5, 0xA5),
	sofef00_gen_cmd(0x29),
	sofef00_gen_cmd(0x9F, 0x5A, 0x5A),
};

static struct sofef00_panel_cmd fajita_off_cmds[] = {
	sofef00_gen_cmd(0x9F, 0xA5, 0xA5),
	sofef00_gen_cmd(0x28),
	sofef00_gen_cmd(0xF0, 0x5A, 0x5A),
	sofef00_gen_cmd(0xB0, 0x50),
	sofef00_gen_cmd(0xB9, 0x82),
	sofef00_gen_cmd(0xF0, 0xA5, 0xA5),
	sofef00_gen_cmd(0x10),
	sofef00_gen_cmd(0x9F, 0x5A, 0x5A),
	sofef00_gen_cmd(0xF0, 0x5A, 0x5A),
	sofef00_gen_cmd(0xB0, 0x05),
	sofef00_gen_cmd(0xF4, 0x01),
	sofef00_gen_cmd(0xF0, 0xA5, 0xA5,),
};

static const struct sofef00_panel_desc fajita_panel = {
	.mode = {
		.clock = (1080 + 72 + 16 + 36) * (2340 + 32 + 4 + 18) * 60 / 1000,
		.hdisplay = 1080,
		.hsync_start = 1080 + 72,
		.hsync_end = 1080 + 72 + 16,
		.htotal = 1080 + 72 + 16 + 36,
		.vdisplay = 2340,
		.vsync_start = 2340 + 32,
		.vsync_end = 2340 + 32 + 4,
		.vtotal = 2340 + 32 + 4 + 18,
		.width_mm = 68,
		.height_mm = 145,
	},
	.on_cmds = fajita_on_cmds,
	.on_cmds_len = ARRAY_SIZE(fajita_on_cmds),
	.enable_cmds = NULL,
	.enable_cmds_len = 0,
	.prepare_cmds = NULL,
	.prepare_cmds_len = 0,
	.off_cmds = fajita_off_cmds,
	.off_cmds_len = ARRAY_SIZE(fajita_off_cmds),
};

static inline
struct sofef00_panel *to_sofef00_panel(struct drm_panel *panel)
{
	return container_of(panel, struct sofef00_panel, panel);
}

static int sofef00_send_cmds(struct sofef00_panel *ctx, struct sofef00_panel_cmd *cmds, size_t len)
{
	int ret, i = 0;
	for (i = 0; i < len; i++)
	{
		ret = mipi_dsi_dcs_write_buffer(ctx->dsi, cmds[i].data, cmds[i].len);
		if (ret < 0) {
			dev_err(&ctx->dsi->dev, "Failed to write buffer %d, ret=%d", i, ret);
			return ret;
		}
	}
	return 0;
}

static void sofef00_panel_reset(struct sofef00_panel *ctx)
{
	if (!ctx->reset_gpio)
		return;

	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(2000, 3000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(12000, 13000);
}

static int sofef00_panel_on(struct sofef00_panel *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	struct sofef00_panel_cmd *cmds = ctx->desc->on_cmds;
	int cmds_len = ctx->desc->on_cmds_len;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	ret = sofef00_send_cmds(ctx, cmds, cmds_len);
	if (ret)
		dev_err(dev, "Failed to send panel on commands");
	
	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	return 0;
}

static int sofef00_panel_off(struct sofef00_panel *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	struct sofef00_panel_cmd *cmds = ctx->desc->off_cmds;
	int cmds_len = ctx->desc->off_cmds_len;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = sofef00_send_cmds(ctx, cmds, cmds_len);
	if (ret)
		dev_err(dev, "Failed to send panel off commands");

	return 0;
}

static int sofef00_panel_prepare(struct drm_panel *panel)
{
	struct sofef00_panel *ctx = to_sofef00_panel(panel);
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	struct sofef00_panel_cmd *cmds = ctx->desc->prepare_cmds;
	int cmds_len = ctx->desc->prepare_cmds_len;
	int ret;

	if (ctx->prepared)
		return 0;

	ret = regulator_enable(ctx->supply);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulator: %d\n", ret);
		return ret;
	}

	sofef00_panel_reset(ctx);

	ret = sofef00_panel_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		return ret;
	}

	ctx->prepared = true;
	return 0;
}

static int sofef00_panel_enable(struct drm_panel *panel)
{
	struct sofef00_panel *ctx = to_sofef00_panel(panel);
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	struct sofef00_panel_cmd *cmds = ctx->desc->enable_cmds;
	int cmds_len = ctx->desc->enable_cmds_len;
	int ret;

	return 0;

	dev_info(dev, "ENABLING PANEL");

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = sofef00_send_cmds(ctx, cmds, cmds_len);
	if (ret) {
		dev_err(dev, "Failed to send panel enable commands");
		return ret;
	}
	
	return 0;
}

static int sofef00_panel_unprepare(struct drm_panel *panel)
{
	struct sofef00_panel *ctx = to_sofef00_panel(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (!ctx->prepared)
		return 0;

	ret = sofef00_panel_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	regulator_disable(ctx->supply);

	ctx->prepared = false;
	return 0;
}

static int sofef00_panel_get_modes(struct drm_panel *panel, struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct sofef00_panel *ctx = to_sofef00_panel(panel);

	mode = drm_mode_duplicate(connector->dev, &ctx->desc->mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs sofef00_panel_panel_funcs = {
	.prepare = sofef00_panel_prepare,
	.enable = sofef00_panel_enable,
	.unprepare = sofef00_panel_unprepare,
	.get_modes = sofef00_panel_get_modes,
};

static int sofef00_panel_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	int err;
	unsigned short brightness;

	brightness = (unsigned short)backlight_get_brightness(bl);
	// This panel needs the high and low bytes swapped for the brightness value
	brightness = __swab16(brightness);

	err = mipi_dsi_dcs_set_display_brightness(dsi, brightness);
	if (err < 0)
		return err;

	return 0;
}

static const struct backlight_ops sofef00_panel_bl_ops = {
	.update_status = sofef00_panel_bl_update_status,
};

static struct backlight_device *
sofef00_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_PLATFORM,
		.brightness = 1023,
		.max_brightness = 1023,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &sofef00_panel_bl_ops, &props);
}

static int sofef00_panel_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct sofef00_panel *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->desc = of_device_get_match_data(dev);

	if (!ctx->desc) {
		dev_err(dev, "Missing panel description\n");
		return -ENODEV;
	}

	ctx->supply = devm_regulator_get(dev, "vddio");
	if (IS_ERR(ctx->supply)) {
		ret = PTR_ERR(ctx->supply);
		dev_err(dev, "Failed to get vddio regulator: %d\n", ret);
		return ret;
	}

	ctx->reset_gpio = devm_gpiod_get_optional(dev, "reset", GPIOD_ASIS);
	if (IS_ERR(ctx->reset_gpio)) {
		ret = PTR_ERR(ctx->reset_gpio);
		dev_warn(dev, "Failed to get reset-gpios: %d\n", ret);
		return ret;
	}

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags |= MIPI_DSI_CLOCK_NON_CONTINUOUS;

	drm_panel_init(&ctx->panel, dev, &sofef00_panel_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	ctx->panel.backlight = sofef00_create_backlight(dsi);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to create backlight\n");
	
	//sofef00_panel_off(ctx);

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to attach to DSI host: %d\n", ret);
		return ret;
	}

	return 0;
}

static int sofef00_panel_remove(struct mipi_dsi_device *dsi)
{
	struct sofef00_panel *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);

	return 0;
}

static const struct of_device_id sofef00_panel_of_match[] = {
	{ // OnePlus 6 / enchilada
		.compatible = "samsung,sofef00",
		.data = &enchilada_panel,
	},
	{ // OnePlus 6T / fajita
		.compatible = "samsung,s6e3fc2x01",
		.data = &fajita_panel,
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sofef00_panel_of_match);

static struct mipi_dsi_driver sofef00_panel_driver = {
	.probe = sofef00_panel_probe,
	.remove = sofef00_panel_remove,
	.driver = {
		.name = "panel-sofef00",
		.of_match_table = sofef00_panel_of_match,
	},
};

module_mipi_dsi_driver(sofef00_panel_driver);

MODULE_AUTHOR("Caleb Connolly <caleb@connolly.tech>");
MODULE_DESCRIPTION("DRM driver for Samsung AMOLED DSI panels found in OnePlus 6/6T phones");
MODULE_LICENSE("GPL v2");
