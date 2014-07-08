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
 *    Simon Bernard - Please refer to git log
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

/*
Contains code snippets which are:

 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.

*/


#include "internals.h"


/*
 * Modulo mask (+1 and +0.5 for rounding) for a random number to get the tick number for the random
 * retransmission time between COAP_RESPONSE_TIMEOUT and COAP_RESPONSE_TIMEOUT*COAP_RESPONSE_RANDOM_FACTOR.
 */
#define COAP_RESPONSE_TIMEOUT_TICKS         (CLOCK_SECOND * COAP_RESPONSE_TIMEOUT)
#define COAP_RESPONSE_TIMEOUT_BACKOFF_MASK  ((CLOCK_SECOND * COAP_RESPONSE_TIMEOUT * (COAP_RESPONSE_RANDOM_FACTOR - 1)) + 1.5)

static int prv_check_addr(void * leftSessionH,
                          void * rightSessionH)
{
    if ((leftSessionH == NULL)
     || (rightSessionH == NULL)
     || (leftSessionH != rightSessionH))
    {
        return 0;
    }

    return 1;
}

lwm2m_transaction_t * transaction_new(coap_method_t method,
                                      lwm2m_uri_t * uriP,
                                      uint16_t mID,
                                      lwm2m_endpoint_type_t peerType,
                                      void * peerP)
{
    lwm2m_transaction_t * transacP;
    int result;

    transacP = (lwm2m_transaction_t *)lwm2m_malloc(sizeof(lwm2m_transaction_t));

    if (transacP == NULL) return NULL;
    memset(transacP, 0, sizeof(lwm2m_transaction_t));

    transacP->message = lwm2m_malloc(sizeof(coap_packet_t));
    if (transacP->message == NULL) goto error;

    coap_init_message(transacP->message, COAP_TYPE_CON, method, mID);

    transacP->mID = mID;
    transacP->peerType = peerType;
    transacP->peerP = peerP;

    if (uriP != NULL)
    {
        result = snprintf(transacP->objStringID, LWM2M_STRING_ID_MAX_LEN, "%hu", uriP->objectId);
        if (result < 0 || result > LWM2M_STRING_ID_MAX_LEN) goto error;

        coap_set_header_uri_path_segment(transacP->message, transacP->objStringID);

        if (LWM2M_URI_IS_SET_INSTANCE(uriP))
        {
            result = snprintf(transacP->instanceStringID, LWM2M_STRING_ID_MAX_LEN, "%hu", uriP->instanceId);
            if (result < 0 || result > LWM2M_STRING_ID_MAX_LEN) goto error;
            coap_set_header_uri_path_segment(transacP->message, transacP->instanceStringID);
        }
        else
        {
            if (LWM2M_URI_IS_SET_RESOURCE(uriP))
            {
                coap_set_header_uri_path_segment(transacP->message, NULL);
            }
        }
        if (LWM2M_URI_IS_SET_RESOURCE(uriP))
        {
            result = snprintf(transacP->resourceStringID, LWM2M_STRING_ID_MAX_LEN, "%hu", uriP->resourceId);
            if (result < 0 || result > LWM2M_STRING_ID_MAX_LEN) goto error;
            coap_set_header_uri_path_segment(transacP->message, transacP->resourceStringID);
        }
    }

    return transacP;

error:
    lwm2m_free(transacP);
    return NULL;
}

void transaction_free(lwm2m_transaction_t * transacP)
{
    if (transacP->message) lwm2m_free(transacP->message);
    if (transacP->buffer) lwm2m_free(transacP->buffer);
    lwm2m_free(transacP);
}

void transaction_remove(lwm2m_context_t * contextP,
                        lwm2m_transaction_t * transacP)
{
    if (NULL != contextP->transactionList)
    {
        if (transacP == contextP->transactionList)
        {
            contextP->transactionList = contextP->transactionList->next;
        }
        else
        {
            lwm2m_transaction_t *previous = contextP->transactionList;
            while (previous->next && previous->next != transacP)
            {
                previous = previous->next;
            }
            if (NULL != previous->next)
            {
                previous->next = previous->next->next;
            }
        }
    }
    transaction_free(transacP);
}


