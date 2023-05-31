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
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
 *
 *******************************************************************************/

#include "internals.h"

#ifdef LWM2M_SUPPORT_SENML_CBOR

#ifdef LWM2M_VERSION_1_0
#error SenML CBOR not supported with LWM2M 1.0
#endif

#define PRV_CBOR_BUFFER_SIZE 1024

#define SENML_CBOR_BASE_SUM_LABEL -6
#define SENML_CBOR_BASE_VALUE_LABEL -5
#define SENML_CBOR_BASE_UNIT_LABEL -4
#define SENML_CBOR_BASE_TIME_LABEL -3
#define SENML_CBOR_BASE_NAME_LABEL -2
#define SENML_CBOR_BASE_VERSION_LABEL -1
#define SENML_CBOR_NAME_LABEL 0
#define SENML_CBOR_UNIT_LABEL 1
#define SENML_CBOR_NUMERIC_VALUE_LABEL 2
#define SENML_CBOR_STRING_VALUE_LABEL 3
#define SENML_CBOR_BOOLEAN_VALUE_LABEL 4
#define SENML_CBOR_SUM_LABEL 5
#define SENML_CBOR_TIME_LABEL 6
#define SENML_CBOR_UPDATE_TIME_LABEL 7
#define SENML_CBOR_DATA_VALUE_LABEL 8
// From string labels
#define SENML_CBOR_UNKNOWN_LABEL 9
#define SENML_CBOR_OBJECT_LINK_LABEL 10

// Same as cbor_get_type_and_value, but ignores a semantic tag
static int prv_get_cbor_type_and_value(const uint8_t *buffer, size_t bufferLen, cbor_type_t *cborTypeP,
                                       uint64_t *valP) {
    int res;
    int result = 0;
    res = cbor_get_type_and_value(buffer, bufferLen, cborTypeP, valP);
    if (res <= 0)
        return res;
    result += res;
    if (*cborTypeP == CBOR_TYPE_SEMANTIC_TAG) {
        // Just ignore the tag
        res = cbor_get_type_and_value(buffer + result, bufferLen - result, cborTypeP, valP);
        if (res <= 0)
            return res;
        result += res;
    }
    return result;
}

