#include "dtlsconnection.h"
#if defined LWM2M_CLIENT_MODE && defined DTLS
#include "liblwm2m.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/ssl.h"
#include "mbedtls/timing.h"
#include "object_utils.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

typedef struct _dtlsconnection_t {
    connection_t conn;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_timing_delay_context timer;
    uint8_t *recvBuffer;
    size_t len;
} dtlsconnection_t;

static void dtlsconnection_deinit(void *conn) {
    dtlsconnection_t *dtlsConn = (dtlsconnection_t *)conn;
    mbedtls_ssl_config_free(&dtlsConn->conf);
    mbedtls_ctr_drbg_free(&dtlsConn->ctr_drbg);
    mbedtls_entropy_free(&dtlsConn->entropy);
    mbedtls_ssl_free(&dtlsConn->ssl);
}

static int dtlsconnection_recv(lwm2m_context_t *context, uint8_t *buffer, size_t len, void *conn) {
    dtlsconnection_t *dtlsConn = (dtlsconnection_t *)conn;
    int ret = 0;
    dtlsConn->recvBuffer = buffer;
    dtlsConn->len = len;

    do {
        ret = mbedtls_ssl_read(&dtlsConn->ssl, buffer, len);
    } while (ret == MBEDTLS_ERR_SSL_WANT_WRITE);

    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret < 0) {
#if defined(MBEDTLS_ERROR_C)
        char error_buf[200];
        mbedtls_strerror(ret, error_buf, 200);
        printf("Last error was: -0x%04x - %s\n\n", (unsigned int)-ret, error_buf);
#else
        printf("Last error was: -0x%04x\n\n", (unsigned int)-ret);
#endif /* MBEDTLS_ERROR_C */
    }

    if (ret > 0) {
        lwm2m_handle_packet(context, buffer, ret, dtlsConn);
        return ret;
    }
    return 0;
}

static int dtlsconnection_send(uint8_t const *buffer, size_t len, void *conn) {
    dtlsconnection_t *dtlsConn = (dtlsconnection_t *)conn;
    int ret = 0;
    do {
        ret = mbedtls_ssl_write(&dtlsConn->ssl, buffer, len);
    } while (ret == MBEDTLS_ERR_SSL_WANT_WRITE);

    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret < 0) {
#if defined(MBEDTLS_ERROR_C)
        char error_buf[200];
        mbedtls_strerror(ret, error_buf, 200);
        printf("Last error was: -0x%04x - %s\n\n", (unsigned int)-ret, error_buf);
#else
        printf("Last error was: -0x%04x\n\n", (unsigned int)-ret);
#endif /* MBEDTLS_ERROR_C */
    }
    return ret;
}

static int dtlsconnection_mbedtls_send(void *ctx, uint8_t const *buffer, size_t length) {
    dtlsconnection_t *connP = (dtlsconnection_t *)ctx;
    int nbSent;
    size_t offset;
    offset = 0;
    while (offset != length) {
        nbSent = sendto(connP->conn.sock, buffer + offset, length - offset, 0, (struct sockaddr *)&(connP->conn.addr),
                        connP->conn.addrLen);
        if (nbSent == -1)
            return -1;
        offset += nbSent;
    }
    return length;
}

static int dtlsconnection_mbedtls_recv(void *ctx, uint8_t *buf, size_t len) {
    dtlsconnection_t *conn = (dtlsconnection_t *)ctx;
    size_t minLen = MIN(len, conn->len);
    if (conn->recvBuffer != NULL) {
        memcpy(buf, conn->recvBuffer, minLen);
        if (minLen < conn->len) {
            conn->recvBuffer = conn->recvBuffer + minLen;
            conn->len = conn->len - minLen;
        } else {
            conn->recvBuffer = NULL;
            conn->len = 0;
        }

        return minLen;
    }
    return MBEDTLS_ERR_SSL_WANT_READ;
}

