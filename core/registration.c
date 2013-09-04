/*
Copyright (c) 2013, Intel Corporation

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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#define QUERY_TEMPLATE "ep="
#define QUERY_LENGTH 3
#define QUERY_DELIMITER '&'

#define MAX_LOCATION_LENGTH 10  // strlen("/rd/65534") + 1


static void prv_getParameters(multi_option_t * query,
                              char ** nameP)
{
    const char * start;
    int length;

    *nameP = NULL;

    while (query != NULL)
    {
        if (query->len > QUERY_LENGTH)
        {
            if (strncmp(query->data, QUERY_TEMPLATE, QUERY_LENGTH) == 0)
            {
                *nameP = (char *)malloc(query->len - QUERY_LENGTH + 1);
                if (*nameP != NULL)
                {
                    memcpy(*nameP, query->data + QUERY_LENGTH, query->len - QUERY_LENGTH);
                    (*nameP)[query->len - QUERY_LENGTH] = 0;
                }
                break;
            }
        }
        query = query->next;
    }
}

static int prv_getId(uint8_t * data,
                     uint16_t length,
                     uint16_t * objId,
                     uint16_t * instanceId)
{
    int value;
    uint16_t limit;
    uint16_t end;

    limit = 0;
    while (limit < length && data[limit] != '/' && data[limit] != ' ') limit++;
    value = prv_get_number(data, limit);
    if (value < 0 || value >= LWM2M_MAX_ID) return 0;
    *objId = value;

    if (limit != length)
    {
        limit += 1;
        end = limit;
        while (end < length && data[end] != ' ') end++;
        if (end != limit)
        {
            value = prv_get_number(data + limit, end - limit);
            if (value >= 0 && value < LWM2M_MAX_ID)
            {
                *instanceId = value;
                return 2;
            }
        }
    }

    return 1;
}

static lwm2m_client_object_t * prv_decodeRegisterPayload(uint8_t * payload,
                                                         uint16_t payloadLength)
{
    lwm2m_client_object_t * objList;
    uint16_t id;
    uint16_t instance;
    uint16_t start;
    uint16_t end;
    int result;

    objList = NULL;
    start = 0;
    while (start < payloadLength)
    {
        while (start < payloadLength && payload[start] == ' ') start++;
        if (start == payloadLength) return objList;
        end = start;
        while (end < payloadLength && payload[end] != ',') end++;
        result = prv_getId(payload + start, end - start, &id, &instance);
        if (result != 0)
        {
            lwm2m_client_object_t * objectP;

            objectP = (lwm2m_client_object_t *)lwm2m_list_find((lwm2m_list_t *)objList, id);
            if (objectP == NULL)
            {
                objectP = (lwm2m_client_object_t *)malloc(sizeof(lwm2m_client_object_t));
                memset(objectP, 0, sizeof(lwm2m_client_object_t));
                if (objectP == NULL) return objList;
                objectP->id = id;
                objList = (lwm2m_client_object_t *)LWM2M_LIST_ADD(objList, objectP);
            }
            if (result == 2)
            {
                lwm2m_list_t * instanceP;

                instanceP = lwm2m_list_find(objectP->instanceList, instance);
                if (instanceP == NULL)
                {
                    instanceP = (lwm2m_list_t *)malloc(sizeof(lwm2m_list_t));
                    memset(instanceP, 0, sizeof(lwm2m_list_t));
                    instanceP->id = instance;
                    objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, instanceP);
                }
            }
        }
        start = end + 1;
    }

    return objList;
}

static lwm2m_client_t * prv_getClientByName(lwm2m_context_t * contextP,
                                            char * name)
{
    lwm2m_client_t * targetP;

    targetP = contextP->clientList;
    while (targetP != NULL && strcmp(name, targetP->name) != 0)
    {
        targetP = targetP->next;
    }

    return targetP;
}

static void prv_freeClientObjectList(lwm2m_client_object_t * objects)
{
    while (objects != NULL)
    {
        lwm2m_client_object_t * objP;

        while (objects->instanceList != NULL)
        {
            lwm2m_list_t * target;

            target = objects->instanceList;
            objects->instanceList = objects->instanceList->next;
            free(target);
        }

        objP = objects;
        objects = objects->next;
        free(objP);
    }
}

static void prv_freeClient(lwm2m_client_t * clientP)
{
    if (clientP->addr != NULL) free(clientP->addr);
    if (clientP->name != NULL) free(clientP->name);
    prv_freeClientObjectList(clientP->objectList);
    free(clientP);
}

static int prv_getLocationString(uint16_t id,
                                 char location[MAX_LOCATION_LENGTH])
{
    int result;

    memset(location, 0, MAX_LOCATION_LENGTH);

    result = snprintf(location, MAX_LOCATION_LENGTH, "/"URI_REGISTRATION_SEGMENT"/%hu", id);
    if (result <= 0 || result > MAX_LOCATION_LENGTH)
    {
        return 0;
    }

    return result;
}

int lwm2m_register(lwm2m_context_t * contextP)
{
    char * query;
    char payload[64];
    int payload_length;
    lwm2m_server_t * targetP;

    payload_length = prv_getRegisterPayload(contextP, payload, 64);
    if (payload_length == 0) return INTERNAL_SERVER_ERROR_5_00;

    query = (char*)malloc(QUERY_LENGTH + strlen(contextP->endpointName) + 1);
    if (query == NULL) return INTERNAL_SERVER_ERROR_5_00;
    strcpy(query, QUERY_TEMPLATE);
    strcpy(query + QUERY_LENGTH, contextP->endpointName);

    targetP = contextP->serverList;
    while (targetP != NULL)
    {
        coap_packet_t message[1];
        lwm2m_transaction_t * transaction;

        coap_init_message(message, COAP_TYPE_CON, COAP_POST, contextP->nextMID++);
        coap_set_header_uri_path(message, "/"URI_REGISTRATION_SEGMENT);
        coap_set_header_uri_query(message, query);
        coap_set_payload(message, payload, payload_length);

        transaction = transaction_new(message->mid, ENDPOINT_SERVER, (void *)targetP);
        if (transaction != NULL)
        {
            transaction->packet_len = coap_serialize_message(message, transaction->packet);
            if (transaction->packet_len > 0)
            {
                contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);
                (void)transaction_send(contextP, transaction);
                targetP->status = STATE_REG_PENDING;
                targetP->mid = transaction->mID;
            }
        }

        targetP = targetP->next;
    }

    return 0;
}

void registration_deregister(lwm2m_context_t * contextP,
                             lwm2m_server_t * serverP)
{
    coap_packet_t message[1];
    uint8_t pktBuffer[COAP_MAX_PACKET_SIZE+1];
    size_t pktBufferLen = 0;

    if (serverP->status != STATE_REGISTERED) return;

    coap_init_message(message, COAP_TYPE_NON, COAP_DELETE, contextP->nextMID++);
    coap_set_header_uri_path(message, serverP->location);

    pktBufferLen = coap_serialize_message(message, pktBuffer);
    if (0 != pktBufferLen)
    {
        coap_send_message(contextP->socket, serverP->addr, serverP->addrLen, pktBuffer, pktBufferLen);
    }

    serverP->status = STATE_UNKNOWN;
}

void handle_registration_reply(lwm2m_context_t * contextP,
                               lwm2m_transaction_t * transacP,
                               coap_packet_t * message)
{
    lwm2m_server_t * targetP;

    targetP = (lwm2m_server_t *)(transacP->peerP);

    switch(targetP->status)
    {
    case STATE_REG_PENDING:
     if (message->mid == targetP->mid
      && message->type == COAP_TYPE_ACK
      && message->location_path_len != 0
      && message->location_path != NULL)
     {
        if (message->code == CREATED_2_01)
        {
            targetP->status = STATE_REGISTERED;
            targetP->location = (char *)malloc(message->location_path_len + 1);
            if (targetP->location != NULL)
            {
                memcpy(targetP->location, message->location_path, message->location_path_len);
                targetP->location[message->location_path_len] = 0;
            }
        }
        else if (message->code == BAD_REQUEST_4_00)
        {
            targetP->status = STATE_UNKNOWN;
            targetP->mid = 0;
        }
     }
     break;
    default:
        break;
    }
}

coap_status_t handle_registration_request(lwm2m_context_t * contextP,
                                          lwm2m_uri_t * uriP,
                                          struct sockaddr * fromAddr,
                                          socklen_t fromAddrLen,
                                          coap_packet_t * message,
                                          coap_packet_t * response)
{
    coap_status_t result;

    switch(message->code)
    {
    case COAP_POST:
    {
        char * name = NULL;
        lwm2m_client_object_t * objects;
        lwm2m_client_t * clientP;
        char location[MAX_LOCATION_LENGTH];

        if (uriP->flag & LWM2M_URI_MASK_ID != 0) return COAP_400_BAD_REQUEST;
        prv_getParameters(message->uri_query, &name);
        if (name == NULL) return COAP_400_BAD_REQUEST;
        objects = prv_decodeRegisterPayload(message->payload, message->payload_len);
        if (objects == NULL)
        {
            free(name);
            return COAP_400_BAD_REQUEST;
        }
        clientP = prv_getClientByName(contextP, name);
        if (clientP != NULL)
        {
            // we reset this registration
            free(clientP->name);
            free(clientP->addr);
            prv_freeClientObjectList(clientP->objectList);
            clientP->objectList = NULL;
        }
        else
        {
            clientP = (lwm2m_client_t *)malloc(sizeof(lwm2m_client_t));
            if (clientP == NULL)
            {
                free(name);
                prv_freeClientObjectList(objects);
                return COAP_500_INTERNAL_SERVER_ERROR;
            }
            memset(clientP, 0, sizeof(lwm2m_client_t));
            clientP->internalID = lwm2m_list_newId((lwm2m_list_t *)contextP->clientList);
            contextP->clientList = (lwm2m_client_t *)LWM2M_LIST_ADD(contextP->clientList, clientP);
        }
        clientP->name = name;
        clientP->objectList = objects;
        clientP->addr = (struct sockaddr *)malloc(fromAddrLen);
        if (clientP->addr == NULL)
        {
            prv_freeClient(clientP);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
        memcpy(clientP->addr, fromAddr, fromAddrLen);
        clientP->addrLen = fromAddrLen;

        if (prv_getLocationString(clientP->internalID, location) == 0)
        {
            prv_freeClient(clientP);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
        coap_set_header_location_path(response, location);

        result = CREATED_2_01;
    }
    break;

    case COAP_PUT:
        result = COAP_501_NOT_IMPLEMENTED;
        break;

    case COAP_DELETE:
    {
        lwm2m_client_t * clientP;

        if (uriP->flag & LWM2M_URI_MASK_ID != LWM2M_URI_FLAG_OBJECT_ID) return COAP_400_BAD_REQUEST;

        contextP->clientList = (lwm2m_client_t *)LWM2M_LIST_RM(contextP->clientList, uriP->id, &clientP);
        if (clientP == NULL) return COAP_400_BAD_REQUEST;
        prv_freeClient(clientP);
        result = DELETED_2_02;
    }
    break;

    default:
        return COAP_400_BAD_REQUEST;
    }

    return result;
}
