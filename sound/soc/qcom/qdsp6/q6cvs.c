// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2012-2017, The Linux Foundation. All rights reserved.
// Copyright (c) 2020, Stephan Gerhold

#include <linux/module.h>
#include <linux/of.h>
#include <linux/soc/qcom/apr.h>
#include "q6cvs.h"
#include "q6voice-common.h"

#define VSS_ISTREAM_CMD_CREATE_PASSIVE_CONTROL_SESSION	0x00011140

struct cvs_create_passive_control_session_cmd {
	struct apr_hdr hdr;
	char name[20];
} __packed;

struct q6voice_session *q6cvs_session_create(enum q6voice_path_type path)
{
	struct cvs_create_passive_control_session_cmd cmd;
	struct q6voice_session *cvs;
	const char *session_name;

	cmd.hdr.pkt_size = sizeof(cmd);
	cmd.hdr.opcode = VSS_ISTREAM_CMD_CREATE_PASSIVE_CONTROL_SESSION;

	session_name = q6voice_get_session_name(path);
	if (session_name)
		strlcpy(cmd.name, session_name, sizeof(cmd.name));

	cvs = q6voice_session_create(Q6VOICE_SERVICE_CVS, path, &cmd.hdr);

	return cvs;
}
EXPORT_SYMBOL_GPL(q6cvs_session_create);

static int q6cvs_probe(struct apr_device *adev)
{
	return q6voice_common_probe(adev, Q6VOICE_SERVICE_CVS);
}

static const struct of_device_id q6cvs_device_id[]  = {
	{ .compatible = "qcom,q6cvs" },
	{},
};
MODULE_DEVICE_TABLE(of, q6cvs_device_id);

static struct apr_driver qcom_q6cvs_driver = {
	.probe = q6cvs_probe,
	.remove = q6voice_common_remove,
	.callback = q6voice_common_callback,
	.driver = {
		.name = "qcom-q6cvs",
		.of_match_table = of_match_ptr(q6cvs_device_id),
	},
};

module_apr_driver(qcom_q6cvs_driver);

MODULE_AUTHOR("Stephan Gerhold <stephan@gerhold.net>");
MODULE_DESCRIPTION("Q6 Core Voice Stream");
MODULE_LICENSE("GPL v2");
