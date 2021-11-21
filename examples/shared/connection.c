/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v20.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    Pascal Rieux - Please refer to git log
 *
 *******************************************************************************/

#include "connection.h"
#include "commandline.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

static int connection_send(uint8_t const *buffer, size_t length, void *userData) {
    int nbSent;
    size_t offset;
    connection_t *connP = (connection_t *)userData;
    offset = 0;
    while (offset != length) {
        nbSent =
            sendto(connP->sock, buffer + offset, length - offset, 0, (struct sockaddr *)&(connP->addr), connP->addrLen);
        if (nbSent == -1)
            return -1;
        offset += nbSent;
    }
    return 0;
}

static int connection_recv(lwm2m_context_t *ctx, uint8_t *buffer, size_t length, void *userData) {
    lwm2m_handle_packet(ctx, buffer, length, userData);
    return 0;
}

static connection_t *connection_find(connection_t *connList, struct sockaddr_storage const *addr, size_t addrLen) {
    connection_t *connP;

    connP = connList;
    while (connP != NULL) {
        if ((connP->addrLen == addrLen) && (memcmp(&(connP->addr), addr, addrLen) == 0)) {
            return connP;
        }
        connP = connP->next;
    }

    return connP;
}

static void connection_free(connection_t *conn) {
    if (conn->deinitFunc) {
        conn->deinitFunc(conn);
    }
    lwm2m_free(conn);
}

static void connectionlayer_free_connlist(connection_t *connList) {
    while (connList != NULL) {
        connection_t *nextP;

        nextP = connList->next;
        connection_free(connList);

        connList = nextP;
    }
}

lwm2m_connection_layer_t *connectionlayer_create(lwm2m_context_t *context) {
    lwm2m_connection_layer_t *layerCtx = (lwm2m_connection_layer_t *)lwm2m_malloc(sizeof(lwm2m_connection_layer_t));
    if (layerCtx == NULL) {
        return NULL;
    }
    layerCtx->ctx = context;
    layerCtx->connList = NULL;
    return layerCtx;
}

int connectionlayer_handle_packet(lwm2m_connection_layer_t *connLayerP, struct sockaddr_storage *addr, size_t addrLen,
                                  uint8_t *buffer, size_t length) {
    connection_t *connP = connection_find(connLayerP->connList, addr, addrLen);
    if (connP == NULL) {
        return -1;
    }
    return connP->recvFunc(connLayerP->ctx, buffer, length, connP);
}

connection_t *connectionlayer_find_connection(lwm2m_connection_layer_t *connLayerP, struct sockaddr_storage const *addr,
                                              size_t addrLen) {
    return connection_find(connLayerP->connList, addr, addrLen);
}

void connectionlayer_free(lwm2m_connection_layer_t *connLayerP) {
    if (connLayerP == NULL) {
        return;
    }
    connectionlayer_free_connlist(connLayerP->connList);
    lwm2m_free(connLayerP);
}

void connectionlayer_free_connection(lwm2m_connection_layer_t *connLayerP, connection_t *conn) {
    connection_t *connItor = connLayerP->connList;
    if (connLayerP->connList == conn) {
        connLayerP->connList = conn->next;
        connection_free(conn);
    } else {
        while (connItor != NULL && connItor->next != conn) {
            connItor = connItor->next;
        }
        if (connItor != NULL) {
            connItor->next = conn->next;
            connection_free(conn);
        }
    }
}

int create_socket(const char *portStr, int addressFamily) {
    int s = -1;
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = addressFamily;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if (0 != getaddrinfo(NULL, portStr, &hints, &res)) {
        return -1;
    }

    for (p = res; p != NULL && s == -1; p = p->ai_next) {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s >= 0) {
            if (-1 == bind(s, p->ai_addr, p->ai_addrlen)) {
                close(s);
                s = -1;
            }
        }
    }

    freeaddrinfo(res);

    return s;
}

void connectionlayer_add_connection(lwm2m_connection_layer_t *connLayer, connection_t *conn) {
    conn->next = connLayer->connList;
    connLayer->connList = conn;
}

static void connection_new_incoming_internal(connection_t *conn, int sock, struct sockaddr_storage *addr,
                                             size_t addrLen) {
    conn->sock = sock;
    memcpy(&(conn->addr), addr, addrLen);
    conn->addrLen = addrLen;
    conn->sendFunc = connection_send;
    conn->recvFunc = connection_recv;
}

connection_t *connection_new_incoming(lwm2m_connection_layer_t *connLayerP, int sock, struct sockaddr_storage *addr,
                                      size_t addrLen) {
    connection_t *connP = (connection_t *)lwm2m_malloc(sizeof(connection_t));
    if (connP != NULL) {
        connection_new_incoming_internal(connP, sock, addr, addrLen);
        connectionlayer_add_connection(connLayerP, connP);
    }

    return connP;
}

