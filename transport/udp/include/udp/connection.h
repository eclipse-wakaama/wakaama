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
 *
 *******************************************************************************/

#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <arpa/inet.h>
#include <liblwm2m.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define LWM2M_STANDARD_PORT_STR "5683"
#define LWM2M_STANDARD_PORT 5683
#define LWM2M_DTLS_PORT_STR "5684"
#define LWM2M_DTLS_PORT 5684
#define LWM2M_BSSERVER_PORT_STR "5685"
#define LWM2M_BSSERVER_PORT 5685

typedef struct _lwm2m_connection_t {
    struct _lwm2m_connection_t *next;
    int sock;
    struct sockaddr_in6 addr;
    size_t addrLen;
} lwm2m_connection_t;

int lwm2m_create_socket(const char *portStr, int ai_family);

lwm2m_connection_t *lwm2m_connection_find(lwm2m_connection_t *connList, struct sockaddr_storage *addr, size_t addrLen);
lwm2m_connection_t *lwm2m_connection_new_incoming(lwm2m_connection_t *connList, int sock, struct sockaddr *addr,
                                                  size_t addrLen);
lwm2m_connection_t *lwm2m_connection_create(lwm2m_connection_t *connList, int sock, char *host, char *port,
                                            int addressFamily);

lwm2m_connection_t *lwm2m_connection_remove_one(lwm2m_connection_t *connList);

void lwm2m_connection_free(lwm2m_connection_t *connList);

int lwm2m_connection_send(lwm2m_connection_t *connP, uint8_t *buffer, size_t length);

#endif
