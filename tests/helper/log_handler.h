/*******************************************************************************
 *
 * Copyright (c) 2024 GARDENA GmbH
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v20.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *   Lukas Woodtli, GARDENA GmbH - Please refer to git log
 *
 *******************************************************************************/
#ifndef WAKAAMA_LOG_HANDLER_H
#define WAKAAMA_LOG_HANDLER_H

#include "internals.h"

#if LWM2M_LOG_LEVEL != LWM2M_LOG_DISABLED

char *test_log_handler_get_captured_message(void);

void test_log_handler_clear_captured_message(void);

#endif

#endif
