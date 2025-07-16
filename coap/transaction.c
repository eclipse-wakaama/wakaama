/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
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
 *    David Navarro, Intel Corporation - initial API and implementation
 *    Simon Bernard - Please refer to git log
 *    Toby Jaffey - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Tuve Nordius, Husqvarna Group - Please refer to git log
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

/************************************************************************
 *  Function for communications transactions.
 *
 *  Basic specification: rfc7252
 *
 *  Transaction implements processing of piggybacked and separate response communication
 *  dialogs specified in section 2.2 of the above specification.
 *  The caller registers a callback function, which is called, when either the result is
 *  received or a timeout occurs.
 *
 *  Supported dialogs:
 *  Requests (GET - DELETE):
 *  - CON with mid, without token => regular finished with corresponding ACK.MID
 *  - CON with mid, with token => regular finished with corresponding ACK.MID and response containing
 *                  the token. Supports both versions, with piggybacked ACK and separate ACK/response.
 *                  Though the ACK.MID may be lost in the separate version, a matching response may
 *                  finish the transaction even without the ACK.MID.
 *  - NON without token => no transaction, no result expected!
 *  - NON with token => regular finished with response containing the token.
 *  Responses (COAP_201_CREATED - ?):
 *  - CON with mid => regular finished with corresponding ACK.MID
 */

#include "internals.h"


/*
 * Modulo mask (+1 and +0.5 for rounding) for a random number to get the tick number for the random
 * retransmission time between COAP_RESPONSE_TIMEOUT and COAP_RESPONSE_TIMEOUT*COAP_RESPONSE_RANDOM_FACTOR.
 */
#define COAP_RESPONSE_TIMEOUT_TICKS         (CLOCK_SECOND * COAP_RESPONSE_TIMEOUT)
#define COAP_RESPONSE_TIMEOUT_BACKOFF_MASK  ((CLOCK_SECOND * COAP_RESPONSE_TIMEOUT * (COAP_RESPONSE_RANDOM_FACTOR - 1)) + 1.5)

static int prv_checkFinished(lwm2m_transaction_t * transacP,
                             coap_packet_t * receivedMessage)
{
    int len;
    uint8_t* token;
    coap_packet_t * transactionMessage = (coap_packet_t *) transacP->message;

    if (transactionMessage->mid == receivedMessage->mid) {
        if (COAP_DELETE < transactionMessage->code) {
            // response
            return transacP->ack_received ? 1 : 0;
        }
        if (!IS_OPTION(transactionMessage, COAP_OPTION_TOKEN)) {
            // request without token
            return transacP->ack_received ? 1 : 0;
        }
    }

    if (COAP_DELETE >= transactionMessage->code && IS_OPTION(transactionMessage, COAP_OPTION_TOKEN)) {
        // request with token
        len = coap_get_header_token(receivedMessage, &token);
        if (transactionMessage->token_len == len) {
            if (memcmp(transactionMessage->token, token, len) == 0)
                return 1;
        }
    }

    return 0;
}

lwm2m_transaction_t * transaction_new(void * sessionH,
                                      coap_method_t method,
                                      char * altPath,
                                      lwm2m_uri_t * uriP,
                                      uint16_t mID,
                                      uint8_t token_len,
                                      uint8_t* token)
{
    lwm2m_transaction_t * transacP;
    int result;

    LOG_ARG_DBG("method: %d, altPath: \"%s\", mID: %d, token_len: %d", method, STR_NULL2EMPTY(altPath), mID, token_len);
    LOG_ARG_DBG("%s", LOG_URI_TO_STRING(uriP));

    // no transactions without peer
    if (NULL == sessionH) return NULL;

    transacP = (lwm2m_transaction_t *)lwm2m_malloc(sizeof(lwm2m_transaction_t));

    if (NULL == transacP) return NULL;
    memset(transacP, 0, sizeof(lwm2m_transaction_t));

    transacP->message = lwm2m_malloc(sizeof(coap_packet_t));
    if (NULL == transacP->message) goto error;

    coap_init_message(transacP->message, COAP_TYPE_CON, method, mID);

    transacP->peerH = sessionH;

    transacP->mID = mID;

    if (altPath != NULL)
    {
        // TODO: Support multi-segment alternative path
        coap_set_header_uri_path_segment(transacP->message, altPath + 1);
    }
    if (NULL != uriP && LWM2M_URI_IS_SET_OBJECT(uriP))
    {
        char stringID[LWM2M_STRING_ID_MAX_LEN];

        result = utils_intToText(uriP->objectId, (uint8_t*)stringID, LWM2M_STRING_ID_MAX_LEN);
        if (result == 0) goto error;
        stringID[result] = 0;
        coap_set_header_uri_path_segment(transacP->message, stringID);

        if (LWM2M_URI_IS_SET_INSTANCE(uriP))
        {
            result = utils_intToText(uriP->instanceId, (uint8_t*)stringID, LWM2M_STRING_ID_MAX_LEN);
            if (result == 0) goto error;
            stringID[result] = 0;
            coap_set_header_uri_path_segment(transacP->message, stringID);
            if (LWM2M_URI_IS_SET_RESOURCE(uriP))
            {
                result = utils_intToText(uriP->resourceId, (uint8_t*)stringID, LWM2M_STRING_ID_MAX_LEN);
                if (result == 0) goto error;
                stringID[result] = 0;
                coap_set_header_uri_path_segment(transacP->message, stringID);
#ifndef LWM2M_VERSION_1_0
                if (LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP))
                {
                    result = utils_intToText(uriP->resourceInstanceId, (uint8_t*)stringID, LWM2M_STRING_ID_MAX_LEN);
                    if (result == 0) goto error;
                    stringID[result] = 0;
                    coap_set_header_uri_path_segment(transacP->message, stringID);
                }
#endif
            }
        }
    }
    if (0 < token_len)
    {
        if (NULL != token)
        {
            coap_set_header_token(transacP->message, token, token_len);
        }
        else {
            // generate a token
            uint8_t temp_token[COAP_TOKEN_LEN];
            time_t tv_sec = lwm2m_gettime();

            // initialize first 6 bytes, leave the last 2 random
            temp_token[0] = mID;
            temp_token[1] = mID >> 8;
            temp_token[2] = tv_sec;
            temp_token[3] = tv_sec >> 8;
            temp_token[4] = tv_sec >> 16;
            temp_token[5] = tv_sec >> 24;
            // use just the provided amount of bytes
            coap_set_header_token(transacP->message, temp_token, token_len);
        }
    }

    LOG_ARG_DBG("Exiting on success. new transac=%p", (void *)transacP);
    return transacP;

