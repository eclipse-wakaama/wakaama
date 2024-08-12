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
 *    Fabien Fleutot - Please refer to git log
 *    Toby Jaffey - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
 *    Tuve Nordius, Husqvarna Group - Please refer to git log
 *
 *******************************************************************************/
/*
 Copyright (c) 2013, 2014 Intel Corporation

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
     * Neither the name of Intel Corporation nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.

 David Navarro <david.navarro@intel.com>

*/

#ifndef _LWM2M_INTERNALS_H_
#define _LWM2M_INTERNALS_H_

#include "liblwm2m.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "er-coap-13/er-coap-13.h"
#include "reporting.h"

#if LWM2M_LOG_LEVEL != LWM2M_LOG_DISABLED
#include <inttypes.h>

#ifdef _WIN32
#define LWM2M_NEW_LINE "\r\n"
#else
#define LWM2M_NEW_LINE "\n"
#endif

#ifndef LWM2M_LOG_MAX_MSG_TXT_SIZE
#define LWM2M_LOG_MAX_MSG_TXT_SIZE 200
#endif

/* clang-format off */
#define STR_LOGGING_LEVEL(level) \
((level) == LWM2M_LOGGING_DBG ? "DBG" : \
((level) == LWM2M_LOGGING_INFO ? "INFO" : \
((level) == LWM2M_LOGGING_WARN ? "WARN" : \
((level) == LWM2M_LOGGING_ERR ? "ERR" : \
((level) == LWM2M_LOGGING_FATAL ? "FATAL" : \
"unknown")))))
/* clang-format on */

/** Format the message part of a log entry. This function is not thread save.
 * This function should not be called directly. It's used internally in the logging system. */
char *lwm2m_log_fmt_message(const char *fmt, ...);

/** Basic logging macro. Usually this is not called directly. Use the macros for a given level (e.g. LOG_DBG).
 * Supports arguments if the message (including format specifiers) is a string literal. Otherwise the use uf
 * LOG_ARG_DBG (and related macros) are needed.
 * The limitation that the message needs to be a literal string is due to C11 limitations regarding empty __VA_ARGS__
 * preceded by an coma. The workaround is inspired by: https://stackoverflow.com/a/53875012 */
#define LOG_L(LEVEL, ...) LOG_ARG_L_INTERNAL(LEVEL, __VA_ARGS__, '\0')

/* For internal use only. Required for workaround with empty __VA_ARGS__ in C11 */
#define LOG_ARG_L_INTERNAL(LEVEL, FMT, ...) LOG_ARG_L(LEVEL, FMT "%c", __VA_ARGS__)

/** Basic logging macro that supports arguments. Usually this is not called directly. Use the macros for a given level
 * (e.g. LOG_ARG_DBG). */
#define LOG_ARG_L(LEVEL, FMT, ...)                                                                                     \
    do {                                                                                                               \
        char *txt = lwm2m_log_fmt_message(FMT, __VA_ARGS__);                                                           \
        lwm2m_log_handler(LEVEL, txt, __func__, __LINE__, __FILE__);                                                   \
    } while (0)

#define LOG_URI_TO_STRING(URI) uri_logging_to_string(URI)

/* clang-format off */
#define STR_STATUS(S)                                           \
((S) == STATE_DEREGISTERED ? "STATE_DEREGISTERED" :             \
((S) == STATE_REG_HOLD_OFF ? "STATE_REG_HOLD_OFF" :             \
((S) == STATE_REG_PENDING ? "STATE_REG_PENDING" :               \
((S) == STATE_REGISTERED ? "STATE_REGISTERED" :                 \
((S) == STATE_REG_FAILED ? "STATE_REG_FAILED" :                 \
((S) == STATE_REG_UPDATE_PENDING ? "STATE_REG_UPDATE_PENDING" : \
((S) == STATE_REG_UPDATE_NEEDED ? "STATE_REG_UPDATE_NEEDED" :   \
((S) == STATE_REG_FULL_UPDATE_NEEDED ? "STATE_REG_FULL_UPDATE_NEEDED" :   \
((S) == STATE_DEREG_PENDING ? "STATE_DEREG_PENDING" :           \
((S) == STATE_BS_HOLD_OFF ? "STATE_BS_HOLD_OFF" :               \
((S) == STATE_BS_INITIATED ? "STATE_BS_INITIATED" :             \
((S) == STATE_BS_PENDING ? "STATE_BS_PENDING" :                 \
((S) == STATE_BS_FINISHED ? "STATE_BS_FINISHED" :               \
((S) == STATE_BS_FINISHING ? "STATE_BS_FINISHING" :             \
((S) == STATE_BS_FAILING ? "STATE_BS_FAILING" :                 \
((S) == STATE_BS_FAILED ? "STATE_BS_FAILED" :                   \
"Unknown"))))))))))))))))

