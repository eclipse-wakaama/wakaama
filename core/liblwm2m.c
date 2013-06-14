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

#include <stdio.h>


lwm2m_context_t * lwm2m_init()
{
    lwm2m_context_t * contextP;

    contextP = (lwm2m_context_t *)malloc(sizeof(lwm2m_context_t));
    if (NULL != contextP)
    {
        memset(contextP, 0, sizeof(lwm2m_context_t));
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
            contextP->objectList[i]->closeFunc(contextP->objectList[i]->userData);
        }
        free(contextP->objectList[i]);
    }

    while (NULL != contextP->serverList)
    {
        lwm2m_server_t * targetP;

        targetP = contextP->serverList;
        contextP->serverList = contextP->serverList->next;

        if (NULL != targetP->uri) free (targetP->uri);
        if (NULL != targetP->privateKey) free (targetP->privateKey);
        if (NULL != targetP->publicKey) free (targetP->publicKey);
        free(targetP);
    }

    if (NULL != contextP->objectList)
    {
        free(contextP->objectList);
    }

    free(contextP);
}


int lwm2m_add_server(lwm2m_context_t * contextP,
                     lwm2m_server_t * serverP)
{
    serverP->next = contextP->serverList;
    contextP->serverList = serverP;
}


int lwm2m_add_object(lwm2m_context_t * contextP,
                     lwm2m_object_t * objectP)
{
    lwm2m_object_t ** newArray;
    int i;

    newArray = (lwm2m_object_t **)malloc((contextP->numObject + 1) * sizeof(lwm2m_object_t *));
    if (NULL == newArray) return -1;

    for (i = 0;
         i < contextP->numObject && contextP->objectList[i]->objID < objectP->objID;
         i++)
    {
        newArray[i] = contextP->objectList[i];
    }

    if (i < contextP->numObject
     && contextP->objectList[i]->objID == objectP->objID)
    {
        // duplicate ID
        free(newArray);
        return -1;
    }

    newArray[i] = objectP;

    while(i < contextP->numObject)
    {
        newArray[i+1] = contextP->objectList[i];
        i++;
    }

    contextP->numObject++;
    if (NULL != contextP->objectList) free(contextP->objectList);
    contextP->objectList = newArray;

    return 0;
}

static void handle_response(coap_packet_t * message)
{
}

static coap_status_t handle_request(lwm2m_context_t * contextP,
                                    coap_packet_t * message,
                                    coap_packet_t * response,
                                    uint8_t *coap_buffer,
                                    uint16_t preferred_size,
                                    int32_t *offset)
{
    lwm2m_uri_t * uriP;
    coap_status_t result = NOT_FOUND_4_04;

    uriP = lwm2m_decode_uri(message->uri_path);

    if (NULL == uriP)
    {
        return BAD_REQUEST_4_00;
    }

    switch (message->code)
    {
    case COAP_GET:
        {
            char * buffer = NULL;
            int length = 0;

            result = object_read(contextP, uriP, &buffer, &length);
            if (NULL != buffer)
            {
                coap_set_payload(response, buffer, length);
                // lwm2m_handle_packet will free buffer
            }
        }
        break;
    case COAP_POST:
        result = NOT_IMPLEMENTED_5_01;
        break;
    case COAP_PUT:
        {
            result = object_write(contextP, uriP, message->payload, message->payload_len);
        }
        break;
    case COAP_DELETE:
        result = NOT_IMPLEMENTED_5_01;
        break;
    default:
        result = BAD_REQUEST_4_00;
        break;
    }

    coap_set_status_code(response, result);

    free(uriP);

    if (result < BAD_REQUEST_4_00)
    {
        return NO_ERROR;
    }

    return result;
}

/* This function is an adaptation of function coap_receive() from Erbium's er-coap-13-engine.c.
 * Erbium is Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
 * All rights reserved.
 */