static int prv_parseItem(const uint8_t *buffer, size_t bufferLen, senml_record_t *recordP, char *baseUri,
                         time_t *baseTime, lwm2m_data_t *baseValue) {
    int res;
    int offset;
    cbor_type_t cborType;
    uint64_t val;
    int count;
    const uint8_t *name = NULL;
    size_t nameLength = 0;
    bool timeSeen = false;
    bool bnSeen = false;
    bool btSeen = false;
    bool bvSeen = false;
    bool bverSeen = false;

    offset = 0;
    res = prv_get_cbor_type_and_value(buffer, bufferLen, &cborType, &val);
    if (res <= 0)
        return -1;
    offset += res;
    if (cborType != CBOR_TYPE_MAP)
        return -1;
    count = (int)val;
    if (((uint64_t)count) != val || count <= 0 || count > (int)(bufferLen / 2))
        return -1;

    recordP->ids[0] = LWM2M_MAX_ID;
    recordP->ids[1] = LWM2M_MAX_ID;
    recordP->ids[2] = LWM2M_MAX_ID;
    recordP->ids[3] = LWM2M_MAX_ID;
    memset(&recordP->value, 0, sizeof(recordP->value));
    recordP->value.id = LWM2M_MAX_ID;
    recordP->time = 0;

    for (; count > 0; count--) {
        int label;
        int64_t ival;
        lwm2m_data_t data;

        res = cbor_get_singular(buffer + offset, bufferLen - offset, &data);
        if (res <= 0)
            return -1;
        offset += res;
        if (data.type == LWM2M_TYPE_STRING) {
            if (data.value.asBuffer.length == 3 && memcmp(data.value.asBuffer.buffer, "vlo", 3) == 0) {
                label = SENML_CBOR_OBJECT_LINK_LABEL;
            } else {
                /* Label ending in _ must be supported or generate error. */
                if (data.value.asBuffer.buffer[data.value.asBuffer.length - 1] == '_')
                    return -1;
                label = SENML_CBOR_UNKNOWN_LABEL;
            }
        } else if (lwm2m_data_decode_int(&data, &ival) != 0) {
            if (ival >= SENML_CBOR_BASE_SUM_LABEL && ival <= SENML_CBOR_DATA_VALUE_LABEL) {
                label = (int)ival;
            } else {
                // This is never a valid label
                return -1;
            }
        } else {
            label = SENML_CBOR_UNKNOWN_LABEL;
        }

        res = cbor_get_singular(buffer + offset, bufferLen - offset, &data);
        if (res <= 0)
            return -1;
        offset += res;

        switch (label) {
        case SENML_CBOR_BASE_VALUE_LABEL:
            if (bvSeen)
                return -1;
            bvSeen = true;
            memcpy(baseValue, &data, sizeof(lwm2m_data_t));
            /* Convert explicit 0 to implicit 0 */
            switch (baseValue->type) {
            case LWM2M_TYPE_INTEGER:
                if (baseValue->value.asInteger == 0) {
                    baseValue->type = LWM2M_TYPE_UNDEFINED;
                }
                break;
            case LWM2M_TYPE_UNSIGNED_INTEGER:
                if (baseValue->value.asUnsigned == 0) {
                    baseValue->type = LWM2M_TYPE_UNDEFINED;
                }
                break;
            case LWM2M_TYPE_FLOAT:
                if (baseValue->value.asFloat == 0.0) {
                    baseValue->type = LWM2M_TYPE_UNDEFINED;
                }
                break;
            default:
                // Only numeric types are valid
                return -1;
            }
            break;
        case SENML_CBOR_BASE_TIME_LABEL:
            if (btSeen)
                return -1;
            btSeen = true;
            if (lwm2m_data_decode_int(&data, &ival) == 0)
                return -1;
            *baseTime = (time_t)ival;
            break;
        case SENML_CBOR_BASE_NAME_LABEL:
            if (bnSeen)
                return -1;
            bnSeen = true;
            if (data.type != LWM2M_TYPE_STRING)
                return -1;
            if (data.value.asBuffer.length > 0) {
                if (data.value.asBuffer.length == 1 && data.value.asBuffer.buffer[0] != '/')
                    return -1;
                if (data.value.asBuffer.length > URI_MAX_STRING_LEN)
                    return -1;
                memcpy(baseUri, data.value.asBuffer.buffer, data.value.asBuffer.length);
                baseUri[data.value.asBuffer.length] = '\0';
            } else {
                baseUri[0] = '\0';
            }
            break;
        case SENML_CBOR_BASE_VERSION_LABEL:
            if (bverSeen)
                return -1;
            bverSeen = true;
            if (lwm2m_data_decode_int(&data, &ival) == 0)
                return -1;
            /* Only the default version (10) is supported */
            if (ival != 10)
                return -1;
            break;
        case SENML_CBOR_NAME_LABEL:
            if (name)
                return -1;
            if (data.type != LWM2M_TYPE_STRING)
                return -1;
            name = data.value.asBuffer.buffer;
            nameLength = data.value.asBuffer.length;
            break;
        case SENML_CBOR_NUMERIC_VALUE_LABEL:
            if (recordP->value.type != LWM2M_TYPE_UNDEFINED)
                return -1;
            switch (data.type) {
            case LWM2M_TYPE_INTEGER:
            case LWM2M_TYPE_UNSIGNED_INTEGER:
            case LWM2M_TYPE_FLOAT:
                recordP->value.type = data.type;
                memcpy(&recordP->value.value, &data.value, sizeof(data.value));
                break;
            default:
                // Not numeric
                return -1;
            }
            break;
        case SENML_CBOR_STRING_VALUE_LABEL:
            if (recordP->value.type != LWM2M_TYPE_UNDEFINED)
                return -1;
            if (data.type != LWM2M_TYPE_STRING)
                return -1;
            /* Don't use lwm2m_data_encode_nstring here. It would copy the buffer */
            recordP->value.type = LWM2M_TYPE_STRING;
            recordP->value.value.asBuffer.buffer = data.value.asBuffer.buffer;
            recordP->value.value.asBuffer.length = data.value.asBuffer.length;
            break;
        case SENML_CBOR_BOOLEAN_VALUE_LABEL: {
            bool bval;
            if (recordP->value.type != LWM2M_TYPE_UNDEFINED)
                return -1;
            if (lwm2m_data_decode_bool(&data, &bval) == 0)
                return -1;
            recordP->value.type = LWM2M_TYPE_BOOLEAN;
            recordP->value.value.asBoolean = bval;
        } break;
        case SENML_CBOR_TIME_LABEL:
            if (timeSeen)
                return -1;
            timeSeen = true;
            if (lwm2m_data_decode_int(&data, &ival) == 0)
                return -1;
            recordP->time = (time_t)ival;
            break;
        case SENML_CBOR_DATA_VALUE_LABEL:
            if (recordP->value.type != LWM2M_TYPE_UNDEFINED)
                return -1;
            if (data.type != LWM2M_TYPE_OPAQUE)
                return -1;
            /* Don't use lwm2m_data_encode_opaque here. It would copy the buffer */
            recordP->value.type = LWM2M_TYPE_OPAQUE;
            recordP->value.value.asBuffer.buffer = data.value.asBuffer.buffer;
            recordP->value.value.asBuffer.length = data.value.asBuffer.length;
            break;
        case SENML_CBOR_OBJECT_LINK_LABEL:
            if (recordP->value.type != LWM2M_TYPE_UNDEFINED)
                return -1;
            if (data.type != LWM2M_TYPE_STRING)
                return -1;
            if (!utils_textToObjLink(data.value.asBuffer.buffer, data.value.asBuffer.length,
                                     &recordP->value.value.asObjLink.objectId,
                                     &recordP->value.value.asObjLink.objectInstanceId)) {
                return -1;
            }
            recordP->value.type = LWM2M_TYPE_OBJECT_LINK;
            break;
        case SENML_CBOR_BASE_SUM_LABEL:
        case SENML_CBOR_BASE_UNIT_LABEL:
        case SENML_CBOR_UNIT_LABEL:
        case SENML_CBOR_SUM_LABEL:
        case SENML_CBOR_UPDATE_TIME_LABEL:
        default:
            // ignore
            break;
        }
    }

    /* Combine with base values */
    recordP->time += *baseTime;
    if (baseUri[0] || name) {
        lwm2m_uri_t uri;
        size_t length = strlen(baseUri);
        char uriStr[URI_MAX_STRING_LEN];
        if (length > sizeof(uriStr))
            return -1;
        memcpy(uriStr, baseUri, length);
        if (nameLength) {
            if (nameLength + length > sizeof(uriStr))
                return -1;
            memcpy(uriStr + length, name, nameLength);
            length += nameLength;
        }
        if (!lwm2m_stringToUri(uriStr, length, &uri))
            return -1;
        if (LWM2M_URI_IS_SET_OBJECT(&uri)) {
            recordP->ids[0] = uri.objectId;
        }
        if (LWM2M_URI_IS_SET_INSTANCE(&uri)) {
            recordP->ids[1] = uri.instanceId;
        }
        if (LWM2M_URI_IS_SET_RESOURCE(&uri)) {
            recordP->ids[2] = uri.resourceId;
        }
        if (LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri)) {
            recordP->ids[3] = uri.resourceInstanceId;
        }
    }
    if (baseValue->type != LWM2M_TYPE_UNDEFINED) {
        if (recordP->value.type == LWM2M_TYPE_UNDEFINED) {
            memcpy(&recordP->value, baseValue, sizeof(*baseValue));
        } else {
            switch (recordP->value.type) {
            case LWM2M_TYPE_INTEGER:
                switch (baseValue->type) {
                case LWM2M_TYPE_INTEGER:
                    recordP->value.value.asInteger += baseValue->value.asInteger;
                    break;
                case LWM2M_TYPE_UNSIGNED_INTEGER:
                    recordP->value.value.asInteger += baseValue->value.asUnsigned;
                    break;
                case LWM2M_TYPE_FLOAT:
                    recordP->value.value.asInteger += baseValue->value.asFloat;
                    break;
                default:
                    return -1;
                }
                break;
            case LWM2M_TYPE_UNSIGNED_INTEGER:
                switch (baseValue->type) {
                case LWM2M_TYPE_INTEGER:
                    recordP->value.value.asUnsigned += baseValue->value.asInteger;
                    break;
                case LWM2M_TYPE_UNSIGNED_INTEGER:
                    recordP->value.value.asUnsigned += baseValue->value.asUnsigned;
                    break;
                case LWM2M_TYPE_FLOAT:
                    recordP->value.value.asUnsigned += baseValue->value.asFloat;
                    break;
                default:
                    return -1;
                }
                break;
            case LWM2M_TYPE_FLOAT:
                switch (baseValue->type) {
                case LWM2M_TYPE_INTEGER:
                    recordP->value.value.asFloat += baseValue->value.asInteger;
                    break;
                case LWM2M_TYPE_UNSIGNED_INTEGER:
                    recordP->value.value.asFloat += baseValue->value.asUnsigned;
                    break;
                case LWM2M_TYPE_FLOAT:
                    recordP->value.value.asFloat += baseValue->value.asFloat;
                    break;
                default:
                    return -1;
                }
                break;
            default:
                return -1;
            }
        }
    }

    return offset;
}

