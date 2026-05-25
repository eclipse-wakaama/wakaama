/*
 * CoAP message deduplication tracking.
 * RFC7252 section 4.2
 */

#include <stdbool.h>

#include "message_dedup.h"
#include <internals.h>
#include <liblwm2m.h>

void coap_cleanup_message_deduplication_step(coap_msg_dedup_t **message_dedup, const time_t current_time,
                                             time_t *timeout) {
    LOG_DBG("Entering");
    coap_msg_dedup_t *message_dedup_check = *message_dedup;
    coap_msg_dedup_t *message_dedup_check_prev = *message_dedup;
    while (message_dedup_check != NULL) {
        time_t diff = current_time - message_dedup_check->timestamp;
        if (diff >= LWM2M_COAP_MESSAGE_EXCHANGE_LIFETIME) {
            LOG_ARG_DBG("Message %d deduplication period ended", message_dedup_check->mid);
            if (message_dedup_check_prev != *message_dedup) {
                message_dedup_check_prev->next = message_dedup_check->next;
            } else {
                *message_dedup = message_dedup_check->next;
                message_dedup_check_prev = message_dedup_check->next;
            }
            coap_msg_dedup_t *message_dedup_check_next = message_dedup_check->next;
            lwm2m_free(message_dedup_check->full_response);
            lwm2m_free(message_dedup_check);
            message_dedup_check = message_dedup_check_next;
        } else {
            LOG_ARG_DBG("Message %d check deduplication", message_dedup_check->mid);
            time_t message_dedup_timeout;
            if ((message_dedup_timeout =
                     (message_dedup_check->timestamp + LWM2M_COAP_MESSAGE_EXCHANGE_LIFETIME) - current_time) < 0) {
                message_dedup_timeout = 0;
            }
            if (message_dedup_timeout < *timeout) {
                LOG_ARG_DBG("Message %d check again in %ds deduplication", message_dedup_check->mid,
                            message_dedup_timeout);
                *timeout = message_dedup_timeout;
            }
            message_dedup_check_prev = message_dedup_check;
            message_dedup_check = message_dedup_check->next;
        }
    }
}

bool coap_check_message_duplication(coap_msg_dedup_t **message_dedup, const uint16_t mid, const void *session,
                                    uint8_t **response, size_t *response_len) {
    LOG_DBG("Entering");
    coap_msg_dedup_t *message_dedup_check = *message_dedup;
    while (message_dedup_check != NULL) {
        bool is_equal = lwm2m_session_is_equal(message_dedup_check->session, (void *)session, NULL);
        if (message_dedup_check->mid == mid && is_equal) {
            LOG_ARG_DBG("Duplicate, ignore mid %d (session: %p)", mid, session);
            *response = message_dedup_check->full_response;
            *response_len = message_dedup_check->full_response_len;
            return true;
        }
        message_dedup_check = message_dedup_check->next;
    }
    LOG_ARG_DBG("Register mid %d (session: %p) for deduplication check", mid, session);
    /* The message was not received in the past. Remember for future checks. */
    coap_msg_dedup_t *new_message;
    new_message = lwm2m_malloc(sizeof(coap_msg_dedup_t));
    if (new_message == NULL) {
        /* Memory allocation failed, mark packet as duplicate. Further allocations during packet processing would fail
         * anyway. */
        return true;
    }
    memset(new_message, 0, sizeof(coap_msg_dedup_t));
    new_message->mid = mid;
    new_message->session = (void *)session;
    new_message->timestamp = lwm2m_gettime();

    /* Add message id to deduplication list */
    coap_msg_dedup_t *message_dedup_temp = *message_dedup;
    *message_dedup = new_message;
    (*message_dedup)->next = message_dedup_temp;

    return false;
}

bool coap_deduplication_set_response(coap_msg_dedup_t **message_dedup, uint16_t mid, const void *session,
                                     coap_packet_t *coap_response) {
    LOG_DBG("Entering");
    coap_msg_dedup_t *message_dedup_check = coap_deduplication_find(*message_dedup, mid, session);
    if (message_dedup_check != NULL) {
        LOG_ARG_DBG("Set response to message mid %" PRIu16, mid);
        message_dedup_check->full_response_len = message_serialize(coap_response, &message_dedup_check->full_response);
        return true;
    }

    return false;
}

coap_msg_dedup_t *coap_deduplication_find(coap_msg_dedup_t *message_dedup, const uint16_t mid, const void *session) {
    LOG_DBG("Entering");
    coap_msg_dedup_t *message_dedup_check = message_dedup;
    while (message_dedup_check != NULL) {
        bool is_equal = lwm2m_session_is_equal(message_dedup_check->session, (void *)session, NULL);
        if (message_dedup_check->mid == mid && is_equal) {
            LOG_ARG_DBG("Found deduplication data for message mid %" PRIu16, mid);
            return message_dedup_check;
        }
        message_dedup_check = message_dedup_check->next;
    }
    return NULL;
}

void coap_deduplication_free(lwm2m_context_t *ctx) {
    LOG_DBG("Remove and free the whole message deduplication list");
    while (ctx->message_dedup != NULL) {
        coap_msg_dedup_t *msg_dedup;
        msg_dedup = ctx->message_dedup;
        ctx->message_dedup = ctx->message_dedup->next;
        lwm2m_free(msg_dedup->full_response);
        lwm2m_free(msg_dedup);
    }
}
