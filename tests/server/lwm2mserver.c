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

#include "core/liblwm2m.h"
#include "externals/er-coap-13/er-coap-13.h"

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
#include <errno.h>
#include <signal.h>

#include "commandline.h"

#define MAX_PACKET_SIZE 128
#define SERVER_PORT "5684"

static int g_quit = 0;

static uint8_t prv_buffer_send(int sock,
                          uint8_t * buffer,
                          size_t length,
                          uint8_t * addr,
                          size_t addrLen)
{
    size_t nbSent;
    size_t offset;

    offset = 0;
    while (offset != length)
    {
        nbSent = sendto(sock, buffer + offset, length - offset, 0, (struct sockaddr *)addr, addrLen);
        if (nbSent == -1) return COAP_500_INTERNAL_SERVER_ERROR;
        offset += nbSent;
    }
    return COAP_NO_ERROR;
}

static void prv_output_buffer(FILE * fd,
                              uint8_t * buffer,
                              int length)
{
    int i;
    uint8_t array[16];

    i = 0;
    while (i < length)
    {
        int j;
        fprintf(fd, "  ");

        memcpy(array, buffer+i, 16);

        for (j = 0 ; j < 16 && i+j < length; j++)
        {
            fprintf(fd, "%02X ", array[j]);
        }
        while (j < 16)
        {
            fprintf(fd, "   ");
            j++;
        }
        fprintf(fd, "  ");
        for (j = 0 ; j < 16 && i+j < length; j++)
        {
            if (isprint(array[j]))
                fprintf(fd, "%c ", array[j]);
            else
                fprintf(fd, ". ");
        }
        fprintf(fd, "\n");

        i += 16;
    }
}

static void prv_dump_client(lwm2m_client_t * targetP)
{
    lwm2m_client_object_t * objectP;

    fprintf(stdout, "Client #%d:\r\n", targetP->internalID);
    fprintf(stdout, "\tname: \"%s\"\r\n", targetP->name);
    fprintf(stdout, "\tobjects: ");
    for (objectP = targetP->objectList; objectP != NULL ; objectP = objectP->next)
    {
        if (objectP->instanceList == NULL)
        {
            fprintf(stdout, "%d, ", objectP->id);
        }
        else
        {
            lwm2m_list_t * instanceP;

            for (instanceP = objectP->instanceList; instanceP != NULL ; instanceP = instanceP->next)
            {
                fprintf(stdout, "%d/%d, ", objectP->id, instanceP->id);
            }
        }
    }
    fprintf(stdout, "\r\n");
}

static void prv_output_clients(char * buffer,
                               void * user_data)
{
    lwm2m_context_t * lwm2mH = (lwm2m_context_t *) user_data;
    lwm2m_client_t * targetP;

    targetP = lwm2mH->clientList;

    if (targetP == NULL)
    {
        fprintf(stdout, "No client.\r\n");
        return;
    }

    for (targetP = lwm2mH->clientList ; targetP != NULL ; targetP = targetP->next)
    {
        prv_dump_client(targetP);
    }
}

static int prv_read_id(char * buffer,
                       uint16_t * idP)
{
    int nb;
    int value;

    nb = sscanf(buffer, "%d", &value);
    if (nb == 1)
    {
        if (value < 0 || value > LWM2M_MAX_ID)
        {
            nb = 0;
        }
        else
        {
            *idP = value;
        }
    }

    return nb;
}

static void prv_result_callback(uint16_t clientID,
                                lwm2m_uri_t * uriP,
                                int status,
                                uint8_t * data,
                                int dataLength,
                                void * userData)
{
    fprintf(stdout, "\r\nClient #%d %d", clientID, uriP->objectId);
    if (LWM2M_URI_IS_SET_INSTANCE(uriP))
        fprintf(stdout, "/%d", uriP->instanceId);
    else if (LWM2M_URI_IS_SET_RESOURCE(uriP))
        fprintf(stdout, "/");
    if (LWM2M_URI_IS_SET_RESOURCE(uriP))
            fprintf(stdout, "/%d", uriP->resourceId);
    fprintf(stdout, " returned status %d.%2d\r\n", (status&0xE0)>>5, status&0x1F);

    if (data != NULL)
    {
        fprintf(stdout, "%d bytes received:\r\n", dataLength);
        prv_output_buffer(stdout, data, dataLength);
    }

    fprintf(stdout, "\r\n> ");
    fflush(stdout);
}

