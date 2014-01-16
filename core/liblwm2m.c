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
#include <netdb.h>

#include <stdio.h>


lwm2m_context_t * lwm2m_init(int socket,
                             char * endpointName,
                             uint16_t numObject,
                             lwm2m_object_t * objectList[],
                             lwm2m_buffer_send_callback_t bufferSendCallback)
{
    lwm2m_context_t * contextP;

    if (NULL == bufferSendCallback)
        return NULL;

    contextP = (lwm2m_context_t *)malloc(sizeof(lwm2m_context_t));
    if (NULL != contextP)
    {
        memset(contextP, 0, sizeof(lwm2m_context_t));
        contextP->socket = socket;
        contextP->bufferSendCallback = bufferSendCallback;
#ifdef LWM2M_CLIENT_MODE
        contextP->endpointName = strdup(endpointName);
        if (contextP->endpointName == NULL)
        {
            free(contextP);
            return NULL;
        }
        if (numObject != 0)
        {
            contextP->objectList = (lwm2m_object_t **)malloc(numObject * sizeof(lwm2m_object_t *));
            if (NULL != contextP->objectList)
            {
                memcpy(contextP->objectList, objectList, numObject * sizeof(lwm2m_object_t *));
                contextP->numObject = numObject;
            }
            else
            {
                free(contextP->endpointName);
                free(contextP);
                return NULL;
            }
        }
#endif
    }

    return contextP;
}

void lwm2m_close(lwm2m_context_t * contextP)
{
    int i;

#ifdef LWM2M_CLIENT_MODE
    for (i = 0 ; i < contextP->numObject ; i++)
    {
        if (NULL != contextP->objectList[i]->closeFunc)
        {
            contextP->objectList[i]->closeFunc(contextP->objectList[i]);
        }
        free(contextP->objectList[i]);
    }

    if (NULL != contextP->bootstrapServer)
    {
        if (NULL != contextP->bootstrapServer->uri) free (contextP->bootstrapServer->uri);
        if (NULL != contextP->bootstrapServer->security.privateKey) free (contextP->bootstrapServer->security.privateKey);
        if (NULL != contextP->bootstrapServer->security.publicKey) free (contextP->bootstrapServer->security.publicKey);
        free(contextP->bootstrapServer);
    }

    while (NULL != contextP->serverList)
    {
        lwm2m_server_t * targetP;

        targetP = contextP->serverList;
        contextP->serverList = contextP->serverList->next;

        registration_deregister(contextP, targetP);

        if (NULL != targetP->addr) free (targetP->addr);
        if (NULL != targetP->security.privateKey) free (targetP->security.privateKey);
        if (NULL != targetP->security.publicKey) free (targetP->security.publicKey);
        free(targetP);
    }

    if (NULL != contextP->objectList)
    {
        free(contextP->objectList);
    }

    free(contextP->endpointName);
#endif

#ifdef LWM2M_SERVER_MODE
    while (NULL != contextP->clientList)
    {
        lwm2m_client_t * clientP;

        clientP = contextP->clientList;
        contextP->clientList = contextP->clientList->next;

        prv_freeClient(clientP);
    }
#endif

    while (NULL != contextP->transactionList)
    {
        lwm2m_transaction_t * transacP;

        transacP = contextP->transactionList;
        contextP->transactionList = contextP->transactionList->next;

        transaction_free(transacP);
    }

    free(contextP);
}

#ifdef LWM2M_CLIENT_MODE
int lwm2m_set_bootstrap_server(lwm2m_context_t * contextP,
                               lwm2m_bootstrap_server_t * serverP)
{
    if (NULL != contextP->bootstrapServer)
    {
        if (NULL != contextP->bootstrapServer->uri) free (contextP->bootstrapServer->uri);
        if (NULL != contextP->bootstrapServer->security.privateKey) free (contextP->bootstrapServer->security.privateKey);
        if (NULL != contextP->bootstrapServer->security.publicKey) free (contextP->bootstrapServer->security.publicKey);
        free(contextP->bootstrapServer);
    }
    contextP->bootstrapServer = serverP;
}

int lwm2m_add_server(lwm2m_context_t * contextP,
                     uint16_t shortID,
                     uint8_t * addr,
                     size_t addrLen,
                     lwm2m_security_t * securityP)
{
    lwm2m_server_t * serverP;
    int status = COAP_500_INTERNAL_SERVER_ERROR;

    serverP = (lwm2m_server_t *)malloc(sizeof(lwm2m_server_t));
    if (serverP != NULL)
    {
        memset(serverP, 0, sizeof(lwm2m_server_t));
        memcpy(&(serverP->security), securityP, sizeof(lwm2m_security_t));
        serverP->shortID = shortID;
        serverP->addr = (uint8_t *)malloc(addrLen);
        if (serverP->addr != NULL)
        {
            memcpy(serverP->addr, addr, addrLen);
            serverP->addrLen = addrLen;

            contextP->serverList = (lwm2m_server_t*)LWM2M_LIST_ADD(contextP->serverList, serverP);

            status = 0;
        }
        else
        {
            free(serverP);
        }
    }

    return status;
}
#endif

int lwm2m_step(lwm2m_context_t * contextP,
               struct timeval * timeoutP)
{
    lwm2m_transaction_t * transacP;
    struct timeval tv;

    if (0 != gettimeofday(&tv, NULL)) return COAP_500_INTERNAL_SERVER_ERROR;

    transacP = contextP->transactionList;
    while (transacP != NULL)
    {
        // transaction_send() may remove transaction from the linked list
        lwm2m_transaction_t * nextP = transacP->next;
        int removed = 0;

        if (transacP->retrans_time <= tv.tv_sec)
        {
            removed = transaction_send(contextP, transacP);
        }

        if (0 == removed)
        {
            time_t interval;

            if (transacP->retrans_time > tv.tv_sec)
            {
                interval = transacP->retrans_time - tv.tv_sec;
            }
            else
            {
                interval = 1;
            }

            if (timeoutP->tv_sec > interval)
            {
                timeoutP->tv_sec = interval;
            }
        }

        transacP = nextP;
    }

    return 0;
}
