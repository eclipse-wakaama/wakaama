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
#include <stdio.h>
#include <stdbool.h>


static lwm2m_observed_t * prv_findObserved(lwm2m_context_t * contextP,
                                           lwm2m_uri_t * uriP,
                                           bool strict)
{
    lwm2m_observed_t * targetP;

    targetP = contextP->observedList;
    while (targetP != NULL)
    {
        if (targetP->uri.objectId == uriP->objectId
         && targetP->uri.instanceId == uriP->instanceId)
        {
            if (!LWM2M_URI_IS_SET_RESOURCE(uriP))
            {
                if ((targetP->uri.flag & LWM2M_URI_FLAG_RESOURCE_ID) == 0)
                {
                    return targetP;
                }
            }
            else
            {
                if ((targetP->uri.flag & LWM2M_URI_FLAG_RESOURCE_ID) == 0)
                {
                    if (strict == false)
                    {
                        return targetP;
                    }
                }
                else
                {
                    if (targetP->uri.resourceId == uriP->resourceId)
                    {
                        return targetP;
                    }
                }
            }
        }

        targetP = targetP->next;
    }

    return targetP;
}

static void prv_unlinkObserved(lwm2m_context_t * contextP,
                               lwm2m_observed_t * observedP)
{
    if (contextP->observedList == observedP)
    {
        contextP->observedList = contextP->observedList->next;
    }
    else
    {
        lwm2m_observed_t * parentP;

        parentP = contextP->observedList;
        while (parentP->next != NULL
            && parentP->next != observedP)
        {
            parentP = parentP->next;
        }
        if (parentP->next != NULL)
        {
            parentP->next = parentP->next->next;
        }
    }
}

static lwm2m_server_t * prv_findServer(lwm2m_context_t * contextP,
                                       struct sockaddr * fromAddr,
                                       socklen_t fromAddrLen)
{
    lwm2m_server_t * targetP;

    targetP = contextP->serverList;
    while (targetP != NULL
        && targetP->addrLen != fromAddrLen
        && memcmp(targetP->addr, fromAddr, fromAddrLen) != 0)
    {
        targetP = targetP->next;
    }

    return targetP;
}

static lwm2m_watcher_t * prv_findWatcher(lwm2m_observed_t * observedP,
                                         lwm2m_server_t * serverP)
{
    lwm2m_watcher_t * targetP;

    targetP = observedP->watcherList;
    while (targetP != NULL
        && targetP->server != serverP)
    {
        targetP = targetP->next;
    }

    return targetP;
}

static void prv_removeWatcher(lwm2m_observed_t * observedP,
                              lwm2m_server_t * serverP)
{
    lwm2m_watcher_t * targetP = NULL;

    if (observedP->watcherList->server == serverP)
    {
        targetP = observedP->watcherList;
        observedP->watcherList = observedP->watcherList->next;
    }
    else
    {
        lwm2m_watcher_t * parentP;

        parentP = observedP->watcherList;
        while (parentP->next != NULL
            && parentP->next->server != serverP)
        {
            parentP = parentP->next;
        }
        if (parentP->next != NULL)
        {
            targetP = parentP->next;
            parentP->next = parentP->next->next;
        }
    }

    if (targetP != NULL) free(targetP);
}

