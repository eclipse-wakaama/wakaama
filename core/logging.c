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
#include "liblwm2m.h"

#ifdef LWM2M_WITH_LOGS

void lwm2m_log_handler(const lwm2m_logging_level_t level, const char *const msg, const char *const func, const int line,
                       const char *const file) {
    (void)file;
    lwm2m_printf("%s - [%s:%d] %s" LWM2M_NEW_LINE, STR_LOGGING_LEVEL(level), func, line, msg);
}

#endif /* LWM2M_WITH_LOGS */
