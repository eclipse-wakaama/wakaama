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

/*
Contains code snippets which are:

 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.

*/


#include "internals.h"
#include <stdlib.h>
#include <string.h>


lwm2m_transaction_t * transaction_new(uint16_t mID,
                                      lwm2m_endpoint_type_t peerType,
                                      void * peerP)
{
    lwm2m_transaction_t * transacP;

    transacP = (lwm2m_transaction_t *)malloc(sizeof(lwm2m_transaction_t));

    if (NULL != transacP)
    {
        memset(transacP, 0, sizeof(lwm2m_transaction_t));
        transacP->mID = mID;
        transacP->peerType = peerType;
        transacP->peerP = peerP;
    }

    return transacP;
}

void transaction_remove(lwm2m_context_t * contextP,
                        lwm2m_transaction_t * transacP)
{
    if (NULL != contextP->transactionList)
    {
        if (transacP == contextP->transactionList)
        {
            contextP->transactionList = contextP->transactionList->next;
        }
        else
        {
            lwm2m_transaction_t *previous = contextP->transactionList;
            while (previous->next && previous->next != transacP)
            {
                previous = previous->next;
            }
            if (NULL != previous->next)
            {
                previous->next = previous->next->next;
            }
        }
    }
    free(transacP);
}

void transaction_send(lwm2m_context_t * contextP,
                      lwm2m_transaction_t * transacP)
{
    switch(transacP->peerType)
    {
    case ENDPOINT_CLIENT:
        // not implemented yet
        break;

    case ENDPOINT_SERVER:
        sendto(contextP->socket,
               transacP->packet, transacP->packet_len,
               0,
               ((lwm2m_server_t*)transacP->peerP)->addr, ((lwm2m_server_t*)transacP->peerP)->addrLen);
        break;

    case ENDPOINT_BOOTSTRAP:
        // not implemented yet
        break;

    default:
        return;
    }

    if (transacP->retrans_counter < COAP_MAX_RETRANSMIT)
    {
        // TODO: handle retransit timer

        transacP->retrans_counter++;
    }
    else
    {
        transaction_remove(contextP, transacP);
    }
}
