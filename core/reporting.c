/*******************************************************************************
 *
 * Copyright (c) 2024
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
 *    Nathanaël Semhoun - Please refer to git log
 *
 *******************************************************************************/

#include "internals.h"
#include <stdio.h>

#ifdef LWM2M_SERVER_MODE

#ifndef LWM2M_VERSION_1_0
uint8_t reporting_handleSend(lwm2m_context_t *contextP, void *fromSessionH, coap_packet_t *message) {
    lwm2m_client_t *clientP;
    lwm2m_media_type_t format;

    LOG_DBG("Entering");

    if (message->code != COAP_POST)
        return COAP_400_BAD_REQUEST;

    for (clientP = contextP->clientList; clientP != NULL; clientP = clientP->next) {
        if (clientP->sessionH == fromSessionH)
            break;
    }
    if (clientP == NULL)
        return COAP_400_BAD_REQUEST;

    format = utils_convertMediaType(message->content_type);

    if (format != LWM2M_CONTENT_SENML_JSON && format != LWM2M_CONTENT_SENML_CBOR) {
        return COAP_400_BAD_REQUEST;
    }

    if (contextP->reportingSendCallback != NULL) {
        contextP->reportingSendCallback(contextP, clientP->internalID, NULL, message->code, NULL, format,
                                        message->payload, message->payload_len, contextP->reportingSendUserData);
    }

    return COAP_204_CHANGED;
}
void lwm2m_reporting_set_send_callback(lwm2m_context_t *contextP, lwm2m_result_callback_t callback, void *userData) {
    LOG_DBG("Entering");
    contextP->reportingSendCallback = callback;
    contextP->reportingSendUserData = userData;
}
#endif

#endif
