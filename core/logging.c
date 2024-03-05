/*******************************************************************************
 *
 * Copyright (c) 2023 GARDENA GmbH
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

#include "internals.h"

#if LWM2M_LOG_LEVEL != LWM2M_LOG_DISABLED
#include "liblwm2m.h"
#include <stdarg.h>

char *lwm2m_log_fmt_message(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    static char txt[LWM2M_MAX_LOG_MSG_TXT_SIZE];
    memset(txt, 0, sizeof txt); // just to be safe
    vsnprintf(txt, LWM2M_MAX_LOG_MSG_TXT_SIZE, fmt, args);
    return txt;
}

#ifndef LWM2M_LOG_CUSTOM_HANDLER
void lwm2m_log_handler(const lwm2m_logging_level_t level, const char *const msg, const char *const func, const int line,
                       const char *const file) {
    (void)file;
    lwm2m_printf("%s - [%s:%d] %s" LWM2M_NEW_LINE, STR_LOGGING_LEVEL(level), func, line, msg);
}
#endif

#endif
