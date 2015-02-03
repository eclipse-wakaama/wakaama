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
 *    David Navarro, Intel Corporation - initial API and implementation
 *    Fabien Fleutot - Please refer to git log
 *    Toby Jaffey - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *******************************************************************************/
/*
 Copyright (c) 2013, 2014 Intel Corporation

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 * Neither the name of Intel Corporation nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.

 David Navarro <david.navarro@intel.com>

 */

#ifndef _LWM2M_INTERNALS_H_
#define _LWM2M_INTERNALS_H_

#include "liblwm2m.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "er-coap-13/er-coap-13.h"

#ifdef WITH_LOGS
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...)
#endif

#define LWM2M_DEFAULT_LIFETIME  86400

#define LWM2M_MAX_PACKET_SIZE 198

#define URI_REGISTRATION_SEGMENT        "rd"
#define URI_REGISTRATION_SEGMENT_LEN    2
#define URI_BOOTSTRAP_SEGMENT           "bs"
#define URI_BOOTSTRAP_SEGMENT_LEN       2

#define LWM2M_URI_FLAG_DM           (uint8_t)0x00
#define LWM2M_URI_FLAG_REGISTRATION (uint8_t)0x20
#define LWM2M_URI_FLAG_BOOTSTRAP    (uint8_t)0x40

#define LWM2M_URI_MASK_TYPE (uint8_t)0x70
#define LWM2M_URI_MASK_ID   (uint8_t)0x07

typedef struct
{
    lwm2m_uri_t uri;
    lwm2m_result_callback_t callback;
    void * userData;
} dm_data_t;

typedef struct _obs_list_
{
    struct _obs_list_ * next;
    lwm2m_observed_t * item;
} obs_list_t;

/**
 * resource for blockwise transfer
 */
struct _lwm2m_blockwise_
{
    struct _lwm2m_blockwise_ * next;
    lwm2m_uri_t uri;
    uint32_t time;
    uint8_t etag_len;
    uint8_t etag[COAP_ETAG_LEN];
    uint16_t size;
    uint16_t length;
    uint8_t* data;
};

// defined in uri.c
int lwm2m_get_number(const char * uriString, size_t uriLength);
lwm2m_uri_t * lwm2m_decode_uri(multi_option_t *uriPath);

// defined in objects.c
coap_status_t object_read(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, char ** bufferP, int * lengthP);
coap_status_t object_write(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, char * buffer, int length);
coap_status_t object_attrib(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, multi_option_t * uri_query, void * fromSessionH);
coap_status_t object_create(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, char * buffer, int length);
coap_status_t object_execute(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, char * buffer, int length);
coap_status_t object_delete(lwm2m_context_t * contextP, lwm2m_uri_t * uriP);
bool object_isInstanceNew(lwm2m_context_t * contextP, uint16_t objectId, uint16_t instanceId);
int prv_getRegisterPayload(lwm2m_context_t * contextP, char * buffer, size_t length);
int object_getServers(lwm2m_context_t * contextP);
int object_updateServersInfo(lwm2m_context_t * contextP, lwm2m_uri_t * uriP);

// defined in transaction.c
lwm2m_transaction_t * transaction_new(coap_method_t method, lwm2m_uri_t * uriP, uint16_t mID, lwm2m_endpoint_type_t peerType, void * peerP);
int transaction_send(lwm2m_context_t * contextP, lwm2m_transaction_t * transacP);
void transaction_free(lwm2m_transaction_t * transacP);
void transaction_remove(lwm2m_context_t * contextP, lwm2m_transaction_t * transacP);
void transaction_handle_response(lwm2m_context_t * contextP, void * fromSessionH, coap_packet_t * message);

// defined in blockwise.c
lwm2m_blockwise_t* blockwise_get(lwm2m_context_t * contextP, const lwm2m_uri_t * uriP);
lwm2m_blockwise_t * blockwise_new(lwm2m_context_t * contextP, const lwm2m_uri_t * uriP, coap_packet_t * messageP, bool detach);
void blockwise_prepare(lwm2m_blockwise_t * blockwiseP, uint32_t block_num, uint16_t block_size,
        coap_packet_t * response);
void blockwise_append(lwm2m_blockwise_t * blockwiseP, uint32_t block_offset, coap_packet_t * response);
void blockwise_remove(lwm2m_context_t * contextP, const lwm2m_uri_t * uriP);
void blockwise_free(lwm2m_context_t * contextP, uint32_t time);

// defined in management.c
coap_status_t handle_dm_request(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, void * fromSessionH, coap_packet_t * message, coap_packet_t * response, bool* notify_change);

// defined in observe.c
coap_status_t handle_observe_request(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, void * fromSessionH, coap_packet_t * message, coap_packet_t * response);
void cancel_observe(lwm2m_context_t * contextP, int32_t mid, void * fromSessionH);
time_t lwm2m_notify(lwm2m_context_t * contextP, struct timeval * tv);

// defined in registration.c
coap_status_t handle_registration_request(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, void * fromSessionH, coap_packet_t * message, coap_packet_t * response);
void registration_deregister(lwm2m_context_t * contextP, lwm2m_server_t * serverP);
void handle_registration_response(lwm2m_context_t * contextP, void * fromSessionH, coap_packet_t * message);
void prv_freeClient(lwm2m_client_t * clientP);

// defined in packet.c
coap_status_t message_send(lwm2m_context_t * contextP, coap_packet_t * message, void * sessionH);

// defined in observe.c
void handle_observe_notify(lwm2m_context_t * contextP, void * fromSessionH, coap_packet_t * message);
void observation_remove(lwm2m_client_t * clientP, lwm2m_observation_t * observationP);

// defined in utils.c
lwm2m_binding_t lwm2m_stringToBinding(const char *buffer, size_t length);

// defined in attributes.c
lwm2m_attribute_data_t * lwm2m_getAttributes(lwm2m_server_t * serverP, lwm2m_uri_t * uriP);
void lwm2m_updateTransmissionAttributes(lwm2m_attribute_data_t * attributeP, struct timeval *tv);
uint8_t lwm2m_evalAttributes(lwm2m_attribute_data_t* attrData, const char* resValBuf, int bufLen, struct timeval tv, bool *notifyResult);
int lwm2m_adjustTimeout(time_t nextTime, time_t currentTime, struct timeval* timeoutP);
/**
  sets the attributes of a object/instance/resource
 * @return coap result
*/
uint8_t lwm2m_setAttributes(lwm2m_context_t * contextP, lwm2m_uri_t * uriP,lwm2m_attribute_type_t type, const char* value, int length, lwm2m_object_t * objectP,  lwm2m_server_t * serverP);

#endif
