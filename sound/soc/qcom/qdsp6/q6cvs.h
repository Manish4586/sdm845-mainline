/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _Q6_CVS_H
#define _Q6_CVS_H

#include "q6voice.h"

struct q6voice_session;

struct q6voice_session *q6cvs_session_create(enum q6voice_path_type path);

#endif /*_Q6_CVS_H */
