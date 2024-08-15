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
#ifndef WAKAAMA_CBOR_H
#define WAKAAMA_CBOR_H

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

int cbor_get_type_and_value(const uint8_t *buffer, size_t bufferLen, cbor_type_t *type, uint64_t *value);
int cbor_get_singular(const uint8_t *buffer, size_t bufferLen, lwm2m_data_t *dataP);
int cbor_put_type_and_value(uint8_t *buffer, size_t bufferLen, cbor_type_t type, uint64_t val);
int cbor_put_singular(uint8_t *buffer, size_t bufferLen, const lwm2m_data_t *dataP);
int cbor_parse(const lwm2m_uri_t *uriP, const uint8_t *buffer, size_t bufferLen, lwm2m_data_t **dataP);
int cbor_serialize(const lwm2m_uri_t *uriP, int size, const lwm2m_data_t *dataP, uint8_t **bufferP);
#endif

#endif /* WAKAAMA_CBOR_H */
