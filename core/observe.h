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
#ifndef WAKAAMA_OBSERVE_H
#define WAKAAMA_OBSERVE_H
#include "internals.h"

uint8_t observe_handleRequest(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, lwm2m_server_t *serverP, int size,
                              lwm2m_data_t *dataP, coap_packet_t *message, coap_packet_t *response);
void observe_cancel(lwm2m_context_t *contextP, uint16_t mid, void *fromSessionH);
uint8_t observe_setParameters(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, lwm2m_server_t *serverP,
                              lwm2m_attributes_t *attrP);
void observe_step(lwm2m_context_t *contextP, time_t currentTime, time_t *timeoutP);
void observe_clear(lwm2m_context_t *contextP, lwm2m_uri_t *uriP);
bool observe_handleNotify(lwm2m_context_t *contextP, void *fromSessionH, coap_packet_t *message,
                          coap_packet_t *response);
void observe_remove(lwm2m_observation_t *observationP);
lwm2m_observed_t *observe_findByUri(lwm2m_context_t *contextP, lwm2m_uri_t *uriP);

#endif /* WAKAAMA_OBSERVE_H */