void transaction_handle_response(lwm2m_context_t * contextP,
                                 void * fromSessionH,
                                 coap_packet_t * message)
{
    lwm2m_transaction_t * transacP;

    transacP = contextP->transactionList;

    while (transacP != NULL)
    {
        void * targetSessionH;

        targetSessionH = NULL;
        switch (transacP->peerType)
        {
    #ifdef LWM2M_SERVER_MODE
        case ENDPOINT_CLIENT:
            targetSessionH = ((lwm2m_client_t *)transacP->peerP)->sessionH;
            break;
    #endif

    #ifdef LWM2M_CLIENT_MODE
        case ENDPOINT_SERVER:
            targetSessionH = ((lwm2m_server_t *)transacP->peerP)->sessionH;
            break;
    #endif

        default:
            break;
        }

        if (prv_check_addr(fromSessionH, targetSessionH))
        {
            if (transacP->mID == message->mid)
            {
                // HACK: If a message is sent from the monitor callback,
                // it will arrive before the registration ACK.
                // So we resend transaction that were denied for authentication reason.
                if (message->code != COAP_401_UNAUTHORIZED || transacP->retrans_counter >= COAP_MAX_RETRANSMIT)
                {
                    if (transacP->callback != NULL)
                    {
                        transacP->callback(transacP, message);
                    }
                    transaction_remove(contextP, transacP);
                }
                // we found our guy, exit
                return;
            }
        }

        transacP = transacP->next;
    }
}

int transaction_send(lwm2m_context_t * contextP,
                     lwm2m_transaction_t * transacP)
{
    if (transacP->buffer == NULL)
    {
        uint8_t tempBuffer[LWM2M_MAX_PACKET_SIZE];
        int length;

        length = coap_serialize_message(transacP->message, tempBuffer);
        if (length <= 0) return COAP_500_INTERNAL_SERVER_ERROR;

        transacP->buffer = (uint8_t*)lwm2m_malloc(length);
        if (transacP->buffer == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

        memcpy(transacP->buffer, tempBuffer, length);
        transacP->buffer_len = length;
    }

    switch(transacP->peerType)
    {
    case ENDPOINT_CLIENT:
        LOG("Sending %d bytes\r\n", transacP->buffer_len);
        contextP->bufferSendCallback(((lwm2m_client_t*)transacP->peerP)->sessionH,
                                     transacP->buffer, transacP->buffer_len, contextP->bufferSendUserData);

        break;

    case ENDPOINT_SERVER:
        LOG("Sending %d bytes\r\n", transacP->buffer_len);
        contextP->bufferSendCallback(((lwm2m_server_t*)transacP->peerP)->sessionH,
                                     transacP->buffer, transacP->buffer_len, contextP->bufferSendUserData);
        break;

    case ENDPOINT_BOOTSTRAP:
        // not implemented yet
        break;

    default:
        return 0;
    }

    if (transacP->retrans_counter == 0)
    {
        struct timeval tv;

        if (0 == lwm2m_gettimeofday(&tv, NULL))
        {
            transacP->retrans_time = tv.tv_sec;
            transacP->retrans_counter = 1;
        }
        else
        {
            // crude error handling
            transacP->retrans_counter = COAP_MAX_RETRANSMIT;
        }
    }

    if (transacP->retrans_counter < COAP_MAX_RETRANSMIT)
    {
        transacP->retrans_time += COAP_RESPONSE_TIMEOUT * transacP->retrans_counter;
        transacP->retrans_counter++;
    }
    else
    {
        if (transacP->callback)
        {
            transacP->callback(transacP, NULL);
        }
        transaction_remove(contextP, transacP);
        return -1;
    }

    return 0;
}