error:
    LOG_DBG("Exiting on failure");
    if(transacP->message)
    {
        coap_free_header(transacP->message);
        lwm2m_free(transacP->message);
    }
    lwm2m_free(transacP);
    return NULL;
}

void transaction_free(lwm2m_transaction_t * transacP)
{
    LOG_ARG_DBG("Entering. transaction=%p", (void *)transacP);
    if (transacP->message)
    {
       coap_free_header(transacP->message);
       lwm2m_free(transacP->message);
       transacP->message = NULL;
    }

    if (transacP->payload) {
        lwm2m_free(transacP->payload);
        transacP->payload = NULL;
    }

    if (transacP->buffer) {
        lwm2m_free(transacP->buffer);
        transacP->buffer = NULL;
    }

    lwm2m_free(transacP);
}

void transaction_remove(lwm2m_context_t * contextP,
                        lwm2m_transaction_t * transacP)
{
    LOG_ARG_DBG("Entering. transaction=%p", (void *)transacP);
    contextP->transactionList = (lwm2m_transaction_t *) LWM2M_LIST_RM(contextP->transactionList, transacP->mID, NULL);
    transaction_free(transacP);
}

bool transaction_handleResponse(lwm2m_context_t * contextP,
                                 void * fromSessionH,
                                 coap_packet_t * message,
                                 coap_packet_t * response)
{
    bool found = false;
    bool reset = false;
    lwm2m_transaction_t * transacP;

    LOG_DBG("Entering");
    transacP = contextP->transactionList;

    while (NULL != transacP)
    {
        if (lwm2m_session_is_equal(fromSessionH, transacP->peerH, contextP->userData) == true)
        {
            if (!transacP->ack_received)
            {
                if ((COAP_TYPE_ACK == message->type) || (COAP_TYPE_RST == message->type))
                {
                    if (transacP->mID == message->mid)
                    {
                        found = true;
                        transacP->ack_received = true;
                        reset = COAP_TYPE_RST == message->type;
                    }
                }
            }

            if (reset || prv_checkFinished(transacP, message))
            {
                // HACK: If a message is sent from the monitor callback,
                // it will arrive before the registration ACK.
                // So we resend transaction that were denied for authentication reason.
                if (!reset)
                {
                    if (COAP_TYPE_CON == message->type && NULL != response)
                    {
                        coap_init_message(response, COAP_TYPE_ACK, 0, message->mid);
                        message_send(contextP, response, fromSessionH);
                    }

                    if ((COAP_401_UNAUTHORIZED == message->code) && (COAP_MAX_RETRANSMIT > transacP->retrans_counter))
                    {
                        transacP->ack_received = false;
                        transacP->retrans_time += COAP_RESPONSE_TIMEOUT;
                        return true;
                    }
                }
                if (transacP->callback != NULL)
                {
                    transacP->callback(contextP, transacP, message);
                }
                transaction_remove(contextP, transacP);
                return true;
            }
            // if we found our guy, exit
            if (found)
            {
                time_t tv_sec = lwm2m_gettime();
                if (0 <= tv_sec)
                {
                    transacP->retrans_time = tv_sec;
                }
                if (message->code == COAP_EMPTY_MESSAGE_CODE) {
                    // if empty ack received, set timeout for separate response
                    transacP->retrans_time += COAP_SEPARATE_TIMEOUT;
                } else {
                    transacP->retrans_time += COAP_RESPONSE_TIMEOUT * transacP->retrans_counter;
                }
                return true;
            }
        }

        transacP = transacP->next;
    }
    return false;
}