int lwm2m_handle_packet(lwm2m_context_t * contextP,
                        uint8_t * buffer,
                        int length,
                        int socket,
                        struct sockaddr_storage fromAddr,
                        socklen_t fromAddrLen)
{
    coap_status_t coap_error_code = NO_ERROR;
    static coap_packet_t message[1];
    static coap_packet_t response[1];
    static coap_transaction_t * transaction = NULL;

    coap_error_code = coap_parse_message(message, buffer, (uint16_t)length);
    if (coap_error_code==NO_ERROR)
    {
        fprintf(stdout, "  Parsed: ver %u, type %u, tkl %u, code %u, mid %u\r\n", message->version, message->type, message->token_len, message->code, message->mid);
        fprintf(stdout, "  Payload: %.*s\r\n\n", message->payload_len, message->payload);

        if (message->code >= COAP_GET && message->code <= COAP_DELETE)
        {
            /* Use transaction buffer for response to confirmable request. */
            if ( (transaction = coap_new_transaction(message->mid, socket, &fromAddr, fromAddrLen)) )
            {
                uint32_t block_num = 0;
                uint16_t block_size = REST_MAX_CHUNK_SIZE;
                uint32_t block_offset = 0;
                int32_t new_offset = 0;

                /* prepare response */
                if (message->type==COAP_TYPE_CON)
                {
                    /* Reliable CON requests are answered with an ACK. */
                    coap_init_message(response, COAP_TYPE_ACK, CONTENT_2_05, message->mid);
                }
                else
                {
                    /* Unreliable NON requests are answered with a NON as well. */
                    coap_init_message(response, COAP_TYPE_NON, CONTENT_2_05, coap_get_mid());
                }

                /* mirror token */
                if (message->token_len)
                {
                    coap_set_header_token(response, message->token, message->token_len);
                }

                /* get offset for blockwise transfers */
                if (coap_get_header_block2(message, &block_num, NULL, &block_size, &block_offset))
                {
                    fprintf(stdout, "Blockwise: block request %lu (%u/%u) @ %lu bytes\n", block_num, block_size, REST_MAX_CHUNK_SIZE, block_offset);
                    block_size = MIN(block_size, REST_MAX_CHUNK_SIZE);
                    new_offset = block_offset;
                }

                coap_error_code = handle_request(contextP, message, response, transaction->packet+COAP_MAX_HEADER_SIZE, block_size, &new_offset);
                if (coap_error_code==NO_ERROR)
                {
                    /* Apply blockwise transfers. */
                    if ( IS_OPTION(message, COAP_OPTION_BLOCK1) && response->code<BAD_REQUEST_4_00 && !IS_OPTION(response, COAP_OPTION_BLOCK1) )
                    {
                        fprintf(stdout, "Block1 NOT IMPLEMENTED\n");

                        coap_error_code = NOT_IMPLEMENTED_5_01;
                        coap_error_message = "NoBlock1Support";
                    }
                    else if ( IS_OPTION(message, COAP_OPTION_BLOCK2) )
                    {
                        /* unchanged new_offset indicates that resource is unaware of blockwise transfer */
                        if (new_offset==block_offset)
                        {
                            fprintf(stdout, "Blockwise: unaware resource with payload length %u/%u\n", response->payload_len, block_size);
                            if (block_offset >= response->payload_len)
                            {
                                fprintf(stdout, "handle_incoming_data(): block_offset >= response->payload_len\n");

                                response->code = BAD_OPTION_4_02;
                                coap_set_payload(response, "BlockOutOfScope", 15); /* a const char str[] and sizeof(str) produces larger code size */
                            }
                            else
                            {
                                coap_set_header_block2(response, block_num, response->payload_len - block_offset > block_size, block_size);
                                coap_set_payload(response, response->payload+block_offset, MIN(response->payload_len - block_offset, block_size));
                            } /* if (valid offset) */
                        }
                        else
                        {
                            /* resource provides chunk-wise data */
                            fprintf(stdout, "Blockwise: blockwise resource, new offset %ld\n", new_offset);
                            coap_set_header_block2(response, block_num, new_offset!=-1 || response->payload_len > block_size, block_size);
                            if (response->payload_len > block_size) coap_set_payload(response, response->payload, block_size);
                        } /* if (resource aware of blockwise) */
                    }
                    else if (new_offset!=0)
                    {
                        fprintf(stdout, "Blockwise: no block option for blockwise resource, using block size %u\n", REST_MAX_CHUNK_SIZE);

                        coap_set_header_block2(response, 0, new_offset!=-1, REST_MAX_CHUNK_SIZE);
                        coap_set_payload(response, response->payload, MIN(response->payload_len, REST_MAX_CHUNK_SIZE));
                    } /* if (blockwise request) */
                } /* no errors/hooks */

                /* Serialize response. */
                if (coap_error_code==NO_ERROR)
                {
                    if ((transaction->packet_len = coap_serialize_message(response, transaction->packet))==0)
                    {
                        coap_error_code = PACKET_SERIALIZATION_ERROR;
                    }
                    free(response->payload);
                    response->payload = NULL;
                    response->payload_len = 0;
                }
            }
            else
            {
                coap_error_code = SERVICE_UNAVAILABLE_5_03;
                coap_error_message = "NoFreeTraBuffer";
            } /* if (transaction buffer) */
        }
        else
        {
            /* Responses */

            if (message->type==COAP_TYPE_ACK)
            {
              fprintf(stdout, "Received ACK\n");
            }
            else if (message->type==COAP_TYPE_RST)
            {
                fprintf(stdout, "Received RST\n");
                /* Cancel possible subscriptions. */
 //               coap_remove_observer_by_mid(&UIP_IP_BUF->srcipaddr, UIP_UDP_BUF->srcport, message->mid);
            }

            if ( (transaction = coap_get_transaction_by_mid(message->mid)) )
            {
                /* Free transaction memory before callback, as it may create a new transaction. */
                coap_clear_transaction(transaction);

                handle_response(message);
            }
            transaction = NULL;
        } /* Request or Response */
    } /* if (parsed correctly) */
    else
    {
        fprintf(stderr, "Message parsing failed %d\r\n", coap_error_code);
    }

    if (coap_error_code==NO_ERROR)
    {
        if (transaction) coap_send_transaction(transaction);
    }
    else if (coap_error_code==MANUAL_RESPONSE)
    {
        fprintf(stdout, "Clearing transaction for manual response");
        coap_clear_transaction(transaction);
    }
    else
    {
        uint8_t buffer[COAP_MAX_PACKET_SIZE+1];
        size_t bufferLen = 0;

        fprintf(stdout, "ERROR %u: %s\n", coap_error_code, coap_error_message);
        coap_clear_transaction(transaction);

        /* Set to sendable error code. */
        if (coap_error_code >= 192)
        {
            coap_error_code = INTERNAL_SERVER_ERROR_5_00;
        }
        /* Reuse input buffer for error message. */
        coap_init_message(message, COAP_TYPE_ACK, coap_error_code, message->mid);
        coap_set_payload(message, coap_error_message, strlen(coap_error_message));
        bufferLen = coap_serialize_message(message, buffer);
        if (0 != bufferLen)
        {
            coap_send_message(socket, &fromAddr, fromAddrLen, buffer, bufferLen);
        }
    }
}

