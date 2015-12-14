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

#ifdef LWM2M_CLIENT_MODE

#define PRV_QUERY_BUFFER_LENGTH 200

static void bootstrap_failed(lwm2m_server_t * bootstrapServer)
{
    LOG("[BOOTSTRAP] Bootstrap failed\r\n");

    bootstrapServer->status = STATE_BS_FAILED;
}

static void handle_bootstrap_response(lwm2m_server_t * bootstrapServer,
                                      coap_packet_t * message)
{
    if (COAP_204_CHANGED == message->code)
    {
        LOG("[BOOTSTRAP] Received ACK/2.04, Bootstrap pending, waiting for DEL/PUT from BS server...\r\n");
        bootstrapServer->status = STATE_BS_PENDING;
    }
    else
    {
        bootstrap_failed(bootstrapServer);
    }
}

static void prv_handleBootstrapReply(lwm2m_transaction_t * transaction,
                                     void * message)
{
    lwm2m_server_t * bootstrapServer = (lwm2m_server_t *)transaction->userData;
    coap_packet_t * coapMessage = (coap_packet_t *)message;

    LOG("[BOOTSTRAP] Handling bootstrap reply...\r\n");

    if (bootstrapServer->status == STATE_BS_INITIATED)
    {
        if (NULL != coapMessage && COAP_TYPE_RST != coapMessage->type)
        {
            handle_bootstrap_response(bootstrapServer, coapMessage);
        }
        else
        {
            bootstrap_failed(bootstrapServer);
        }
    }
}

// start a device initiated bootstrap
static void bootstrap_initiating_request(lwm2m_context_t * context,
                                         lwm2m_server_t * bootstrapServer)
{
    char query[PRV_QUERY_BUFFER_LENGTH];
    int query_length = 0;

    query_length = snprintf(query, sizeof(query), "?ep=%s", context->endpointName);
    if (query_length <= 1)
    {
        bootstrapServer->status = STATE_BS_FAILED;
        return;
    }

    if (bootstrapServer->sessionH == NULL)
    {
        bootstrapServer->sessionH = context->connectCallback(bootstrapServer->secObjInstID, context->userData);
    }

    if (bootstrapServer->sessionH != NULL)
    {
        lwm2m_transaction_t * transaction = NULL;

        LOG("[BOOTSTRAP] Bootstrap session starting...\r\n");

        transaction = transaction_new(COAP_TYPE_CON, COAP_POST, NULL, NULL, context->nextMID++, 4, NULL, ENDPOINT_SERVER, (void *)bootstrapServer);
        if (transaction == NULL)
        {
            bootstrapServer->status = STATE_BS_FAILED;
            return;
        }

        coap_set_header_uri_path(transaction->message, "/"URI_BOOTSTRAP_SEGMENT);
        coap_set_header_uri_query(transaction->message, query);
        transaction->callback = prv_handleBootstrapReply;
        transaction->userData = (void *)bootstrapServer;
        context->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(context->transactionList, transaction);
        if (transaction_send(context, transaction) == 0)
        {
            LOG("[BOOTSTRAP] CI bootstrap requested to BS server\r\n");
            bootstrapServer->status = STATE_BS_INITIATED;
        }
    }
    else
    {
        LOG("No bootstrap session handler found\r\n");
        bootstrapServer->status = STATE_BS_FAILED;
    }
}

void bootstrap_step(lwm2m_context_t * contextP,
                    uint32_t currentTime,
                    time_t* timeoutP)
{
    lwm2m_server_t * targetP;

    targetP = contextP->bootstrapServerList;
    while (targetP != NULL)
    {
        switch (targetP->status)
        {
        case STATE_DEREGISTERED:
            targetP->registration = currentTime + targetP->lifetime;
            targetP->status = STATE_BS_HOLD_OFF;
            if (*timeoutP > targetP->lifetime)
            {
                *timeoutP = targetP->lifetime;
            }
            break;

        case STATE_BS_HOLD_OFF:
            if (targetP->registration <= currentTime)
            {
                bootstrap_initiating_request(contextP, targetP);
            }
            else if (*timeoutP > targetP->registration - currentTime)
            {
                *timeoutP = targetP->registration - currentTime;
            }
            break;

        case STATE_BS_INITIATED:
        case STATE_BS_PENDING:
            // waiting
            break;

        case STATE_BS_FINISHED:
            // do nothing
            break;

        case STATE_BS_FAILED:
            // do nothing
            break;

        default:
            break;
        }

        targetP = targetP->next;
    }
}

coap_status_t handle_bootstrap_finish(lwm2m_context_t * context,
                                      void * fromSessionH)
{
    lwm2m_server_t * bootstrapServer;

    bootstrapServer = utils_findBootstrapServer(context, fromSessionH);
    if (bootstrapServer != NULL
     && bootstrapServer->status == STATE_BS_PENDING)
    {
        bootstrapServer->status = STATE_BS_FINISHED;
        return COAP_204_CHANGED;
    }

    return COAP_IGNORE;
}

/*
 * Reset the bootstrap servers statuses
 *
 * TODO: handle LWM2M Servers the client is registered to ?
 *
 */
