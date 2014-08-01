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
 *    domedambrosio - Please refer to git log
 *    Toby Jaffey - Please refer to git log
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

#include "internals.h"
#include <stdio.h>


#ifdef LWM2M_CLIENT_MODE
coap_status_t handle_dm_request(lwm2m_context_t * contextP,
                                lwm2m_uri_t * uriP,
                                void * fromSessionH,
                                coap_packet_t * message,
                                coap_packet_t * response)
{
    coap_status_t result;

    switch (message->code)
    {
    case COAP_GET:
        {
            char * buffer = NULL;
            int length = 0;

            result = object_read(contextP, uriP, &buffer, &length);
            if (result == COAP_205_CONTENT)
            {
                if (IS_OPTION(message, COAP_OPTION_OBSERVE))
                {
                    result = handle_observe_request(contextP, uriP, fromSessionH, message, response);
                }
                if (result == COAP_205_CONTENT)
                {
                    coap_set_payload(response, buffer, length);
                    // lwm2m_handle_packet will free buffer
                }
            }
        }
        break;
    case COAP_POST:
        {
            if (!LWM2M_URI_IS_SET_INSTANCE(uriP))
            {
                result = object_create(contextP, uriP, message->payload, message->payload_len);
                if (result == COAP_201_CREATED)
                {
                    //longest uri is /65535/65535 = 12 + 1 (null) chars
                    char location_path[13] = "";
                    //instanceId expected
                    if ((uriP->flag & LWM2M_URI_FLAG_INSTANCE_ID) == 0)
                    {
                        result = COAP_500_INTERNAL_SERVER_ERROR;
                        break;
                    }

                    if (sprintf(location_path, "/%d/%d", uriP->objectId, uriP->instanceId) < 0)
                    {
                        result = COAP_500_INTERNAL_SERVER_ERROR;
                        break;
                    }
                    coap_set_header_location_path(response, location_path);
                }
            }
            else if (!LWM2M_URI_IS_SET_RESOURCE(uriP))
            {
                if (object_isInstanceNew(contextP, uriP->objectId, uriP->instanceId))
                {
                    result = object_create(contextP, uriP, message->payload, message->payload_len);
                }
                else
                {
                    result = object_write(contextP, uriP, message->payload, message->payload_len);
                }
            }
            else
            {
                result = object_execute(contextP, uriP, message->payload, message->payload_len);
            }
        }
        break;
    case COAP_PUT:
        {
            if (LWM2M_URI_IS_SET_INSTANCE(uriP))
            {
                result = object_write(contextP, uriP, message->payload, message->payload_len);
            }
            else
            {
                result = BAD_REQUEST_4_00;
            }
        }
        break;
    case COAP_DELETE:
        {
            if (LWM2M_URI_IS_SET_INSTANCE(uriP) && !LWM2M_URI_IS_SET_RESOURCE(uriP))
            {
                result = object_delete(contextP, uriP);
            }
            else
            {
                result = BAD_REQUEST_4_00;
            }
        }
        break;
    default:
        result = BAD_REQUEST_4_00;
        break;
    }

    return result;
}
#endif

#ifdef LWM2M_SERVER_MODE

#define ID_AS_STRING_MAX_LEN 8

static void dm_result_callback(lwm2m_transaction_t * transacP,
                               void * message)
{
    dm_data_t * dataP = (dm_data_t *)transacP->userData;

    if (message == NULL)
    {
        dataP->callback(((lwm2m_client_t*)transacP->peerP)->internalID,
                        &dataP->uri,
                        COAP_503_SERVICE_UNAVAILABLE,
                        NULL, 0,
                        dataP->userData);
    }
    else
    {
        coap_packet_t * packet = (coap_packet_t *)message;

        //if packet is a CREATE response and the instanceId was assigned by the client
        if (packet->code == COAP_201_CREATED
         && packet->location_path != NULL)
        {
            char * locationString = NULL;
            int result = 0;
            lwm2m_uri_t locationUri;

            locationString = coap_get_multi_option_as_string(packet->location_path);
            if (locationString == NULL)
            {
                fprintf(stderr, "Error: coap_get_multi_option_as_string() failed for Location_path option in dm_result_callback()\n");
                return;
            }

            result = lwm2m_stringToUri(locationString, strlen(locationString), &locationUri);
            if (result == 0)
            {
                fprintf(stderr, "Error: lwm2m_stringToUri() failed for Location_path option in dm_result_callback()\n");
                lwm2m_free(locationString);
                return;
            }

            ((dm_data_t*)transacP->userData)->uri.instanceId = locationUri.instanceId;
            ((dm_data_t*)transacP->userData)->uri.flag = locationUri.flag;

            lwm2m_free(locationString);
        }

        dataP->callback(((lwm2m_client_t*)transacP->peerP)->internalID,
                        &dataP->uri,
                        packet->code,
                        packet->payload,
                        packet->payload_len,
                        dataP->userData);
    }
    lwm2m_free(dataP);
}

