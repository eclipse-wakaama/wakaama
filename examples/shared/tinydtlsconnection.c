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
 *    David Navarro, Intel Corporation - initial API and implementation
 *    Christian Renz - Please refer to git log
 *
 *******************************************************************************/

#include "dtlsconnection.h"
#if defined LWM2M_CLIENT_MODE && defined DTLS

#include "commandline.h"
#include "dtls.h"
#include "object_utils.h"
#include "tinydtls.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define COAP_PORT "5683"
#define COAPS_PORT "5684"
#define URI_LENGTH 256

#define DTLS_NAT_TIMEOUT 40

typedef struct _dtls_connection_t {
    connection_t conn;
    session_t *dtlsSession;
    int securityInstId;
    dtls_context_t *dtlsContext;
    time_t lastSend; // last time a data was sent to the server (used for NAT timeouts)
} dtlsconnection_t;

dtls_context_t *dtlsContext;

/* Returns the number sent, or -1 for errors */
static int send_data(dtlsconnection_t *connP, uint8_t const *buffer, size_t length) {
    int nbSent;
    size_t offset;

#ifdef LWM2M_WITH_LOGS
    char s[INET6_ADDRSTRLEN];
    in_port_t port;

    s[0] = 0;

    if (AF_INET == connP->conn.addr.sin6_family) {
        struct sockaddr_in *saddr = (struct sockaddr_in *)&connP->conn.addr;
        inet_ntop(saddr->sin_family, &saddr->sin_addr, s, INET6_ADDRSTRLEN);
        port = saddr->sin_port;
    } else if (AF_INET6 == connP->conn.addr.sin6_family) {
        struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)&connP->conn.addr;
        inet_ntop(saddr->sin6_family, &saddr->sin6_addr, s, INET6_ADDRSTRLEN);
        port = saddr->sin6_port;
    }

    fprintf(stderr, "Sending %d bytes to [%s]:%hu\r\n", (int)length, s, ntohs(port));

    output_buffer(stderr, buffer, length, 0);
#endif

    offset = 0;
    while (offset != length) {
        nbSent = sendto(connP->conn.sock, buffer + offset, length - offset, 0, (struct sockaddr *)&(connP->conn.addr),
                        connP->conn.addrLen);
        if (nbSent == -1)
            return -1;
        offset += nbSent;
    }
    connP->lastSend = lwm2m_gettime();
    return offset;
}

/**************************  TinyDTLS Callbacks  ************************/

/* This function is the "key store" for tinyDTLS. It is called to
 * retrieve a key for the given identity within this particular
 * session. */
static int get_psk_info(struct dtls_context_t *ctx, const session_t *session, dtls_credentials_type_t type,
                        const unsigned char *id, size_t id_len, unsigned char *result, size_t result_length) {

    lwm2m_connection_layer_t *connLayer = (lwm2m_connection_layer_t *)ctx->app;

    // find connection
    dtlsconnection_t *cnx =
        (dtlsconnection_t *)connectionlayer_find_connection(connLayer, &(session->addr.st), session->size);
    if (cnx == NULL) {
        printf("GET PSK session not found\n");
        return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
    }

    switch (type) {
    case DTLS_PSK_IDENTITY: {
        size_t idLen;
        uint8_t *id;
        int ret = security_get_psk_identity(connLayer->ctx, cnx->securityInstId, &id, &idLen);
        if (ret <= 0) {
            printf("cannot read psk identity from security object\n");
            return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
        }
        if (result_length < idLen) {
            printf("cannot set psk_identity -- buffer too small\n");
            return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
        }

        memcpy(result, id, idLen);
        lwm2m_free(id);
        return idLen;
    }
    case DTLS_PSK_KEY: {
        size_t keyLen;
        uint8_t *key;
        int ret = security_get_psk(connLayer->ctx, cnx->securityInstId, &key, &keyLen);
        if (ret <= 0) {
            printf("cannot read psk from security object\n");
            return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
        }
        if (result_length < keyLen) {
            printf("cannot set psk -- buffer too small\n");
            return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
        }

        memcpy(result, key, keyLen);
        lwm2m_free(key);
        return keyLen;
    }
    case DTLS_PSK_HINT: {
        // PSK_HINT is optional and can be empty.
        return 0;
    }
    default:
        printf("unsupported request type: %d\n", type);
    }

    return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
}

/* The callback function must return the number of bytes
 * that were sent, or a value less than zero to indicate an
 * error. */
static int send_to_peer(struct dtls_context_t *ctx, session_t *session, uint8 *data, size_t len) {

    lwm2m_connection_layer_t *connLayer = (lwm2m_connection_layer_t *)ctx->app;

    // find connection
    dtlsconnection_t *cnx =
        (dtlsconnection_t *)connectionlayer_find_connection(connLayer, &(session->addr.st), session->size);
    if (cnx != NULL) {
        // send data to peer

        // TODO: nat expiration?
        int res = send_data(cnx, data, len);
        if (res < 0) {
            return -1;
        }
        return res;
    }
    return -1;
}

static int read_from_peer(struct dtls_context_t *ctx, session_t *session, uint8 *data, size_t len) {

    lwm2m_connection_layer_t *connLayer = (lwm2m_connection_layer_t *)ctx->app;

    // find connection
    dtlsconnection_t *cnx =
        (dtlsconnection_t *)connectionlayer_find_connection(connLayer, &(session->addr.st), session->size);
    if (cnx != NULL) {
        lwm2m_handle_packet(connLayer->ctx, (uint8_t *)data, len, (void *)cnx);
        return 0;
    }
    return -1;
}
/**************************   TinyDTLS Callbacks Ends ************************/

