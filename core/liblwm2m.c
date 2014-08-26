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
 *    Fabien Fleutot - Please refer to git log
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

#include "internals.h"

#include <stdlib.h>
#include <string.h>

#include <stdio.h>


lwm2m_context_t * lwm2m_init(char * endpointName,
                             uint16_t numObject,
                             lwm2m_object_t * objectList[],
                             lwm2m_buffer_send_callback_t bufferSendCallback,
                             void * bufferSendUserData)
{
    lwm2m_context_t * contextP;

    if (NULL == bufferSendCallback)
        return NULL;

#ifdef LWM2M_CLIENT_MODE
    if (numObject != 0)
    {
        int i;

        for (i = 0 ; i < numObject ; i++)
        {
            if (objectList[i]->objID <= LWM2M_ACL_OBJECT_ID)
            {
                // Use of a reserved object ID
                return NULL;
            }
        }
    }
#endif

    contextP = (lwm2m_context_t *)lwm2m_malloc(sizeof(lwm2m_context_t));
    if (NULL != contextP)
    {
        memset(contextP, 0, sizeof(lwm2m_context_t));
        contextP->bufferSendCallback = bufferSendCallback;
        contextP->bufferSendUserData = bufferSendUserData;
        srand(time(NULL));
        contextP->nextMID = rand();
#ifdef LWM2M_CLIENT_MODE
        contextP->endpointName = strdup(endpointName);
        if (contextP->endpointName == NULL)
        {
            lwm2m_free(contextP);
            return NULL;
        }
        if (numObject != 0)
        {
            contextP->objectList = (lwm2m_object_t **)lwm2m_malloc(numObject * sizeof(lwm2m_object_t *));
            if (NULL != contextP->objectList)
            {
                memcpy(contextP->objectList, objectList, numObject * sizeof(lwm2m_object_t *));
                contextP->numObject = numObject;
            }
            else
            {
                lwm2m_free(contextP->endpointName);
                lwm2m_free(contextP);
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
        lwm2m_free(contextP->objectList[i]);
    }

    if (NULL != contextP->bootstrapServer)
    {
        if (NULL != contextP->bootstrapServer->uri) lwm2m_free (contextP->bootstrapServer->uri);
        if (NULL != contextP->bootstrapServer->security.privateKey) lwm2m_free (contextP->bootstrapServer->security.privateKey);
        if (NULL != contextP->bootstrapServer->security.publicKey) lwm2m_free (contextP->bootstrapServer->security.publicKey);
        lwm2m_free(contextP->bootstrapServer);
    }

    while (NULL != contextP->serverList)
    {
        lwm2m_server_t * targetP;

        targetP = contextP->serverList;
        contextP->serverList = contextP->serverList->next;

        registration_deregister(contextP, targetP);

        if (NULL != targetP->location) lwm2m_free(targetP->location);
        if (NULL != targetP->security.privateKey) lwm2m_free (targetP->security.privateKey);
        if (NULL != targetP->security.publicKey) lwm2m_free (targetP->security.publicKey);
        if (NULL != targetP->sms) lwm2m_free (targetP->sms);
        lwm2m_free(targetP);
    }

    while (NULL != contextP->observedList)
    {
        lwm2m_observed_t * targetP;

        targetP = contextP->observedList;
        contextP->observedList = contextP->observedList->next;

        while (NULL != targetP->watcherList)
        {
            lwm2m_watcher_t * watcherP;

            watcherP = targetP->watcherList;
            targetP->watcherList = targetP->watcherList->next;
            lwm2m_free(watcherP);
        }
        lwm2m_free(targetP);
    }

   if (NULL != contextP->objectList)
    {
        lwm2m_free(contextP->objectList);
    }

    lwm2m_free(contextP->endpointName);
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

    lwm2m_free(contextP);
}

#ifdef LWM2M_CLIENT_MODE
void lwm2m_set_bootstrap_server(lwm2m_context_t * contextP,
                               lwm2m_bootstrap_server_t * serverP)
{
    if (NULL != contextP->bootstrapServer)
    {
        if (NULL != contextP->bootstrapServer->uri) lwm2m_free (contextP->bootstrapServer->uri);
        if (NULL != contextP->bootstrapServer->security.privateKey) lwm2m_free (contextP->bootstrapServer->security.privateKey);
        if (NULL != contextP->bootstrapServer->security.publicKey) lwm2m_free (contextP->bootstrapServer->security.publicKey);
        lwm2m_free(contextP->bootstrapServer);
    }
    contextP->bootstrapServer = serverP;
}

int lwm2m_add_server(lwm2m_context_t * contextP,
                     uint16_t shortID,
                     uint32_t lifetime,
                     char * sms,
                     lwm2m_binding_t binding,
                     void * sessionH,
                     lwm2m_security_t * securityP)
{
    lwm2m_server_t * serverP;
    int status = COAP_500_INTERNAL_SERVER_ERROR;

    serverP = (lwm2m_server_t *)lwm2m_malloc(sizeof(lwm2m_server_t));
    if (serverP != NULL)
    {
        memset(serverP, 0, sizeof(lwm2m_server_t));
        memcpy(&(serverP->security), securityP, sizeof(lwm2m_security_t));
        serverP->shortID = shortID;
        serverP->lifetime = lifetime;
        serverP->binding = binding;
        if (sms != NULL)
        {
            // copy the SMS number
            int len = strlen(sms);
            serverP->sms = (char*) lwm2m_malloc(strlen(sms)+1);
            memcpy(serverP->sms, sms, len+1);
        }
        serverP->sessionH = sessionH;
        contextP->serverList = (lwm2m_server_t*)LWM2M_LIST_ADD(contextP->serverList, serverP);

        status = COAP_NO_ERROR;
    }

    return status;
}
#endif

int lwm2m_step(lwm2m_context_t * contextP,
               struct timeval * timeoutP)
{
    lwm2m_transaction_t * transacP;
    struct timeval tv;
#ifdef LWM2M_SERVER_MODE
    lwm2m_client_t * clientP;
#endif

    if (0 != lwm2m_gettimeofday(&tv, NULL)) return COAP_500_INTERNAL_SERVER_ERROR;

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

#ifdef LWM2M_SERVER_MODE
    // monitor clients lifetime
    clientP = contextP->clientList;
    while (clientP != NULL)
    {
        lwm2m_client_t * nextP = clientP->next;

        if (clientP->endOfLife <= tv.tv_sec)
        {
            contextP->clientList = (lwm2m_client_t *)LWM2M_LIST_RM(contextP->clientList, clientP->internalID, NULL);
            if (contextP->monitorCallback != NULL)
            {
                contextP->monitorCallback(clientP->internalID, NULL, DELETED_2_02, NULL, 0, contextP->monitorUserData);
            }
            prv_freeClient(clientP);
        }
        else
        {
            time_t interval;

            interval = clientP->endOfLife - tv.tv_sec;

            if (timeoutP->tv_sec > interval)
            {
                timeoutP->tv_sec = interval;
            }
        }
        clientP = nextP;
    }
#endif

    return 0;
}
