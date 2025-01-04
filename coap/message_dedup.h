#ifndef _COAP_MESSAGE_DEDUP_H_
#define _COAP_MESSAGE_DEDUP_H_

#include <stdint.h>
#include <time.h>

#include <liblwm2m.h>

typedef struct _coap_msg_dedup_ {
    struct _coap_msg_dedup_ *next;
    uint16_t mid;
    void *session;
    uint8_t coap_response_code;
    time_t timestamp;
} coap_msg_dedup_t;

/**
 * Cleanup message ids after EXCHANGE_LIFETIME.
 * @param message_dedup list of message ids for deduplication
 * @param current_time current timestamp
 * @param timeout next timeout in main loop
 */
void coap_cleanup_message_deduplication_step(coap_msg_dedup_t **message_dedup, time_t current_time, time_t *timeout);

/**
 * Check whether a message was already received. Add new messages to the deduplication tracking list.
 * @param message_dedup list of message ids for deduplication
 * @param mid message id
 * @param session pointer to the session the message was received from
 * @param coap_response_code CoAP response code to be used for answering duplicate messages
 * @return true if the message was already seen within in the EXCHANGE_LIFETIME window, false otherwise.
 */
bool coap_check_message_duplication(coap_msg_dedup_t **message_dedup, uint16_t mid, const void *session,
                                    uint8_t *coap_response_code);

/**
 * Set response code to be used in acks to a duplicate message. Acknowledgements to duplicate messages must have the
 * same CoAP return code as the relies to the first received message.
 * @param message_dedup list of message ids for deduplication
 * @param mid message id
 * @param session pointer to the session the message was received from
 * @param coap_response_code CoAP response code to be used for answering duplicate messages
 * @return false if no matching message was found, this is an internal error and should not happen
 */
bool coap_deduplication_set_response_code(coap_msg_dedup_t **message_dedup, uint16_t mid, const void *session,
                                          uint8_t coap_response_code);

/**
 * Remove and free the whole message deduplication list
 * @param ctx lwm2m context
 */
void coap_deduplication_free(lwm2m_context_t *ctx);

#endif // _COAP_MESSAGE_DEDUP_H_
