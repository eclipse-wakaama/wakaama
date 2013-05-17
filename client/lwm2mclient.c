
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

void handle_packet(uint8_t * buffer, int length)
{
    coap_status_t coap_error_code = NO_ERROR;
    static coap_packet_t message[1];

    coap_error_code = coap_parse_message(message, buffer, (uint16_t)length);
    if (coap_error_code==NO_ERROR)
    {
        fprintf(stdout, "  Parsed: ver %u, type %u, tkl %u, code %u, mid %u\n", message->version, message->type, message->token_len, message->code, message->mid);
        fprintf(stdout, "  URL: %.*s\n", message->uri_path_len, message->uri_path);
        fprintf(stdout, "  Payload: %.*s\n\n", message->payload_len, message->payload);
    }
    else
    {
        fprintf(stderr, "Message parsing failed %d\r\n", coap_error_code);
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

                    handle_packet(buffer, numBytes);
                }
            }
        }
    }

    close(socket);

    return 0;
}
