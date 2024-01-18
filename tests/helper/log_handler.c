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
#include "log_handler.h"
#include "internals.h"

#include <unistd.h>

#if LWM2M_LOG_LEVEL != LWM2M_LOG_DISABLED

/* Buffer for captured log message */
#define CAPTURE_BUF_SIZE 1000
static char capture_buf[CAPTURE_BUF_SIZE];
static size_t capture_buf_index = 0;

#ifdef LWM2M_LOG_CUSTOM_HANDLER
void lwm2m_log_handler(const lwm2m_logging_level_t level, const char *const msg, const char *const func, const int line,
                       const char *const file) {
    (void)file;
    (void)line;

    if (capture_buf_index < CAPTURE_BUF_SIZE) {
        capture_buf_index += snprintf(capture_buf + capture_buf_index, CAPTURE_BUF_SIZE - capture_buf_index,
                                      "%s - [%s] %s" LWM2M_NEW_LINE, STR_LOGGING_LEVEL(level), func, msg);
    }
}
#endif

char *test_log_handler_get_captured_message(void) {
    capture_buf[CAPTURE_BUF_SIZE - 1] = '\0'; // just to be safe
    return capture_buf;
}

void test_log_handler_clear_captured_message(void) {
    memset(capture_buf, '\0', CAPTURE_BUF_SIZE);
    capture_buf_index = 0;
}

#endif