static bool prv_convertValue(const senml_record_t *recordP, lwm2m_data_t *targetP) {
    switch (recordP->value.type) {
    case LWM2M_TYPE_STRING:
        lwm2m_data_encode_nstring((const char *)recordP->value.value.asBuffer.buffer,
                                  recordP->value.value.asBuffer.length, targetP);
        break;
    case LWM2M_TYPE_OPAQUE:
        lwm2m_data_encode_opaque(recordP->value.value.asBuffer.buffer, recordP->value.value.asBuffer.length, targetP);
        break;
    default:
        if (recordP->value.type != LWM2M_TYPE_UNDEFINED) {
            targetP->type = recordP->value.type;
        }
        memcpy(&targetP->value, &recordP->value.value, sizeof(targetP->value));
        break;
    case LWM2M_TYPE_OBJECT:
    case LWM2M_TYPE_OBJECT_INSTANCE:
    case LWM2M_TYPE_MULTIPLE_RESOURCE:
    case LWM2M_TYPE_CORE_LINK:
        /* Should never happen */
        return false;
    }

    return true;
}

int senml_cbor_parse(const lwm2m_uri_t *uriP, const uint8_t *buffer, size_t bufferLen, lwm2m_data_t **dataP) {
    int count;
    senml_record_t *recordArray;
    int recordIndex;
    char baseUri[URI_MAX_STRING_LEN + 1];
    time_t baseTime;
    lwm2m_data_t baseValue;
    cbor_type_t cborType;
    uint64_t val;
    int res;
    size_t offset;

    LOG_ARG("bufferLen: %d", bufferLen);
    LOG_URI(uriP);
    *dataP = NULL;
    recordArray = NULL;

    offset = 0;
    res = prv_get_cbor_type_and_value(buffer, bufferLen, &cborType, &val);
    if (res <= 0)
        goto error;
    offset += res;
    if (cborType != CBOR_TYPE_ARRAY)
        goto error;
    count = (int)val;
    if (((uint64_t)count) != val || count <= 0 || ((size_t)count) > bufferLen / 3)
        goto error;
    recordArray = (senml_record_t *)lwm2m_malloc(count * sizeof(senml_record_t));
    if (recordArray == NULL)
        goto error;

    baseUri[0] = '\0';
    baseTime = 0;
    memset(&baseValue, 0, sizeof(baseValue));
    for (recordIndex = 0; recordIndex < count; recordIndex++) {
        res = prv_parseItem(buffer + offset, bufferLen - offset, recordArray + recordIndex, baseUri, &baseTime,
                            &baseValue);
        if (res <= 0)
            goto error;
        offset += res;
    }
    if (offset != bufferLen)
        goto error;

    count = senml_convert_records(uriP, recordArray, count, prv_convertValue, dataP);
    recordArray = NULL;

    if (count > 0) {
        LOG_ARG("Parsing successful. count: %d", count);
        return count;
    }

error:
    LOG("Parsing failed");
    if (recordArray != NULL) {
        lwm2m_free(recordArray);
    }
    return -1;
}

