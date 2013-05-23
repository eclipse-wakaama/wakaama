
#include "lwm2mclient.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>

#include "externals/er-coap-13/er-coap-13.h"
#include "externals/er-coap-13/er-coap-13-transactions.h"

#define MAX_PACKET_SIZE 128

static int g_quit = 0;

void handle_sigint(int signum)
{
    g_quit = 1;
}

void print_usage(void)
{
    fprintf(stderr, "Usage: lwm2mclient\r\n");
    fprintf(stderr, "Launch a LWM2M client.\r\n\n");
}

int get_socket()
{
    int s = -1;
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = PF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, "5683", &hints, &res);

    for(p = res ; p != NULL && s == -1 ; p = p->ai_next)
    {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s >= 0)
        {
            if (-1 == bind(s, p->ai_addr, p->ai_addrlen))
            {
                close(s);
                s = -1;
            }
        }
    }

    freeaddrinfo(res);

    return s;
}


static int prv_get_number(const char * uriString,
                          size_t uriLength,
                          int * headP)
{
    int result = 0;
    int mul = 0;

    while (*headP < uriLength && uriString[*headP] != '/')
    {
        if ('0' <= uriString[*headP] && uriString[*headP] <= '9')
        {
            if (0 == mul)
            {
                mul = 10;
            }
            else
            {
                result *= mul;
                mul *= 10;
            }
            result += uriString[*headP] - '0';
        }
        else
        {
            return -1;
        }
        *headP += 1;
    }

    if (uriString[*headP] == '/')
        *headP += 1;

    return result;
}


lwm2m_uri_t * decode_uri(const char * uriString,
                         size_t uriLength)
{
    lwm2m_uri_t * uriP;
    int head = 0;
    int readNum;

    if (NULL == uriString || 0 == uriLength) return NULL;

    uriP = (lwm2m_uri_t *)malloc(sizeof(lwm2m_uri_t));
    if (NULL == uriP) return NULL;

    uriP->objID = LWM2M_URI_NO_ID;
    uriP->objInstance = LWM2M_URI_NO_INSTANCE;
    uriP->resID = LWM2M_URI_NO_ID;
    uriP->resInstance = LWM2M_URI_NO_INSTANCE;

    // Read object ID
    readNum = prv_get_number(uriString, uriLength, &head);
    if (readNum < 0 || readNum >= LWM2M_URI_NO_ID) goto error;
    uriP->objID = (uint16_t)readNum;
    if (head >= uriLength) return uriP;

    // Read object instance
    if (uriString[head] == '/')
    {
        // no instance
        head++;
    }
    else
    {
        readNum = prv_get_number(uriString, uriLength, &head);
        if (readNum < 0 || readNum >= LWM2M_URI_NO_INSTANCE) goto error;
        uriP->objInstance = (uint8_t)readNum;
    }
    if (head >= uriLength) return uriP;

    // Read ressource ID
    if (uriString[head] == '/')
    {
        // no ID
        head++;
    }
    else
    {
        readNum = prv_get_number(uriString, uriLength, &head);
        if (readNum < 0 || readNum >= LWM2M_URI_NO_ID) goto error;
        uriP->resID = (uint16_t)readNum;
    }
    if (head >= uriLength) return uriP;

    // Read ressource instance
    if (uriString[head] == '/')
    {
        // no instance
        head++;
    }
    else
    {
        if (LWM2M_URI_NO_ID == uriP->resID) goto error;
        readNum = prv_get_number(uriString, uriLength, &head);
        if (readNum < 0 || readNum >= LWM2M_URI_NO_INSTANCE) goto error;
        uriP->resInstance = (uint8_t)readNum;
    }
    if (head < uriLength) goto error;

    return uriP;

error:
    free(uriP);
    return NULL;
}


void handle_response(coap_packet_t * message)
{
}