#define STR_MEDIA_TYPE(M)                                        \
((M) == LWM2M_CONTENT_TEXT ? "LWM2M_CONTENT_TEXT" :              \
((M) == LWM2M_CONTENT_LINK ? "LWM2M_CONTENT_LINK" :              \
((M) == LWM2M_CONTENT_OPAQUE ? "LWM2M_CONTENT_OPAQUE" :          \
((M) == LWM2M_CONTENT_TLV ? "LWM2M_CONTENT_TLV" :                \
((M) == LWM2M_CONTENT_JSON ? "LWM2M_CONTENT_JSON" :              \
((M) == LWM2M_CONTENT_SENML_JSON ? "LWM2M_CONTENT_SENML_JSON" :  \
((M) == LWM2M_CONTENT_CBOR ? "LWM2M_CONTENT_CBOR" :              \
((M) == LWM2M_CONTENT_SENML_CBOR ? "LWM2M_CONTENT_SENML_CBOR" :  \
"Unknown"))))))))

#define STR_DATA_TYPE(t) \
((t) == LWM2M_TYPE_UNDEFINED ? "LWM2M_TYPE_UNDEFINED" : \
((t) == LWM2M_TYPE_OBJECT ? "LWM2M_TYPE_OBJECT" : \
((t) == LWM2M_TYPE_OBJECT_INSTANCE ? "LWM2M_TYPE_OBJECT_INSTANCE" : \
((t) == LWM2M_TYPE_MULTIPLE_RESOURCE ? "LWM2M_TYPE_MULTIPLE_RESOURCE" : \
((t) == LWM2M_TYPE_STRING ? "LWM2M_TYPE_STRING" : \
((t) == LWM2M_TYPE_OPAQUE ? "LWM2M_TYPE_OPAQUE" : \
((t) == LWM2M_TYPE_INTEGER ? "LWM2M_TYPE_INTEGER" : \
((t) == LWM2M_TYPE_UNSIGNED_INTEGER ? "LWM2M_TYPE_UNSIGNED_INTEGER" : \
((t) == LWM2M_TYPE_FLOAT ? "LWM2M_TYPE_FLOAT" : \
((t) == LWM2M_TYPE_BOOLEAN ? "LWM2M_TYPE_BOOLEAN" : \
((t) == LWM2M_TYPE_OBJECT_LINK ? "LWM2M_TYPE_OBJECT_LINK" : \
((t) == LWM2M_TYPE_CORE_LINK ? "LWM2M_TYPE_CORE_LINK" : \
"Unknown"))))))))))))

#define STR_STATE(S)                                \
((S) == STATE_INITIAL ? "STATE_INITIAL" :      \
((S) == STATE_BOOTSTRAP_REQUIRED ? "STATE_BOOTSTRAP_REQUIRED" :      \
((S) == STATE_BOOTSTRAPPING ? "STATE_BOOTSTRAPPING" :  \
((S) == STATE_REGISTER_REQUIRED ? "STATE_REGISTER_REQUIRED" :        \
((S) == STATE_REGISTERING ? "STATE_REGISTERING" :      \
((S) == STATE_READY ? "STATE_READY" :      \
"Unknown"))))))
/* clang-format on */

#define STR_NULL2EMPTY(S) ((const char *)(S) ? (const char *)(S) : "")
#endif

// suppresses warnings about semicolon after empty macro expansion
#define LWM2M_LOG_NOP                                                                                                  \
    do {                                                                                                               \
    } while (0)

#if LWM2M_LOG_LEVEL <= LWM2M_DBG
#define LOG_DBG(...) LOG_L(LWM2M_LOGGING_DBG, __VA_ARGS__)
#define LOG_ARG_DBG(STR, ...) LOG_ARG_L(LWM2M_LOGGING_DBG, STR, __VA_ARGS__)
#else
#define LOG_DBG(...) LWM2M_LOG_NOP
#define LOG_ARG_DBG(STR, ...) LWM2M_LOG_NOP
#endif