connection_t *dtlsconnection_create(lwm2m_connection_layer_t *connLayerP, uint16_t securityInstance, int sock,
                                    char *host, char *port, int addressFamily) {
    size_t identityLen = 0;
    uint8_t *identity = NULL;
    size_t pskLen = 0;
    uint8_t *psk = NULL;
    dtlsconnection_t *dtlsConn = NULL;

    int ret = security_get_psk_identity(connLayerP->ctx, securityInstance, &identity, &identityLen);
    if (ret <= 0) {
        return NULL;
    }
    ret = security_get_psk(connLayerP->ctx, securityInstance, &psk, &pskLen);
    if (ret <= 0) {
        lwm2m_free(identity);
        return NULL;
    }

    dtlsConn = (dtlsconnection_t *)lwm2m_malloc(sizeof(dtlsconnection_t));
    if (dtlsConn == NULL) {
        lwm2m_free(identity);
        lwm2m_free(psk);
        return NULL;
    }
    memset(dtlsConn, 0, sizeof(dtlsconnection_t));
    if (connection_create_inplace(&dtlsConn->conn, sock, host, port, addressFamily) <= 0) {
        lwm2m_free(identity);
        lwm2m_free(psk);
        lwm2m_free(dtlsConn);
        return NULL;
    }
    dtlsConn->conn.sendFunc = dtlsconnection_send;
    dtlsConn->conn.recvFunc = dtlsconnection_recv;
    dtlsConn->conn.deinitFunc = dtlsconnection_deinit;

    mbedtls_ssl_init(&dtlsConn->ssl);
    mbedtls_ssl_config_init(&dtlsConn->conf);
    mbedtls_ctr_drbg_init(&dtlsConn->ctr_drbg);
    mbedtls_entropy_init(&dtlsConn->entropy);
    if ((ret = mbedtls_ctr_drbg_seed(&dtlsConn->ctr_drbg, mbedtls_entropy_func, &dtlsConn->entropy,
                                     (const unsigned char *)identity, identityLen)) != 0) {
        dtlsconnection_deinit(dtlsConn);
        lwm2m_free(identity);
        lwm2m_free(psk);
        lwm2m_free(dtlsConn);
        return NULL;
    }

    if ((ret = mbedtls_ssl_config_defaults(&dtlsConn->conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_DATAGRAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        dtlsconnection_deinit(dtlsConn);
        lwm2m_free(identity);
        lwm2m_free(psk);
        lwm2m_free(dtlsConn);
        return NULL;
    }
    static int ciphersuites[] = {MBEDTLS_TLS_PSK_WITH_AES_128_CCM, 0};

    mbedtls_ssl_conf_ciphersuites(&dtlsConn->conf, ciphersuites);

    if ((ret = mbedtls_ssl_conf_psk(&dtlsConn->conf, (uint8_t *)psk, pskLen, (uint8_t *)identity, identityLen)) != 0) {
        dtlsconnection_deinit(dtlsConn);
        lwm2m_free(identity);
        lwm2m_free(psk);
        lwm2m_free(dtlsConn);
        return NULL;
    }
    lwm2m_free(identity);
    lwm2m_free(psk);

    mbedtls_ssl_conf_rng(&dtlsConn->conf, mbedtls_ctr_drbg_random, &dtlsConn->ctr_drbg);
    mbedtls_ssl_set_bio(&dtlsConn->ssl, dtlsConn, dtlsconnection_mbedtls_send, dtlsconnection_mbedtls_recv, NULL);

    mbedtls_ssl_set_timer_cb(&dtlsConn->ssl, &dtlsConn->timer, mbedtls_timing_set_delay, mbedtls_timing_get_delay);

    if ((ret = mbedtls_ssl_setup(&dtlsConn->ssl, &dtlsConn->conf)) != 0) {
        dtlsconnection_deinit(dtlsConn);
        lwm2m_free(dtlsConn);
        return NULL;
    }

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
