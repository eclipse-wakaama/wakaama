#ifndef _COAP_MESSAGE_DEDUP_H_
#define _COAP_MESSAGE_DEDUP_H_

#include <stdint.h>
#include <time.h>

#include <liblwm2m.h>

/*
 * EXCHANGE_LIFETIME https://datatracker.ietf.org/doc/html/rfc7252#section-4.8.2
 * Time to block message ids from a client to prevent receiving duplicate packets.
 * Special value for Lemonbeat transmissions: Expected worst case latency per packet: 1s.
 * EXCHANGE_LIFETIME = (ACK_TIMEOUT * ((2 ** MAX_RETRANSMIT) - 1) + LEMONBEAT_DELAY) * ACK_RANDOM_FACTOR
 * With ACK_RANDOM_FACTOR=1.5, LEMONBEAT_DELAY=1, MAX_RETRANSMIT=4
 * (2 * ((2 ** 4) - 1) + 1) * 1.5 = 46.5 -> ~47s
 */
#define EXCHANGE_LIFETIME 47

/**
 * Cleanup message ids after EXCHANGE_LIFETIME.
 * @param message_dedup list of message ids for deduplication
 * @param current_time current timestamp
 * @param timeout next timeout in main loop
 */
void coap_cleanup_message_deduplication_step(coap_msg_dedup_t **message_dedup, time_t current_time, time_t *timeout);

/**
 * Check whether a message was already received.
 * @param message_dedup list of message ids for deduplication
 * @param mid message id
 * @param session pointer to the session the message was received from
 * @return true if the message was already seen within in the EXCHANGE_LIFETIME window, false otherwise.
 */
bool coap_check_message_duplication(coap_msg_dedup_t **message_dedup, uint16_t mid, const void *session);

/**
 * Remove and free the whole message deduplication list
 * @param ctx lwm2m context
 */
void coap_deduplication_free(lwm2m_context_t *ctx);

#endif // _COAP_MESSAGE_DEDUP_H_