#if LWM2M_LOG_LEVEL <= LWM2M_INFO
#define LOG_INFO(...) LOG_L(LWM2M_LOGGING_INFO, __VA_ARGS__)
#define LOG_ARG_INFO(STR, ...) LOG_ARG_L(LWM2M_LOGGING_INFO, STR, __VA_ARGS__)
#else
#define LOG_INFO(...) LWM2M_LOG_NOP
#define LOG_ARG_INFO(STR, ...) LWM2M_LOG_NOP
#endif

#if LWM2M_LOG_LEVEL <= LWM2M_WARN
#define LOG_WARN(...) LOG_L(LWM2M_LOGGING_WARN, __VA_ARGS__)
#define LOG_ARG_WARN(STR, ...) LOG_ARG_L(LWM2M_LOGGING_WARN, STR, __VA_ARGS__)
#else
#define LOG_WARN(...) LWM2M_LOG_NOP
#define LOG_ARG_WARN(STR, ...) LWM2M_LOG_NOP
#endif

#if LWM2M_LOG_LEVEL <= LWM2M_ERR
#define LOG_ERR(...) LOG_L(LWM2M_LOGGING_ERR, __VA_ARGS__)
#define LOG_ARG_ERR(STR, ...) LOG_ARG_L(LWM2M_LOGGING_ERR, STR, __VA_ARGS__)
#else
#define LOG_ERR(...) LWM2M_LOG_NOP
#define LOG_ARG_ERR(STR, ...) LWM2M_LOG_NOP
#endif

#if LWM2M_LOG_LEVEL <= LWM2M_FATAL
#define LOG_FATAL(...) LOG_L(LWM2M_LOGGING_FATAL, __VA_ARGS__)
#define LOG_ARG_FATAL(STR, ...) LOG_ARG_L(LWM2M_LOGGING_FATAL, STR, __VA_ARGS__)
#else
#define LOG_FATAL(...) LWM2M_LOG_NOP
#define LOG_ARG_FATAL(STR, ...) LWM2M_LOG_NOP
#endif

#define LWM2M_DEFAULT_LIFETIME  86400

#ifdef LWM2M_SUPPORT_SENML_CBOR
#define REG_LWM2M_RESOURCE_TYPE ">;rt=\"oma.lwm2m\";ct=112,"
#define REG_LWM2M_RESOURCE_TYPE_LEN 23
#elif defined(LWM2M_SUPPORT_SENML_JSON)
#define REG_LWM2M_RESOURCE_TYPE     ">;rt=\"oma.lwm2m\";ct=110,"
#define REG_LWM2M_RESOURCE_TYPE_LEN 23
#elif defined(LWM2M_SUPPORT_JSON)
#define REG_LWM2M_RESOURCE_TYPE     ">;rt=\"oma.lwm2m\";ct=11543,"
#define REG_LWM2M_RESOURCE_TYPE_LEN 25
#else
#define REG_LWM2M_RESOURCE_TYPE     ">;rt=\"oma.lwm2m\","
#define REG_LWM2M_RESOURCE_TYPE_LEN 17
#endif
#define REG_START           "<"
#define REG_DEFAULT_PATH    "/"

#define REG_OBJECT_MIN_LEN  5   // "</n>,"
#define REG_PATH_END        ">,"
#define REG_VERSION_START   ">;ver="
#define REG_PATH_SEPARATOR  '/'

#define REG_OBJECT_PATH             "<%s/%hu>,"
#define REG_OBJECT_INSTANCE_PATH    "<%s/%hu/%hu>,"

#define URI_REGISTRATION_SEGMENT        "rd"
#define URI_REGISTRATION_SEGMENT_LEN    2
#define URI_BOOTSTRAP_SEGMENT           "bs"
#define URI_BOOTSTRAP_SEGMENT_LEN       2
#define URI_SEND_SEGMENT "dp"
#define URI_SEND_SEGMENT_LEN 2

