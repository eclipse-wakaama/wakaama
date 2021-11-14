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

typedef int (*connection_send_func_t)(uint8_t const *, size_t, void *);
typedef int (*connection_recv_func_t)(lwm2m_context_t *, uint8_t *, size_t, void *);
typedef void (*connection_deinit_func_t)(void *);

typedef struct _connection_t {
    struct _connection_t *next;
    int sock;
    struct sockaddr_in6 addr;
    size_t addrLen;
    connection_send_func_t sendFunc;
    connection_recv_func_t recvFunc;
    connection_deinit_func_t deinitFunc;
} connection_t;

typedef struct _lwm2m_connection_layer_t {
    lwm2m_context_t *ctx;
    connection_t *connList;
} lwm2m_connection_layer_t;

lwm2m_connection_layer_t *connectionlayer_create(lwm2m_context_t *context);
int connectionlayer_handle_packet(lwm2m_connection_layer_t *connLayerP, struct sockaddr_storage *addr, size_t addrLen,
                                  uint8_t *buffer, size_t length);
void connectionlayer_free(lwm2m_connection_layer_t *connLayerP);
void connectionlayer_free_connection(lwm2m_connection_layer_t *connLayerP, connection_t *conn);
connection_t *connectionlayer_find_connection(lwm2m_connection_layer_t *connLayerP, struct sockaddr_storage const *addr,
                                              size_t addrLen);
void connectionlayer_add_connection(lwm2m_connection_layer_t *connLayer, connection_t *conn);

int create_socket(const char *portStr, int ai_family);

connection_t *connection_new_incoming(lwm2m_connection_layer_t *connLayerP, int sock, struct sockaddr_storage *addr,
                                      size_t addrLen);
connection_t *connection_create(lwm2m_connection_layer_t *connLayerP, int sock, char *host, char *port,
                                int addressFamily);
int connection_create_inplace(connection_t *conn, int sock, char *host, char *port, int addressFamily);

#endif