int transaction_send(lwm2m_context_t * contextP,
                     lwm2m_transaction_t * transacP)
{
    bool maxRetriesReached = false;

    LOG_ARG_DBG("Entering: transaction=%p", (void *)transacP);
    if (transacP->buffer == NULL)
    {
        transacP->buffer_len = coap_serialize_get_size(transacP->message);
        if (transacP->buffer_len == 0)
        {
           transaction_remove(contextP, transacP);
           return COAP_500_INTERNAL_SERVER_ERROR;
        }

        transacP->buffer = (uint8_t*)lwm2m_malloc(transacP->buffer_len);
        if (transacP->buffer == NULL)
        {
           transaction_remove(contextP, transacP);
           return COAP_500_INTERNAL_SERVER_ERROR;
        }

        transacP->buffer_len = coap_serialize_message(transacP->message, transacP->buffer);
        if (transacP->buffer_len == 0)
        {
            lwm2m_free(transacP->buffer);
            transacP->buffer = NULL;
            transaction_remove(contextP, transacP);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
    }

    if (!transacP->ack_received)
    {
        long unsigned timeout = 0;

        if (0 == transacP->retrans_counter)
        {
            time_t tv_sec = lwm2m_gettime();
            if (0 <= tv_sec)
            {
                transacP->retrans_time = tv_sec + COAP_RESPONSE_TIMEOUT;
                transacP->retrans_counter = 1;
            }
            else
            {
                maxRetriesReached = true;
            }
        }
        else
        {
            timeout = COAP_RESPONSE_TIMEOUT << (transacP->retrans_counter - 1);
        }

        if (COAP_MAX_RETRANSMIT + 1 >= transacP->retrans_counter)
        {
            (void)lwm2m_buffer_send(transacP->peerH, transacP->buffer, transacP->buffer_len, contextP->userData);

            transacP->retrans_time += timeout;
            transacP->retrans_counter += 1;
        }
        else
        {
            maxRetriesReached = true;
        }
    }
    else
    {
        LOG_WARN("ACK received but separate response timed out!");
        goto error;
    }
    if (maxRetriesReached)
    {
        goto error;
    }

    return 0;
error:
    if (transacP->callback)
    {
        LOG_ARG_DBG("transaction %p expired..calling callback", (void *)transacP);
        transacP->callback(contextP, transacP, NULL);
    }
    transaction_remove(contextP, transacP);
    return -1;
}

void transaction_step(lwm2m_context_t * contextP,
                      time_t currentTime,
                      time_t * timeoutP)
{
    lwm2m_transaction_t * transacP;

    LOG_DBG("Entering");
    transacP = contextP->transactionList;
    while (transacP != NULL)
    {
        // transaction_send() may remove transaction from the linked list
        lwm2m_transaction_t * nextP = transacP->next;
        int removed = 0;

        if (transacP->retrans_time <= currentTime)
        {
            removed = transaction_send(contextP, transacP);
        }

        if (0 == removed)
        {
            time_t interval;

            if (transacP->retrans_time > currentTime)
            {
                interval = transacP->retrans_time - currentTime;
            }
            else
            {
                interval = 1;
            }

            if (*timeoutP > interval)
            {
                *timeoutP = interval;
            }
        }
        else
        {
            *timeoutP = 1;
        }

        transacP = nextP;
    }
}

bool transaction_set_payload(lwm2m_transaction_t *transaction, uint8_t *buffer, size_t length) {
    // copy payload as we might need it beyond scope of the current request / method call (e.g. in case of
    // retransmissions or block transfer)
    uint8_t *transaction_payload = (uint8_t *)lwm2m_malloc(length);
    if (transaction_payload == NULL) {
        return false;
    }
    memcpy(transaction_payload, buffer, length);

    transaction->payload = transaction_payload;
    transaction->payload_len = length;
    const uint16_t lwm2m_coap_block_size = lwm2m_get_coap_block_size();
    if (length > lwm2m_coap_block_size) {
        coap_set_header_block1(transaction->message, 0, true, lwm2m_coap_block_size);
    }

    coap_set_payload(transaction->message, buffer, MIN(length, lwm2m_coap_block_size));
    return true;
}

bool transaction_free_userData(lwm2m_context_t * context, lwm2m_transaction_t * transaction)
{
    lwm2m_transaction_t * target = context->transactionList;
    while (target != NULL){
        if (target->userData == transaction->userData && target != transaction) return false;
        target = target->next;
    }
    lwm2m_free(transaction->userData);
    transaction->userData = NULL;
    return true;
}

/*
 * Remove transactions from a specific client.
 */
void transaction_remove_client(lwm2m_context_t *contextP, lwm2m_client_t *clientP) {
    lwm2m_transaction_t *transacP;

    LOG_DBG("Entering");
    transacP = contextP->transactionList;
    while (transacP != NULL) {
        lwm2m_transaction_t *nextP = transacP->next;

        if (lwm2m_session_is_equal(transacP->peerH, clientP->sessionH, contextP->userData)) {
            LOG_DBG("Found session to remove");
            transaction_remove(contextP, transacP);
        }
        transacP = nextP;
    }
}