#define QUERY_STARTER        "?"
#define QUERY_NAME           "ep="
#define QUERY_NAME_LEN       3       // strlen("ep=")
#define QUERY_PCT            "pct="
#define QUERY_PCT_LEN        4
#define QUERY_SMS            "sms="
#define QUERY_SMS_LEN        4
#define QUERY_LIFETIME       "lt="
#define QUERY_LIFETIME_LEN   3
#define QUERY_VERSION        "lwm2m="
#define QUERY_VERSION_LEN    6
#define QUERY_BINDING        "b="
#define QUERY_BINDING_LEN    2
#define QUERY_QUEUE_MODE     "Q"
#define QUERY_QUEUE_MODE_LEN 1
#define QUERY_DELIMITER      "&"
#define QUERY_DELIMITER_LEN  1

#ifdef LWM2M_VERSION_1_0
#define LWM2M_VERSION      "1.0"
#define LWM2M_VERSION_LEN  3
#else
#define LWM2M_VERSION      "1.1"
#define LWM2M_VERSION_LEN  3
#endif

#define QUERY_VERSION_FULL      QUERY_VERSION LWM2M_VERSION
#define QUERY_VERSION_FULL_LEN  QUERY_VERSION_LEN+LWM2M_VERSION_LEN

#define REG_URI_START                    '<'
#define REG_URI_END                      '>'
#define REG_DELIMITER                    ','
#define REG_ATTR_SEPARATOR               ';'
#define REG_ATTR_EQUALS                  '='
#define REG_ATTR_TYPE_KEY                "rt"
#define REG_ATTR_TYPE_KEY_LEN            2
#define REG_ATTR_TYPE_VALUE              "\"oma.lwm2m\""
#define REG_ATTR_TYPE_VALUE_LEN          11
#define REG_ATTR_CONTENT_KEY             "ct"
#define REG_ATTR_CONTENT_KEY_LEN         2
#define REG_ATTR_CONTENT_TLV             "11542"
#define REG_ATTR_CONTENT_TLV_LEN         5
#define REG_ATTR_CONTENT_JSON            "11543"
#define REG_ATTR_CONTENT_JSON_LEN        5
#define REG_ATTR_CONTENT_JSON_OLD        "1543"
#define REG_ATTR_CONTENT_JSON_OLD_LEN    4
#define REG_ATTR_CONTENT_SENML_JSON      "110"
#define REG_ATTR_CONTENT_SENML_JSON_LEN  3
#define REG_ATTR_CONTENT_SENML_CBOR "112"
#define REG_ATTR_CONTENT_SENML_CBOR_LEN 3

#define ATTR_SERVER_ID_STR       "ep="
#define ATTR_SERVER_ID_LEN       3
#define ATTR_MIN_PERIOD_STR      "pmin="
#define ATTR_MIN_PERIOD_LEN      5
#define ATTR_MAX_PERIOD_STR      "pmax="
#define ATTR_MAX_PERIOD_LEN      5
#define ATTR_GREATER_THAN_STR    "gt="
#define ATTR_GREATER_THAN_LEN    3
#define ATTR_LESS_THAN_STR       "lt="
#define ATTR_LESS_THAN_LEN       3
#define ATTR_STEP_STR            "st="
#define ATTR_STEP_LEN            3
#define ATTR_DIMENSION_STR       "dim="
#define ATTR_DIMENSION_LEN       4
#define ATTR_VERSION_STR         "ver="
#define ATTR_VERSION_LEN         4
#define ATTR_SSID_STR            "ssid="
#define ATTR_SSID_LEN            5
#define ATTR_URI_STR             "uri=\""
#define ATTR_URI_LEN             5

#ifdef LWM2M_VERSION_1_0
#define URI_MAX_STRING_LEN    18      // /65535/65535/65535
#else
#define URI_MAX_STRING_LEN    24      // /65535/65535/65535/65535
#endif
#define _PRV_64BIT_BUFFER_SIZE 8

#define LINK_ITEM_START             "<"
#define LINK_ITEM_START_SIZE        1
#define LINK_ITEM_END               ">,"
#define LINK_ITEM_END_SIZE          2
#define LINK_ITEM_DIM_START         ">;dim="
#define LINK_ITEM_DIM_START_SIZE    6
#define LINK_ITEM_ATTR_END          ","
#define LINK_ITEM_ATTR_END_SIZE     1
#define LINK_URI_SEPARATOR          "/"
#define LINK_URI_SEPARATOR_SIZE     1
#define LINK_ATTR_SEPARATOR         ";"
#define LINK_ATTR_SEPARATOR_SIZE    1