static int prv_serializeBaseName(const uint8_t *baseUriStr, size_t baseUriLen, uint8_t *buffer, size_t bufferLen) {
    int head = 0;
    int res;
    lwm2m_data_t data;
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_STRING;
    data.value.asBuffer.buffer = (uint8_t *)baseUriStr;
    data.value.asBuffer.length = baseUriLen;

    // label
    res = cbor_put_type_and_value(buffer + head, bufferLen - head, CBOR_TYPE_NEGATIVE_INTEGER,
                                  SENML_CBOR_BASE_NAME_LABEL);
    if (res <= 0)
        return -1;
    head += res;

    // value
    res = cbor_put_singular(buffer + head, bufferLen - head, &data);
    if (res <= 0)
        return -1;
    return head + res;
}

static int prv_serializeName(const uint8_t *parentUriStr, size_t parentUriLen, uint16_t id, uint8_t *buffer,
                             size_t bufferLen) {
    uint8_t uriStr[URI_MAX_STRING_LEN];
    size_t uriLen;
    int res;
    int head = 0;
    lwm2m_data_t data;

    uriLen = 0;
    if (parentUriLen > 0) {
        if (sizeof(uriStr) < parentUriLen)
            return -1;
        memcpy(uriStr, parentUriStr, parentUriLen);
        uriLen += parentUriLen;
    }
    res = utils_intToText(id, uriStr + parentUriLen, sizeof(uriStr) - parentUriLen);
    if (res <= 0)
        return -1;
    uriLen += res;

    // label
    res = cbor_put_type_and_value(buffer + head, bufferLen - head, CBOR_TYPE_UNSIGNED_INTEGER, SENML_CBOR_NAME_LABEL);
    if (res <= 0)
        return -1;
    head += res;

    // value
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_STRING;
    data.value.asBuffer.buffer = uriStr;
    data.value.asBuffer.length = uriLen;
    res = cbor_put_singular(buffer + head, bufferLen - head, &data);
    if (res <= 0)
        return -1;
    return head + res;
}

