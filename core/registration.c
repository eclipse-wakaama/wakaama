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


#define REGISTRATION_URI "/rd"
#define QUERY_TEMPLATE "ep="
#define QUERY_LENGTH 3


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
        coap_set_header_uri_path(message, REGISTRATION_URI);
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

void handle_server_reply(lwm2m_context_t * contextP,
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
