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
#ifdef LWM2M_BOOTSTRAP

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
    int res;

    query_length = utils_stringCopy(query, PRV_QUERY_BUFFER_LENGTH, "?ep=");
    if (query_length < 0)
    {
        bootstrapServer->status = STATE_BS_FAILED;
        return;
    }
    res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, context->endpointName);
    if (res < 0)
    {
        bootstrapServer->status = STATE_BS_FAILED;
        return;
    }
    query_length += res;

    if (bootstrapServer->sessionH == NULL)
    {
        bootstrapServer->sessionH = lwm2m_connect_server(bootstrapServer->secObjInstID, context->userData);
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
        if (targetP->sessionH == NULL)
        {
            targetP->sessionH = lwm2m_connect_server(targetP->secObjInstID, contextP->userData);
        }
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
    case STATE_BS_HOLD_OFF:
        serverP->status = STATE_BS_PENDING;
        break;

    case STATE_BS_INITIATED:
        // The ACK was probably lost
        serverP->status = STATE_BS_PENDING;
        break;

    case STATE_DEREGISTERED:
        // server initiated bootstrap
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

static void prv_tag_server(lwm2m_context_t * contextP,
                           uint16_t id)
{
    lwm2m_server_t * targetP;

    targetP = (lwm2m_server_t *)LWM2M_LIST_FIND(contextP->bootstrapServerList, id);
    if (targetP == NULL)
    {
        targetP = (lwm2m_server_t *)LWM2M_LIST_FIND(contextP->serverList, id);
    }
    if (targetP != NULL)
    {
        targetP->status = STATE_DIRTY;
    }
}

static void prv_tag_all_servers(lwm2m_context_t * contextP,
                                lwm2m_server_t * serverP)
{
    lwm2m_server_t * targetP;

    targetP = contextP->bootstrapServerList;
    while (targetP != NULL)
    {
        if (targetP != serverP)
        {
            targetP->status = STATE_DIRTY;
        }
        targetP = targetP->next;
    }
    targetP = contextP->serverList;
    while (targetP != NULL)
    {
        targetP->status = STATE_DIRTY;
        targetP = targetP->next;
    }
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
                    if (uriP->objectId == LWM2M_SECURITY_OBJECT_ID
                     && result == COAP_204_CHANGED)
                    {
                        prv_tag_server(contextP, uriP->instanceId);
                    }
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
            if (LWM2M_URI_IS_SET_RESOURCE(uriP))
            {
                result = BAD_REQUEST_4_00;
            }
            else
            {
                result = object_delete(contextP, uriP);
                if (uriP->objectId == LWM2M_SECURITY_OBJECT_ID
                 && result == COAP_202_DELETED)
                {
                    if (LWM2M_URI_IS_SET_INSTANCE(uriP))
                    {
                        prv_tag_server(contextP, uriP->instanceId);
                    }
                    else
                    {
                        prv_tag_all_servers(contextP, NULL);
                    }
                }
            }
        }
        break;

    case COAP_GET:
    case COAP_POST:
    default:
        result = BAD_REQUEST_4_00;
        break;
    }

    if (result == COAP_202_DELETED
     || result == COAP_204_CHANGED)
    {
        if (serverP->status != STATE_BS_PENDING)
        {
            serverP->status = STATE_BS_PENDING;
            contextP->state = STATE_BOOTSTRAPPING;
        }
    }

    return result;
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
        lwm2m_uri_t uri;

        memset(&uri, 0, sizeof(lwm2m_uri_t));
        uri.flag = LWM2M_URI_FLAG_OBJECT_ID;
        uri.objectId = objectP->objID;

        if (objectP->objID == LWM2M_SECURITY_OBJECT_ID)
        {
            lwm2m_list_t * instanceP;

            instanceP = objectP->instanceList;
            while (NULL != instanceP
                && result == COAP_202_DELETED)
            {
                if (instanceP->id == serverP->secObjInstID)
                {
                    instanceP = instanceP->next;
                }
                else
                {
                    uri.flag = LWM2M_URI_FLAG_OBJECT_ID | LWM2M_URI_FLAG_INSTANCE_ID;
                    uri.instanceId = instanceP->id;
                    result = object_delete(contextP, &uri);
                    instanceP = objectP->instanceList;
                }
            }
            if (result == COAP_202_DELETED)
            {
                prv_tag_all_servers(contextP, serverP);
            }
        }
        else
        {
            result = object_delete(contextP, &uri);
            if (result == METHOD_NOT_ALLOWED_4_05)
            {
                // Fake a successful deletion for static objects like the Device object.
                result = COAP_202_DELETED;
            }
        }
    }

    return result;
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