#define ATTR_FLAG_NUMERIC (uint8_t)(LWM2M_ATTR_FLAG_LESS_THAN | LWM2M_ATTR_FLAG_GREATER_THAN | LWM2M_ATTR_FLAG_STEP)

typedef struct
{
    uint16_t clientID;
    lwm2m_uri_t uri;
    lwm2m_result_callback_t callback;
    void * userData;
} dm_data_t;

typedef struct
{
    uint16_t                id;
    uint16_t                client;
    lwm2m_uri_t             uri;
    lwm2m_result_callback_t callback;
    void *                  userData;
    lwm2m_context_t *       contextP;
} observation_data_t;

#ifdef LWM2M_BOOTSTRAP_SERVER_MODE
typedef struct
{
    lwm2m_uri_t uri;
    lwm2m_bootstrap_callback_t callback;
    void *      userData;
} bs_data_t;
#endif

typedef enum {
    LWM2M_REQUEST_TYPE_UNKNOWN,
    LWM2M_REQUEST_TYPE_DM,
    LWM2M_REQUEST_TYPE_REGISTRATION,
    LWM2M_REQUEST_TYPE_BOOTSTRAP,
    LWM2M_REQUEST_TYPE_DELETE_ALL,
    LWM2M_REQUEST_TYPE_SEND,
} lwm2m_request_type_t;

#ifdef LWM2M_SUPPORT_SENML_CBOR
typedef enum {
    CBOR_TYPE_UNKNOWN,
    CBOR_TYPE_UNSIGNED_INTEGER,
    CBOR_TYPE_NEGATIVE_INTEGER,
    CBOR_TYPE_FLOAT,
    CBOR_TYPE_SEMANTIC_TAG,
    CBOR_TYPE_SIMPLE_VALUE,
    CBOR_TYPE_BREAK,
    CBOR_TYPE_BYTE_STRING,
    CBOR_TYPE_TEXT_STRING,
    CBOR_TYPE_ARRAY,
    CBOR_TYPE_MAP,
} cbor_type_t;
#endif

#if defined(LWM2M_SUPPORT_JSON) || defined(LWM2M_SUPPORT_SENML_JSON) || defined(LWM2M_SUPPORT_SENML_CBOR)
typedef struct {
    uint16_t ids[4];
    lwm2m_data_t value; /* Any buffer will be within the parsed data */
    time_t time;
} senml_record_t;

typedef bool (*senml_convertValue)(const senml_record_t *recordP, lwm2m_data_t *targetP);
#endif

// defined in uri.c
lwm2m_request_type_t uri_decode(char * altPath, multi_option_t *uriPath, uint8_t code, lwm2m_uri_t *uriP);
int uri_getNumber(uint8_t * uriString, size_t uriLength);

// defined in objects.c
uint8_t object_readData(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, int * sizeP, lwm2m_data_t ** dataP);
uint8_t object_read(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, const uint16_t * accept, uint8_t acceptNum, lwm2m_media_type_t * formatP, uint8_t ** bufferP, size_t * lengthP);
uint8_t object_write(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, lwm2m_media_type_t format, uint8_t * buffer, size_t length, bool partial);
uint8_t object_create(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, lwm2m_media_type_t format, uint8_t * buffer, size_t length);
uint8_t object_execute(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, uint8_t * buffer, size_t length);
#ifdef LWM2M_RAW_BLOCK1_REQUESTS
uint8_t object_raw_block1_write(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, lwm2m_media_type_t format, uint8_t * buffer, size_t length, uint32_t block_num, uint8_t block_more);
uint8_t object_raw_block1_create(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, lwm2m_media_type_t format, uint8_t * buffer, size_t length, uint32_t block_num, uint8_t block_more);
uint8_t object_raw_block1_execute(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, uint8_t * buffer, size_t length, uint32_t block_num, uint8_t block_more);
#endif
uint8_t object_delete(lwm2m_context_t * contextP, lwm2m_uri_t * uriP);
uint8_t object_discover(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, lwm2m_server_t * serverP, uint8_t ** bufferP, size_t * lengthP);
uint8_t object_checkReadable(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, lwm2m_attributes_t * attrP);
bool object_isInstanceNew(lwm2m_context_t * contextP, uint16_t objectId, uint16_t instanceId);
int object_getRegisterPayloadBufferLength(lwm2m_context_t * contextP);
int object_getRegisterPayload(lwm2m_context_t * contextP, uint8_t * buffer, size_t length);
int object_getServers(lwm2m_context_t * contextP, bool checkOnly);
uint8_t object_createInstance(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, lwm2m_data_t * dataP);
uint8_t object_writeInstance(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, lwm2m_data_t * dataP);
#ifndef LWM2M_VERSION_1_0
uint8_t object_readCompositeData(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, size_t numUris, int *sizeP,
                                 lwm2m_data_t **dataP);
