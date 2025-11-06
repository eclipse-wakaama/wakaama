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
#ifndef WAKAAMA_UTILS_H
#define WAKAAMA_UTILS_H
#include "internals.h"

lwm2m_data_type_t utils_depthToDatatype(uri_depth_t depth);
lwm2m_version_t utils_stringToVersion(uint8_t *buffer, size_t length);
lwm2m_binding_t utils_stringToBinding(uint8_t *buffer, size_t length);
lwm2m_media_type_t utils_convertMediaType(coap_content_type_t type);
uint8_t utils_getResponseFormat(uint8_t accept_num, const uint16_t *accept, int numData, const lwm2m_data_t *dataP,
                                bool singleResource, lwm2m_media_type_t *format);
int utils_isAltPathValid(const char *altPath);
/** Copy a string.
 *
 * Use utils_strncpy() if possible!
 * This is less safe than utils_strncpy().
 *
 * @param buffer The destination buffer
 * @param length The max number of bytes to be written to in the destination buffer
 * @param str The source string (needs to be NULL-terminated)
 * @return The number of bytes written
 */
int utils_stringCopy(char *buffer, size_t length, const char *str);
size_t utils_intToText(int64_t data, uint8_t *string, size_t length);
size_t utils_uintToText(uint64_t data, uint8_t *string, size_t length);
size_t utils_floatToText(double data, uint8_t *string, size_t length, bool allowExponential);
size_t utils_objLinkToText(uint16_t objectId, uint16_t objectInstanceId, uint8_t *string, size_t length);
int utils_textToInt(const uint8_t *buffer, int length, int64_t *dataP);
int utils_textToUInt(const uint8_t *buffer, int length, uint64_t *dataP);
int utils_textToFloat(const uint8_t *buffer, int length, double *dataP, bool allowExponential);
int utils_textToObjLink(const uint8_t *buffer, int length, uint16_t *objectId, uint16_t *objectInstanceId);
void utils_copyValue(void *dst, const void *src, size_t len);
size_t utils_base64GetSize(size_t dataLen);
size_t utils_base64Encode(const uint8_t *dataP, size_t dataLen, uint8_t *bufferP, size_t bufferLen);
size_t utils_base64GetDecodedSize(const char *dataP, size_t dataLen);
size_t utils_base64Decode(const char *dataP, size_t dataLen, uint8_t *bufferP, size_t bufferLen);
#ifdef LWM2M_CLIENT_MODE
lwm2m_server_t *utils_findServer(lwm2m_context_t *contextP, void *fromSessionH);
lwm2m_server_t *utils_findBootstrapServer(lwm2m_context_t *contextP, void *fromSessionH);
#endif
#if defined(LWM2M_SERVER_MODE) || defined(LWM2M_BOOTSTRAP_SERVER_MODE)
lwm2m_client_t *utils_findClient(lwm2m_context_t *contextP, void *fromSessionH);
#endif
/** A safer version of `strlen`. Finds the number of chars (bytes) of the given string up to a given max. length.
 *
 * The behaviour is undefined if the maximum length value is bigger than the actually reserved memory behind the @a str
 * buffer.
 *
 * @param str The null-terminated string to find the length of.
 * @param max_size Max size of string.
 * @return The size of the string if it is smaller or equal to max_size. Otherwise max_size.
 */
size_t utils_strnlen(const char *str, size_t max_size);

/** A safer version of `strncpy`. Copies at most src_size bytes to dest, but checks for available space.
 *
 * If dest is not NULL and dest_size is not 0, then the destination buffer is always terminated with '\0'.
 *
 * @param dest The char buffer where to copy the string.
 * @param dest_size The size of the destination buffer.
 * @param src The source string.
 * @param src_size The size of the source string buffer. The effective source string can be shorter.
 */
size_t utils_strncpy(char *dest, const size_t dest_size, const char *src, const size_t src_size);

#endif /* WAKAAMA_UTILS_H */
