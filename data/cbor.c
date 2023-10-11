/*******************************************************************************
 *
 * Copyright (c) 2019 Telular, a business unit of AMETEK, Inc.
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
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
 *
 *******************************************************************************/
#include <float.h>
#include <math.h>

#include "internals.h"
#include <assert.h>

#ifdef LWM2M_SUPPORT_SENML_CBOR

/*
 * Ensure type assumptions taken are correct on the platform.
 */
#if (__STDC_VERSION__ >= 201112L)
static_assert(sizeof(float) == sizeof(uint32_t), "sizeof(float) != sizeof(uint32_t)");
static_assert(sizeof(double) == sizeof(uint64_t), "sizeof(double) != sizeof(uint64_t)");
#endif

#ifdef LWM2M_VERSION_1_0
#error CBOR not supported with LWM2M 1.0
#endif

#define CBOR_UNSIGNED_INTEGER 0
#define CBOR_NEGATIVE_INTEGER 1
#define CBOR_BYTE_STRING 2
#define CBOR_TEXT_STRING 3
#define CBOR_ARRAY 4
#define CBOR_MAP 5
#define CBOR_SEMANTIC_TAG 6
#define CBOR_FLOATING_OR_SIMPLE 7

#define CBOR_AI_ONE_BYTE_VALUE 24
#define CBOR_AI_TWO_BYTE_VALUE 25
#define CBOR_AI_FOUR_BYTE_VALUE 26
#define CBOR_AI_EIGHT_BYTE_VALUE 27
#define CBOR_AI_INDEFINITE_OR_BREAK 31

#define CBOR_SIMPLE_FALSE 20
#define CBOR_SIMPLE_TRUE 21
#define CBOR_SIMPLE_NULL 22
#define CBOR_SIMPLE_UNDEFINED 23

int cbor_get_type_and_value(const uint8_t *buffer, size_t bufferLen, cbor_type_t *type, uint64_t *value) {
    uint8_t mt;
    uint8_t ai;
    uint64_t val;
    double dval;
    int result = 1;

    if (bufferLen < 1)
        return -1;

    mt = buffer[0] >> 5;
    ai = buffer[0] & 0x1f;

    switch (ai) {
    default:
        val = ai;
        break;
    case CBOR_AI_ONE_BYTE_VALUE:
        if (bufferLen < 2)
            return -1;
        val = buffer[1];
        result += 1;
        break;
    case CBOR_AI_TWO_BYTE_VALUE:
        if (bufferLen < 3)
            return -1;
        uint16_t val16;
        utils_copyValue(&val16, &buffer[1], 2);
        val = val16;
        result += 2;
        break;
    case CBOR_AI_FOUR_BYTE_VALUE:
        if (bufferLen < 5)
            return -1;
        uint32_t val32;
        utils_copyValue(&val32, &buffer[1], 4);
        val = val32;
        result += 4;
        break;
    case CBOR_AI_EIGHT_BYTE_VALUE:
        if (bufferLen < 9)
            return -1;
        utils_copyValue(&val, &buffer[1], 8);
        result += 8;
        break;
    case 28:
    case 29:
    case 30:
        // Not defined
        return -1;
    }

    switch (mt) {
    case CBOR_UNSIGNED_INTEGER:
        *type = CBOR_TYPE_UNSIGNED_INTEGER;
        *value = val;
        break;
    case CBOR_NEGATIVE_INTEGER:
        if (val >> 63 != 0)
            return -1; // Can't convert properly
        *type = CBOR_TYPE_NEGATIVE_INTEGER;
        *value = ~val;
        break;
    case CBOR_BYTE_STRING:
        *type = CBOR_TYPE_BYTE_STRING;
        *value = val;
        break;
    case CBOR_TEXT_STRING:
        *type = CBOR_TYPE_TEXT_STRING;
        *value = val;
        break;
    case CBOR_ARRAY:
        *type = CBOR_TYPE_ARRAY;
        *value = val;
        break;
    case CBOR_MAP:
        *type = CBOR_TYPE_MAP;
        *value = val;
        break;
    case CBOR_SEMANTIC_TAG:
        *type = CBOR_TYPE_SEMANTIC_TAG;
        *value = val;
        break;
    case CBOR_FLOATING_OR_SIMPLE:
        switch (ai) {
        default:
            *type = CBOR_TYPE_SIMPLE_VALUE;
            *value = val;
            break;
        case CBOR_AI_TWO_BYTE_VALUE: {
            uint16_t half = val;
            int exp = (half >> 10) & 0x1f;
            int mant = half & 0x3ff;
            if (exp == 0) {
                dval = ldexp(mant, -24);
            } else if (exp != 31) {
                dval = ldexp(mant + 1024, exp - 25);
            } else if (mant == 0) {
                dval = INFINITY;
            } else {
                dval = NAN;
            }
            if ((half & 0x8000) != 0) {
                dval = -dval;
            }
        }
            *type = CBOR_TYPE_FLOAT;
            memcpy(value, &dval, sizeof(dval));
            break;
        case CBOR_AI_FOUR_BYTE_VALUE: {
            float fval;
            uint32_t val32 = (uint32_t)val;
            memcpy(&fval, &val32, sizeof(fval));
            dval = fval;
        }
            *type = CBOR_TYPE_FLOAT;
            memcpy(value, &dval, sizeof(dval));
            break;
        case CBOR_AI_EIGHT_BYTE_VALUE:
            *type = CBOR_TYPE_FLOAT;
            *value = val;
            break;
        case CBOR_AI_INDEFINITE_OR_BREAK:
            *type = CBOR_TYPE_BREAK;
            *value = 0;
            break;
        }
        break;
    default:
        // should not be possible
        return -1;
    }

    return result;
}

