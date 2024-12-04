/*******************************************************************************
 *
 * Copyright (c) 2015 Sierra Wireless and others.
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
 *    Pascal Rieux - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    David Navarro, Intel Corporation - Please refer to git log
 *    Tuve Nordius, Husqvarna Group - Please refer to git log
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
 *
 *******************************************************************************/

#include "internals.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef LWM2M_CLIENT_MODE
#ifdef LWM2M_BOOTSTRAP

#define PRV_QUERY_BUFFER_LENGTH 200


static void prv_handleResponse(lwm2m_server_t * bootstrapServer,
                               coap_packet_t * message)
{
    if (COAP_204_CHANGED == message->code)
    {
        LOG_DBG("Received ACK/2.04, Bootstrap pending, waiting for DEL/PUT from BS server...");
        bootstrapServer->status = STATE_BS_PENDING;
        bootstrapServer->registration = lwm2m_gettime() + COAP_EXCHANGE_LIFETIME;
    }
    else
    {
        bootstrapServer->status = STATE_BS_FAILING;
    }
}

static void prv_handleBootstrapReply(lwm2m_context_t * contextP,
                                     lwm2m_transaction_t * transaction,
                                     void * message)
{
    lwm2m_server_t * bootstrapServer = (lwm2m_server_t *)transaction->userData;
    coap_packet_t * coapMessage = (coap_packet_t *)message;

    (void)contextP; /* unused */

    LOG_DBG("Entering");

    if (bootstrapServer->status == STATE_BS_INITIATED)
    {
        if (NULL != coapMessage && COAP_TYPE_RST != coapMessage->type)
        {
            prv_handleResponse(bootstrapServer, coapMessage);
        }
        else
        {
            bootstrapServer->status = STATE_BS_FAILING;
        }
    }
}

// start a device initiated bootstrap
static void prv_requestBootstrap(lwm2m_context_t * context,
                                 lwm2m_server_t * bootstrapServer)
{
    char query[PRV_QUERY_BUFFER_LENGTH];
    int query_length = 0;
    int res;

    LOG_DBG("Entering");

    query_length = utils_stringCopy(query, PRV_QUERY_BUFFER_LENGTH, QUERY_STARTER QUERY_NAME);
    if (query_length < 0)
    {
        bootstrapServer->status = STATE_BS_FAILING;
        return;
    }
    res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, context->endpointName);
    if (res < 0)
    {
        bootstrapServer->status = STATE_BS_FAILING;
        return;
    }
    query_length += res;

#ifndef LWM2M_VERSION_1_0
#if defined(LWM2M_SUPPORT_SENML_CBOR) || defined(LWM2M_SUPPORT_SENML_JSON) || defined(LWM2M_SUPPORT_TLV)
    res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, QUERY_DELIMITER QUERY_PCT);
    if (res < 0)
    {
        bootstrapServer->status = STATE_BS_FAILING;
        return;
    }
    query_length += res;
#if defined(LWM2M_SUPPORT_SENML_CBOR)
    res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, REG_ATTR_CONTENT_SENML_CBOR);
#elif defined(LWM2M_SUPPORT_SENML_JSON)
    res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, REG_ATTR_CONTENT_SENML_JSON);
#elif defined(LWM2M_SUPPORT_TLV)
    res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, REG_ATTR_CONTENT_TLV);
#else
    /* This shouldn't be possible. */
#warning No supported content type for bootstrap. Bootstrap will always fail.
    res = -1;
#endif
    if (res < 0)
    {
        bootstrapServer->status = STATE_BS_FAILING;
        return;
    }
    query_length += res;
#endif
#endif

    if (bootstrapServer->sessionH == NULL)
    {
        bootstrapServer->sessionH = lwm2m_connect_server(bootstrapServer->secObjInstID, context->userData);
    }

    if (bootstrapServer->sessionH != NULL)
    {
        lwm2m_transaction_t * transaction = NULL;

        LOG_DBG("Bootstrap server connection opened");

        transaction = transaction_new(bootstrapServer->sessionH, COAP_POST, NULL, NULL, context->nextMID++, 4, NULL);
        if (transaction == NULL)
        {
            bootstrapServer->status = STATE_BS_FAILING;
            return;
        }

        coap_set_header_uri_path(transaction->message, "/"URI_BOOTSTRAP_SEGMENT);
        coap_set_header_uri_query(transaction->message, query);
        transaction->callback = prv_handleBootstrapReply;
        transaction->userData = (void *)bootstrapServer;
        context->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(context->transactionList, transaction);
        if (transaction_send(context, transaction) == 0)
        {
            LOG_DBG("CI bootstrap requested to BS server");
            bootstrapServer->status = STATE_BS_INITIATED;
        }
    }
    else
    {
        LOG_DBG("Connecting bootstrap server failed");
        bootstrapServer->status = STATE_BS_FAILED;
    }
}

