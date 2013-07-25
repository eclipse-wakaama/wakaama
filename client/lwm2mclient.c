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


#include "core/liblwm2m.h"

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

#define MAX_PACKET_SIZE 128

static int g_quit = 0;

extern lwm2m_object_t * get_object_device();
extern lwm2m_object_t * get_test_object();

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

int main(int argc, char *argv[])
{
    int socket;
    fd_set readfds;
    struct timeval tv;
    int result;
    lwm2m_context_t * lwm2mH = NULL;
    lwm2m_object_t * objArray[2];
    lwm2m_security_t security;

    socket = get_socket();
    if (socket < 0)
    {
        fprintf(stderr, "Failed to open socket: %d\r\n", errno);
        return -1;
    }

    objArray[0] = get_object_device();
    if (NULL == objArray[0])
    {
        fprintf(stderr, "Failed to create Device object\r\n");
        return -1;
    }

    objArray[1] = get_test_object();
    if (NULL == objArray[1])
    {
        fprintf(stderr, "Failed to create test object\r\n");
        return -1;
    }

    lwm2mH = lwm2m_init(socket, "testlwm2mclient", 2, objArray);
    if (NULL == lwm2mH)
    {
        fprintf(stderr, "lwm2m_init() failed\r\n");
        return -1;
    }

    signal(SIGINT, handle_sigint);

    memset(&security, 0, sizeof(lwm2m_security_t));
    result = lwm2m_add_server(lwm2mH, 123, "::1", 5684, &security);
    if (result != 0)
    {
        fprintf(stderr, "lwm2m_add_server() failed: 0x%X\r\n", result);
        return -1;
    }
    result = lwm2m_register(lwm2mH);
    if (result != 0)
    {
        fprintf(stderr, "lwm2m_register() failed: 0x%X\r\n", result);
        return -1;
    }

    while (0 == g_quit)
    {
        FD_ZERO(&readfds);
        FD_SET(socket, &readfds);

        tv.tv_usec = 0;
        tv.tv_sec = 1; /* 1 second select wait */

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

                    lwm2m_handle_packet(lwm2mH, buffer, numBytes, socket, (struct sockaddr *)&addr, addrLen);
                }
            }
        }
    }

    lwm2m_close(lwm2mH);
    close(socket);

    return 0;
}