static void prv_read_client(char * buffer,
                            void * user_data)
{
    lwm2m_context_t * lwm2mH = (lwm2m_context_t *) user_data;
    uint16_t clientId;
    lwm2m_uri_t uri;
    int result;

    result = prv_read_id(buffer, &clientId);
    if (result != 1) goto syntax_error;

    buffer = get_next_arg(buffer);
    if (buffer[0] == 0) goto syntax_error;

    result = lwm2m_stringToUri(buffer, strlen(buffer), &uri);
    if (result == 0) goto syntax_error;

    result = lwm2m_dm_read(lwm2mH, clientId, &uri, prv_result_callback, NULL);

    if (result == 0)
    {
        fprintf(stdout, "OK");
    }
    else
    {
        fprintf(stdout, "Error %d.%2d", (result&0xE0)>>5, result&0x1F);
    }
    return;

syntax_error:
    fprintf(stdout, "Syntax error !");
}

static void prv_write_client(char * buffer,
                             void * user_data)
{
    lwm2m_context_t * lwm2mH = (lwm2m_context_t *) user_data;
    uint16_t clientId;
    lwm2m_uri_t uri;
    char * uriString;
    int i;
    int result;

    result = prv_read_id(buffer, &clientId);
    if (result != 1) goto syntax_error;

    buffer = get_next_arg(buffer);
    if (buffer[0] == 0) goto syntax_error;
    uriString = buffer;

    buffer = get_next_arg(buffer);
    if (buffer[0] == 0) goto syntax_error;

    i = 0;
    while (uriString + i < buffer && !isspace(uriString[i]))
    {
        i++;
    }
    result = lwm2m_stringToUri(uriString, i, &uri);
    if (result == 0) goto syntax_error;

    result = lwm2m_dm_write(lwm2mH, clientId, &uri, buffer, strlen(buffer), prv_result_callback, NULL);

    if (result == 0)
    {
        fprintf(stdout, "OK");
    }
    else
    {
        fprintf(stdout, "Error %d.%2d", (result&0xE0)>>5, result&0x1F);
    }
    return;

syntax_error:
    fprintf(stdout, "Syntax error !");
}

static void prv_exec_client(char * buffer,
                            void * user_data)
{
    lwm2m_context_t * lwm2mH = (lwm2m_context_t *) user_data;
    uint16_t clientId;
    lwm2m_uri_t uri;
    char * uriString;
    int i;
    int result;

    result = prv_read_id(buffer, &clientId);
    if (result != 1) goto syntax_error;

    buffer = get_next_arg(buffer);
    if (buffer[0] == 0) goto syntax_error;
    uriString = buffer;

    buffer = get_next_arg(buffer);

    i = 0;
    while (uriString + i < buffer && !isspace(uriString[i]))
    {
        i++;
    }
    result = lwm2m_stringToUri(uriString, i, &uri);
    if (result == 0) goto syntax_error;

    if (buffer[0] == 0)
    {
        result = lwm2m_dm_execute(lwm2mH, clientId, &uri, NULL, 0, prv_result_callback, NULL);
    }
    else
    {
        result = lwm2m_dm_execute(lwm2mH, clientId, &uri, buffer, strlen(buffer), prv_result_callback, NULL);
    }

    if (result == 0)
    {
        fprintf(stdout, "OK");
    }
    else
    {
        fprintf(stdout, "Error %d.%2d", (result&0xE0)>>5, result&0x1F);
    }
    return;

syntax_error:
    fprintf(stdout, "Syntax error !");
}

static void prv_delete_client(char * buffer,
                              void * user_data)
{
    lwm2m_context_t * lwm2mH = (lwm2m_context_t *) user_data;
    uint16_t clientId;
    lwm2m_uri_t uri;
    int result;

    result = prv_read_id(buffer, &clientId);
    if (result != 1) goto syntax_error;

    buffer = get_next_arg(buffer);
    if (buffer[0] == 0) goto syntax_error;

    result = lwm2m_stringToUri(buffer, strlen(buffer), &uri);
    if (result == 0) goto syntax_error;

    result = lwm2m_dm_delete(lwm2mH, clientId, &uri, prv_result_callback, NULL);

    if (result == 0)
    {
        fprintf(stdout, "OK");
    }
    else
    {
        fprintf(stdout, "Error %d.%2d", (result&0xE0)>>5, result&0x1F);
    }
    return;

syntax_error:
    fprintf(stdout, "Syntax error !");
}