int cbor_get_singular(const uint8_t *buffer, size_t bufferLen, lwm2m_data_t *dataP) {
    cbor_type_t mt;
    uint64_t val;
    int result = 0;
    int res;
    bool tagSeen = false;
    bool skipTag;

    do {
        skipTag = false;
        dataP->type = LWM2M_TYPE_UNDEFINED;
        res = cbor_get_type_and_value(buffer + result, bufferLen - result, &mt, &val);
        if (res < 1)
            return -1;
        result += res;

        switch (mt) {
        case CBOR_TYPE_UNSIGNED_INTEGER:
            dataP->type = LWM2M_TYPE_UNSIGNED_INTEGER;
            dataP->value.asUnsigned = val;
            break;
        case CBOR_TYPE_NEGATIVE_INTEGER:
            dataP->type = LWM2M_TYPE_INTEGER;
            dataP->value.asInteger = (int64_t)val;
            break;
        case CBOR_TYPE_FLOAT:
            dataP->type = LWM2M_TYPE_FLOAT;
            memcpy(&dataP->value.asFloat, &val, sizeof(val));
            break;
        case CBOR_TYPE_SEMANTIC_TAG:
            // Only 1 tag allowed.
            if (tagSeen)
                return -1;
            tagSeen = true;
            // Ignore the tag and get the tagged value.
            skipTag = true;
            break;
        case CBOR_TYPE_SIMPLE_VALUE:
            switch (val) {
            case CBOR_SIMPLE_FALSE:
                dataP->type = LWM2M_TYPE_BOOLEAN;
                dataP->value.asBoolean = false;
                break;
            case CBOR_SIMPLE_TRUE:
                dataP->type = LWM2M_TYPE_BOOLEAN;
                dataP->value.asBoolean = true;
                break;
            default:
                // Not valid.
                return -1;
            }
            break;
        case CBOR_TYPE_BREAK:
            // Indefinite lengths not supported.
            return -1;
        case CBOR_TYPE_BYTE_STRING:
            if (val > bufferLen - result)
                return -1;
            dataP->type = LWM2M_TYPE_OPAQUE;
            dataP->value.asBuffer.length = val;
            if (val > 0) {
                dataP->value.asBuffer.buffer = (uint8_t *)buffer + result;
            } else {
                dataP->value.asBuffer.buffer = NULL;
            }
            result += val;
            break;
        case CBOR_TYPE_TEXT_STRING:
            if (val > bufferLen - result)
                return -1;
            dataP->type = LWM2M_TYPE_STRING;
            dataP->value.asBuffer.length = val;
            if (val > 0) {
                dataP->value.asBuffer.buffer = (uint8_t *)buffer + result;
            } else {
                dataP->value.asBuffer.buffer = NULL;
            }
            result += val;
            break;
        case CBOR_TYPE_ARRAY:
        case CBOR_TYPE_MAP:
            // Not valid for a singular resource or resource instance.
            return -1;
        default:
            // Not valid.
            return -1;
        }
    } while (skipTag);

    return result;
}

/**
 * Serialize CBOR values from internal representation into a buffer.
 * @param buffer output buffer
 * @param bufferLen output buffer length
 * @param mt CBOR major type
 * @param val
 * @return 0 on error, else number of bytes
 */
