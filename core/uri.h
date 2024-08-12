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
#ifndef WAKAAMA_URI_H
#define WAKAAMA_URI_H
#include "internals.h"

typedef enum {
    LWM2M_REQUEST_TYPE_UNKNOWN,
    LWM2M_REQUEST_TYPE_DM,
    LWM2M_REQUEST_TYPE_REGISTRATION,
    LWM2M_REQUEST_TYPE_BOOTSTRAP,
    LWM2M_REQUEST_TYPE_DELETE_ALL,
    LWM2M_REQUEST_TYPE_SEND,
} lwm2m_request_type_t;

lwm2m_request_type_t uri_decode(char *altPath, multi_option_t *uriPath, uint8_t code, lwm2m_uri_t *uriP);
int uri_getNumber(uint8_t *uriString, size_t uriLength);

#endif /* WAKAAMA_URI_H */