static void prv_observe_client(char * buffer,
                               void * user_data)
{
    lwm2m_context_t * lwm2mH = (lwm2m_context_t *) user_data;
    uint16_t clientId;
    lwm2m_uri_t uri;
    int result;

    result = prv_read_id(buffer, &clientId);
    if (result != 1) goto syntax_error;

    buffer = get_next_arg(buffer);
    if (buffer[0] == 0) goto syntax_error;

    result = lwm2m_stringToUri(buffer, strlen(buffer), &uri);
    if (result == 0) goto syntax_error;

    result = lwm2m_observe(lwm2mH, clientId, &uri, prv_result_callback, NULL);

    if (result == 0)
    {
        fprintf(stdout, "OK");
    }
    else
    {
        fprintf(stdout, "Error %d.%2d", (result&0xE0)>>5, result&0x1F);
    }
    return;

syntax_error:
    fprintf(stdout, "Syntax error !");
}

static void prv_cancel_client(char * buffer,
                              void * user_data)
{
    lwm2m_context_t * lwm2mH = (lwm2m_context_t *) user_data;
    uint16_t clientId;
    lwm2m_uri_t uri;
    int result;

    result = prv_read_id(buffer, &clientId);
    if (result != 1) goto syntax_error;

    buffer = get_next_arg(buffer);
    if (buffer[0] == 0) goto syntax_error;

    result = lwm2m_stringToUri(buffer, strlen(buffer), &uri);
    if (result == 0) goto syntax_error;

    result = lwm2m_observe_cancel(lwm2mH, clientId, &uri, prv_result_callback, NULL);

    if (result == 0)
    {
        fprintf(stdout, "OK");
    }
    else
    {
        fprintf(stdout, "Error %d.%2d", (result&0xE0)>>5, result&0x1F);
    }
    return;

syntax_error:
    fprintf(stdout, "Syntax error !");
}

static void prv_monitor_callback(uint16_t clientID,
                                 lwm2m_uri_t * uriP,
                                 int status,
                                 uint8_t * data,
                                 int dataLength,
                                 void * userData)
{
    lwm2m_context_t * lwm2mH = (lwm2m_context_t *) userData;
    lwm2m_client_t * targetP;
    lwm2m_client_object_t * objectP;

    switch (status)
	{
	case CREATED_2_01:
	    fprintf(stdout, "\r\nNew client #%d registered.\r\n", clientID);

	    targetP = (lwm2m_client_t *)lwm2m_list_find((lwm2m_list_t *)lwm2mH->clientList, clientID);

        prv_dump_client(targetP);
	    break;

	case DELETED_2_02:
	    fprintf(stdout, "\r\nClient #%d unregistered.\r\n", clientID);
	    break;

	default:
		fprintf(stdout, "\r\nMonitor callback called with an unknown status: %d.\r\n", status);
		break;
	}

    fprintf(stdout, "\r\n> ");
    fflush(stdout);
}


static void prv_quit(char * buffer,
                     void * user_data)
{
    g_quit = 1;
}

void handle_sigint(int signum)
{
    prv_quit(NULL, NULL);
}

