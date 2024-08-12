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
#ifndef WAKAAMA_BOOTSTRAP_H
#define WAKAAMA_BOOTSTRAP_H
#include "er-coap-13/er-coap-13.h"
#include "internals.h"

void bootstrap_step(lwm2m_context_t *contextP, time_t currentTime, time_t *timeoutP);
uint8_t bootstrap_handleCommand(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, lwm2m_server_t *serverP,
                                coap_packet_t *message, coap_packet_t *response);
uint8_t bootstrap_handleDeleteAll(lwm2m_context_t *context, void *fromSessionH);
uint8_t bootstrap_handleFinish(lwm2m_context_t *context, void *fromSessionH);
uint8_t bootstrap_handleRequest(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, void *fromSessionH,
                                coap_packet_t *message, coap_packet_t *response);
void bootstrap_start(lwm2m_context_t *contextP);
lwm2m_status_t bootstrap_getStatus(lwm2m_context_t *contextP);

#endif /* WAKAAMA_BOOTSTRAP_H */