static uint8_t prv_handleBootstrapDiscover(lwm2m_context_t * contextP,
                                           lwm2m_uri_t * uriP,
                                           uint8_t ** bufferP,
                                           size_t * lengthP)
{
    size_t length;
    size_t res;
    int res2;
    lwm2m_object_t * objectP;
    uint8_t buffer[REG_OBJECT_MIN_LEN + 5];
    lwm2m_list_t *instanceP;
    lwm2m_uri_t uri;
    int size;
    lwm2m_data_t *dataP;
    uint64_t val;

    if (uriP && LWM2M_URI_IS_SET_OBJECT(uriP))
    {
        if (LWM2M_URI_IS_SET_INSTANCE(uriP)) return COAP_400_BAD_REQUEST;
        if (LWM2M_URI_IS_SET_RESOURCE(uriP)) return COAP_400_BAD_REQUEST;
#ifndef LWM2M_VERSION_1_0
        if (LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP)) return COAP_400_BAD_REQUEST;
#endif

        objectP = (lwm2m_object_t *)LWM2M_LIST_FIND(contextP->objectList, uriP->objectId);
        if (objectP == NULL)
        {
            return COAP_404_NOT_FOUND;
        }
    }

    length = 0;
#if !defined(LWM2M_VERSION_1_0)
    // LwM2M 1.1.1 adds "</>;" at the beginning
    length += 4;
