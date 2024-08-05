/*******************************************************************************
 *
 * Copyright (c) 2015 Intel Corporation and others.
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
 *    Simon Bernard - initial API and implementation
 *    Christian Renz - Please refer to git log
 *
 *******************************************************************************/

#ifndef DTLS_CONNECTION_H_
#define DTLS_CONNECTION_H_

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "liblwm2m.h"
#include "tinydtls/dtls.h"
#include "tinydtls/tinydtls.h"

#define LWM2M_STANDARD_PORT_STR "5683"
#define LWM2M_STANDARD_PORT 5683
#define LWM2M_DTLS_PORT_STR "5684"
#define LWM2M_DTLS_PORT 5684
#define LWM2M_BSSERVER_PORT_STR "5685"
#define LWM2M_BSSERVER_PORT 5685

// after 40sec of inactivity we rehandshake
#define DTLS_NAT_TIMEOUT 40

typedef struct _lwm2m_dtls_connection_t {
    struct _lwm2m_dtls_connection_t *next;
    int sock;
    struct sockaddr_in6 addr;
    size_t addrLen;
    session_t *dtlsSession;
    lwm2m_object_t *securityObj;
    int securityInstId;
    lwm2m_context_t *lwm2mH;
    dtls_context_t *dtlsContext;
    time_t lastSend; // last time a data was sent to the server (used for NAT timeouts)
} lwm2m_dtls_connection_t;

int lwm2m_create_socket(const char *portStr, int ai_family);

lwm2m_dtls_connection_t *lwm2m_connection_find(lwm2m_dtls_connection_t *connList, const struct sockaddr_storage *addr,
                                               size_t addrLen);
lwm2m_dtls_connection_t *lwm2m_connection_new_incoming(lwm2m_dtls_connection_t *connList, int sock,
                                                       const struct sockaddr *addr, size_t addrLen);
lwm2m_dtls_connection_t *lwm2m_connection_create(lwm2m_dtls_connection_t *connList, int sock,
                                                 lwm2m_object_t *securityObj, int instanceId, lwm2m_context_t *lwm2mH,
                                                 int addressFamily);

void lwm2m_connection_free(lwm2m_dtls_connection_t *connList);

int lwm2m_connection_send(lwm2m_dtls_connection_t *connP, uint8_t *buffer, size_t length);
int lwm2m_connection_handle_packet(lwm2m_dtls_connection_t *connP, uint8_t *buffer, size_t length);

// rehandshake a connection, useful when your NAT timed out and your client has a new IP/PORT
int lwm2m_connection_rehandshake(lwm2m_dtls_connection_t *connP, bool sendCloseNotify);

#endif