void bootstrap_start(lwm2m_context_t * contextP)
{
    lwm2m_server_t * targetP;

    targetP = contextP->bootstrapServerList;
    while (targetP != NULL)
    {
        targetP->status = STATE_DEREGISTERED;
        targetP = targetP->next;
    }
}

/*
 * Returns STATE_BS_PENDING if at least one bootstrap is still pending
 * Returns STATE_BS_FINISHED if at least one bootstrap succeeded and no bootstrap is pending
 * Returns STATE_BS_FAILED if all bootstrap failed.
 */
lwm2m_status_t bootstrap_get_status(lwm2m_context_t * contextP)
{
    lwm2m_server_t * targetP;
    lwm2m_status_t bs_status;

    targetP = contextP->bootstrapServerList;
    bs_status = STATE_BS_FAILED;

    while (targetP != NULL)
    {
        switch (targetP->status)
        {
            case STATE_BS_FINISHED:
                if (bs_status == STATE_BS_FAILED)
                {
                    bs_status = STATE_BS_FINISHED;
                }
                break;

            case STATE_BS_HOLD_OFF:
            case STATE_BS_INITIATED:
            case STATE_BS_PENDING:
                bs_status = STATE_BS_PENDING;
                break;

            default:
                break;
        }
        targetP = targetP->next;
    }

    return bs_status;
}

static coap_status_t prv_check_server_status(lwm2m_server_t * serverP)
{
    switch (serverP->status)
    {
    case STATE_DEREGISTERED:
        // Should not happen
        return COAP_IGNORE;

    case STATE_BS_HOLD_OFF:
        serverP->status = STATE_BS_PENDING;
        break;

    case STATE_BS_INITIATED:
        // The ACK was probably lost
        serverP->status = STATE_BS_PENDING;
        break;

    case STATE_BS_PENDING:
        // do nothing
        break;

    case STATE_BS_FINISHED:
    case STATE_BS_FAILED:
    default:
        return COAP_IGNORE;
    }

    return COAP_NO_ERROR;
}

coap_status_t handle_bootstrap_command(lwm2m_context_t * contextP,
                                       lwm2m_uri_t * uriP,
                                       lwm2m_server_t * serverP,
                                       coap_packet_t * message,
                                       coap_packet_t * response)
{
    coap_status_t result;
    lwm2m_media_type_t format;

    format = prv_convertMediaType(message->content_type);

    result = prv_check_server_status(serverP);
    if (result != COAP_NO_ERROR) return result;

    switch (message->code)
    {
    case COAP_PUT:
        {
            if (LWM2M_URI_IS_SET_INSTANCE(uriP))
            {
                if (object_isInstanceNew(contextP, uriP->objectId, uriP->instanceId))
                {
                    result = object_create(contextP, uriP, format, message->payload, message->payload_len);
                    if (COAP_201_CREATED == result)
                    {
                        result = COAP_204_CHANGED;
                    }
                }
                else
                {
                    result = object_write(contextP, uriP, format, message->payload, message->payload_len);
                }
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

    case COAP_GET:
    case COAP_POST:
    default:
        result = BAD_REQUEST_4_00;
        break;
    }

    return result;
}

static void management_delete_all_instances(lwm2m_object_t * object)
{
    if (NULL != object->deleteFunc)
    {
        while (NULL != object->instanceList)
        {
            object->deleteFunc(object->instanceList->id, object);
        }
    }
}

coap_status_t handle_delete_all(lwm2m_context_t * contextP,
                                void * fromSessionH)
{
    lwm2m_server_t * serverP;
    coap_status_t result;
    int i;

    serverP = utils_findBootstrapServer(contextP, fromSessionH);
    if (serverP == NULL) return COAP_IGNORE;
    result = prv_check_server_status(serverP);
    if (result != COAP_NO_ERROR) return result;

    result = COAP_202_DELETED;
    for (i = 0 ; i < contextP->numObject && result == COAP_202_DELETED ; i++)
    {
        lwm2m_object_t * objectP = contextP->objectList[i];

        if (NULL != objectP->instanceList)
        {
            if (objectP->deleteFunc)
            {
                lwm2m_list_t * targetP;

                targetP = objectP->instanceList;
                while (NULL != targetP
                    && result == COAP_202_DELETED)
                {
                    if (objectP->objID == LWM2M_SECURITY_OBJECT_ID
                     && targetP->id == serverP->secObjInstID)
                    {
                        // Do not delete the Security Object instance matching the current bootstrap server
                        targetP = targetP->next;
                    }
                    else
                    {
                        result = objectP->deleteFunc(targetP->id, objectP);
                        targetP = objectP->instanceList;
                    }
                }
            }
            else result = COAP_501_NOT_IMPLEMENTED;
        }
    }

    return DELETED_2_02;
}
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
                          lwm2m_media_type_t format,
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

    coap_set_header_content_type(transaction->message, format);
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

    transaction = transaction_new(COAP_TYPE_CON, COAP_POST, NULL, NULL, contextP->nextMID++, 4, NULL, ENDPOINT_UNKNOWN, sessionH);
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