#endif
    length += QUERY_VERSION_LEN + LWM2M_VERSION_LEN;

    for (objectP = contextP->objectList; objectP; objectP = objectP->next)
    {
        if (uriP && LWM2M_URI_IS_SET_OBJECT(uriP) && uriP->objectId != objectP->objID) continue;

        if (objectP->instanceList == NULL
         || objectP->versionMajor != 0
         || objectP->versionMinor != 0)
        {
            length += 4; // ",</>"
            res = utils_uintToText(objectP->objID, buffer, sizeof(buffer));
            if (res == 0) return COAP_500_INTERNAL_SERVER_ERROR;
            length += res;

            if (objectP->versionMajor != 0
             || objectP->versionMinor != 0)
            {
                length += 2 + ATTR_VERSION_LEN; // ";ver=."
                res = utils_uintToText(objectP->versionMajor, buffer, sizeof(buffer));
                if (res == 0) return COAP_500_INTERNAL_SERVER_ERROR;
                length += res;
                res = utils_uintToText(objectP->versionMinor, buffer, sizeof(buffer));
                if (res == 0) return COAP_500_INTERNAL_SERVER_ERROR;
                length += res;
            }
        }
        for (instanceP = objectP->instanceList; instanceP; instanceP = instanceP->next)
        {
            length += 5; // ",<//>"
            res = utils_uintToText(objectP->objID, buffer, sizeof(buffer));
            if (res == 0) return COAP_500_INTERNAL_SERVER_ERROR;
            length += res;
            res = utils_uintToText(instanceP->id, buffer, sizeof(buffer));
            if (res == 0) return COAP_500_INTERNAL_SERVER_ERROR;
            length += res;

            LWM2M_URI_RESET(&uri);
            uri.objectId = objectP->objID;
            uri.instanceId = instanceP->id;

            if (objectP->objID == LWM2M_SECURITY_OBJECT_ID)
            {
                bool bootstrap;
                uri.resourceId = LWM2M_SECURITY_BOOTSTRAP_ID;
                if (object_readData(contextP, &uri, &size, &dataP) != COAP_205_CONTENT) return COAP_500_INTERNAL_SERVER_ERROR;
                if (size != 1 || lwm2m_data_decode_bool(dataP, &bootstrap) == 0)
                {
                    lwm2m_data_free(size, dataP);
                    return COAP_500_INTERNAL_SERVER_ERROR;
                }
                lwm2m_data_free(size, dataP);

                if (!bootstrap)
                {
                    uri.resourceId = LWM2M_SECURITY_SHORT_SERVER_ID;
                    if (object_readData(contextP, &uri, &size, &dataP) != COAP_205_CONTENT) return COAP_500_INTERNAL_SERVER_ERROR;
                    if (size != 1 || lwm2m_data_decode_uint(dataP, &val) == 0 )
                    {
                        lwm2m_data_free(size, dataP);
                        return COAP_500_INTERNAL_SERVER_ERROR;
                    }
                    lwm2m_data_free(size, dataP);

                    length += 1 + ATTR_SSID_LEN; // ";ssid="
                    res = utils_uintToText(val, buffer, sizeof(buffer));
                    if (res == 0) return COAP_500_INTERNAL_SERVER_ERROR;
                    length += res;

#ifndef LWM2M_VERSION_1_0
                    length += 3 + ATTR_URI_LEN; // ";uri=\"\"\""
                    uri.resourceId = LWM2M_SECURITY_URI_ID;
                    if (object_readData(contextP, &uri, &size, &dataP) != COAP_205_CONTENT) return COAP_500_INTERNAL_SERVER_ERROR;
                    if (size != 1 || dataP->type != LWM2M_TYPE_STRING)
                    {
                        lwm2m_data_free(size, dataP);
                        return COAP_500_INTERNAL_SERVER_ERROR;
                    }
                    length += dataP->value.asBuffer.length;
                    lwm2m_data_free(size, dataP);
#endif
                }
            }
            else if (objectP->objID == LWM2M_SERVER_OBJECT_ID)
            {
                uri.resourceId = LWM2M_SERVER_SHORT_ID_ID;
                if (object_readData(contextP, &uri, &size, &dataP) != COAP_205_CONTENT) return COAP_500_INTERNAL_SERVER_ERROR;
                if (size != 1 || lwm2m_data_decode_uint(dataP, &val) == 0)
                {
                    lwm2m_data_free(size, dataP);
                    return COAP_500_INTERNAL_SERVER_ERROR;
                }
                lwm2m_data_free(size, dataP);

                length += 1 + ATTR_SSID_LEN; // ";ssid="
                res = utils_uintToText(val, buffer, sizeof(buffer));
                if (res == 0) return COAP_500_INTERNAL_SERVER_ERROR;
                length += res;
            }
        }
    }

    *lengthP = 0;
    *bufferP = lwm2m_malloc(length);
    if (*bufferP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

#if !defined(LWM2M_VERSION_1_0)
    // LwM2M 1.1.1 adds "</>;" at the beginning
    if (length - (*lengthP) < 4) goto error;
    (*bufferP)[(*lengthP)++] = REG_URI_START;
    (*bufferP)[(*lengthP)++] = REG_PATH_SEPARATOR;
    (*bufferP)[(*lengthP)++] = REG_URI_END;
    (*bufferP)[(*lengthP)++] = REG_ATTR_SEPARATOR;
#endif

    res2 = utils_stringCopy((char*)(*bufferP)+(*lengthP), length-(*lengthP), QUERY_VERSION);
    if (res2 < 0) goto error;
    (*lengthP) += res2;

    res2 = utils_stringCopy((char*)(*bufferP)+(*lengthP), length-(*lengthP), LWM2M_VERSION);
    if (res2 < 0) goto error;
    (*lengthP) += res2;

    for (objectP = contextP->objectList; objectP; objectP = objectP->next)
    {
        if (uriP && LWM2M_URI_IS_SET_OBJECT(uriP) && uriP->objectId != objectP->objID) continue;

        if (objectP->instanceList == NULL
         || objectP->versionMajor != 0
         || objectP->versionMinor != 0)
        {
            if (length - (*lengthP) < 3) goto error;
            (*bufferP)[(*lengthP)++] = REG_DELIMITER;
            (*bufferP)[(*lengthP)++] = REG_URI_START;
            (*bufferP)[(*lengthP)++] = REG_PATH_SEPARATOR;
            res = utils_uintToText(objectP->objID, (*bufferP)+(*lengthP), length - (*lengthP));
            if (res == 0) goto error;
            (*lengthP) += res;
            if (length - (*lengthP) < 1) goto error;
            (*bufferP)[(*lengthP)++] = REG_URI_END;

            if (objectP->versionMajor != 0
             || objectP->versionMinor != 0)
            {
                if (length - (*lengthP) < 1) goto error;
                (*bufferP)[(*lengthP)++] = REG_ATTR_SEPARATOR;

                res2 = utils_stringCopy((char*)(*bufferP)+(*lengthP), length-(*lengthP), ATTR_VERSION_STR);
                if (res2 < 0) goto error;
                (*lengthP) += res2;

                res = utils_uintToText(objectP->versionMajor, (*bufferP)+(*lengthP), length - (*lengthP));
                if (res == 0) goto error;
                (*lengthP) += res;

                if (length - (*lengthP) < 1) goto error;
                (*bufferP)[(*lengthP)++] = '.';

                res = utils_uintToText(objectP->versionMinor, (*bufferP)+(*lengthP), length - (*lengthP));
                if (res == 0) goto error;
                (*lengthP) += res;
            }
        }
        for (instanceP = objectP->instanceList; instanceP; instanceP = instanceP->next)
        {
            if (length - (*lengthP) < 3) goto error;
            (*bufferP)[(*lengthP)++] = REG_DELIMITER;
            (*bufferP)[(*lengthP)++] = REG_URI_START;
            (*bufferP)[(*lengthP)++] = REG_PATH_SEPARATOR;

            res = utils_uintToText(objectP->objID, (*bufferP)+(*lengthP), length - (*lengthP));
            if (res == 0) goto error;
            (*lengthP) += res;

            if (length - (*lengthP) < 1) goto error;
            (*bufferP)[(*lengthP)++] = REG_PATH_SEPARATOR;

            res = utils_uintToText(instanceP->id, (*bufferP)+(*lengthP), length - (*lengthP));
            if (res == 0) goto error;
            (*lengthP) += res;

            if (length - (*lengthP) < 1) goto error;
            (*bufferP)[(*lengthP)++] = REG_URI_END;

            LWM2M_URI_RESET(&uri);
            uri.objectId = objectP->objID;
            uri.instanceId = instanceP->id;

            if (objectP->objID == LWM2M_SECURITY_OBJECT_ID)
            {
                bool bootstrap;
                uri.resourceId = LWM2M_SECURITY_BOOTSTRAP_ID;
                if (object_readData(contextP, &uri, &size, &dataP) != COAP_205_CONTENT) goto error;
                if (size != 1 || lwm2m_data_decode_bool(dataP, &bootstrap) == 0)
                {
                    lwm2m_data_free(size, dataP);
                    goto error;
                }
                lwm2m_data_free(size, dataP);

                if (!bootstrap)
                {
                    uri.resourceId = LWM2M_SECURITY_SHORT_SERVER_ID;
                    if (object_readData(contextP, &uri, &size, &dataP) != COAP_205_CONTENT) goto error;
                    if (size != 1 || lwm2m_data_decode_uint(dataP, &val) == 0)
                    {
                        lwm2m_data_free(size, dataP);
                        goto error;
                    }
                    lwm2m_data_free(size, dataP);

                    if (length - (*lengthP) < 1) goto error;
                    (*bufferP)[(*lengthP)++] = REG_ATTR_SEPARATOR;

                    res2 = utils_stringCopy((char*)(*bufferP)+(*lengthP), length-(*lengthP), ATTR_SSID_STR);
                    if (res2 < 0) goto error;
                    (*lengthP) += res2;

                    res = utils_uintToText(val, (*bufferP)+(*lengthP), length - (*lengthP));
                    if (res == 0) goto error;
                    (*lengthP) += res;

#ifndef LWM2M_VERSION_1_0
                    if (length - (*lengthP) < 1) goto error;
                    (*bufferP)[(*lengthP)++] = REG_ATTR_SEPARATOR;

                    res2 = utils_stringCopy((char*)(*bufferP)+(*lengthP), length-(*lengthP), ATTR_URI_STR);
                    if (res2 < 0) goto error;
                    (*lengthP) += res2;

                    uri.resourceId = LWM2M_SECURITY_URI_ID;
                    if (object_readData(contextP, &uri, &size, &dataP) != COAP_205_CONTENT) goto error;
                    if (size != 1 || dataP->type != LWM2M_TYPE_STRING)
                    {
                        lwm2m_data_free(size, dataP);
                        goto error;
                    }

                    if (length-(*lengthP) < dataP->value.asBuffer.length)
                    {
                        lwm2m_data_free(size, dataP);
                        goto error;
                    }
                    memcpy((*bufferP)+(*lengthP), dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
                    (*lengthP) += dataP->value.asBuffer.length;
                    lwm2m_data_free(size, dataP);

                    if (length - (*lengthP) < 1) goto error;
                    (*bufferP)[(*lengthP)++] = '"';
#endif
                }
            }
            else if (objectP->objID == LWM2M_SERVER_OBJECT_ID)
            {
                uri.resourceId = LWM2M_SERVER_SHORT_ID_ID;
                if (object_readData(contextP, &uri, &size, &dataP) != COAP_205_CONTENT) goto error;
                if (size != 1 || lwm2m_data_decode_uint(dataP, &val) == 0 )
                {
                    lwm2m_data_free(size, dataP);
                    goto error;
                }
                lwm2m_data_free(size, dataP);

                if (length - (*lengthP) < 1) goto error;
                (*bufferP)[(*lengthP)++] = REG_ATTR_SEPARATOR;

                res2 = utils_stringCopy((char*)(*bufferP)+(*lengthP), length-(*lengthP), ATTR_SSID_STR);
                if (res2 < 0) goto error;
                (*lengthP) += res2;

                res = utils_uintToText(val, (*bufferP)+(*lengthP), length - (*lengthP));
                if (res == 0) goto error;
                (*lengthP) += res;
            }
        }
    }

    return COAP_205_CONTENT;

error:
    lwm2m_free(*bufferP);
    *bufferP = NULL;
    *lengthP = 0;
    return COAP_500_INTERNAL_SERVER_ERROR;
}

#ifndef LWM2M_VERSION_1_0
static uint8_t prv_handleBootstrapRead(lwm2m_context_t * contextP,
                                       lwm2m_uri_t * uriP,
                                       uint8_t acceptNum,
                                       const uint16_t * accept,
                                       lwm2m_media_type_t * formatP,
                                       uint8_t ** bufferP,
                                       size_t * lengthP)
{
    uint8_t result = COAP_400_BAD_REQUEST;
    if (LWM2M_URI_IS_SET_OBJECT(uriP))
    {
        if (uriP->objectId == LWM2M_SERVER_OBJECT_ID
         || uriP->objectId == LWM2M_ACL_OBJECT_ID)
        {
            result = object_read(contextP,
                                 uriP,
                                 accept,
                                 acceptNum,
                                 formatP,
                                 bufferP,
                                 lengthP);
        }
    }
    return result;
}
#endif

void bootstrap_step(lwm2m_context_t * contextP,
                    time_t currentTime,
                    time_t * timeoutP)
{
    lwm2m_server_t * targetP;

    LOG_DBG("entering");
    targetP = contextP->bootstrapServerList;
    while (targetP != NULL)
    {
        LOG_ARG_DBG("Initial status: %s", STR_STATUS(targetP->status));
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
                prv_requestBootstrap(contextP, targetP);
            }
            else if (*timeoutP > targetP->registration - currentTime)
            {
                *timeoutP = targetP->registration - currentTime;
            }
            break;

        case STATE_BS_INITIATED:
            // waiting
            break;

        case STATE_BS_PENDING:
            if (targetP->registration <= currentTime)
            {
               targetP->status = STATE_BS_FAILING;
               *timeoutP = 0;
            }
            else if (*timeoutP > targetP->registration - currentTime)
            {
                *timeoutP = targetP->registration - currentTime;
            }
            break;

        case STATE_BS_FINISHING:
            if (targetP->sessionH != NULL)
            {
                lwm2m_close_connection(targetP->sessionH, contextP->userData);
                targetP->sessionH = NULL;
            }
            targetP->status = STATE_BS_FINISHED;
            *timeoutP = 0;
            break;

        case STATE_BS_FAILING:
            if (targetP->sessionH != NULL)
            {
                lwm2m_close_connection(targetP->sessionH, contextP->userData);
                targetP->sessionH = NULL;
            }
            targetP->status = STATE_BS_FAILED;
            *timeoutP = 0;
            break;

        default:
            break;
        }
        LOG_ARG_DBG("Final status: %s", STR_STATUS(targetP->status));
        targetP = targetP->next;
    }
}

