/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Julien Vermillard, Sierra Wireless
 *
 *******************************************************************************/

#include "internals.h"

#define QUERY_TEMPLATE "bs="
#define QUERY_LENGTH 3

#ifdef LWM2M_CLIENT_MODE

int lwm2m_bootstrap(lwm2m_context_t * contextP)
{
    char * query;
    coap_packet_t message[1];
    uint8_t pktBuffer[COAP_MAX_PACKET_SIZE+1];
    size_t pktBufferLen = 0;

    if (contextP->bootstrapServer == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

    query = (char*) lwm2m_malloc(QUERY_LENGTH + strlen(contextP->endpointName) + 1);
    if (query == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
    strcpy(query, QUERY_TEMPLATE);
    strcpy(query + QUERY_LENGTH, contextP->endpointName);

    // can't use a transaction, becasue transaction needs a server struct
    coap_init_message(message, COAP_TYPE_CON, COAP_POST, contextP->nextMID++);
    coap_set_header_uri_path(message, "/"URI_BOOTSTRAP_SEGMENT);
    coap_set_header_uri_query(message, query);

    pktBufferLen = coap_serialize_message(message, pktBuffer);
    if (0 != pktBufferLen)
    {
        contextP->bufferSendCallback(contextP->bootstrapServer->sessionH, pktBuffer, pktBufferLen, contextP->bufferSendUserData);
        return 0;
    } else {
        return INTERNAL_SERVER_ERROR_5_00;
    }
    lwm2m_free(query);
}

#endif