#endif

// defined in transaction.c
lwm2m_transaction_t * transaction_new(void * sessionH, coap_method_t method, char * altPath, lwm2m_uri_t * uriP, uint16_t mID, uint8_t token_len, uint8_t* token);
int transaction_send(lwm2m_context_t * contextP, lwm2m_transaction_t * transacP);
void transaction_free(lwm2m_transaction_t * transacP);
void transaction_remove(lwm2m_context_t * contextP, lwm2m_transaction_t * transacP);
bool transaction_handleResponse(lwm2m_context_t * contextP, void * fromSessionH, coap_packet_t * message, coap_packet_t * response);
void transaction_step(lwm2m_context_t * contextP, time_t currentTime, time_t * timeoutP);
bool transaction_free_userData(lwm2m_context_t * context, lwm2m_transaction_t * transaction);
bool transaction_set_payload(lwm2m_transaction_t *transaction, uint8_t *buffer, size_t length);

// defined in management.c
uint8_t dm_handleRequest(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, lwm2m_server_t * serverP, coap_packet_t * message, coap_packet_t * response);

// defined in observe.c
uint8_t observe_handleRequest(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, lwm2m_server_t * serverP, int size, lwm2m_data_t * dataP, coap_packet_t * message, coap_packet_t * response);
void observe_cancel(lwm2m_context_t * contextP, uint16_t mid, void * fromSessionH);
uint8_t observe_setParameters(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, lwm2m_server_t * serverP, lwm2m_attributes_t * attrP);
void observe_step(lwm2m_context_t * contextP, time_t currentTime, time_t * timeoutP);
void observe_clear(lwm2m_context_t * contextP, lwm2m_uri_t * uriP);
bool observe_handleNotify(lwm2m_context_t * contextP, void * fromSessionH, coap_packet_t * message, coap_packet_t * response);
void observe_remove(lwm2m_observation_t * observationP);
lwm2m_observed_t * observe_findByUri(lwm2m_context_t * contextP, lwm2m_uri_t * uriP);

// defined in registration.c
uint8_t registration_handleRequest(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, void * fromSessionH, coap_packet_t * message, coap_packet_t * response);
void registration_deregister(lwm2m_context_t * contextP, lwm2m_server_t * serverP);
void registration_freeClient(lwm2m_client_t * clientP);
uint8_t registration_start(lwm2m_context_t * contextP, bool restartFailed);
void registration_step(lwm2m_context_t * contextP, time_t currentTime, time_t * timeoutP);
lwm2m_status_t registration_getStatus(lwm2m_context_t * contextP);

// defined in packet.c
uint8_t message_send(lwm2m_context_t * contextP, coap_packet_t * message, void * sessionH);

// defined in bootstrap.c
void bootstrap_step(lwm2m_context_t * contextP, time_t currentTime, time_t* timeoutP);
uint8_t bootstrap_handleCommand(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, lwm2m_server_t * serverP, coap_packet_t * message, coap_packet_t * response);
uint8_t bootstrap_handleDeleteAll(lwm2m_context_t * context, void * fromSessionH);
uint8_t bootstrap_handleFinish(lwm2m_context_t * context, void * fromSessionH);
uint8_t bootstrap_handleRequest(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, void * fromSessionH, coap_packet_t * message, coap_packet_t * response);
void bootstrap_start(lwm2m_context_t * contextP);
lwm2m_status_t bootstrap_getStatus(lwm2m_context_t * contextP);

#ifdef LWM2M_SUPPORT_TLV
// defined in tlv.c
int tlv_parse(const uint8_t * buffer, size_t bufferLen, lwm2m_data_t ** dataP);
int tlv_serialize(bool isResourceInstance, int size, lwm2m_data_t * dataP, uint8_t ** bufferP);
#endif