static dtls_handler_t cb = {
    .write = send_to_peer,
    .read = read_from_peer,
    .event = NULL,
    //#ifdef DTLS_PSK
    .get_psk_info = get_psk_info,
    //#endif /* DTLS_PSK */
    //#ifdef DTLS_ECC
    //  .get_ecdsa_key = get_ecdsa_key,
    //  .verify_ecdsa_key = verify_ecdsa_key
    //#endif /* DTLS_ECC */
};

static dtls_context_t *get_dtls_context(lwm2m_connection_layer_t *layer) {
    if (dtlsContext == NULL) {
        dtls_init();
        dtlsContext = dtls_new_context(layer);
        if (dtlsContext == NULL)
            fprintf(stderr, "Failed to create the DTLS context\r\n");
        dtls_set_handler(dtlsContext, &cb);
    }
    return dtlsContext;
}

static int connection_rehandshake(dtlsconnection_t *dtlsConn, bool sendCloseNotify) {

    // reset current session
    dtls_peer_t *peer = dtls_get_peer(dtlsConn->dtlsContext, dtlsConn->dtlsSession);
    if (peer != NULL) {
        if (!sendCloseNotify) {
            peer->state = DTLS_STATE_CLOSED;
        }
        dtls_reset_peer(dtlsConn->dtlsContext, peer);
    }

    // start a fresh handshake
    int result = dtls_connect(dtlsConn->dtlsContext, dtlsConn->dtlsSession);
    if (result != 0) {
        printf("error dtls reconnection %d\n", result);
    }
    return result;
}

static int dtlsconnection_send(uint8_t const *buffer, size_t len, void *userdata) {
    dtlsconnection_t *dtlsConn = (dtlsconnection_t *)userdata;

    if (DTLS_NAT_TIMEOUT > 0 && (lwm2m_gettime() - dtlsConn->lastSend) > DTLS_NAT_TIMEOUT) {
        // we need to rehandhake because our source IP/port probably changed for the server
        if (connection_rehandshake(dtlsConn, false) != 0) {
            printf("can't send due to rehandshake error\n");
            return -1;
        }
    }
    if (-1 == dtls_write(dtlsConn->dtlsContext, dtlsConn->dtlsSession, (uint8_t *)buffer, len)) {
        return -1;
    }
    return 1;
}

static int dtlsconnection_recv(lwm2m_context_t *ctx, uint8_t *buffer, size_t len, void *userdata) {
    (void)ctx;
    dtlsconnection_t *dtlsConn = (dtlsconnection_t *)userdata;
    int result = dtls_handle_message(dtlsConn->dtlsContext, dtlsConn->dtlsSession, buffer, len);
    if (result != 0) {
        printf("error dtls handling message %d\n", result);
    }
    return result;
}

static void dtlsconnection_deinit(void *userdata) {
    dtlsconnection_t *dtlsConn = (dtlsconnection_t *)userdata;
    dtls_close(dtlsConn->dtlsContext, dtlsConn->dtlsSession);
    lwm2m_free(dtlsConn->dtlsSession);
}

connection_t *dtlsconnection_create(lwm2m_connection_layer_t *connLayerP, uint16_t securityInstance, int sock,
                                    char *host, char *port, int addressFamily) {
    dtlsconnection_t *dtlsConn = (dtlsconnection_t *)lwm2m_malloc(sizeof(dtlsconnection_t));
    if (dtlsConn == NULL) {
        return NULL;
    }
    if (connection_create_inplace(&dtlsConn->conn, sock, host, port, addressFamily) <= 0) {
        lwm2m_free(dtlsConn);
        return NULL;
    }
    dtlsConn->dtlsSession = (session_t *)lwm2m_malloc(sizeof(session_t));
    if (dtlsConn->dtlsSession == NULL) {
        lwm2m_free(dtlsConn);
    }
    memset(dtlsConn->dtlsSession, 0, sizeof(session_t));
    dtlsConn->dtlsSession->addr.sin6 = dtlsConn->conn.addr;
    dtlsConn->dtlsSession->size = dtlsConn->conn.addrLen;
    dtlsConn->lastSend = lwm2m_gettime();
    dtlsConn->securityInstId = securityInstance;
    dtlsConn->dtlsContext = get_dtls_context(connLayerP);
    if (dtlsConn->dtlsContext == NULL) {
        lwm2m_free(dtlsConn->dtlsSession);
        lwm2m_free(dtlsConn);
        return NULL;
    }
    dtlsConn->conn.sendFunc = dtlsconnection_send;
    dtlsConn->conn.recvFunc = dtlsconnection_recv;
    dtlsConn->conn.deinitFunc = dtlsconnection_deinit;
    dtlsConn->lastSend = lwm2m_gettime();

    connectionlayer_add_connection(connLayerP, (connection_t *)dtlsConn);
    return (connection_t *)dtlsConn;
}

#else
connection_t *dtlsconnection_create(lwm2m_connection_layer_t *connLayerP, uint16_t securityInstance, int sock,
                                    char *host, char *port, int addressFamily) {
    (void)connLayerP;
    (void)securityInstance;
    (void)sock;
    (void)host;
    (void)port;
    (void)addressFamily;
    return NULL;
}
#endif
