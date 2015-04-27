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
 *    Pascal Rieux - Please refer to git log
 *    
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

#define REG_LWM2M_RESOURCE_TYPE     ">;rt=\"oma.lwm2m\","
#define REG_LWM2M_RESOURCE_TYPE_LEN 17
#define REG_ALT_PATH_LINK           "<%s"REG_LWM2M_RESOURCE_TYPE
#define REG_OBJECT_PATH             "<%s/%hu>,"
#define REG_OBJECT_INSTANCE_PATH    "<%s/%hu/%hu>,"

#define URI_REGISTRATION_SEGMENT        "rd"
#define URI_REGISTRATION_SEGMENT_LEN    2
#define URI_BOOTSTRAP_SEGMENT           "bs"
#define URI_BOOTSTRAP_SEGMENT_LEN       2

#define LWM2M_URI_FLAG_DM           (uint8_t)0x00
#define LWM2M_URI_FLAG_DELETE_ALL   (uint8_t)0x10
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

// defined in uri.c
int lwm2m_get_number(char * uriString, size_t uriLength);
lwm2m_uri_t * lwm2m_decode_uri(char * altPath, multi_option_t *uriPath);
int prv_get_number(uint8_t * uriString, size_t uriLength);

// defined in objects.c
coap_status_t object_read(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, uint8_t ** bufferP, size_t * lengthP);
coap_status_t object_write(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, uint8_t * buffer, size_t length);
coap_status_t object_create(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, uint8_t * buffer, size_t length);
coap_status_t object_execute(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, uint8_t * buffer, size_t length);
coap_status_t object_delete(lwm2m_context_t * contextP, lwm2m_uri_t * uriP);
bool object_isInstanceNew(lwm2m_context_t * contextP, uint16_t objectId, uint16_t instanceId);
int prv_getRegisterPayload(lwm2m_context_t * contextP, uint8_t * buffer, size_t length);
int object_getServers(lwm2m_context_t * contextP);

// defined in transaction.c
lwm2m_transaction_t * transaction_new(coap_method_t method, char * altPath, lwm2m_uri_t * uriP, uint16_t mID, lwm2m_endpoint_type_t peerType, void * peerP);
int transaction_send(lwm2m_context_t * contextP, lwm2m_transaction_t * transacP);
void transaction_free(lwm2m_transaction_t * transacP);
void transaction_remove(lwm2m_context_t * contextP, lwm2m_transaction_t * transacP);
int transaction_handle_response(lwm2m_context_t * contextP, void * fromSessionH, coap_packet_t * message);

// defined in management.c
coap_status_t handle_dm_request(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, void * fromSessionH, coap_packet_t * message, coap_packet_t * response);
coap_status_t handle_delete_all(lwm2m_context_t * context);

// defined in observe.c
coap_status_t handle_observe_request(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, lwm2m_server_t * serverP, coap_packet_t * message, coap_packet_t * response);
void cancel_observe(lwm2m_context_t * contextP, uint16_t mid, void * fromSessionH);

// defined in registration.c
coap_status_t handle_registration_request(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, void * fromSessionH, coap_packet_t * message, coap_packet_t * response);
void registration_deregister(lwm2m_context_t * contextP, lwm2m_server_t * serverP);
void prv_freeClient(lwm2m_client_t * clientP);
void registration_update(lwm2m_context_t * contextP, time_t currentTime, time_t * timeoutP);

// defined in packet.c
coap_status_t message_send(lwm2m_context_t * contextP, coap_packet_t * message, void * sessionH);

// defined in observe.c
void handle_observe_notify(lwm2m_context_t * contextP, void * fromSessionH, coap_packet_t * message);
void observation_remove(lwm2m_client_t * clientP, lwm2m_observation_t * observationP);

// defined in bootstrap.c
void handle_bootstrap_ack(lwm2m_context_t * context, coap_packet_t * message, void * fromSessionH);
void bootstrap_failed(lwm2m_context_t * context);
void reset_bootstrap_timer(lwm2m_context_t * context);
void update_bootstrap_state(lwm2m_context_t * contextP, uint32_t currentTime, time_t* timeoutP);
void delete_bootstrap_server_list(lwm2m_context_t * contextP);

// defined in liblwm2m.c
void delete_transaction_list(lwm2m_context_t * context);
void delete_server_list(lwm2m_context_t * context);
void delete_observed_list(lwm2m_context_t * contextP);

// defined in utils.c
lwm2m_binding_t lwm2m_stringToBinding(uint8_t *buffer, size_t length);
int prv_isAltPathValid(char * altPath);
#ifdef LWM2M_CLIENT_MODE
lwm2m_server_t * prv_findServer(lwm2m_context_t * contextP, void * fromSessionH);
lwm2m_server_t * utils_findBootstrapServer(lwm2m_context_t * contextP, void * fromSessionH);
#endif

#endif