// defined in json.c
#ifdef LWM2M_SUPPORT_JSON
int json_parse(lwm2m_uri_t * uriP, const uint8_t * buffer, size_t bufferLen, lwm2m_data_t ** dataP);
int json_serialize(lwm2m_uri_t * uriP, int size, lwm2m_data_t * tlvP, uint8_t ** bufferP);
#endif

// defined in senml_json.c
#ifdef LWM2M_SUPPORT_SENML_JSON
int senml_json_parse(const lwm2m_uri_t * uriP, const uint8_t * buffer, size_t bufferLen, lwm2m_data_t ** dataP);
int senml_json_serialize(const lwm2m_uri_t * uriP, int size, const lwm2m_data_t * tlvP, uint8_t ** bufferP);
#endif

#ifdef LWM2M_SUPPORT_SENML_CBOR
// defined in cbor.c
int cbor_get_type_and_value(const uint8_t *buffer, size_t bufferLen, cbor_type_t *type, uint64_t *value);
int cbor_get_singular(const uint8_t *buffer, size_t bufferLen, lwm2m_data_t *dataP);
int cbor_put_type_and_value(uint8_t *buffer, size_t bufferLen, cbor_type_t type, uint64_t val);
int cbor_put_singular(uint8_t *buffer, size_t bufferLen, const lwm2m_data_t *dataP);
#if !defined(LWM2M_VERSION_1_0) && !defined(LWM2M_VERSION_1_1)
int cbor_parse(const lwm2m_uri_t *uriP, const uint8_t *buffer, size_t bufferLen, lwm2m_data_t **dataP);
int cbor_serialize(const lwm2m_uri_t *uriP, int size, const lwm2m_data_t *dataP, uint8_t **bufferP);
#endif

// defined in senml_cbor.c
int senml_cbor_parse(const lwm2m_uri_t *uriP, const uint8_t *buffer, size_t bufferLen, lwm2m_data_t **dataP);
int senml_cbor_serialize(const lwm2m_uri_t *uriP, int size, const lwm2m_data_t *tlvP, uint8_t **bufferP);
#endif

// defined in senml_common.c
#if defined(LWM2M_SUPPORT_JSON) || defined(LWM2M_SUPPORT_SENML_JSON) || defined(LWM2M_SUPPORT_SENML_CBOR)
int senml_convert_records(const lwm2m_uri_t *uriP, senml_record_t *recordArray, int numRecords,
                          senml_convertValue convertValue, lwm2m_data_t **dataP);
lwm2m_data_t *senml_extendData(lwm2m_data_t *parentP, lwm2m_data_type_t type, uint16_t id);
int senml_dataStrip(int size, lwm2m_data_t *dataP, lwm2m_data_t **resultP);
lwm2m_data_t *senml_findDataItem(lwm2m_data_t *listP, size_t count, uint16_t id);
uri_depth_t senml_decreaseLevel(uri_depth_t level);
int senml_findAndCheckData(const lwm2m_uri_t *uriP, uri_depth_t baseLevel, size_t size, const lwm2m_data_t *tlvP,
                           lwm2m_data_t **targetP, uri_depth_t *targetLevelP);
#endif

// defined in json_common.c
#if defined(LWM2M_SUPPORT_JSON) || defined(LWM2M_SUPPORT_SENML_JSON)
size_t json_skipSpace(const uint8_t * buffer,size_t bufferLen);
int json_split(const uint8_t * buffer, size_t bufferLen, size_t * tokenStartP, size_t * tokenLenP, size_t * valueStartP, size_t * valueLenP);
int json_itemLength(const uint8_t * buffer, size_t bufferLen);
int json_countItems(const uint8_t * buffer, size_t bufferLen);
int json_convertNumeric(const uint8_t *value, size_t valueLen, lwm2m_data_t *targetP);
int json_convertTime(const uint8_t *valueStart, size_t valueLen, time_t *t);
size_t json_unescapeString(uint8_t *dst, const uint8_t *src, size_t len);
size_t json_escapeString(uint8_t *dst, size_t dstLen, const uint8_t *src, size_t srcLen);
lwm2m_data_t * json_extendData(lwm2m_data_t * parentP);
int json_dataStrip(int size, lwm2m_data_t * dataP, lwm2m_data_t ** resultP);
lwm2m_data_t * json_findDataItem(lwm2m_data_t * listP, size_t count, uint16_t id);
uri_depth_t json_decreaseLevel(uri_depth_t level);
int json_findAndCheckData(const lwm2m_uri_t * uriP, uri_depth_t baseLevel, size_t size, const lwm2m_data_t * tlvP, lwm2m_data_t ** targetP, uri_depth_t *targetLevelP);
#endif