uint8_t bootstrap_handleFinish(lwm2m_context_t * context,
                               void * fromSessionH)
{
    lwm2m_server_t * bootstrapServer;

    LOG_DBG("Entering");
    bootstrapServer = utils_findBootstrapServer(context, fromSessionH);
    if (bootstrapServer != NULL
     && bootstrapServer->status == STATE_BS_PENDING)
    {
        if (object_getServers(context, true) == 0)
        {
            LOG_DBG("Bootstrap server status changed to STATE_BS_FINISHING");
            bootstrapServer->status = STATE_BS_FINISHING;
            return COAP_204_CHANGED;
        }
        else
        {
           return COAP_406_NOT_ACCEPTABLE;
        }
    }

    return COAP_IGNORE;
}

/*
 * Reset the bootstrap servers statuses
 *
 * TODO: handle LwM2M Servers the client is registered to ?
 *
 */
void bootstrap_start(lwm2m_context_t * contextP)
{
    lwm2m_server_t * targetP;

    LOG_DBG("Entering");
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
lwm2m_status_t bootstrap_getStatus(lwm2m_context_t * contextP)
{
    lwm2m_server_t * targetP;
    lwm2m_status_t bs_status;

    LOG_DBG("Entering");
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
            case STATE_BS_FINISHING:
            case STATE_BS_FAILING:
                bs_status = STATE_BS_PENDING;
                break;

            default:
                break;
        }
        targetP = targetP->next;
    }

    LOG_ARG_DBG("Returned status: %s", STR_STATUS(bs_status));

    return bs_status;
}

