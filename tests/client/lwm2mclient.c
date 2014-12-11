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
 *    Benjamin Cabé - Please refer to git log
 *    Fabien Fleutot - Please refer to git log
 *    Simon Bernard - Please refer to git log
 *    Julien Vermillard - Please refer to git log
 *    Axel Lorente - Please refer to git log
 *    Toby Jaffey - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
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


#include "liblwm2m.h"
#include "commandline.h"
#include "connection.h"

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

#define MAX_PACKET_SIZE 128 //ensure sync with: er_coap_13.h REST_MAX_CHUNK_SIZE!

static int g_quit = 0;

extern lwm2m_object_t * get_object_device();
extern lwm2m_object_t * get_object_firmware();
extern lwm2m_object_t * get_object_location();
extern lwm2m_object_t * get_test_object();
extern lwm2m_object_t * get_server_object();
extern lwm2m_object_t * get_security_object();

extern char * get_server_uri(lwm2m_object_t * objectP, uint16_t serverID);

typedef struct
{
    lwm2m_object_t * securityObjP;
    int sock;
    connection_t * connList;
} client_data_t;

static void prv_quit(char * buffer,
                     void * user_data)
{
    g_quit = 1;
}

void handle_sigint(int signum)
{
    g_quit = 2;
}

void print_usage(void)
{
    fprintf(stderr, "Usage: lwm2mclient [[[localPort] server] serverPort]\r\n");
    fprintf(stderr, "Launch a LWM2M client.\r\n\n");
}

static void * prv_connect_server(uint16_t serverID,
                                 void * userData)
{
    client_data_t * dataP;
    char * uri;
    char * host;
    char * portStr;
    int port;
    char * ptr;
    connection_t * newConnP = NULL;

    dataP = (client_data_t *)userData;

    uri = get_server_uri(dataP->securityObjP, serverID);
    if (uri == NULL) return NULL;

    // parse uri in the form "coaps://[host]:[port]"
    if (0==strncmp(uri, "coaps://", strlen("coaps://")))
      host = uri+strlen("coaps://");
    else 
    if (0==strncmp(uri, "coap://",  strlen("coap://")))
      host = uri+strlen("coap://");
    else goto exit;
    
    portStr = strchr(host, ':');
    if (portStr == NULL) goto exit;
    // split strings
    *portStr = 0;
    portStr++;
    port = strtol(portStr, &ptr, 10);
    if (*ptr != 0) goto exit;

    fprintf(stdout, "Trying to connect to LWM2M Server at %s:%d\r\n", host, port);
    newConnP = connection_create(dataP->connList, dataP->sock, host, port);
    if (newConnP == NULL)
    {
        fprintf(stderr, "Connection creation failed.\r\n");
    }
    else
    {
        dataP->connList = newConnP;
    }

exit:
    free(uri);
    return (void *)newConnP;
}

