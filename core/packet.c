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
#include <netdb.h>

#include <stdio.h>


static int prv_check_addr(uint8_t * leftAddr,
                          size_t leftAddrLen,
                          uint8_t * rightAddr,
                          size_t rightAddrLen)
{
    if (leftAddrLen != rightAddrLen) return 0;

    if (memcmp(leftAddr, rightAddr, leftAddrLen) != 0) return 0;

    return 1;
}

static void handle_reset(lwm2m_context_t * contextP,
                         uint8_t * fromAddr,
                         size_t fromAddrLen,
                         coap_packet_t * message)
{
#ifdef LWM2M_CLIENT_MODE
    cancel_observe(contextP, message->mid, fromAddr, fromAddrLen);
#endif
}

static void handle_response(lwm2m_context_t * contextP,
                            lwm2m_transaction_t * transacP,
                            uint8_t * fromAddr,
                            size_t fromAddrLen,
                            coap_packet_t * message)
{
    uint8_t * targetAddr;
    size_t targetAddrLen;

    switch (transacP->peerType)
    {
#ifdef LWM2M_SERVER_MODE
    case ENDPOINT_CLIENT:
        targetAddr = ((lwm2m_client_t *)transacP->peerP)->addr;
        targetAddrLen = ((lwm2m_client_t *)transacP->peerP)->addrLen;
        break;
#endif

#ifdef LWM2M_CLIENT_MODE
    case ENDPOINT_SERVER:
        targetAddr = ((lwm2m_server_t *)transacP->peerP)->addr;
        targetAddrLen = ((lwm2m_server_t *)transacP->peerP)->addrLen;
        break;
#endif

    default:
        return;
    }

    if (prv_check_addr(fromAddr, fromAddrLen, targetAddr, targetAddrLen))
    {
        transacP->callback(transacP, message);
        transaction_remove(contextP, transacP);
    }
}