static uint8_t prv_checkServerStatus(lwm2m_server_t * serverP)
{
    LOG_ARG_DBG("Initial status: %s", STR_STATUS(serverP->status));

    switch (serverP->status)
    {
    case STATE_BS_HOLD_OFF:
    case STATE_BS_INITIATED: // The ACK was probably lost
        serverP->status = STATE_BS_PENDING;
        LOG_ARG_DBG("Status changed to: %s", STR_STATUS(serverP->status));
        break;

    case STATE_DEREGISTERED:
        // server initiated bootstrap
    case STATE_BS_PENDING:
        serverP->registration = lwm2m_gettime() + COAP_EXCHANGE_LIFETIME;
        break;

    case STATE_BS_FINISHED:
    case STATE_BS_FINISHING:
    case STATE_BS_FAILING:
    case STATE_BS_FAILED:
    default:
        LOG_DBG("Returning COAP_IGNORE");
        return COAP_IGNORE;
    }

    return COAP_NO_ERROR;
}

static void prv_tagServer(lwm2m_context_t * contextP,
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
        targetP->dirty = true;
    }
}

static void prv_tagAllServer(lwm2m_context_t * contextP,
                             lwm2m_server_t * serverP)
{
    lwm2m_server_t * targetP;

    targetP = contextP->bootstrapServerList;
    while (targetP != NULL)
    {
        if (targetP != serverP)
        {
            targetP->dirty = true;
        }
        targetP = targetP->next;
    }
    targetP = contextP->serverList;
    while (targetP != NULL)
    {
        targetP->dirty = true;
        targetP = targetP->next;
    }
}