// defined in discover.c
int discover_serialize(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, lwm2m_server_t * serverP, int size, lwm2m_data_t * dataP, uint8_t ** bufferP);

// defined in block.c
#ifdef LWM2M_RAW_BLOCK1_REQUESTS
uint8_t coap_block1_handler(lwm2m_block_data_t **blockData, const char *uri, uint16_t mid, const uint8_t *buffer,
                            size_t length, uint16_t blockSize, uint32_t blockNum, bool blockMore,
                            uint8_t **outputBuffer, size_t *outputLength);
#else
uint8_t coap_block1_handler(lwm2m_block_data_t **blockData, const char *uri, const uint8_t *buffer, size_t length,
                            uint16_t blockSize, uint32_t blockNum, bool blockMore, uint8_t **outputBuffer,
                            size_t *outputLength);
#endif
void block1_delete(lwm2m_block_data_t ** pBlockDataHead, char * uri);
uint8_t coap_block2_handler(lwm2m_block_data_t **blockData, uint16_t mid, const uint8_t *buffer, size_t length,
                            uint16_t blockSize, uint32_t blockNum, bool blockMore, uint8_t **outputBuffer,
                            size_t *outputLength);
void coap_block2_set_expected_mid(lwm2m_block_data_t *blockDataHead, uint16_t currentMid, uint16_t expectedMid);
void free_block_data(lwm2m_block_data_t * blockData);
void block2_delete(lwm2m_block_data_t ** pBlockDataHead, uint16_t mid);

// defined in utils.c
lwm2m_data_type_t utils_depthToDatatype(uri_depth_t depth);
lwm2m_version_t utils_stringToVersion(uint8_t *buffer, size_t length);
lwm2m_binding_t utils_stringToBinding(uint8_t *buffer, size_t length);
lwm2m_media_type_t utils_convertMediaType(coap_content_type_t type);
uint8_t utils_getResponseFormat(uint8_t accept_num,
                                const uint16_t *accept,
                                int numData,
                                const lwm2m_data_t *dataP,
                                bool singleResource,
                                lwm2m_media_type_t *format);
int utils_isAltPathValid(const char * altPath);
int utils_stringCopy(char * buffer, size_t length, const char * str);
size_t utils_intToText(int64_t data, uint8_t * string, size_t length);
size_t utils_uintToText(uint64_t data, uint8_t * string, size_t length);
size_t utils_floatToText(double data, uint8_t * string, size_t length, bool allowExponential);
size_t utils_objLinkToText(uint16_t objectId,
                           uint16_t objectInstanceId,
                           uint8_t * string,
                           size_t length);
int utils_textToInt(const uint8_t * buffer, int length, int64_t * dataP);
int utils_textToUInt(const uint8_t * buffer, int length, uint64_t * dataP);
int utils_textToFloat(const uint8_t * buffer, int length, double * dataP, bool allowExponential);
int utils_textToObjLink(const uint8_t * buffer,
                        int length,
                        uint16_t * objectId,
                        uint16_t * objectInstanceId);
void utils_copyValue(void * dst, const void * src, size_t len);
size_t utils_base64GetSize(size_t dataLen);
size_t utils_base64Encode(const uint8_t * dataP, size_t dataLen, uint8_t * bufferP, size_t bufferLen);
size_t utils_base64GetDecodedSize(const char * dataP, size_t dataLen);
size_t utils_base64Decode(const char * dataP, size_t dataLen, uint8_t * bufferP, size_t bufferLen);
#ifdef LWM2M_CLIENT_MODE
lwm2m_server_t * utils_findServer(lwm2m_context_t * contextP, void * fromSessionH);
lwm2m_server_t * utils_findBootstrapServer(lwm2m_context_t * contextP, void * fromSessionH);
#endif
#if defined(LWM2M_SERVER_MODE) || defined(LWM2M_BOOTSTRAP_SERVER_MODE)
lwm2m_client_t * utils_findClient(lwm2m_context_t * contextP, void * fromSessionH);
#endif

#endif
