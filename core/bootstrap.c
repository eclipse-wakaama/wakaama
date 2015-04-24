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
 *
 *******************************************************************************/

#ifdef LWM2M_BOOTSTRAP
#ifdef LWM2M_CLIENT_MODE

#include "internals.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PRV_QUERY_BUFFER_LENGTH 200

static void prv_handleBootstrapReply(lwm2m_transaction_t * transaction, void * message)
{
    LOG("[BOOTSTRAP] Handling bootstrap reply...\r\n");
    lwm2m_context_t * context = (lwm2m_context_t *)transaction->userData;
    coap_packet_t * coapMessage = (coap_packet_t *)message;
    if ((NULL != coapMessage) && (coapMessage->type == COAP_TYPE_ACK))
    {
        handle_bootstrap_ack(context, coapMessage, NULL);
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
            transaction = transaction_new(COAP_POST, NULL, NULL, context->nextMID++, ENDPOINT_SERVER, (void *)bootstrapServer);
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
                bootstrapServer->mid = transaction->mID;
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

void handle_bootstrap_ack(lwm2m_context_t * context,
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
#endif

#endif
