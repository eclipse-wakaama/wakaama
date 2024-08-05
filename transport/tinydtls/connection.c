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

#include "tinydtls/connection.h"
#include "commandline.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define COAP_PORT "5683"
#define COAPS_PORT "5684"
#define URI_LENGTH 256

dtls_context_t *dtlsContext;

typedef struct _dtls_app_context_ {
    lwm2m_context_t *lwm2mH;
    lwm2m_dtls_connection_t *connList;
} dtls_app_context_t;

static char *get_opaque_value(size_t *length, int size, lwm2m_data_t *dataP) {
    if (dataP != NULL && dataP->type == LWM2M_TYPE_OPAQUE) {
        char *buff;

        buff = (char *)lwm2m_malloc(dataP->value.asBuffer.length);
        if (buff != 0) {
            memcpy(buff, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
            *length = dataP->value.asBuffer.length;
        }
        lwm2m_data_free(size, dataP);

        return buff;
    } else {
        return NULL;
    }
}

/********************* Security Obj Helpers **********************/
char *security_get_uri(lwm2m_context_t *lwm2mH, lwm2m_object_t *obj, int instanceId, char *uriBuffer,
                       size_t bufferSize) {
    int size = 1;
    lwm2m_data_t *dataP = lwm2m_data_new(size);
    dataP->id = 0; // security server uri

    obj->readFunc(lwm2mH, instanceId, &size, &dataP, obj);
    if (dataP != NULL && dataP->type == LWM2M_TYPE_STRING && dataP->value.asBuffer.length > 0) {
        if (bufferSize > dataP->value.asBuffer.length) {
            memset(uriBuffer, 0, dataP->value.asBuffer.length + 1);
            strncpy(uriBuffer, (const char *)dataP->value.asBuffer.buffer, dataP->value.asBuffer.length); // NOSONAR
            lwm2m_data_free(size, dataP);
            return uriBuffer;
        }
    }
    lwm2m_data_free(size, dataP);
    return NULL;
}

int64_t security_get_mode(lwm2m_context_t *lwm2mH, lwm2m_object_t *obj, int instanceId) {
    int64_t mode;
    int size = 1;
    lwm2m_data_t *dataP = lwm2m_data_new(size);
    dataP->id = 2; // security mode

    obj->readFunc(lwm2mH, instanceId, &size, &dataP, obj);
    if (0 != lwm2m_data_decode_int(dataP, &mode)) {
        lwm2m_data_free(size, dataP);
        return mode;
    }

    lwm2m_data_free(size, dataP);
    fprintf(stderr, "Unable to get security mode : use not secure mode");
    return LWM2M_SECURITY_MODE_NONE;
}

char *security_get_public_id(lwm2m_context_t *lwm2mH, lwm2m_object_t *obj, int instanceId, size_t *length) {
    int size = 1;
    lwm2m_data_t *dataP = lwm2m_data_new(size);
    dataP->id = 3; // public key or id

    obj->readFunc(lwm2mH, instanceId, &size, &dataP, obj);
    return get_opaque_value(length, size, dataP);
}

char *security_get_secret_key(lwm2m_context_t *lwm2mH, lwm2m_object_t *obj, int instanceId, size_t *length) {
    int size = 1;
    lwm2m_data_t *dataP = lwm2m_data_new(size);
    dataP->id = 5; // secret key

    obj->readFunc(lwm2mH, instanceId, &size, &dataP, obj);
    return get_opaque_value(length, size, dataP);
}

/********************* Security Obj Helpers Ends **********************/

/* Returns the number sent, or -1 for errors */
int send_data(lwm2m_dtls_connection_t *connP, uint8_t *buffer, size_t length) {
    int nbSent;
    size_t offset;

#if LWM2M_LOG_LEVEL != LWM2M_LOG_DISABLED
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

    fprintf(stderr, "Sending %d bytes to [%s]:%hu\r\n", (int)length, s, ntohs(port));

    output_buffer(stderr, buffer, length, 0);
#endif

    offset = 0;
    while (offset != length) {
        nbSent =
            sendto(connP->sock, buffer + offset, length - offset, 0, (struct sockaddr *)&(connP->addr), connP->addrLen);
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

    dtls_app_context_t *appContext = (dtls_app_context_t *)ctx->app;

    // find connection
    lwm2m_dtls_connection_t *cnx = lwm2m_connection_find(appContext->connList, &(session->addr.st), session->size);
    if (cnx == NULL) {
        printf("GET PSK session not found\n");
        return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
    }

    switch (type) {
    case DTLS_PSK_IDENTITY: {
        size_t idLen = 0;
        char *id2;
        id2 = security_get_public_id(appContext->lwm2mH, cnx->securityObj, cnx->securityInstId, &idLen);
        if (result_length < idLen) {
            printf("cannot set psk_identity -- buffer too small\n");
            return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
        }

        memcpy(result, id2, idLen);
        lwm2m_free(id2);
        return idLen;
    }
    case DTLS_PSK_KEY: {
        size_t keyLen = 0;
        char *key;
        key = security_get_secret_key(appContext->lwm2mH, cnx->securityObj, cnx->securityInstId, &keyLen);

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

    dtls_app_context_t *appContext = (dtls_app_context_t *)ctx->app;

    // find connection
    lwm2m_dtls_connection_t *cnx = lwm2m_connection_find(appContext->connList, &(session->addr.st), session->size);
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

    dtls_app_context_t *appContext = (dtls_app_context_t *)ctx->app;

    // find connection
    lwm2m_dtls_connection_t *cnx = lwm2m_connection_find(appContext->connList, &(session->addr.st), session->size);
    if (cnx != NULL) {
        lwm2m_handle_packet(appContext->lwm2mH, (uint8_t *)data, len, (void *)cnx);
        return 0;
    }
    return -1;
}
/**************************   TinyDTLS Callbacks Ends ************************/

static dtls_handler_t cb = {
    .write = send_to_peer,
    .read = read_from_peer,
    .event = NULL,
    // #ifdef DTLS_PSK
    .get_psk_info = get_psk_info,
    // #endif /* DTLS_PSK */
    // #ifdef DTLS_ECC
    //   .get_ecdsa_key = get_ecdsa_key,
    //   .verify_ecdsa_key = verify_ecdsa_key
    // #endif /* DTLS_ECC */
};

dtls_context_t *get_dtls_context(lwm2m_context_t *lwm2mH, lwm2m_dtls_connection_t *connList) {
    static dtls_app_context_t appContext;
    appContext.lwm2mH = lwm2mH;
    appContext.connList = connList;
    if (dtlsContext == NULL) {
        dtls_init();
        dtlsContext = dtls_new_context(&appContext);
        if (dtlsContext == NULL)
            fprintf(stderr, "Failed to create the DTLS context\r\n");
        dtls_set_handler(dtlsContext, &cb);
    }
    return dtlsContext;
}

int get_port(struct sockaddr *x) {
    if (x->sa_family == AF_INET) {
        return ((struct sockaddr_in *)x)->sin_port;
    } else if (x->sa_family == AF_INET6) {
        return ((struct sockaddr_in6 *)x)->sin6_port;
    } else {
        printf("non IPV4 or IPV6 address\n");
        return -1;
    }
}

int sockaddr_cmp(struct sockaddr *x, struct sockaddr *y) {
    int portX = get_port(x);
    int portY = get_port(y);

    // if the port is invalid of different
    if (portX == -1 || portX != portY) {
        return 0;
    }

    // IPV4?
    if (x->sa_family == AF_INET) {
        // is V4?
        if (y->sa_family == AF_INET) {
            // compare V4 with V4
            return ((struct sockaddr_in *)x)->sin_addr.s_addr == ((struct sockaddr_in *)y)->sin_addr.s_addr;
            // is V6 mapped V4?
        } else if (IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)y)->sin6_addr)) {
            struct in6_addr *addr6 = &((struct sockaddr_in6 *)y)->sin6_addr;
            uint32_t y6to4 =
                addr6->s6_addr[15] << 24 | addr6->s6_addr[14] << 16 | addr6->s6_addr[13] << 8 | addr6->s6_addr[12];
            return y6to4 == ((struct sockaddr_in *)x)->sin_addr.s_addr;
        } else {
            return 0;
        }
    } else if (x->sa_family == AF_INET6 && y->sa_family == AF_INET6) {
        // IPV6 with IPV6 compare
        return memcmp(((struct sockaddr_in6 *)x)->sin6_addr.s6_addr, ((struct sockaddr_in6 *)y)->sin6_addr.s6_addr,
                      16) == 0;
    } else {
        // unknown address type
        printf("non IPV4 or IPV6 address\n");
        return 0;
    }
}

int lwm2m_create_socket(const char *portStr, int ai_family) {
    int s = -1;
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = ai_family;
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

lwm2m_dtls_connection_t *lwm2m_connection_find(lwm2m_dtls_connection_t *connList, const struct sockaddr_storage *addr,
                                               size_t addrLen) {
    lwm2m_dtls_connection_t *connP;

    connP = connList;
    while (connP != NULL) {

        if (sockaddr_cmp((struct sockaddr *)(&connP->addr), (struct sockaddr *)addr)) {
            return connP;
        }

        connP = connP->next;
    }

    return connP;
}

lwm2m_dtls_connection_t *lwm2m_connection_new_incoming(lwm2m_dtls_connection_t *connList, int sock,
                                                       const struct sockaddr *addr, size_t addrLen) {
    lwm2m_dtls_connection_t *connP;

    connP = (lwm2m_dtls_connection_t *)malloc(sizeof(lwm2m_dtls_connection_t));
    if (connP != NULL) {
        memset(connP, 0, sizeof(lwm2m_dtls_connection_t));
        connP->sock = sock;
        memcpy(&(connP->addr), addr, addrLen);
        connP->addrLen = addrLen;
        connP->next = connList;

        connP->dtlsSession = (session_t *)malloc(sizeof(session_t));
        memset(connP->dtlsSession, 0, sizeof(session_t));
        connP->dtlsSession->addr.sin6 = connP->addr;
        connP->dtlsSession->size = connP->addrLen;
        connP->lastSend = lwm2m_gettime();
    }

    return connP;
}

lwm2m_dtls_connection_t *lwm2m_connection_create(lwm2m_dtls_connection_t *connList, int sock,
                                                 lwm2m_object_t *securityObj, int instanceId, lwm2m_context_t *lwm2mH,
                                                 int addressFamily) {
    struct addrinfo hints;
    struct addrinfo *servinfo = NULL;
    struct addrinfo *p;
    int s;
    struct sockaddr *sa;
    socklen_t sl;
    lwm2m_dtls_connection_t *connP = NULL;
    char uriBuf[URI_LENGTH];
    char *uri;
    char *host;
    char *port;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = addressFamily;
    hints.ai_socktype = SOCK_DGRAM;

    uri = security_get_uri(lwm2mH, securityObj, instanceId, uriBuf, URI_LENGTH);
    if (uri == NULL)
        return NULL;

    // parse uri in the form "coaps://[host]:[port]"
    char defaultport[5];
    if (0 == strncmp(uri, "coaps://", strlen("coaps://"))) {      // NOSONAR
        host = uri + strlen("coaps://");                          // NOSONAR
        strncpy(defaultport, COAPS_PORT, 5);                      // NOSONAR
    } else if (0 == strncmp(uri, "coap://", strlen("coap://"))) { // NOSONAR
        host = uri + strlen("coap://");                           // NOSONAR
        strncpy(defaultport, COAP_PORT, 5);                       // NOSONAR
    } else {
        return NULL;
    }
    port = strrchr(host, ':');
    if (port == NULL) {
        port = defaultport;
    } else {
        // remove brackets
        if (host[0] == '[') {
            host++;
            if (*(port - 1) == ']') { // NOSONAR
                *(port - 1) = 0;
            } else {
                return NULL;
            }
        }
        // split strings
        *port = 0;
        port++;
    }

    if (0 != getaddrinfo(host, port, &hints, &servinfo) || servinfo == NULL)
        return NULL;

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
        connP = lwm2m_connection_new_incoming(connList, sock, sa, sl);
        close(s);

        // do we need to start tinydtls?
        if (connP != NULL) {
            connP->securityObj = securityObj;
            connP->securityInstId = instanceId;
            connP->lwm2mH = lwm2mH;

            if (security_get_mode(lwm2mH, connP->securityObj, connP->securityInstId) != LWM2M_SECURITY_MODE_NONE) {
                connP->dtlsContext = get_dtls_context(lwm2mH, connP);
            } else {
                // no dtls session
                connP->dtlsSession = NULL;
            }
        }
    }

    if (NULL != servinfo)
        free(servinfo);

    return connP;
}

void lwm2m_connection_free(lwm2m_dtls_connection_t *connList) {
    dtls_free_context(dtlsContext);
    dtlsContext = NULL;
    while (connList != NULL) {
        lwm2m_dtls_connection_t *nextP;

        nextP = connList->next;
        free(connList);

        connList = nextP;
    }
}

int lwm2m_connection_send(lwm2m_dtls_connection_t *connP, uint8_t *buffer, size_t length) {
    if (connP->dtlsSession == NULL) {
        // no security
        if (0 >= send_data(connP, buffer, length)) {
            return -1;
        }
    } else {
        if (DTLS_NAT_TIMEOUT > 0 && (lwm2m_gettime() - connP->lastSend) > DTLS_NAT_TIMEOUT) {
            // we need to rehandhake because our source IP/port probably changed for the server
            if (lwm2m_connection_rehandshake(connP, false) != 0) {
                printf("can't send due to rehandshake error\n");
                return -1;
            }
        }
        if (-1 == dtls_write(connP->dtlsContext, connP->dtlsSession, buffer, length)) {
            return -1;
        }
    }

    return 0;
}

int lwm2m_connection_handle_packet(lwm2m_dtls_connection_t *connP, uint8_t *buffer, size_t length) {

    if (connP->dtlsSession != NULL) {
        // Let liblwm2m respond to the query depending on the context
        int result = dtls_handle_message(connP->dtlsContext, connP->dtlsSession, buffer, length);
        if (result != 0) {
            printf("error dtls handling message %d\n", result);
        }
        return result;
    } else {
        // no security, just give the plaintext buffer to liblwm2m
        lwm2m_handle_packet(connP->lwm2mH, buffer, length, (void *)connP);
        return 0;
    }
}

int lwm2m_connection_rehandshake(lwm2m_dtls_connection_t *connP, bool sendCloseNotify) {

    // if not a dtls connection we do nothing
    if (connP->dtlsSession == NULL) {
        return 0;
    }

    // reset current session
    dtls_peer_t *peer = dtls_get_peer(connP->dtlsContext, connP->dtlsSession);
    if (peer != NULL) {
        if (!sendCloseNotify) {
            peer->state = DTLS_STATE_CLOSED;
        }
        dtls_reset_peer(connP->dtlsContext, peer);
    }

    // start a fresh handshake
    int result = dtls_connect(connP->dtlsContext, connP->dtlsSession);
    if (result != 0) {
        printf("error dtls reconnection %d\n", result);
    }
    return result;
}

uint8_t lwm2m_buffer_send(void *sessionH, uint8_t *buffer, size_t length, void *userdata) {
    lwm2m_dtls_connection_t *connP = (lwm2m_dtls_connection_t *)sessionH;

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

bool lwm2m_session_is_equal(void *session1, void *session2, void *userData) { return (session1 == session2); }