static coap_status_t handle_request(lwm2m_context_t * contextP,
                                    uint8_t * fromAddr,
                                    size_t fromAddrLen,
                                    coap_packet_t * message,
                                    coap_packet_t * response)
{
    lwm2m_uri_t * uriP;
    coap_status_t result = NOT_FOUND_4_04;

    uriP = lwm2m_decode_uri(message->uri_path);
    if (uriP == NULL) return BAD_REQUEST_4_00;

    switch(uriP->flag & LWM2M_URI_MASK_TYPE)
    {
#ifdef LWM2M_CLIENT_MODE
    case LWM2M_URI_FLAG_DM:
        // TODO: Authentify server
        result = handle_dm_request(contextP, uriP, fromAddr, fromAddrLen, message, response);
        break;

    case LWM2M_URI_FLAG_BOOTSTRAP:
        result = NOT_IMPLEMENTED_5_01;
        break;
#endif

#ifdef LWM2M_SERVER_MODE
   case LWM2M_URI_FLAG_REGISTRATION:
        result = handle_registration_request(contextP, uriP, fromAddr, fromAddrLen, message, response);
        break;
#endif
    default:
        result = BAD_REQUEST_4_00;
        break;
    }

    coap_set_status_code(response, result);

    if (result < BAD_REQUEST_4_00)
    {
        result = NO_ERROR;
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
                        uint8_t * fromAddr,
                        size_t fromAddrLen)
{
    coap_status_t coap_error_code = NO_ERROR;
    static coap_packet_t message[1];
    static coap_packet_t response[1];
    uint8_t pktBuffer[COAP_MAX_PACKET_SIZE+1];
    size_t pktBufferLen = 0;

    coap_error_code = coap_parse_message(message, buffer, (uint16_t)length);
    if (coap_error_code==NO_ERROR)
    {
        LOG("  Parsed: ver %u, type %u, tkl %u, code %u, mid %u\r\n", message->version, message->type, message->token_len, message->code, message->mid);
        LOG("  Payload: %.*s\r\n\n", message->payload_len, message->payload);

        if (message->code >= COAP_GET && message->code <= COAP_DELETE)
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
                LOG("Blockwise: block request %u (%u/%u) @ %u bytes\n", block_num, block_size, REST_MAX_CHUNK_SIZE, block_offset);
                block_size = MIN(block_size, REST_MAX_CHUNK_SIZE);
                new_offset = block_offset;
            }

            coap_error_code = handle_request(contextP, fromAddr, fromAddrLen, message, response);
            if (coap_error_code==NO_ERROR)
            {
                /* Apply blockwise transfers. */
                if ( IS_OPTION(message, COAP_OPTION_BLOCK1) && response->code<BAD_REQUEST_4_00 && !IS_OPTION(response, COAP_OPTION_BLOCK1) )
                {
                    LOG("Block1 NOT IMPLEMENTED\n");

                    coap_error_code = NOT_IMPLEMENTED_5_01;
                    coap_error_message = "NoBlock1Support";
                }
                else if ( IS_OPTION(message, COAP_OPTION_BLOCK2) )
                {
                    /* unchanged new_offset indicates that resource is unaware of blockwise transfer */
                    if (new_offset==block_offset)
                    {
                        LOG("Blockwise: unaware resource with payload length %u/%u\n", response->payload_len, block_size);
                        if (block_offset >= response->payload_len)
                        {
                            LOG("handle_incoming_data(): block_offset >= response->payload_len\n");

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
                        LOG("Blockwise: blockwise resource, new offset %d\n", new_offset);
                        coap_set_header_block2(response, block_num, new_offset!=-1 || response->payload_len > block_size, block_size);
                        if (response->payload_len > block_size) coap_set_payload(response, response->payload, block_size);
                    } /* if (resource aware of blockwise) */
                }
                else if (new_offset!=0)
                {
                    LOG("Blockwise: no block option for blockwise resource, using block size %u\n", REST_MAX_CHUNK_SIZE);

                    coap_set_header_block2(response, 0, new_offset!=-1, REST_MAX_CHUNK_SIZE);
                    coap_set_payload(response, response->payload, MIN(response->payload_len, REST_MAX_CHUNK_SIZE));
                } /* if (blockwise request) */

                coap_error_code = message_send(contextP, response, fromAddr, fromAddrLen);

                free(response->payload);
                response->payload = NULL;
                response->payload_len = 0;
            }
        }
        else
        {
            /* Responses */
            lwm2m_transaction_t * transaction;

            if (message->type==COAP_TYPE_ACK)
            {
                LOG("Received ACK\n");
            }
            else if (message->type==COAP_TYPE_RST)
            {
                LOG("Received RST\n");
                /* Cancel possible subscriptions. */
                handle_reset(contextP, fromAddr, fromAddrLen, message);
            }

            transaction = (lwm2m_transaction_t *)lwm2m_list_find((lwm2m_list_t *)contextP->transactionList, message->mid);
            if (NULL != transaction)
            {
                handle_response(contextP, transaction, fromAddr, fromAddrLen, message);
            }
#ifdef LWM2M_SERVER_MODE
            else if (message->code = COAP_204_CHANGED
                  && IS_OPTION(message, COAP_OPTION_OBSERVE))
            {
                handle_observe_notify(contextP, fromAddr, fromAddrLen, message);
            }
#endif
        } /* Request or Response */
    } /* if (parsed correctly) */
    else
    {
        LOG("Message parsing failed %d\r\n", coap_error_code);
    }

    if (coap_error_code != NO_ERROR)
    {
        LOG("ERROR %u: %s\n", coap_error_code, coap_error_message);

        /* Set to sendable error code. */
        if (coap_error_code >= 192)
        {
            coap_error_code = INTERNAL_SERVER_ERROR_5_00;
        }
        /* Reuse input buffer for error message. */
        coap_init_message(message, COAP_TYPE_ACK, coap_error_code, message->mid);
        coap_set_payload(message, coap_error_message, strlen(coap_error_message));
        message_send(contextP, message, fromAddr, fromAddrLen);
    }
}


coap_status_t message_send(lwm2m_context_t * contextP,
                           coap_packet_t * message,
                           uint8_t * addr,
                           size_t addrLen)
{
    coap_status_t result = INTERNAL_SERVER_ERROR_5_00;
    uint8_t pktBuffer[COAP_MAX_PACKET_SIZE+1];
    size_t pktBufferLen = 0;

    pktBufferLen = coap_serialize_message(message, pktBuffer);
    if (0 != pktBufferLen)
    {
        result = contextP->bufferSendCallback(contextP->socket, pktBuffer, pktBufferLen, addr, addrLen);
    }

    return result;
}