coap_status_t handle_observe_request(lwm2m_context_t * contextP,
                                     lwm2m_uri_t * uriP,
                                     struct sockaddr * fromAddr,
                                     socklen_t fromAddrLen,
                                     coap_packet_t * message,
                                     coap_packet_t * response)
{
    lwm2m_observed_t * observedP;
    lwm2m_watcher_t * watcherP;
    lwm2m_server_t * serverP;

    LOG("handle_observe_request()\r\n");

    if (!LWM2M_URI_IS_SET_INSTANCE(uriP)) return COAP_400_BAD_REQUEST;
    if (message->token_len == 0) return COAP_400_BAD_REQUEST;

    serverP = prv_findServer(contextP, fromAddr, fromAddrLen);
    if (serverP == NULL || serverP->status != STATE_REGISTERED) return COAP_401_UNAUTHORIZED;

    observedP = prv_findObserved(contextP, uriP, true);
    if (observedP == NULL)
    {
        observedP = (lwm2m_observed_t *)malloc(sizeof(lwm2m_observed_t));
        if (observedP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        memset(observedP, 0, sizeof(lwm2m_observed_t));
        memcpy(&(observedP->uri), uriP, sizeof(lwm2m_uri_t));
        observedP->next = contextP->observedList;
        contextP->observedList = observedP;
    }

    watcherP = prv_findWatcher(observedP, serverP);
    if (watcherP == NULL)
    {
        watcherP = (lwm2m_watcher_t *)malloc(sizeof(lwm2m_watcher_t));
        if (watcherP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        memset(watcherP, 0, sizeof(lwm2m_watcher_t));
        watcherP->server = serverP;
        watcherP->tokenLen = message->token_len;
        memcpy(watcherP->token, message->token, message->token_len);
        watcherP->next = observedP->watcherList;
        observedP->watcherList = watcherP;
    }

    coap_set_header_observe(response, watcherP->counter++);

    return COAP_205_CONTENT;
}

void cancel_observe(lwm2m_context_t * contextP,
                    lwm2m_uri_t * uriP,
                    struct sockaddr * fromAddr,
                    socklen_t fromAddrLen)
{
    lwm2m_observed_t * observedP;
    lwm2m_watcher_t * watcherP;
    lwm2m_server_t * serverP;

    LOG("cancel_observe()\r\n");

    if (!LWM2M_URI_IS_SET_INSTANCE(uriP)) return;

    observedP = prv_findObserved(contextP, uriP, true);
    if (observedP == NULL) return;

    serverP = prv_findServer(contextP, fromAddr, fromAddrLen);
    if (serverP == NULL) return;

    prv_removeWatcher(observedP, serverP);

    if (observedP->watcherList == NULL)
    {
        prv_unlinkObserved(contextP, observedP);
        free(observedP);
    }
}

int lwm2m_resource_value_changed(lwm2m_context_t * contextP,
                                 lwm2m_uri_t * uriP,
                                 char * value,
                                 int value_len)
{
    int result;
    lwm2m_observed_t * observedP;
    lwm2m_watcher_t * watcherP;

    observedP = prv_findObserved(contextP, uriP, true);
    if (observedP != NULL)
    {
        coap_packet_t message[1];

        coap_init_message(message, COAP_TYPE_ACK, COAP_204_CHANGED, contextP->nextMID++);
        coap_set_payload(message, value, value_len);

        for (watcherP = observedP->watcherList ; watcherP != NULL ; watcherP = watcherP->next)
        {
            coap_set_header_token(message, watcherP->token, watcherP->tokenLen);
            coap_set_header_observe(message, watcherP->counter++);
            result = message_send(contextP, message, watcherP->server->addr, watcherP->server->addrLen);
        }
    }

    observedP = prv_findObserved(contextP, uriP, false);
    if (observedP != NULL)
    {
        char * buffer = NULL;
        int length = 0;

        result = object_read(contextP, uriP, &buffer, &length);
        if (result == COAP_205_CONTENT)
        {
            coap_packet_t message[1];

            coap_init_message(message, COAP_TYPE_ACK, COAP_204_CHANGED, contextP->nextMID++);
            coap_set_payload(message, buffer, length);

            for (watcherP = observedP->watcherList ; watcherP != NULL ; watcherP = watcherP->next)
            {
                coap_set_header_token(message, watcherP->token, watcherP->tokenLen);
                coap_set_header_observe(message, watcherP->counter++);
                result = message_send(contextP, message, watcherP->server->addr, watcherP->server->addrLen);
            }
        }
    }

    return result;
}

static lwm2m_observation_t * prv_findObservationByURI(lwm2m_client_t * clientP,
                                                      lwm2m_uri_t * uriP)
{
    lwm2m_observation_t * targetP;

    targetP = clientP->observationList;
    while (targetP != NULL)
    {
        if (targetP->uri.objectId == uriP->objectId
         && targetP->uri.flag == uriP->flag
         && targetP->uri.instanceId == uriP->instanceId
         && targetP->uri.resourceId == uriP->resourceId)
        {
            return targetP;
        }

        targetP = targetP->next;
    }

    return targetP;
}

static void prv_observationRemove(lwm2m_client_t * clientP,
                                  lwm2m_observation_t * observationP)
{
    if (clientP->observationList == observationP)
    {
        clientP->observationList = clientP->observationList->next;
    }
    else
    {
        lwm2m_observation_t * parentP;

        parentP = clientP->observationList;
        while (parentP->next != NULL
            && parentP->next != observationP)
        {
            parentP = parentP->next;
        }
        if (parentP->next != NULL)
        {
            parentP->next = parentP->next->next;
        }
    }

    free(observationP);
}

static void prv_obsRequestCallback(lwm2m_transaction_t * transacP,
                                   void * message)
{
    lwm2m_observation_t * observationP = (lwm2m_observation_t *)transacP->userData;
    coap_packet_t * packet = (coap_packet_t *)message;
    uint8_t code;

    if (message == NULL)
    {
        code = COAP_503_SERVICE_UNAVAILABLE;
    }
    else if (packet->code == COAP_205_CONTENT
         && !IS_OPTION(packet, COAP_OPTION_OBSERVE))
    {
        code = COAP_405_METHOD_NOT_ALLOWED;
    }
    else
    {
        code = packet->code;
    }

    if (code != COAP_205_CONTENT)
    {
        observationP->callback(((lwm2m_client_t*)transacP->peerP)->internalID,
                               &observationP->uri,
                               code,
                               NULL, 0,
                               observationP->userData);
        prv_observationRemove(((lwm2m_client_t*)transacP->peerP), observationP);
    }
    else
    {
        observationP->status = STATE_REGISTERED;
        observationP->callback(((lwm2m_client_t*)transacP->peerP)->internalID,
                               &observationP->uri,
                               COAP_205_CONTENT,
                               packet->payload, packet->payload_len,
                               observationP->userData);
    }
}

static void prv_obsCancelCallback(lwm2m_transaction_t * transacP,
                                  void * message)
{
    lwm2m_observation_t * observationP = (lwm2m_observation_t *)transacP->userData;

    if (message == NULL)
    {
        observationP->callback(((lwm2m_client_t*)transacP->peerP)->internalID,
                               &observationP->uri,
                               COAP_503_SERVICE_UNAVAILABLE,
                               NULL, 0,
                               observationP->userData);
    }
    else
    {
        coap_packet_t * packet = (coap_packet_t *)message;

        observationP->callback(((lwm2m_client_t*)transacP->peerP)->internalID,
                               &observationP->uri,
                               packet->code,
                               packet->payload, packet->payload_len,
                               observationP->userData);
    }
    prv_observationRemove(((lwm2m_client_t*)transacP->peerP), observationP);
}

int lwm2m_observe(lwm2m_context_t * contextP,
                  uint16_t clientID,
                  lwm2m_uri_t * uriP,
                  lwm2m_result_callback_t callback,
                  void * userData)
{
    lwm2m_client_t * clientP;
    lwm2m_transaction_t * transactionP;
    lwm2m_observation_t * observationP;
    uint8_t token[4];

    clientP = (lwm2m_client_t *)lwm2m_list_find((lwm2m_list_t *)contextP->clientList, clientID);
    if (clientP == NULL) return COAP_404_NOT_FOUND;

    observationP = (lwm2m_observation_t *)malloc(sizeof(lwm2m_observation_t));
    if (observationP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
    memset(observationP, 0, sizeof(lwm2m_observation_t));

    transactionP = transaction_new(COAP_GET, uriP, contextP->nextMID++, ENDPOINT_CLIENT, (void *)clientP);
    if (transactionP == NULL)
    {
        free(observationP);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    observationP->id = lwm2m_list_newId((lwm2m_list_t *)clientP->observationList);
    memcpy(&observationP->uri, uriP, sizeof(lwm2m_uri_t));
    observationP->status = STATE_REG_PENDING;
    observationP->callback = callback;
    observationP->userData = userData;
    clientP->observationList = (lwm2m_observation_t *)LWM2M_LIST_ADD(clientP->observationList, observationP);

    token[0] = clientP->internalID >> 8;
    token[1] = clientP->internalID & 0xFF;
    token[2] = observationP->id >> 8;
    token[3] = observationP->id & 0xFF;

    coap_set_header_observe(transactionP->message, 0);
    coap_set_header_token(transactionP->message, token, sizeof(token));

    transactionP->callback = prv_obsRequestCallback;
    transactionP->userData = (void *)observationP;

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transactionP);

    return transaction_send(contextP, transactionP);
}

int lwm2m_observe_cancel(lwm2m_context_t * contextP,
                         uint16_t clientID,
                         lwm2m_uri_t * uriP,
                         lwm2m_result_callback_t callback,
                         void * userData)
{
    lwm2m_client_t * clientP;
    lwm2m_transaction_t * transactionP;
    lwm2m_observation_t * observationP;

    clientP = (lwm2m_client_t *)lwm2m_list_find((lwm2m_list_t *)contextP->clientList, clientID);
    if (clientP == NULL) return COAP_404_NOT_FOUND;

    observationP = prv_findObservationByURI(clientP, uriP);
    if (observationP == NULL) return COAP_404_NOT_FOUND;

    transactionP = transaction_new(COAP_GET, uriP, contextP->nextMID++, ENDPOINT_CLIENT, (void *)clientP);
    if (transactionP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transactionP);

    transactionP->callback = prv_obsCancelCallback;
    transactionP->userData = (void *)observationP;

    return transaction_send(contextP, transactionP);
}

void handle_observe_notify(lwm2m_context_t * contextP,
                           struct sockaddr * fromAddr,
                           socklen_t fromAddrLen,
                           coap_packet_t * message)
{
    uint8_t * tokenP;
    int token_len;
    uint16_t clientID;
    uint16_t obsID;
    lwm2m_client_t * clientP;
    lwm2m_observation_t * observationP;
    uint32_t count;

    token_len = coap_get_header_token(message, (const uint8_t **)&tokenP);
    if (token_len != sizeof(uint32_t)) return;

    clientID = (tokenP[0] << 8) | tokenP[1];
    obsID = (tokenP[2] << 8) | tokenP[3];

    clientP = (lwm2m_client_t *)lwm2m_list_find((lwm2m_list_t *)contextP->clientList, clientID);
    if (clientP == NULL) return;

    observationP = (lwm2m_observation_t *)lwm2m_list_find((lwm2m_list_t *)clientP->observationList, obsID);
    if (observationP == NULL) return;

    if (1 != coap_get_header_observe(message, &count)) return;

    observationP->callback(clientID,
                           &observationP->uri,
                           (int)count,
                           message->payload, message->payload_len,
                           observationP->userData);
}