static int prv_serialize_value(uint8_t *buffer, size_t bufferLen, uint8_t mt, uint64_t val) {
    LOG_ARG("bufferLen: %d, mt: %d, val: %d (0x%x)", bufferLen, mt, val, val);
    int buffer_index = 0;
    uint8_t ai = CBOR_AI_EIGHT_BYTE_VALUE;

    /* Calculate additional information (ai) bits. */
    if (val < 0xff) {
        if ((uint8_t)val < 24) {
            /* Value fits in a single byte. */
            if (bufferLen < 1)
                return 0;
            ai = (uint8_t)val;
        } else {
            /* Value needs an extra byte. */
            if (bufferLen < 2)
                return 0;
            ai = CBOR_AI_ONE_BYTE_VALUE;
        }
    } else if (val < 0xffff) {
        /* Value needs two extra bytes. */
        if (bufferLen < 3)
            return 0;
        ai = CBOR_AI_TWO_BYTE_VALUE;
    } else if (val < 0xffffffff) {
        /* Value needs four extra bytes. */
        if (bufferLen < 5)
            return 0;
        ai = CBOR_AI_FOUR_BYTE_VALUE;
    } else if (bufferLen < 9)
        return 0;

    LOG_ARG("ai: %d", ai);

    /* Serialize values according to */
    buffer[buffer_index++] = (mt << 5) | ai;
    switch (ai) {
    case CBOR_AI_EIGHT_BYTE_VALUE:
        utils_copyValue(&buffer[buffer_index], &val, 8);
        buffer_index += 8;
        break;
    case CBOR_AI_FOUR_BYTE_VALUE: {
        int32_t val32 = (int32_t)val;
        utils_copyValue(&buffer[buffer_index], &val32, 4);
        buffer_index += 4;
        break;
    }
    case CBOR_AI_TWO_BYTE_VALUE: {
        int16_t val16 = (int16_t)val;
        utils_copyValue(&buffer[buffer_index], &val16, 2);
        buffer_index += 2;
        break;
    }
    case CBOR_AI_ONE_BYTE_VALUE:
        buffer[buffer_index] = (uint8_t)val;
        buffer_index += 1;
        break;
    default:
        break;
    }

    return buffer_index;
}

/**
 * Serialize CBOR float values from internal representation into a buffer.
 * @param buffer output buffer
 * @param bufferLen output buffer length
 * @param mt CBOR major type
 * @param val
 * @return 0 on error, else number of bytes
 */