static int prv_serializeValue(const lwm2m_data_t *tlvP, uint8_t *buffer, size_t bufferLen) {
    int res;
    int head = 0;
    int label;

    switch (tlvP->type) {
    case LWM2M_TYPE_STRING:
    case LWM2M_TYPE_CORE_LINK:
        label = SENML_CBOR_STRING_VALUE_LABEL;
        break;
    case LWM2M_TYPE_OPAQUE:
        label = SENML_CBOR_DATA_VALUE_LABEL;
        break;
    case LWM2M_TYPE_INTEGER:
    case LWM2M_TYPE_UNSIGNED_INTEGER:
    case LWM2M_TYPE_FLOAT:
        label = SENML_CBOR_NUMERIC_VALUE_LABEL;
        break;
    case LWM2M_TYPE_BOOLEAN:
        label = SENML_CBOR_BOOLEAN_VALUE_LABEL;
        break;
    case LWM2M_TYPE_OBJECT_LINK:
        label = SENML_CBOR_OBJECT_LINK_LABEL;
        break;
    default:
        return -1;
    }

    // label
    if (label == SENML_CBOR_OBJECT_LINK_LABEL) {
        lwm2m_data_t data;
        data.type = LWM2M_TYPE_STRING;
        data.value.asBuffer.buffer = (uint8_t *)"vlo";
        data.value.asBuffer.length = 3;
        res = cbor_put_singular(buffer + head, bufferLen - head, &data);
    } else {
        res = cbor_put_type_and_value(buffer + head, bufferLen - head, CBOR_TYPE_UNSIGNED_INTEGER, label);
    }
    if (res <= 0)
        return -1;
    head += res;

    // value
    res = cbor_put_singular(buffer + head, bufferLen - head, tlvP);
    if (res <= 0)
        return -1;
    return head + res;
}