uint8_t bootstrap_handleCommand(lwm2m_context_t * contextP,
                                lwm2m_uri_t * uriP,
                                lwm2m_server_t * serverP,
                                coap_packet_t * message,
                                coap_packet_t * response)
{
    uint8_t result;
    lwm2m_media_type_t format;

    LOG_ARG_DBG("Code: %02X", message->code);
    LOG_ARG_DBG("%s", LOG_URI_TO_STRING(uriP));
    format = utils_convertMediaType(message->content_type);

    result = prv_checkServerStatus(serverP);
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
                    result = object_write(contextP, uriP, format, message->payload, message->payload_len, false);
                    if (uriP->objectId == LWM2M_SECURITY_OBJECT_ID
                     && result == COAP_204_CHANGED)
                    {
                        prv_tagServer(contextP, uriP->instanceId);
                    }
                }
            }
            else
            {
                lwm2m_data_t * dataP = NULL;
                int size = 0;
                int i;

                if (message->payload_len == 0 || message->payload == 0)
                {
                    result = COAP_400_BAD_REQUEST;
                }
                else
                {
                    size = lwm2m_data_parse(uriP, message->payload, message->payload_len, format, &dataP);
                    if (size == 0)
                    {
                        result = COAP_500_INTERNAL_SERVER_ERROR;
                        break;
                    }

                    for (i = 0 ; i < size ; i++)
                    {
                        if(dataP[i].type == LWM2M_TYPE_OBJECT_INSTANCE)
                        {
                            if (object_isInstanceNew(contextP, uriP->objectId, dataP[i].id))
                            {
                                result = object_createInstance(contextP, uriP, &dataP[i]);
                                if (COAP_201_CREATED == result)
                                {
                                    result = COAP_204_CHANGED;
                                }
                            }
                            else
                            {
                                result = object_writeInstance(contextP, uriP, &dataP[i]);
                                if (uriP->objectId == LWM2M_SECURITY_OBJECT_ID
                                 && result == COAP_204_CHANGED)
                                {
                                    prv_tagServer(contextP, dataP[i].id);
                                }
                            }

                            if(result != COAP_204_CHANGED) // Stop object create or write when result is error
                            {
                                break;
                            }
                        }
                        else
                        {
                            result = COAP_400_BAD_REQUEST;
                        }
                    }
                    lwm2m_data_free(size, dataP);
                }
            }
        }
        break;

    case COAP_DELETE:
        {
            if (LWM2M_URI_IS_SET_RESOURCE(uriP))
            {
                result = COAP_400_BAD_REQUEST;
            }
            else
            {
                if (uriP->objectId == LWM2M_SECURITY_OBJECT_ID
                 && !LWM2M_URI_IS_SET_INSTANCE(uriP))
                {
                    lwm2m_object_t *objectP;
                    result = COAP_202_DELETED;
                    for (objectP = contextP->objectList; objectP != NULL; objectP = objectP->next)
                    {
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
                                    lwm2m_uri_t uri;

                                    LWM2M_URI_RESET(&uri);
                                    uri.objectId = objectP->objID;
                                    uri.instanceId = instanceP->id;
                                    result = object_delete(contextP, &uri);
                                    instanceP = objectP->instanceList;
                                }
                            }
                            if (result == COAP_202_DELETED)
                            {
                                prv_tagAllServer(contextP, serverP);
                            }
                        }
                    }
                }
                else
                {
                    result = object_delete(contextP, uriP);
                    if (uriP->objectId == LWM2M_SECURITY_OBJECT_ID
                     && result == COAP_202_DELETED)
                    {
                        if (LWM2M_URI_IS_SET_INSTANCE(uriP))
                        {
                            prv_tagServer(contextP, uriP->instanceId);
                        }
                        else
                        {
                            prv_tagAllServer(contextP, NULL);
                        }
                    }
                }
            }
        }
        break;

    case COAP_GET:
        {
            uint8_t * buffer = NULL;
            size_t length = 0;
            if (IS_OPTION(message, COAP_OPTION_ACCEPT)
             && message->accept_num == 1
             && message->accept[0] == APPLICATION_LINK_FORMAT)
            {
                /* Bootstrap-Discover */
                format = LWM2M_CONTENT_LINK;
                result = prv_handleBootstrapDiscover(contextP,
                                                     uriP,
                                                     &buffer,
                                                     &length);
            }
            else
            {
#ifndef LWM2M_VERSION_1_0
                /* Bootstrap-Read */
                result = prv_handleBootstrapRead(contextP,
                                                 uriP,
                                                 message->accept_num,
                                                 message->accept,
                                                 &format,
                                                 &buffer,
                                                 &length);
#else
                result = COAP_400_BAD_REQUEST;
#endif
            }
            if (COAP_205_CONTENT == result)
            {
                coap_set_header_content_type(response, format);
                coap_set_payload(response, buffer, length);
                // lwm2m_handle_packet will free buffer
            }
        }
        break;
    case COAP_POST:
    default:
        result = COAP_400_BAD_REQUEST;
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
    LOG_ARG_DBG("Server status: %s", STR_STATUS(serverP->status));

    return result;
}