connection_t *connection_create(lwm2m_connection_layer_t *connLayerP, int sock, char *host, char *port,
                                int addressFamily) {
    connection_t *conn = (connection_t *)lwm2m_malloc(sizeof(connection_t));
    if (conn == NULL) {
        return NULL;
    }
    if (connection_create_inplace(conn, sock, host, port, addressFamily) > 0) {
        connectionlayer_add_connection(connLayerP, conn);
    } else {
        lwm2m_free(conn);
        conn = NULL;
    }
    return conn;
}

int connection_create_inplace(connection_t *conn, int sock, char *host, char *port, int addressFamily) {
    struct addrinfo hints;
    struct addrinfo *servinfo = NULL;
    struct addrinfo *p;
    int s;
    struct sockaddr *sa;
    socklen_t sl;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = addressFamily;
    hints.ai_socktype = SOCK_DGRAM;

    if (0 != getaddrinfo(host, port, &hints, &servinfo) || servinfo == NULL)
        return -1;

    // we test the various addresses
    s = -1;
    for (p = servinfo; p != NULL && s == -1; p = p->ai_next) {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s >= 0) {
            sa = p->ai_addr;
            sl = p->ai_addrlen;
            if (-1 == connect(s, p->ai_addr, p->ai_addrlen)) {
                close(s);
                s = -1;
            }
        }
    }
    if (s >= 0) {
        connection_new_incoming_internal(conn, sock, (struct sockaddr_storage *)sa, sl);
        close(s);
        return 1;
    }
    if (NULL != servinfo) {
        freeaddrinfo(servinfo);
    }

    return 0;
}

uint8_t lwm2m_buffer_send(void *sessionH, uint8_t *buffer, size_t length, void *userdata) {
    connection_t *connP = (connection_t *)sessionH;

    (void)userdata; /* unused */

    if (connP == NULL) {
        fprintf(stderr, "#> failed sending %lu bytes, missing connection\r\n", length);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

#ifdef LWM2M_WITH_LOGS
    char s[INET6_ADDRSTRLEN];
    in_port_t port;

    s[0] = 0;

    if (AF_INET == connP->addr.sin6_family) {
        struct sockaddr_in *saddr = (struct sockaddr_in *)&connP->addr;
        inet_ntop(saddr->sin_family, &saddr->sin_addr, s, INET6_ADDRSTRLEN);
        port = saddr->sin_port;
    } else if (AF_INET6 == connP->addr.sin6_family) {
        struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)&connP->addr;
        inet_ntop(saddr->sin6_family, &saddr->sin6_addr, s, INET6_ADDRSTRLEN);
        port = saddr->sin6_port;
    }

    fprintf(stderr, "Sending %lu bytes to [%s]:%hu\r\n", length, s, ntohs(port));

    output_buffer(stderr, buffer, length, 0);
#endif

    if (-1 == connP->sendFunc(buffer, length, connP)) {
        fprintf(stderr, "#> failed sending %lu bytes\r\n", length);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    return COAP_NO_ERROR;
}

bool lwm2m_session_is_equal(void *session1, void *session2, void *userData) {
    (void)userData; /* unused */

    return (session1 == session2);
}

/*

int get_port(struct sockaddr *x)
{
   if (x->sa_family == AF_INET)
   {
       return ((struct sockaddr_in *)x)->sin_port;
   } else if (x->sa_family == AF_INET6) {
       return ((struct sockaddr_in6 *)x)->sin6_port;
   } else {
       printf("non IPV4 or IPV6 address\n");
       return  -1;
   }
}

int sockaddr_cmp(struct sockaddr *x, struct sockaddr *y)
{
    int portX = get_port(x);
    int portY = get_port(y);

    // if the port is invalid of different
    if (portX == -1 || portX != portY)
    {
        return 0;
    }

    // IPV4?
    if (x->sa_family == AF_INET)
    {
        // is V4?
        if (y->sa_family == AF_INET)
        {
            // compare V4 with V4
            return ((struct sockaddr_in *)x)->sin_addr.s_addr == ((struct sockaddr_in *)y)->sin_addr.s_addr;
            // is V6 mapped V4?
        } else if (IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)y)->sin6_addr)) {
            struct in6_addr* addr6 = &((struct sockaddr_in6 *)y)->sin6_addr;
            uint32_t y6to4 = addr6->s6_addr[15] << 24 | addr6->s6_addr[14] << 16 | addr6->s6_addr[13] << 8 |
addr6->s6_addr[12]; return y6to4 == ((struct sockaddr_in *)x)->sin_addr.s_addr; } else { return 0;
        }
    } else if (x->sa_family == AF_INET6 && y->sa_family == AF_INET6) {
        // IPV6 with IPV6 compare
        return memcmp(((struct sockaddr_in6 *)x)->sin6_addr.s6_addr, ((struct sockaddr_in6 *)y)->sin6_addr.s6_addr, 16)
== 0; } else {
        // unknown address type
        printf("non IPV4 or IPV6 address\n");
        return 0;
    }
}

*/