static int prv_serializeTlv(const lwm2m_data_t *tlvP, const uint8_t *baseUriStr, size_t baseUriLen,
                            uri_depth_t baseLevel, const uint8_t *parentUriStr, size_t parentUriLen, uri_depth_t level,
                            bool *baseNameOutput, uint8_t *buffer, size_t bufferLen) {
    int res;
    int head = 0;

    // Count how many labels
    res = 0;
    if (!*baseNameOutput && baseUriLen > 0)
        res++;
    if (!baseUriLen || level > baseLevel)
        res++;
    switch (tlvP->type) {
    case LWM2M_TYPE_UNDEFINED:
    case LWM2M_TYPE_OBJECT:
    case LWM2M_TYPE_OBJECT_INSTANCE:
    case LWM2M_TYPE_MULTIPLE_RESOURCE:
        // no value
        break;
    default:
        res++;
        break;
    }

    // Start the record
    res = cbor_put_type_and_value(buffer + head, bufferLen - head, CBOR_TYPE_MAP, res);
    if (res <= 0)
        return -1;
    head += res;

    if (!*baseNameOutput && baseUriLen > 0) {
        res = prv_serializeBaseName(baseUriStr, baseUriLen, buffer + head, bufferLen - head);
        if (res <= 0)
            return -1;
        head += res;
        *baseNameOutput = true;
    }

    /* TODO: support base time */

    if (!baseUriLen || level > baseLevel) {
        res = prv_serializeName(parentUriStr, parentUriLen, tlvP->id, buffer + head, bufferLen - head);
        if (res <= 0)
            return -1;
        head += res;
    }

    switch (tlvP->type) {
    case LWM2M_TYPE_UNDEFINED:
    case LWM2M_TYPE_OBJECT:
    case LWM2M_TYPE_OBJECT_INSTANCE:
    case LWM2M_TYPE_MULTIPLE_RESOURCE:
        // no value
        break;
    default:
        res = prv_serializeValue(tlvP, buffer + head, bufferLen - head);
        if (res < 0)
            return -1;
        head += res;
        break;
    }

    /* TODO: support time */

    return head;
}

static int prv_serializeData(const lwm2m_data_t *tlvP, const uint8_t *baseUriStr, size_t baseUriLen,
                             uri_depth_t baseLevel, const uint8_t *parentUriStr, size_t parentUriLen, uri_depth_t level,
                             bool *baseNameOutput, int *numRecords, uint8_t *buffer, size_t bufferLen) {
    size_t head;
    int res;
    uint8_t uriStr[URI_MAX_STRING_LEN];
    size_t uriLen;

    head = 0;

    switch (tlvP->type) {
    case LWM2M_TYPE_MULTIPLE_RESOURCE:
    case LWM2M_TYPE_OBJECT:
    case LWM2M_TYPE_OBJECT_INSTANCE: {
        if (tlvP->value.asChildren.count == 0) {
            res = prv_serializeTlv(tlvP, baseUriStr, baseUriLen, baseLevel, parentUriStr, parentUriLen, level,
                                   baseNameOutput, buffer + head, bufferLen - head);
            if (res < 0)
                return -1;
            head += res;
            *numRecords += 1;
        } else {
            size_t index;

            uriLen = 0;
            if (parentUriLen > 0) {
                if (URI_MAX_STRING_LEN < parentUriLen)
                    return -1;
                memcpy(uriStr, parentUriStr, parentUriLen);
                uriLen += parentUriLen;
            }
            res = utils_intToText(tlvP->id, uriStr + uriLen, sizeof(uriStr) - uriLen);
            if (res <= 0)
                return -1;
            uriLen += res;
            if (uriLen >= sizeof(uriStr))
                return -1;
            uriStr[uriLen++] = '/';

            for (index = 0; index < tlvP->value.asChildren.count; index++) {
                res = prv_serializeData(tlvP->value.asChildren.array + index, baseUriStr, baseUriLen, baseLevel, uriStr,
                                        uriLen, level, baseNameOutput, numRecords, buffer + head, bufferLen - head);
                if (res < 0)
                    return -1;
                head += res;
            }
        }
    } break;

    default:
        res = prv_serializeTlv(tlvP, baseUriStr, baseUriLen, baseLevel, parentUriStr, parentUriLen, level,
                               baseNameOutput, buffer + head, bufferLen - head);
        if (res < 0)
            return -1;
        head += res;
        *numRecords += 1;
        break;
    }

    return (int)head;
}