static int prv_make_operation(lwm2m_context_t * contextP,
                              uint16_t clientID,
                              lwm2m_uri_t * uriP,
                              coap_method_t method,
                              char * buffer,
                              int length,
                              lwm2m_result_callback_t callback,
                              void * userData)
{
    lwm2m_client_t * clientP;
    lwm2m_transaction_t * transaction;
    dm_data_t * dataP;

    clientP = (lwm2m_client_t *)lwm2m_list_find((lwm2m_list_t *)contextP->clientList, clientID);
    if (clientP == NULL) return COAP_404_NOT_FOUND;

    transaction = transaction_new(method, uriP, contextP->nextMID++, ENDPOINT_CLIENT, (void *)clientP);
    if (transaction == NULL) return INTERNAL_SERVER_ERROR_5_00;

    if (buffer != NULL)
    {
        // TODO: Take care of fragmentation
        coap_set_payload(transaction->message, buffer, length);
    }

    if (callback != NULL)
    {
        dataP = (dm_data_t *)lwm2m_malloc(sizeof(dm_data_t));
        if (dataP == NULL)
        {
            transaction_free(transaction);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
        memcpy(&dataP->uri, uriP, sizeof(lwm2m_uri_t));
        dataP->callback = callback;
        dataP->userData = userData;

        transaction->callback = dm_result_callback;
        transaction->userData = (void *)dataP;
    }

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);

    return transaction_send(contextP, transaction);
}

int lwm2m_dm_read(lwm2m_context_t * contextP,
                  uint16_t clientID,
                  lwm2m_uri_t * uriP,
                  lwm2m_result_callback_t callback,
                  void * userData)
{
    return prv_make_operation(contextP, clientID, uriP,
                              COAP_GET, NULL, 0,
                              callback, userData);
}

int lwm2m_dm_write(lwm2m_context_t * contextP,
                   uint16_t clientID,
                   lwm2m_uri_t * uriP,
                   char * buffer,
                   int length,
                   lwm2m_result_callback_t callback,
                   void * userData)
{
    if (!LWM2M_URI_IS_SET_INSTANCE(uriP)
     || length == 0)
    {
        return COAP_400_BAD_REQUEST;
    }

    if (LWM2M_URI_IS_SET_RESOURCE(uriP))
    {
        return prv_make_operation(contextP, clientID, uriP,
                                  COAP_PUT, buffer, length,
                                  callback, userData);
    }
    else
    {
        return prv_make_operation(contextP, clientID, uriP,
                                  COAP_POST, buffer, length,
                                  callback, userData);
    }
}

int lwm2m_dm_execute(lwm2m_context_t * contextP,
                     uint16_t clientID,
                     lwm2m_uri_t * uriP,
                     char * buffer,
                     int length,
                     lwm2m_result_callback_t callback,
                     void * userData)
{
    if (!LWM2M_URI_IS_SET_RESOURCE(uriP))
    {
        return COAP_400_BAD_REQUEST;
    }

    return prv_make_operation(contextP, clientID, uriP,
                              COAP_POST, buffer, length,
                              callback, userData);
}

int lwm2m_dm_create(lwm2m_context_t * contextP,
                    uint16_t clientID,
                    lwm2m_uri_t * uriP,
                    char * buffer,
                    int length,
                    lwm2m_result_callback_t callback,
                    void * userData)
{
    if (LWM2M_URI_IS_SET_RESOURCE(uriP)
     || length == 0)
    {
        return COAP_400_BAD_REQUEST;
    }

    return prv_make_operation(contextP, clientID, uriP,
                              COAP_POST, buffer, length,
                              callback, userData);
}

int lwm2m_dm_delete(lwm2m_context_t * contextP,
                    uint16_t clientID,
                    lwm2m_uri_t * uriP,
                    lwm2m_result_callback_t callback,
                    void * userData)
{
    if (!LWM2M_URI_IS_SET_INSTANCE(uriP)
     || LWM2M_URI_IS_SET_RESOURCE(uriP))
    {
        return COAP_400_BAD_REQUEST;
    }

    return prv_make_operation(contextP, clientID, uriP,
                              COAP_DELETE, NULL, 0,
                              callback, userData);
}
#endif
