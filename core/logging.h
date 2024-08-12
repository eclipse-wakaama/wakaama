/*******************************************************************************
 *
 * Copyright (c) 2024 GARDENA GmbH
 *
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
 *   Lukas Woodtli, GARDENA GmbH - Please refer to git log
 *
 *******************************************************************************/
#ifndef WAKAAMA_LOGGING_H
#define WAKAAMA_LOGGING_H

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

#endif /* WAKAAMA_LOGGING_H */