int senml_cbor_serialize(const lwm2m_uri_t *uriP, int size, const lwm2m_data_t *tlvP, uint8_t **bufferP) {
    int res;
    int index;
    size_t head;
    int numRecords;
    uint8_t bufferCBOR[PRV_CBOR_BUFFER_SIZE];
    uint8_t baseUriStr[URI_MAX_STRING_LEN];
    int baseUriLen;
    uri_depth_t rootLevel;
    uri_depth_t baseLevel;
    int num;
    lwm2m_data_t *targetP;
    const uint8_t *parentUriStr = NULL;
    size_t parentUriLen = 0;

    LOG_ARG("size: %d", size);
    LOG_URI(uriP);
    if (size != 0 && tlvP == NULL)
        return -1;

    baseUriLen = lwm2m_uriToString(uriP, baseUriStr, URI_MAX_STRING_LEN, &baseLevel);
    if (baseUriLen < 0)
        return -1;
    if (baseUriLen > 1 && baseLevel != URI_DEPTH_RESOURCE && baseLevel != URI_DEPTH_RESOURCE_INSTANCE) {
        if (baseUriLen >= URI_MAX_STRING_LEN - 1)
            return 0;
        baseUriStr[baseUriLen++] = '/';
    }

    num = senml_findAndCheckData(uriP, baseLevel, size, tlvP, &targetP, &rootLevel);
    if (num < 0)
        return -1;

    if (baseLevel < rootLevel && baseUriLen > 1 && baseUriStr[baseUriLen - 1] != '/') {
        if (baseUriLen >= URI_MAX_STRING_LEN - 1)
            return 0;
        baseUriStr[baseUriLen++] = '/';
    }

    if (!baseUriLen || baseUriStr[baseUriLen - 1] != '/') {
        parentUriStr = (const uint8_t *)"/";
        parentUriLen = 1;
    }

    head = 0;
    numRecords = 0;
    bool baseNameOutput = false;
    for (index = 0; index < num && head < PRV_CBOR_BUFFER_SIZE; index++) {
        res =
            prv_serializeData(targetP + index, baseUriStr, baseUriLen, baseLevel, parentUriStr, parentUriLen, rootLevel,
                              &baseNameOutput, &numRecords, bufferCBOR + head, PRV_CBOR_BUFFER_SIZE - head);
        if (res < 0)
            return res;
        head += res;
    }

    if (!baseNameOutput && baseUriLen > 0) {
        // Remove trailing /
        if (baseUriLen > 1)
            baseUriLen -= 1;

        // Start the record
        res = cbor_put_type_and_value(bufferCBOR + head, PRV_CBOR_BUFFER_SIZE - head, CBOR_TYPE_MAP, 1);
        if (res < 0)
            return res;
        head += res;

        res = prv_serializeBaseName(baseUriStr, baseUriLen, bufferCBOR + head, PRV_CBOR_BUFFER_SIZE - head);
        if (res < 0)
            return res;
        head += res;

        baseNameOutput = true;
        numRecords++;
    }

    *bufferP = (uint8_t *)lwm2m_malloc(head + 3);
    if (*bufferP == NULL)
        return -1;
    res = cbor_put_type_and_value(*bufferP, 3, CBOR_TYPE_ARRAY, numRecords);
    if (res <= 0)
        return -1;
    memcpy((*bufferP) + res, bufferCBOR, head);
    head += res;

    return head;
}

#endif
