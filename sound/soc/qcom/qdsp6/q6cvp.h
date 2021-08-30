/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _Q6_CVP_H
#define _Q6_CVP_H

#include "q6voice.h"

struct q6voice_session;

struct q6voice_session *q6cvp_session_create(enum q6voice_path_type path,
					     u16 tx_port, u16 rx_port);
int q6cvp_enable(struct q6voice_session *cvp, bool enable);
int q6cvp_send_topology_commit(struct q6voice_session *cvp);

#endif /*_Q6_CVP_H */
