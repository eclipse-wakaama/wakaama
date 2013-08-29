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
                             lwm2m_object_t * objectList[])
{
    lwm2m_context_t * contextP;

    contextP = (lwm2m_context_t *)malloc(sizeof(lwm2m_context_t));
    if (NULL != contextP)
    {
        memset(contextP, 0, sizeof(lwm2m_context_t));
        contextP->socket = socket;
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
    }

    return contextP;
}

void lwm2m_close(lwm2m_context_t * contextP)
{
    int i;

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
    free(contextP);
}

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
                     char * host,
                     uint16_t port,
                     lwm2m_security_t * securityP)
{
    lwm2m_server_t * serverP;
    char portStr[6];
    int status = INTERNAL_SERVER_ERROR_5_00;
    struct addrinfo hints;
    struct addrinfo *servinfo = NULL;
    struct addrinfo *p;
    int sock;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if (0 >= sprintf(portStr, "%hu", port)) return INTERNAL_SERVER_ERROR_5_00;
    if (0 != getaddrinfo(host, portStr, &hints, &servinfo) || servinfo == NULL) return NOT_FOUND_4_04;

    serverP = (lwm2m_server_t *)malloc(sizeof(lwm2m_server_t));
    if (serverP != NULL)
    {
        memset(serverP, 0, sizeof(lwm2m_server_t));

        memcpy(&(serverP->security), securityP, sizeof(lwm2m_security_t));
        serverP->shortID = shortID;
        serverP->port = port;

        // we test the various addresses
        sock = -1;
        for(p = servinfo ; p != NULL && sock == -1 ; p = p->ai_next)
        {
            sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (sock >= 0)
            {
                if (-1 == connect(sock, p->ai_addr, p->ai_addrlen))
                {
                    close(sock);
                    sock = -1;
                }
            }
        }
        if (sock >= 0)
        {
            serverP->addr = (struct sockaddr *)malloc(servinfo->ai_addrlen);
            if (serverP->addr != NULL)
            {
                memcpy(serverP->addr, servinfo->ai_addr, servinfo->ai_addrlen);
                serverP->addrLen = servinfo->ai_addrlen;

                contextP->serverList = (lwm2m_server_t*)LWM2M_LIST_ADD(contextP->serverList, serverP);

                status = 0;
            }
            else
            {
                free(serverP);
            }
            close(sock);
        }
    }

    freeaddrinfo(servinfo);
    return status;
}