coap_status_t handle_request(coap_packet_t * message,
                             coap_packet_t * response,
                             uint8_t *buffer,
                             uint16_t preferred_size,
                             int32_t *offset)
{
    lwm2m_uri_t * uriP;
    coap_status_t result = NOT_FOUND_4_04;

    uriP = decode_uri(message->uri_path, message->uri_path_len);

    if (NULL == uriP)
    {
        fprintf(stderr, "Invalid URI !\r\n");
        return NOT_FOUND_4_04;
    }

    fprintf(stdout, "object ID: %d\r\nobject instance: %d\r\nressource ID: %d\r\n ressource instance: %d\r\n",
                    uriP->objID, uriP->objInstance, uriP->resID, uriP->resInstance);


    if (uriP->objID == 1 &&  uriP->objInstance == 2)
    {
        coap_set_status_code(response, CONTENT_2_05);
        coap_set_payload(response, "Hi there !", strlen("Hi there !"));
        result = NO_ERROR;
    }

    free(uriP);

    return result;
}

/* This function is an adaptation of function coap_receive() from Erbium's er-coap-13-engine.c.
 * Erbium is Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
 * All rights reserved.
 */
static void handle_packet(uint8_t * buffer,
                          int length,
                          int sock,
                          struct sockaddr_storage addr,
                          socklen_t addrLen)
{
    coap_status_t coap_error_code = NO_ERROR;
    static coap_packet_t message[1];
    static coap_packet_t response[1];
    static coap_transaction_t * transaction = NULL;

    coap_error_code = coap_parse_message(message, buffer, (uint16_t)length);
    if (coap_error_code==NO_ERROR)
    {
        fprintf(stdout, "  Parsed: ver %u, type %u, tkl %u, code %u, mid %u\r\n", message->version, message->type, message->token_len, message->code, message->mid);
        fprintf(stdout, "  URL: %.*s\r\n", message->uri_path_len, message->uri_path);
        fprintf(stdout, "  Payload: %.*s\r\n\n", message->payload_len, message->payload);

        if (message->code >= COAP_GET && message->code <= COAP_DELETE)
        {
            /* Use transaction buffer for response to confirmable request. */
            if ( (transaction = coap_new_transaction(message->mid, sock, &addr, addrLen)) )
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

                coap_error_code = handle_request(message, response, transaction->packet+COAP_MAX_HEADER_SIZE, block_size, &new_offset);
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
            coap_send_message(sock, &addr, addrLen, buffer, bufferLen);
        }
    }
}

int main(int argc, char *argv[])
{
    int socket;
    fd_set readfds;
    struct timeval tv;
    int result;

    signal(SIGINT, handle_sigint);

    socket = get_socket();
    if (socket < 0) return -1;

    while (0 == g_quit)
    {
        FD_ZERO(&readfds);
        FD_SET(socket, &readfds);

        tv.tv_usec = 0;
        tv.tv_sec = 0;

        result = select(FD_SETSIZE, &readfds, 0, 0, &tv);

        if ( result < 0 )
        {
            if (errno != EINTR)
            {
              fprintf(stderr, "Error in select(): %d\r\n", errno);
            }
        }
        else if (result > 0)
        {
            if (FD_ISSET(socket, &readfds))
            {
                struct sockaddr_storage addr;
                socklen_t addrLen;
                uint8_t buffer[MAX_PACKET_SIZE];
                int numBytes;

                addrLen = sizeof(addr);
                numBytes = recvfrom(socket, buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)&addr, &addrLen);

                if (numBytes == -1)
                {
                    fprintf(stderr, "Error in recvfrom(): %d\r\n", errno);
                }
                else
                {
                    char s[INET6_ADDRSTRLEN];

                    fprintf(stdout, "%d bytes received from [%s]:%hu\r\n",
                            numBytes,
                            inet_ntop(addr.ss_family,
                                      &(((struct sockaddr_in6*)&addr)->sin6_addr),
                                      s,
                                      INET6_ADDRSTRLEN),
                            &((struct sockaddr_in6*)&addr)->sin6_port);

                    handle_packet(buffer, numBytes, socket, addr, addrLen);
                }
            }
        }
    }

    close(socket);

    return 0;
}