void print_usage(void)
{
    fprintf(stderr, "Usage: lwm2mserver\r\n");
    fprintf(stderr, "Launch a LWM2M server on localhost port "SERVER_PORT".\r\n\n");
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

    getaddrinfo(NULL, SERVER_PORT, &hints, &res);

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

int main(int argc, char *argv[])
{
    int socket;
    fd_set readfds;
    struct timeval tv;
    int result;
    lwm2m_context_t * lwm2mH = NULL;
    int i;
    command_desc_t commands[] =
    {
            {"list", "List registered clients.", NULL, prv_output_clients, NULL},
            {"read", "Read from a client.", " read CLIENT# URI\r\n"
                                            "   CLIENT#: client number as returned by command 'list'\r\n"
                                            "   URI: uri to read such as /3, /3//2, /3/0/2, /1024/11, /1024//1\r\n"
                                            "Result will be displayed asynchronously.", prv_read_client, NULL},
            {"write", "Write to a client.", " write CLIENT# URI DATA\r\n"
                                            "   CLIENT#: client number as returned by command 'list'\r\n"
                                            "   URI: uri to write to such as /3, /3//2, /3/0/2, /1024/11, /1024//1\r\n"
                                            "   DATA: data to write\r\n"
                                            "Result will be displayed asynchronously.", prv_write_client, NULL},
            {"exec", "Execute a client resource.", " exec CLIENT# URI\r\n"
                                            "   CLIENT#: client number as returned by command 'list'\r\n"
                                            "   URI: uri of the resource to execute such as /3/0/2\r\n"
                                            "Result will be displayed asynchronously.", prv_exec_client, NULL},
            {"del", "Delete a client Object instance.", " del CLIENT# URI\r\n"
                                            "   CLIENT#: client number as returned by command 'list'\r\n"
                                            "   URI: uri of the instance to delete such as /1024/11\r\n"
                                            "Result will be displayed asynchronously.", prv_delete_client, NULL},
            {"observe", "Observe from a client.", " observe CLIENT# URI\r\n"
                                            "   CLIENT#: client number as returned by command 'list'\r\n"
                                            "   URI: uri to observe such as /3, /3/0/2, /1024/11\r\n"
                                            "Result will be displayed asynchronously.", prv_observe_client, NULL},
            {"cancel", "Cancel an observe.", " cancel CLIENT# URI\r\n"
                                            "   CLIENT#: client number as returned by command 'list'\r\n"
                                            "   URI: uri on which to cancel an observe such as /3, /3/0/2, /1024/11\r\n"
                                            "Result will be displayed asynchronously.", prv_cancel_client, NULL},

            {"quit", "Quit the server.", NULL, prv_quit, NULL},

            COMMAND_END_LIST
    };

    socket = get_socket();
    if (socket < 0)
    {
        fprintf(stderr, "Error opening socket: %d\r\n", errno);
        return -1;
    }

    lwm2mH = lwm2m_init(socket, "testlwm2mserver", 0, NULL, prv_buffer_send);
    if (NULL == lwm2mH)
    {
        fprintf(stderr, "lwm2m_init() failed\r\n");
        return -1;
    }

    signal(SIGINT, handle_sigint);

    for (i = 0 ; commands[i].name != NULL ; i++)
    {
        commands[i].userData = (void *)lwm2mH;
    }
    fprintf(stdout, "> "); fflush(stdout);

    lwm2m_set_monitoring_callback(lwm2mH, prv_monitor_callback, lwm2mH);

    while (0 == g_quit)
    {
        FD_ZERO(&readfds);
        FD_SET(socket, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        tv.tv_sec = 60;
        tv.tv_usec = 0;

        result = lwm2m_step(lwm2mH, &tv);
        if (result != 0)
        {
            fprintf(stderr, "lwm2m_step() failed: 0x%X\r\n", result);
            return -1;
        }

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
            uint8_t buffer[MAX_PACKET_SIZE];
            int numBytes;

            if (FD_ISSET(socket, &readfds))
            {
                struct sockaddr_storage addr;
                socklen_t addrLen;

                addrLen = sizeof(addr);
                numBytes = recvfrom(socket, buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)&addr, &addrLen);

                if (numBytes == -1)
                {
                    fprintf(stderr, "Error in recvfrom(): %d\r\n", errno);
                }
                else
                {
                    char s[INET6_ADDRSTRLEN];
                    coap_status_t coap_error_code = NO_ERROR;
                    static coap_packet_t message[1];


                    fprintf(stderr, "%d bytes received from [%s]:%hu\r\n",
                            numBytes,
                            inet_ntop(addr.ss_family,
                                      &(((struct sockaddr_in6*)&addr)->sin6_addr),
                                      s,
                                      INET6_ADDRSTRLEN),
                            ntohs(((struct sockaddr_in6*)&addr)->sin6_port));
                    prv_output_buffer(stderr, buffer, numBytes);

                    lwm2m_handle_packet(lwm2mH, buffer, numBytes, (uint8_t *)&addr, addrLen);
                }
            }
            else if (FD_ISSET(STDIN_FILENO, &readfds))
            {
                numBytes = read(STDIN_FILENO, buffer, MAX_PACKET_SIZE);

                if (numBytes > 1)
                {
                    buffer[numBytes - 1] = 0;
                    handle_command(commands, buffer);
                }
                if (g_quit == 0)
                {
                    fprintf(stdout, "\r\n> ");
                    fflush(stdout);
                }
                else
                {
                    fprintf(stdout, "\r\n");
                }
            }
        }
    }

    lwm2m_close(lwm2mH);
    close(socket);

    return 0;
}