static int prv_serialize_float(uint8_t *buffer, size_t bufferLen, double val) {
    int result = 0;
    float fval = (float)val;

    if (bufferLen < 3)
        return 0;
    _Pragma("GCC diagnostic push");
    _Pragma("GCC diagnostic ignored \"-Wfloat-equal\"");
    if (val != val) {
        // NaN
        buffer[0] = (CBOR_FLOATING_OR_SIMPLE << 5) | CBOR_AI_TWO_BYTE_VALUE;
        buffer[1] = 0x7e;
        buffer[2] = 0;
        result = 3;
    } else if (fval == val) {
        uint32_t uval;
        memcpy(&uval, &fval, sizeof(fval));
#ifndef CBOR_NO_FLOAT16_ENCODING
        // Single or half precision
        uint32_t mant = uval & 0x7FFFFF;
        int32_t exp = ((uval >> 23) & 0xFF) - 127;
        uint8_t sign = (uint8_t)((uval & 0x80000000u) >> 24);
        if (exp == 0xFF - 127) {
            // Infinity
            buffer[0] = (CBOR_FLOATING_OR_SIMPLE << 5) | CBOR_AI_TWO_BYTE_VALUE;
            buffer[1] = sign | 0x7c;
            buffer[2] = 0;
            result = 3;
        } else if (mant == 0 && exp == -127) {
            // Positive or negative 0
            buffer[0] = (CBOR_FLOATING_OR_SIMPLE << 5) | CBOR_AI_TWO_BYTE_VALUE;
            buffer[1] = sign;
            buffer[2] = 0;
            result = 3;
        } else if ((mant & 0x7FF) == 0 && exp >= -14 && exp <= 15) {
            // Normalized half precision
            exp = exp + 15;
            mant >>= 13;
            buffer[0] = (CBOR_FLOATING_OR_SIMPLE << 5) | CBOR_AI_TWO_BYTE_VALUE;
            buffer[1] = sign | ((exp << 2) & 0x7C) | ((mant >> 8) & 0x3);
            buffer[2] = mant & 0xFF;
            result = 3;
        } else if (exp >= -24 && exp <= -14 && (mant & ((1 << (1 - exp)) - 1)) == 0) {
            // Denormalized half precision
            mant = (mant + 0x800000) >> (-1 - exp);
            buffer[0] = (CBOR_FLOATING_OR_SIMPLE << 5) | CBOR_AI_TWO_BYTE_VALUE;
            buffer[1] = sign | ((mant >> 8) & 0x3);
            buffer[2] = mant & 0xFF;
            result = 3;
        } else {
#else
        {
#endif
            /* Serialize a single precision value. */
            if (bufferLen < 5)
                return 0;

            buffer[0] = (CBOR_FLOATING_OR_SIMPLE << 5) | CBOR_AI_FOUR_BYTE_VALUE;
            utils_copyValue(&buffer[1], &uval, 4);
            result = 5;
        }
    } else {
        /* Serialize a double precision value. */
        uint64_t uval;
        if (bufferLen < 9)
            return 0;
        memcpy(&uval, &val, sizeof(val));
        buffer[0] = (CBOR_FLOATING_OR_SIMPLE << 5) | CBOR_AI_EIGHT_BYTE_VALUE;
        utils_copyValue(&buffer[1], &uval, 8);
        result = 9;
    }
    _Pragma("GCC diagnostic pop");
    return result;
}

int cbor_put_type_and_value(uint8_t *buffer, size_t bufferLen, cbor_type_t type, uint64_t val) {
    uint8_t mt;

    if (bufferLen < 1)
        return 0;

    if (type == CBOR_TYPE_NEGATIVE_INTEGER)
        val = ~val;
    if (type == CBOR_TYPE_FLOAT) {
        double dval;
        memcpy(&dval, &val, sizeof(val));
        return prv_serialize_float(buffer, bufferLen, dval);
    }

    switch (type) {
    default:
        return 0;
    case CBOR_TYPE_UNSIGNED_INTEGER:
        mt = CBOR_UNSIGNED_INTEGER;
        break;
    case CBOR_TYPE_NEGATIVE_INTEGER:
        mt = CBOR_NEGATIVE_INTEGER;
        break;
    case CBOR_TYPE_SEMANTIC_TAG:
        mt = CBOR_SEMANTIC_TAG;
        break;
    case CBOR_TYPE_SIMPLE_VALUE:
        if (val > 255)
            return 0;
        mt = CBOR_FLOATING_OR_SIMPLE;
        break;
    case CBOR_TYPE_BREAK:
        buffer[0] = CBOR_FLOATING_OR_SIMPLE | 31;
        return 1;
    case CBOR_TYPE_BYTE_STRING:
        mt = CBOR_BYTE_STRING;
        break;
    case CBOR_TYPE_TEXT_STRING:
        mt = CBOR_TEXT_STRING;
        break;
    case CBOR_TYPE_ARRAY:
        mt = CBOR_ARRAY;
        break;
    case CBOR_TYPE_MAP:
        mt = CBOR_MAP;
        break;
    }

    return prv_serialize_value(buffer, bufferLen, mt, val);
}

int cbor_put_singular(uint8_t *buffer, size_t bufferLen, const lwm2m_data_t *dataP) {
    LOG_ARG("bufferLen: %d, dataType: %s", bufferLen, STR_DATA_TYPE(dataP->type));

    int result = 0;
    int res = 0;

    switch (dataP->type) {
    case LWM2M_TYPE_UNDEFINED:
    case LWM2M_TYPE_OBJECT:
    case LWM2M_TYPE_OBJECT_INSTANCE:
    case LWM2M_TYPE_MULTIPLE_RESOURCE:
    default:
        // Not valid
        return 0;

    case LWM2M_TYPE_STRING:
    case LWM2M_TYPE_CORE_LINK:
    case LWM2M_TYPE_OPAQUE:
        res = prv_serialize_value(buffer + result, bufferLen - result,
                                  (LWM2M_TYPE_OPAQUE == dataP->type ? CBOR_BYTE_STRING : CBOR_TEXT_STRING),
                                  dataP->value.asBuffer.length);
        if (dataP->value.asBuffer.length > 0) {
            if (res <= 0)
                return 0;
            result += res;
            memcpy(buffer + result, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
            res = dataP->value.asBuffer.length;
        }
        break;
    case LWM2M_TYPE_INTEGER:
        if (dataP->value.asInteger < 0) {
            res = prv_serialize_value(buffer + result, bufferLen - result, CBOR_NEGATIVE_INTEGER,
                                      ~dataP->value.asInteger);
            break;
        }
        // fall through
    case LWM2M_TYPE_UNSIGNED_INTEGER:
        res = prv_serialize_value(buffer + result, bufferLen - result, CBOR_UNSIGNED_INTEGER, dataP->value.asUnsigned);
        break;
    case LWM2M_TYPE_FLOAT:
        res = prv_serialize_float(buffer + result, bufferLen - result, dataP->value.asFloat);
        break;
    case LWM2M_TYPE_BOOLEAN:
        res = prv_serialize_value(buffer + result, bufferLen - result, CBOR_FLOATING_OR_SIMPLE,
                                  dataP->value.asBoolean ? CBOR_SIMPLE_TRUE : CBOR_SIMPLE_FALSE);
        break;
    case LWM2M_TYPE_OBJECT_LINK:
        res = snprintf((char *)buffer + result + 1, bufferLen - result - 1, "%u:%u", dataP->value.asObjLink.objectId,
                       dataP->value.asObjLink.objectInstanceId);
        if ((int)(bufferLen - result - 1) > res) {
            if (prv_serialize_value(buffer + result, bufferLen - result, CBOR_TEXT_STRING, res) == 1) {
                res += 1;
            } else {
                res = 0;
            }
        } else {
            res = 0;
        }
        break;
    }

    if (res <= 0)
        return 0;
    result += res;

    return result;
}

#if !defined(LWM2M_VERSION_1_1)
int cbor_parse(const lwm2m_uri_t *uriP, const uint8_t *buffer, size_t bufferLen, lwm2m_data_t **dataP) {
    int result = 0;
    uint8_t *tmp;

    LOG_ARG("bufferLen: %d", bufferLen);
    LOG_URI(uriP);

    if (!uriP || (uriP && !LWM2M_URI_IS_SET_RESOURCE(uriP)))
        return 0;

    *dataP = lwm2m_data_new(1);
    if (*dataP == NULL)
        return 0;

    if (LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP)) {
        (*dataP)->id = uriP->resourceInstanceId;
    } else {
        (*dataP)->id = uriP->resourceId;
    }

    if (cbor_get_singular(buffer, bufferLen, *dataP) == (int)bufferLen) {
        switch ((*dataP)->type) {
        case LWM2M_TYPE_UNDEFINED:
        case LWM2M_TYPE_OBJECT:
        case LWM2M_TYPE_OBJECT_INSTANCE:
        case LWM2M_TYPE_MULTIPLE_RESOURCE:
            // Not valid for a singular resource
            break;
        default:
            // Success
            result = 1;
            break;
        case LWM2M_TYPE_STRING:
        case LWM2M_TYPE_OPAQUE:
        case LWM2M_TYPE_CORE_LINK:
            // Buffer is within the input buffer. Need to duplicate it.
            tmp = (*dataP)->value.asBuffer.buffer;
            (*dataP)->value.asBuffer.buffer = lwm2m_malloc((*dataP)->value.asBuffer.length);
            if ((*dataP)->value.asBuffer.buffer != NULL) {
                if (tmp != NULL) {
                    memcpy((*dataP)->value.asBuffer.buffer, tmp, (*dataP)->value.asBuffer.length);
                }
                result = 1;
            }
            break;
        }
    }
    if (result == 0) {
        lwm2m_data_free(1, *dataP);
        *dataP = NULL;
    }

    return result;
}

int cbor_serialize(const lwm2m_uri_t *uriP, int size, const lwm2m_data_t *dataP, uint8_t **bufferP) {
    LOG_URI(uriP);
    LOG_ARG("size: %d, dataType: %s", size, STR_DATA_TYPE(dataP->type));

    int result = 0;
    int res;
    uint8_t tmp[13];

    LOG_URI(uriP);
    (void)uriP;

    *bufferP = NULL;

    if (size != 1)
        return 0;

    switch (dataP->type) {
    case LWM2M_TYPE_STRING:
    case LWM2M_TYPE_CORE_LINK:
    case LWM2M_TYPE_OPAQUE:
        res = prv_serialize_value(tmp, sizeof(tmp),
                                  (LWM2M_TYPE_OPAQUE == dataP->type ? CBOR_BYTE_STRING : CBOR_TEXT_STRING),
                                  dataP->value.asBuffer.length);
        if (res > 0) {
            *bufferP = lwm2m_malloc(res + dataP->value.asBuffer.length);
            if (*bufferP != NULL) {
                result = res + dataP->value.asBuffer.length;
                memcpy(*bufferP, tmp, res);
                memcpy((*bufferP) + res, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
            }
        }
        break;
    default:
        res = cbor_put_singular(tmp, sizeof(tmp), dataP);
        if (res > 0) {
            *bufferP = lwm2m_malloc(res);
            if (*bufferP != NULL) {
                result = res;
                memcpy(*bufferP, tmp, res);
            }
        }
        break;
    }

    return result;
}
#endif

#endif
