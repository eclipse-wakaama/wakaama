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

#include "udp/connection.h"
#include "commandline.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

static int find_and_bind_to_address(struct addrinfo *res) {
    int s = -1;
    for (struct addrinfo *p = res; p != NULL && s == -1; p = p->ai_next) {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s >= 0) {
            if (-1 == bind(s, p->ai_addr, p->ai_addrlen)) {
                close(s);
                s = -1;
            }
        }
    }
    return s;
}

static int find_and_connect_to_address(struct addrinfo *servinfo, struct sockaddr **sa, socklen_t *sl) {
    int s = -1;
    for (struct addrinfo *p = servinfo; p != NULL && s == -1; p = p->ai_next) {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s >= 0) {
            (*sa) = p->ai_addr;
            (*sl) = p->ai_addrlen;
            if (-1 == connect(s, p->ai_addr, p->ai_addrlen)) {
                close(s);
                s = -1;
            }
        }
    }
    return s;
}

int lwm2m_create_socket(const char *portStr, int addressFamily) {
    int s = -1;
    struct addrinfo hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = addressFamily;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if (0 != getaddrinfo(NULL, portStr, &hints, &res)) {
        return -1;
    }

    s = find_and_bind_to_address(res);

    freeaddrinfo(res);

    return s;
}

lwm2m_connection_t *lwm2m_connection_find(lwm2m_connection_t *connList, struct sockaddr_storage *addr, size_t addrLen) {
    lwm2m_connection_t *connP;

    connP = connList;
    while (connP != NULL) {
        if ((connP->addrLen == addrLen) && (memcmp(&(connP->addr), addr, addrLen) == 0)) { // NOSONAR
            return connP;
        }
        connP = connP->next;
    }

    return connP;
}

lwm2m_connection_t *lwm2m_connection_new_incoming(lwm2m_connection_t *connList, int sock, struct sockaddr *addr,
                                                  size_t addrLen) {
    lwm2m_connection_t *connP;

    connP = (lwm2m_connection_t *)lwm2m_malloc(sizeof(lwm2m_connection_t));
    if (connP != NULL) {
        connP->sock = sock;
        memcpy(&(connP->addr), addr, addrLen);
        connP->addrLen = addrLen;
        connP->next = connList;
    }

    return connP;
}

lwm2m_connection_t *lwm2m_connection_create(lwm2m_connection_t *connList, int sock, char *host, char *port,
                                            int addressFamily) {
    struct addrinfo hints;
    struct addrinfo *servinfo = NULL;
    int s;
    struct sockaddr *sa;
    socklen_t sl;
    lwm2m_connection_t *connP = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = addressFamily;
    hints.ai_socktype = SOCK_DGRAM;

    if (0 != getaddrinfo(host, port, &hints, &servinfo) || servinfo == NULL)
        return NULL;

    // we test the various addresses
    s = find_and_connect_to_address(servinfo, &sa, &sl);
    if (s >= 0) {
        connP = lwm2m_connection_new_incoming(connList, sock, sa, sl);
        close(s);
    }
    if (NULL != servinfo) {
        freeaddrinfo(servinfo);
    }

    return connP;
}

void lwm2m_connection_free(lwm2m_connection_t *connList) {
    while (connList != NULL) {
        lwm2m_connection_t *nextP;

        nextP = connList->next;
        lwm2m_free(connList);

        connList = nextP;
    }
}

static int get_address_and_port(const lwm2m_connection_t *connP, char *str, size_t str_len, in_port_t *port) {
    if (AF_INET == connP->addr.sin6_family) {
        struct sockaddr_in *saddr = (struct sockaddr_in *)&connP->addr;
        inet_ntop(saddr->sin_family, &saddr->sin_addr, str, INET6_ADDRSTRLEN);
        *port = saddr->sin_port;
    } else if (AF_INET6 == connP->addr.sin6_family) {
        struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)&connP->addr;
        inet_ntop(saddr->sin6_family, &saddr->sin6_addr, str, INET6_ADDRSTRLEN);
        *port = saddr->sin6_port;
    } else {
        return -1;
    }
    return 0;
}

int lwm2m_connection_send(lwm2m_connection_t *connP, uint8_t *buffer, size_t length) {
    int nbSent;
    size_t offset;

    char s[INET6_ADDRSTRLEN];
    in_port_t port;

    s[0] = 0;

    int ret = get_address_and_port(connP, s, INET6_ADDRSTRLEN, &port);
    if (ret < 0) {
        return ret;
    }

    fprintf(stderr, "Sending %zu bytes to [%s]:%hu\r\n", length, s, ntohs(port));

    output_buffer(stderr, buffer, length, 0);

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

uint8_t lwm2m_buffer_send(void *sessionH, uint8_t *buffer, size_t length, void *userdata) {
    lwm2m_connection_t *connP = (lwm2m_connection_t *)sessionH;

    (void)userdata; /* unused */

    if (connP == NULL) {
        fprintf(stderr, "#> failed sending %zu bytes, missing connection\r\n", length);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    if (-1 == lwm2m_connection_send(connP, buffer, length)) {
        fprintf(stderr, "#> failed sending %zu bytes\r\n", length);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    return COAP_NO_ERROR;
}

bool lwm2m_session_is_equal(void *session1, void *session2, void *userData) {
    (void)userData; /* unused */

    return (session1 == session2);
}
