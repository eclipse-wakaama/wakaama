/*******************************************************************************
 *
 * Copyright (c) 2015 Sierra Wireless and others.
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
 *    Pascal Rieux - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    David Navarro, Intel Corporation - Please refer to git log
 *
 *******************************************************************************/

#include "internals.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef LWM2M_BOOTSTRAP
#ifdef LWM2M_CLIENT_MODE

#define PRV_QUERY_BUFFER_LENGTH 200

static void prv_handleBootstrapReply(lwm2m_transaction_t * transaction, void * message)
{
    LOG("[BOOTSTRAP] Handling bootstrap reply...\r\n");
    lwm2m_context_t * context = (lwm2m_context_t *)transaction->userData;
    coap_packet_t * coapMessage = (coap_packet_t *)message;
    if (NULL != coapMessage && COAP_TYPE_RST != coapMessage->type)
    {
        handle_bootstrap_response(context, coapMessage, NULL);
    }
    else
    {
        bootstrap_failed(context);
    }
}

// start a device initiated bootstrap
int bootstrap_initiating_request(lwm2m_context_t * context)
{
    char query[PRV_QUERY_BUFFER_LENGTH];
    int query_length = 0;
    lwm2m_transaction_t * transaction = NULL;

    query_length = snprintf(query, sizeof(query), "?ep=%s", context->endpointName);
    if (query_length <= 1)
    {
        return INTERNAL_SERVER_ERROR_5_00;
    }

    // find the first bootstrap server
    lwm2m_server_t * bootstrapServer = context->bootstrapServerList;
    while (bootstrapServer != NULL)
    {
        if (bootstrapServer->sessionH == NULL)
        {
            bootstrapServer->sessionH = context->connectCallback(bootstrapServer->secObjInstID, context->userData);
        }
        if (bootstrapServer->sessionH != NULL)
        {
            LOG("[BOOTSTRAP] Bootstrap session starting...\r\n");
            transaction = transaction_new(COAP_TYPE_CON, COAP_POST, NULL, NULL, context->nextMID++, 4, NULL, ENDPOINT_SERVER, (void *)bootstrapServer);
            if (transaction == NULL)
            {
                return INTERNAL_SERVER_ERROR_5_00;
            }
            coap_set_header_uri_path(transaction->message, "/"URI_BOOTSTRAP_SEGMENT);
            coap_set_header_uri_query(transaction->message, query);
            transaction->callback = prv_handleBootstrapReply;
            transaction->userData = (void *)context;
            context->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(context->transactionList, transaction);
            if (transaction_send(context, transaction) == 0)
            {
                LOG("[BOOTSTRAP] DI bootstrap requested to BS server\r\n");
                context->bsState = BOOTSTRAP_INITIATED;
                reset_bootstrap_timer(context);
            }
        }
        else
        {
            LOG("No bootstrap session handler found\r\n");
        }
        bootstrapServer = bootstrapServer->next;
    }
    return NO_ERROR;
}

void handle_bootstrap_response(lwm2m_context_t * context,
        coap_packet_t * message,
        void * fromSessionH)
{
    if (COAP_204_CHANGED == message->code)
    {
        context->bsState = BOOTSTRAP_PENDING;
        LOG("[BOOTSTRAP] Received ACK/2.04, Bootstrap pending, waiting for DEL/PUT from BS server...\r\n");
        reset_bootstrap_timer(context);
    }
    else
    {
        bootstrap_failed(context);
    }
}

void bootstrap_failed(lwm2m_context_t * context)
{
    reset_bootstrap_timer(context);
    context->bsState = BOOTSTRAP_FAILED;
    LOG("[BOOTSTRAP] Bootstrap failed\r\n");
}

void reset_bootstrap_timer(lwm2m_context_t * context)
{
    context->bsStart = lwm2m_gettime();
}