static uint8_t prv_buffer_send(void * sessionH,
                               uint8_t * buffer,
                               size_t length,
                               void * userdata)
{
    connection_t * connP = (connection_t*) sessionH;

    if (connP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

    if (-1 == connection_send(connP, buffer, length))
    {
        return COAP_500_INTERNAL_SERVER_ERROR;
    }
    return COAP_NO_ERROR;
}

static void prv_output_servers(char * buffer,
                               void * user_data)
{
    lwm2m_context_t * lwm2mH = (lwm2m_context_t *) user_data;
    lwm2m_server_t * targetP;

    targetP = lwm2mH->serverList;

    if (targetP == NULL)
    {
        fprintf(stdout, "No server.\r\n");
        return;
    }

    for (targetP = lwm2mH->serverList ; targetP != NULL ; targetP = targetP->next)
    {
        fprintf(stdout, "Server ID %d:\r\n", targetP->shortID);
        fprintf(stdout, "\tstatus: ");
        switch(targetP->status)
        {
        case STATE_UNKNOWN:
            fprintf(stdout, "UNKNOWN\r\n");
            break;
        case STATE_REG_PENDING:
            fprintf(stdout, "REGISTRATION PENDING\r\n");
            break;
        case STATE_REGISTERED:
            fprintf(stdout, "REGISTERED location: \"%s\"\r\n", targetP->location);
            break;
        case STATE_REG_UPDATE_PENDING:
            fprintf(stdout, "REGISTRATION UPDATE PENDING\r\n");
            break;
        case STATE_DEREG_PENDING:
            fprintf(stdout, "DEREGISTRATION PENDING\r\n");
            break;
        }
        fprintf(stdout, "\r\n");
    }
}

static void prv_change(char * buffer,
                       void * user_data)
{
    lwm2m_context_t * lwm2mH = (lwm2m_context_t *) user_data;
    lwm2m_uri_t uri;
    int result;
    size_t length;

    if (buffer[0] == 0) goto syntax_error;

    length = 0;
    // remove white space
    while (length < strlen(buffer) && isspace(buffer[length]))
    {
        length++;
    }
    // find end of URI
    while (length < strlen(buffer) && !isspace(buffer[length]))
    {
        length++;
    }

    result = lwm2m_stringToUri(buffer, length, &uri);
    if (result == 0) goto syntax_error;

    buffer += length;
    while (buffer[0] != 0 && isspace(buffer[0])) buffer++;
    if (buffer[0] == 0)
    {
        lwm2m_resource_value_changed(lwm2mH, &uri);
    }
    else
    {
        int i;

        i = 0;
        while (i < lwm2mH->numObject)
        {
            if (uri.objectId == lwm2mH->objectList[i]->objID)
            {
                if (lwm2mH->objectList[i]->writeFunc != NULL)
                {
                    lwm2m_tlv_t * tlvP;

                    tlvP = lwm2m_tlv_new(1);
                    if (tlvP == NULL)
                    {
                        fprintf(stdout, "Internal allocation failure !\n");
                        return;
                    }
                    tlvP->flags = LWM2M_TLV_FLAG_STATIC_DATA | LWM2M_TLV_FLAG_TEXT_FORMAT;
                    tlvP->id = uri.resourceId;
                    tlvP->length = strlen(buffer);
                    tlvP->value = buffer;

                    if (COAP_204_CHANGED != lwm2mH->objectList[i]->writeFunc(uri.instanceId,
                                                                             1, tlvP,
                                                                             lwm2mH->objectList[i]))
                    {
                        fprintf(stdout, "Failed to change value !\n");
                    }
                    else
                    {
                        lwm2m_resource_value_changed(lwm2mH, &uri);
                    }
                    lwm2m_tlv_free(1, tlvP);
                    return;
                }
                return;
            }
            i++;
        }

        fprintf(stdout, "Object not found !\n");
    }
    return;

syntax_error:
    fprintf(stdout, "Syntax error !\n");
}

static void prv_update(char * buffer,
                       void * user_data)
{
    lwm2m_context_t * lwm2mH = (lwm2m_context_t *) user_data;
    if (buffer[0] == 0) goto syntax_error;

    uint16_t serverId = (uint16_t) atoi(buffer);
    int res = lwm2m_update_registration(lwm2mH, serverId);
    if (res != 0) {
        fprintf(stdout, "Registration update error: %d\n",res);
    }
    return;

syntax_error:
    fprintf(stdout, "Syntax error !\n");
}


#define OBJ_COUNT 6

int main(int argc, char *argv[])
{
    client_data_t data;
    int result;
    lwm2m_context_t * lwm2mH = NULL;
    lwm2m_object_t * objArray[OBJ_COUNT];
    int i;
    char localPort[7], server[30], serverPort[7];
    /*
     * The function start by setting up the command line interface (which may or not be useful depending on your project)
     *
     * This is an array of commands describes as { name, description, long description, callback, userdata }.
     * The firsts tree are easy to understand, the callback is the function that will be called when this command is typed
     * and in the last one will be stored the lwm2m context (allowing access to the server settings and the objects).
     */
    command_desc_t commands[] =
    {
            {"list", "List known servers.", NULL, prv_output_servers, NULL},
            {"change", "Change the value of resource.", " change URI [DATA]\r\n"
                                                        "   URI: uri of the resource such as /3/0, /3/0/2\r\n"
                                                        "   DATA: (optional) new value\r\n", prv_change, NULL},
            {"update", "Trigger a registration update", " update SERVER\r\n"
                                                        "   SERVER: short server id such as 123\r\n", prv_update, NULL},
            {"quit", "Quit the client gracefully.", NULL, prv_quit, NULL},
            {"^C", "Quit the client abruptly (without sending a de-register message).", NULL, NULL, NULL},

            COMMAND_END_LIST
    };

    memset(&data, 0, sizeof(client_data_t));

    strcpy (localPort, "56830");
    strcpy (server,"localhost");
    strcpy (serverPort, LWM2M_STANDARD_PORT_STR);	//see connection.h

    if (argc >= 2) strcpy (localPort,  argv[1]);
    if (argc >= 3) strcpy (server,     argv[2]);
    if (argc >= 4) strcpy (serverPort, argv[3]);

    /*
     *This call an internal function that create an IPV6 socket on the port 5683.
     */
    fprintf(stdout, "Trying to bind LWM2M Client to port %s\r\n", localPort);
    data.sock = create_socket(localPort);
    if (data.sock < 0)
    {
        fprintf(stderr, "Failed to open socket: %d\r\n", errno);
        return -1;
    }

    /*
     * Now the main function fill an array with each object, this list will be later passed to liblwm2m.
     * Those functions are located in their respective object file.
     */
    objArray[0] = get_object_device();
    if (NULL == objArray[0])
    {
        fprintf(stderr, "Failed to create Device object\r\n");
        return -1;
    }

    objArray[1] = get_object_firmware();
    if (NULL == objArray[1])
    {
        fprintf(stderr, "Failed to create Firmware object\r\n");
        return -1;
    }

    objArray[2] = get_test_object();
    if (NULL == objArray[2])
    {
        fprintf(stderr, "Failed to create test object\r\n");
        return -1;
    }

    int serverId = 123;
    objArray[3] = get_server_object(serverId, "U", 300, false);
    if (NULL == objArray[3])
    {
        fprintf(stderr, "Failed to create server object\r\n");
        return -1;
    }

    char serverUri[50];
    sprintf (serverUri, "coap://%s:%s", server, serverPort);
    objArray[4] = get_security_object(serverId, serverUri, false);
    if (NULL == objArray[4])
    {
        fprintf(stderr, "Failed to create security object\r\n");
        return -1;
    }
    data.securityObjP = objArray[4];

    objArray[5] = get_object_location();
    if (NULL == objArray[5])
    {
        fprintf(stderr, "Failed to create location object\r\n");
        return -1;
    }

    /*
     * The liblwm2m library is now initialized with the functions that will be in
     * charge of communication
     */
    lwm2mH = lwm2m_init(prv_connect_server, prv_buffer_send, &data);
    if (NULL == lwm2mH)
    {
        fprintf(stderr, "lwm2m_init() failed\r\n");
        return -1;
    }

    /*
     * We configure the liblwm2m library with the name of the client - which shall be unique for each client -
     * the number of objects we will be passing through and the objects array
     */
    result = lwm2m_configure(lwm2mH, "testlwm2mclient", BINDING_U, NULL, OBJ_COUNT, objArray);
    if (result != 0)
    {
        fprintf(stderr, "lwm2m_set_objects() failed: 0x%X\r\n", result);
        return -1;
    }

    signal(SIGINT, handle_sigint);

    /*
     * This function start your client to the LWM2M servers
     */
    result = lwm2m_start(lwm2mH);
    if (result != 0)
    {
        fprintf(stderr, "lwm2m_register() failed: 0x%X\r\n", result);
        return -1;
    }

    /*
     * As you now have your lwm2m context complete you can pass it as an argument to all the command line functions
     * precedently viewed (first point)
     */
    for (i = 0 ; commands[i].name != NULL ; i++)
    {
        commands[i].userData = (void *)lwm2mH;
    }
    fprintf(stdout, "> "); fflush(stdout);

    /*
     * We now enter in a while loop that will handle the communications from the server
     */
    while (0 == g_quit)
    {
        struct timeval tv;
        fd_set readfds;

        FD_ZERO(&readfds);
        FD_SET(data.sock, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        tv.tv_sec = 60;
        tv.tv_usec = 0;

        /*
         * This function does two things:
         *  - first it does the work needed by liblwm2m (eg. (re)sending some packets).
         *  - Secondly it adjust the timeout value (default 60s) depending on the state of the transaction
         *    (eg. retransmission) and the time between the next operation
         */
        result = lwm2m_step(lwm2mH, &tv);
        if (result != 0)
        {
            fprintf(stderr, "lwm2m_step() failed: 0x%X\r\n", result);
            return -1;
        }

        /*
         * This part will set up an interruption until an event happen on SDTIN or the socket until "tv" timed out (set
         * with the precedent function)
         */
        result = select(FD_SETSIZE, &readfds, NULL, NULL, &tv);

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

            /*
             * If an event happen on the socket
             */
            if (FD_ISSET(data.sock, &readfds))
            {
                struct sockaddr_storage addr;
                socklen_t addrLen;

                addrLen = sizeof(addr);

                /*
                 * We retrieve the data received
                 */
                numBytes = recvfrom(data.sock, buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)&addr, &addrLen);

                if (numBytes == -1)
                {
                    fprintf(stderr, "Error in recvfrom(): %d\r\n", errno);
                }
                else
                {
                    char s[INET6_ADDRSTRLEN];
                    connection_t * connP;

                    fprintf(stderr, "%d bytes received from [%s]:%hu\r\n",
                            numBytes,
                            inet_ntop(addr.ss_family,
                                      &(((struct sockaddr_in6*)&addr)->sin6_addr),
                                      s,
                                      INET6_ADDRSTRLEN),
                            ntohs(((struct sockaddr_in6*)&addr)->sin6_port));

                    /*
                     * Display it in the STDERR
                     */
                    output_buffer(stderr, buffer, numBytes);

                    connP = connection_find(data.connList, &addr, addrLen);
                    if (connP != NULL)
                    {
                        /*
                         * Let liblwm2m respond to the query depending on the context
                         */
                        lwm2m_handle_packet(lwm2mH, buffer, numBytes, connP);
                    }
                }
            }

            /*
             * If the event happened on the SDTIN
             */
            else if (FD_ISSET(STDIN_FILENO, &readfds))
            {
                numBytes = read(STDIN_FILENO, buffer, MAX_PACKET_SIZE);

                if (numBytes > 1)
                {
                    buffer[numBytes - 1] = 0;

                    /*
                     * We call the corresponding callback of the typed command passing it the buffer for further arguments
                     */
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

    /*
     * Finally when the loop is left smoothly - asked by user in the command line interface - we unregister our client from it
     */
    if (g_quit == 1)
    {
        lwm2m_close(lwm2mH);
    }
    close(data.sock);
    connection_free(data.connList);

    return 0;
}