uint8_t bootstrap_handleDeleteAll(lwm2m_context_t * contextP,
                                  void * fromSessionH)
{
    lwm2m_server_t * serverP;
    uint8_t result;
    lwm2m_object_t * objectP;

    LOG_DBG("Entering");
    serverP = utils_findBootstrapServer(contextP, fromSessionH);
    if (serverP == NULL) return COAP_IGNORE;
    result = prv_checkServerStatus(serverP);
    if (result != COAP_NO_ERROR) return result;

    result = COAP_202_DELETED;
    for (objectP = contextP->objectList; objectP != NULL; objectP = objectP->next)
    {
        lwm2m_uri_t uri;

        LWM2M_URI_RESET(&uri);
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
                    uri.instanceId = instanceP->id;
                    result = object_delete(contextP, &uri);
                    instanceP = objectP->instanceList;
                }
            }
            if (result == COAP_202_DELETED)
            {
                prv_tagAllServer(contextP, serverP);
            }
        }
        else
        {
            result = object_delete(contextP, &uri);
            if (result == COAP_405_METHOD_NOT_ALLOWED)
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
uint8_t bootstrap_handleRequest(lwm2m_context_t * contextP,
                                lwm2m_uri_t * uriP,
                                void * fromSessionH,
                                coap_packet_t * message,
                                coap_packet_t * response)
{
    uint8_t result;
    char * name;
    lwm2m_media_type_t format = 0;

    LOG_ARG_DBG("%s", LOG_URI_TO_STRING(uriP));
    if (contextP->bootstrapCallback == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
    if (message->code != COAP_POST) return COAP_400_BAD_REQUEST;
    if (message->uri_query == NULL) return COAP_400_BAD_REQUEST;

    if (lwm2m_strncmp((char *)message->uri_query->data, QUERY_NAME, QUERY_NAME_LEN) != 0)
    {
        return COAP_400_BAD_REQUEST;
    }

    if (message->uri_query->len == QUERY_NAME_LEN) return COAP_400_BAD_REQUEST;
#ifdef LWM2M_VERSION_1_0
    if (message->uri_query->next != NULL) return COAP_400_BAD_REQUEST;
#else
    if (message->uri_query->next != NULL)
    {
        char formatString[6];
        char *endP;
        multi_option_t *uri_query = message->uri_query->next;
        if (lwm2m_strncmp((char *)uri_query->data, QUERY_PCT, QUERY_PCT_LEN) != 0)
        {
            return COAP_400_BAD_REQUEST;
        }

        if (uri_query->len == QUERY_PCT_LEN) return COAP_400_BAD_REQUEST;
        if (uri_query->len > QUERY_PCT_LEN + 5) return COAP_400_BAD_REQUEST;
        if (uri_query->next != NULL) return COAP_400_BAD_REQUEST;

        memcpy(formatString, uri_query->data + QUERY_PCT_LEN, uri_query->len - QUERY_PCT_LEN);
        formatString[uri_query->len - QUERY_PCT_LEN] = 0;

        format = (lwm2m_media_type_t)strtol(formatString, &endP, 10);
        if('\0' != *endP) return COAP_400_BAD_REQUEST;
        switch (format)
        {
#ifdef LWM2M_SUPPORT_SENML_JSON
        case LWM2M_CONTENT_SENML_JSON:
            break;
#endif

#ifdef LWM2M_SUPPORT_SENML_CBOR
        case LWM2M_CONTENT_SENML_CBOR:
            break;
#endif
        default:
#ifdef LWM2M_SUPPORT_TLV
            format = LWM2M_CONTENT_TLV;
#else
            return COAP_400_BAD_REQUEST;
#endif
        }
    }
#endif

    if (message->payload)
    {
        format = utils_convertMediaType(message->content_type);
    }

    name = (char *)lwm2m_malloc(message->uri_query->len - QUERY_NAME_LEN + 1);
    if (name == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

    memcpy(name, message->uri_query->data + QUERY_NAME_LEN, message->uri_query->len - QUERY_NAME_LEN);
    name[message->uri_query->len - QUERY_NAME_LEN] = 0;

    result = contextP->bootstrapCallback(contextP, fromSessionH, COAP_NO_ERROR, NULL, name, format, message->payload, message->payload_len, contextP->bootstrapUserData);

    lwm2m_free(name);

    return result;
}

void lwm2m_set_bootstrap_callback(lwm2m_context_t * contextP,
                                  lwm2m_bootstrap_callback_t callback,
                                  void * userData)
{
    LOG_DBG("Entering");
    contextP->bootstrapCallback = callback;
    contextP->bootstrapUserData = userData;
}

static void prv_resultCallback(lwm2m_context_t * contextP,
                               lwm2m_transaction_t * transacP,
                               void * message)
{
    bs_data_t * dataP = (bs_data_t *)transacP->userData;
    lwm2m_uri_t * uriP;

    (void)contextP; /* unused */

    if (LWM2M_URI_IS_SET_OBJECT(&dataP->uri))
    {
        uriP = &dataP->uri;
    }
    else
    {
        uriP = NULL;
    }

    if (message == NULL)
    {
        dataP->callback(contextP,
                        transacP->peerH,
                        COAP_503_SERVICE_UNAVAILABLE,
                        uriP,
                        NULL,
                        0,
                        NULL,
                        0,
                        dataP->userData);
    }
    else
    {
        coap_packet_t * packet = (coap_packet_t *)message;

        dataP->callback(contextP,
                        transacP->peerH,
                        packet->code,
                        uriP,
                        NULL,
                        utils_convertMediaType(packet->content_type),
                        packet->payload,
                        packet->payload_len,
                        dataP->userData);
    }
    transaction_free_userData(contextP, transacP);
}

int lwm2m_bootstrap_delete(lwm2m_context_t * contextP,
                           void * sessionH,
                           lwm2m_uri_t * uriP)
{
    lwm2m_transaction_t * transaction;
    bs_data_t * dataP;

    LOG_ARG_DBG("%s", LOG_URI_TO_STRING(uriP));
    transaction = transaction_new(sessionH, COAP_DELETE, NULL, uriP, contextP->nextMID++, 4, NULL);
    if (transaction == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

    dataP = (bs_data_t *)lwm2m_malloc(sizeof(bs_data_t));
    if (dataP == NULL)
    {
        transaction_free(transaction);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }
    if (uriP == NULL)
    {
        LWM2M_URI_RESET(&dataP->uri);
    }
    else
    {
        memcpy(&dataP->uri, uriP, sizeof(lwm2m_uri_t));
    }
    dataP->callback = contextP->bootstrapCallback;
    dataP->userData = contextP->bootstrapUserData;

    transaction->callback = prv_resultCallback;
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

    LOG_ARG_DBG("%s", LOG_URI_TO_STRING(uriP));
    if (uriP == NULL
    || buffer == NULL
    || length == 0)
    {
        return COAP_400_BAD_REQUEST;
    }

    transaction = transaction_new(sessionH, COAP_PUT, NULL, uriP, contextP->nextMID++, 4, NULL);
    if (transaction == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

    coap_set_header_content_type(transaction->message, format);

    if (!transaction_set_payload(transaction, buffer, length)) {
        transaction_free(transaction);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    dataP = (bs_data_t *)lwm2m_malloc(sizeof(bs_data_t));
    if (dataP == NULL)
    {
        transaction_free(transaction);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }
    memcpy(&dataP->uri, uriP, sizeof(lwm2m_uri_t));
    dataP->callback = contextP->bootstrapCallback;
    dataP->userData = contextP->bootstrapUserData;

    transaction->callback = prv_resultCallback;
    transaction->userData = (void *)dataP;

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);

    return transaction_send(contextP, transaction);
}

int lwm2m_bootstrap_finish(lwm2m_context_t * contextP,
                           void * sessionH)
{
    lwm2m_transaction_t * transaction;
    bs_data_t * dataP;

    LOG_DBG("Entering");
    transaction = transaction_new(sessionH, COAP_POST, NULL, NULL, contextP->nextMID++, 4, NULL);
    if (transaction == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

    coap_set_header_uri_path(transaction->message, "/"URI_BOOTSTRAP_SEGMENT);

    dataP = (bs_data_t *)lwm2m_malloc(sizeof(bs_data_t));
    if (dataP == NULL)
    {
        transaction_free(transaction);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }
    LWM2M_URI_RESET(&dataP->uri);
    dataP->callback = contextP->bootstrapCallback;
    dataP->userData = contextP->bootstrapUserData;

    transaction->callback = prv_resultCallback;
    transaction->userData = (void *)dataP;

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);

    return transaction_send(contextP, transaction);
}

int lwm2m_bootstrap_discover(lwm2m_context_t * contextP, void * sessionH, lwm2m_uri_t * uriP)
{
    lwm2m_transaction_t * transaction;
    bs_data_t * dataP;

    LOG_ARG_DBG("%s", LOG_URI_TO_STRING(uriP));

    if (uriP
     && (LWM2M_URI_IS_SET_INSTANCE(uriP)
      || LWM2M_URI_IS_SET_RESOURCE(uriP)
#ifndef LWM2M_VERSION_1_0
      || LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP)
#endif
        )) return COAP_400_BAD_REQUEST;

    transaction = transaction_new(sessionH, COAP_GET, NULL, uriP, contextP->nextMID++, 4, NULL);
    if (transaction == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
    coap_set_header_accept(transaction->message, APPLICATION_LINK_FORMAT);

    dataP = (bs_data_t *)lwm2m_malloc(sizeof(bs_data_t));
    if (dataP == NULL)
    {
        transaction_free(transaction);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }
    LWM2M_URI_RESET(&dataP->uri);
    dataP->callback = contextP->bootstrapCallback;
    dataP->userData = contextP->bootstrapUserData;

    transaction->callback = prv_resultCallback;
    transaction->userData = (void *)dataP;

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);

    return transaction_send(contextP, transaction);
}

#ifndef LWM2M_VERSION_1_0
int lwm2m_bootstrap_read(lwm2m_context_t * contextP, void * sessionH, lwm2m_uri_t * uriP)
{
    lwm2m_transaction_t * transaction;
    bs_data_t * dataP;

    LOG_ARG_DBG("%s", LOG_URI_TO_STRING(uriP));

    if (uriP
     && (LWM2M_URI_IS_SET_RESOURCE(uriP)
#ifndef LWM2M_VERSION_1_0
      || LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP)
#endif
        )) return COAP_400_BAD_REQUEST;


    transaction = transaction_new(sessionH, COAP_GET, NULL, uriP, contextP->nextMID++, 4, NULL);
    if (transaction == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

    dataP = (bs_data_t *)lwm2m_malloc(sizeof(bs_data_t));
    if (dataP == NULL)
    {
        transaction_free(transaction);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }
    if (uriP == NULL)
    {
        LWM2M_URI_RESET(&dataP->uri);
    }
    else
    {
        memcpy(&dataP->uri, uriP, sizeof(lwm2m_uri_t));
    }
    dataP->callback = contextP->bootstrapCallback;
    dataP->userData = contextP->bootstrapUserData;

    transaction->callback = prv_resultCallback;
    transaction->userData = (void *)dataP;

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);

    return transaction_send(contextP, transaction);
}
#endif

#endif