void update_bootstrap_state(lwm2m_context_t * context,
        uint32_t currentTime,
        time_t* timeoutP)
{
    if (context->bsState == BOOTSTRAP_REQUESTED)
    {
        context->bsState = BOOTSTRAP_CLIENT_HOLD_OFF;
        context->bsStart = currentTime;
        LOG("[BOOTSTRAP] Bootstrap requested at: %lu, now waiting during ClientHoldOffTime...\r\n",
                (unsigned long)context->bsStart);
    }
    if (context->bsState == BOOTSTRAP_CLIENT_HOLD_OFF)
    {
        lwm2m_server_t * bootstrapServer = context->bootstrapServerList;
        if (bootstrapServer != NULL)
        {
            // get ClientHoldOffTime from bootstrapServer->lifetime
            // (see objects.c => object_getServers())
            int32_t timeToBootstrap = (context->bsStart + bootstrapServer->lifetime) - currentTime;
            LOG("[BOOTSTRAP] ClientHoldOffTime %ld\r\n", (long)timeToBootstrap);
            if (0 >= timeToBootstrap)
            {
                bootstrap_initiating_request(context);
            }
            else if (timeToBootstrap < *timeoutP)
            {
                *timeoutP = timeToBootstrap;
            }
        }
        else
        {
            bootstrap_failed(context);
        }
    }
    if (context->bsState == BOOTSTRAP_PENDING)
    {
        // Use COAP_DEFAULT_MAX_AGE according proposal in
        // https://github.com/OpenMobileAlliance/OMA-LwM2M-Public-Review/issues/35
        int32_t timeToBootstrap = (context->bsStart + COAP_DEFAULT_MAX_AGE) - currentTime;
        LOG("[BOOTSTRAP] Pending %ld\r\n", (long)timeToBootstrap);
        if (0 >= timeToBootstrap)
        {
            // Time out and no error => bootstrap OK
            // TODO: add smarter condition for bootstrap success:
            // 1) security object contains at least one bootstrap server
            // 2) there are coherent configurations for provisioned DM servers
            // if these conditions are not met, then bootstrap has failed and previous security
            // and server object configurations might be restored by client
            LOG("\r\n[BOOTSTRAP] Bootstrap finished at: %lu (difftime: %lu s)\r\n",
                    (unsigned long)currentTime, (unsigned long)(currentTime - context->bsStart));
            context->bsState = BOOTSTRAP_FINISHED;
            context->bsStart = currentTime;
            *timeoutP = 1;
        }
        else if (timeToBootstrap < *timeoutP)
        {
            *timeoutP = timeToBootstrap;
        }
    }
    else if (context->bsState == BOOTSTRAP_FINISHED)
    {
        context->bsStart = currentTime;
        if (0 <= lwm2m_start(context))
        {
            context->bsState = BOOTSTRAPPED;
        }
        else
        {
            bootstrap_failed(context);
        }
        // during next step, lwm2m_update_registrations will connect the client to DM server
    }
    if (BOOTSTRAP_FAILED == context->bsState)
    {
        lwm2m_server_t * bootstrapServer = context->bootstrapServerList;
        if (bootstrapServer != NULL)
        {
            // get ClientHoldOffTime from bootstrapServer->lifetime
            // (see objects.c => object_getServers())
            int32_t timeToBootstrap = (context->bsStart + bootstrapServer->lifetime) - currentTime;
            LOG("[BOOTSTRAP] Bootstrap failed: %lu, now waiting during ClientHoldOffTime %ld ...\r\n",
                    (unsigned long)context->bsStart, (long)timeToBootstrap);
            if (0 >= timeToBootstrap)
            {
                context->bsState = NOT_BOOTSTRAPPED;
                context->bsStart = currentTime;
                LOG("[BOOTSTRAP] Bootstrap failed: retry ...\r\n");
                *timeoutP = 1;
            }
            else if (timeToBootstrap < *timeoutP)
            {
                *timeoutP = timeToBootstrap;
            }
        }
    }
}

coap_status_t handle_bootstrap_finish(lwm2m_context_t * context,
                                      void * fromSessionH)
{
    if (context->bsState == BOOTSTRAP_PENDING)
    {
        context->bsState = BOOTSTRAP_FINISHED;
        return COAP_204_CHANGED;
    }

    return COAP_IGNORE;
}
#endif

#endif

#ifdef LWM2M_BOOTSTRAP_SERVER_MODE
uint8_t handle_bootstrap_request(lwm2m_context_t * contextP,
                                 lwm2m_uri_t * uriP,
                                 void * fromSessionH,
                                 coap_packet_t * message,
                                 coap_packet_t * response)
{
    uint8_t result;
    char * name;

    if (contextP->bootstrapCallback == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
    if (message->code != COAP_POST) return COAP_400_BAD_REQUEST;
    if (message->uri_query == NULL) return COAP_400_BAD_REQUEST;
    if (message->payload != NULL) return COAP_400_BAD_REQUEST;

    if (lwm2m_strncmp((char *)message->uri_query->data, QUERY_TEMPLATE, QUERY_LENGTH) != 0)
    {
        return COAP_400_BAD_REQUEST;
    }

    if (message->uri_query->len == QUERY_LENGTH) return COAP_400_BAD_REQUEST;
    if (message->uri_query->next != NULL) return COAP_400_BAD_REQUEST;

    name = (char *)lwm2m_malloc(message->uri_query->len - QUERY_LENGTH + 1);
    if (name == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

    memcpy(name, message->uri_query->data + QUERY_LENGTH, message->uri_query->len - QUERY_LENGTH);
    name[message->uri_query->len - QUERY_LENGTH] = 0;

    result = contextP->bootstrapCallback(fromSessionH, COAP_NO_ERROR, NULL, name, contextP->bootstrapUserData);

    lwm2m_free(name);

    return result;
}

void lwm2m_set_bootstrap_callback(lwm2m_context_t * contextP,
                                  lwm2m_bootstrap_callback_t callback,
                                  void * userData)
{
    contextP->bootstrapCallback = callback;
    contextP->bootstrapUserData = userData;
}

static void bs_result_callback(lwm2m_transaction_t * transacP,
                               void * message)
{
    bs_data_t * dataP = (bs_data_t *)transacP->userData;
    lwm2m_uri_t * uriP;

    if (dataP->isUri == true)
    {
        uriP = &dataP->uri;
    }
    else
    {
        uriP = NULL;
    }

    if (message == NULL)
    {
        dataP->callback(transacP->peerP,
                        COAP_503_SERVICE_UNAVAILABLE,
                        uriP,
                        NULL,
                        dataP->userData);
    }
    else
    {
        coap_packet_t * packet = (coap_packet_t *)message;

        dataP->callback(transacP->peerP,
                        packet->code,
                        uriP,
                        NULL,
                        dataP->userData);
    }
    lwm2m_free(dataP);
}

int lwm2m_bootstrap_delete(lwm2m_context_t * contextP,
                           void * sessionH,
                           lwm2m_uri_t * uriP)
{
    lwm2m_transaction_t * transaction;
    bs_data_t * dataP;

    transaction = transaction_new(COAP_TYPE_CON, COAP_DELETE, NULL, uriP, contextP->nextMID++, 4, NULL, ENDPOINT_UNKNOWN, sessionH);
    if (transaction == NULL) return INTERNAL_SERVER_ERROR_5_00;

    dataP = (bs_data_t *)lwm2m_malloc(sizeof(bs_data_t));
    if (dataP == NULL)
    {
        transaction_free(transaction);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }
    if (uriP == NULL)
    {
        dataP->isUri = false;
    }
    else
    {
        dataP->isUri = true;
        memcpy(&dataP->uri, uriP, sizeof(lwm2m_uri_t));
    }
    dataP->callback = contextP->bootstrapCallback;
    dataP->userData = contextP->bootstrapUserData;

    transaction->callback = bs_result_callback;
    transaction->userData = (void *)dataP;

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);

    return transaction_send(contextP, transaction);
}

int lwm2m_bootstrap_write(lwm2m_context_t * contextP,
                          void * sessionH,
                          lwm2m_uri_t * uriP,
                          uint8_t * buffer,
                          size_t length)
{
    lwm2m_transaction_t * transaction;
    bs_data_t * dataP;

    if (uriP == NULL
    || buffer == NULL
    || length == 0)
    {
        return COAP_400_BAD_REQUEST;
    }

    transaction = transaction_new(COAP_TYPE_CON, COAP_PUT, NULL, uriP, contextP->nextMID++, 4, NULL, ENDPOINT_UNKNOWN, sessionH);
    if (transaction == NULL) return INTERNAL_SERVER_ERROR_5_00;

    coap_set_payload(transaction->message, buffer, length);

    dataP = (bs_data_t *)lwm2m_malloc(sizeof(bs_data_t));
    if (dataP == NULL)
    {
        transaction_free(transaction);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }
    dataP->isUri = true;
    memcpy(&dataP->uri, uriP, sizeof(lwm2m_uri_t));
    dataP->callback = contextP->bootstrapCallback;
    dataP->userData = contextP->bootstrapUserData;

    transaction->callback = bs_result_callback;
    transaction->userData = (void *)dataP;

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);

    return transaction_send(contextP, transaction);
}

int lwm2m_bootstrap_finish(lwm2m_context_t * contextP,
                           void * sessionH)
{
    lwm2m_transaction_t * transaction;
    bs_data_t * dataP;

    transaction = transaction_new(COAP_TYPE_CON, COAP_PUT, NULL, NULL, contextP->nextMID++, 4, NULL, ENDPOINT_UNKNOWN, sessionH);
    if (transaction == NULL) return INTERNAL_SERVER_ERROR_5_00;

    coap_set_header_uri_path(transaction->message, "/"URI_BOOTSTRAP_SEGMENT);

    dataP = (bs_data_t *)lwm2m_malloc(sizeof(bs_data_t));
    if (dataP == NULL)
    {
        transaction_free(transaction);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }
    dataP->isUri = false;
    dataP->callback = contextP->bootstrapCallback;
    dataP->userData = contextP->bootstrapUserData;

    transaction->callback = bs_result_callback;
    transaction->userData = (void *)dataP;

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);

    return transaction_send(contextP, transaction);
}

#endif
