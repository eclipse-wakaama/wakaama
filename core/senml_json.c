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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>


#ifdef LWM2M_SUPPORT_SENML_JSON

#ifdef LWM2M_VERSION_1_0
#error SenML JSON not supported with LWM2M 1.0
#endif

#define PRV_JSON_BUFFER_SIZE 1024

#define JSON_FALSE_STRING                 "false"
#define JSON_FALSE_STRING_SIZE            5
#define JSON_TRUE_STRING                  "true"
#define JSON_TRUE_STRING_SIZE             4

#define JSON_ITEM_BEGIN                   '{'
#define JSON_ITEM_END                     '}'
#define JSON_ITEM_URI                     "\"n\":\""
#define JSON_ITEM_URI_SIZE                5
#define JSON_ITEM_URI_END                 '"'
#define JSON_ITEM_BOOL                    "\"vb\":"
#define JSON_ITEM_BOOL_SIZE               5
#define JSON_ITEM_NUM                     "\"v\":"
#define JSON_ITEM_NUM_SIZE                4
#define JSON_ITEM_STRING_BEGIN            "\"vs\":\""
#define JSON_ITEM_STRING_BEGIN_SIZE       6
#define JSON_ITEM_STRING_END              '"'
#define JSON_ITEM_OPAQUE_BEGIN            "\"vd\":\""
#define JSON_ITEM_OPAQUE_BEGIN_SIZE       6
#define JSON_ITEM_OPAQUE_END              '"'
#define JSON_ITEM_OBJECT_LINK_BEGIN       "\"vlo\":\""
#define JSON_ITEM_OBJECT_LINK_BEGIN_SIZE  7
#define JSON_ITEM_OBJECT_LINK_END         '"'

#define JSON_BN_HEADER                    "\"bn\":\""
#define JSON_BN_HEADER_SIZE               6
#define JSON_BT_HEADER                    "\"bt\":"
#define JSON_BT_HEADER_SIZE               5
#define JSON_HEADER                       '['
#define JSON_FOOTER                       ']'
#define JSON_SEPARATOR                    ','


#define _GO_TO_NEXT_CHAR(I,B,L)         \
    {                                   \
        I++;                            \
        I += json_skipSpace(B+I, L-I);   \
        if (I == L) goto error;         \
    }

typedef struct
{
    uint16_t        ids[4];
    lwm2m_data_t    value; /* Any buffer will be within the parsed data */
    time_t          time;
} _record_t;

static int prv_parseItem(const uint8_t * buffer,
                         size_t bufferLen,
                         _record_t * recordP,
                         char * baseUri,
                         time_t * baseTime,
                         lwm2m_data_t *baseValue)
{
    size_t index;
    const uint8_t *name = NULL;
    size_t nameLength = 0;
    bool timeSeen = false;
    bool bnSeen = false;
    bool btSeen = false;
    bool bvSeen = false;
    bool bverSeen = false;

    memset(recordP->ids, 0xFF, 4*sizeof(uint16_t));
    memset(&recordP->value, 0, sizeof(recordP->value));
    recordP->time = 0;

    index = 0;
    do
    {
        size_t tokenStart;
        size_t tokenLen;
        size_t valueStart;
        size_t valueLen;
        int next;

        next = json_split(buffer+index,
                          bufferLen-index,
                          &tokenStart,
                          &tokenLen,
                          &valueStart,
                          &valueLen);
        if (next < 0) return -1;
        if (tokenLen == 0) return -1;

        switch (buffer[index+tokenStart])
        {
        case 'b':
            if (tokenLen == 2 && buffer[index+tokenStart+1] == 'n')
            {
                if (bnSeen) return -1;
                bnSeen = true;
                /* Check for " around URI */
                if (valueLen < 2
                 || buffer[index+valueStart] != '"'
                 || buffer[index+valueStart+valueLen-1] != '"')
                {
                    return -1;
                }
                if (valueLen >= 3)
                {
                    if (valueLen == 3 && buffer[index+valueStart+1] != '/') return -1;
                    if (valueLen > URI_MAX_STRING_LEN) return -1;
                    memcpy(baseUri, buffer+index+valueStart+1, valueLen-2);
                    baseUri[valueLen-2] = '\0';
                }
                else
                {
                    baseUri[0] = '\0';
                }
            }
            else if (tokenLen == 2 && buffer[index+tokenStart+1] == 't')
            {
                if (btSeen) return -1;
                btSeen = true;
                if (!json_convertTime(buffer+index+valueStart, valueLen, baseTime))
                    return -1;
            }
            else if (tokenLen == 2 && buffer[index+tokenStart+1] == 'v')
            {
                if (bvSeen) return -1;
                bvSeen = true;
                if (valueLen == 0)
                {
                    baseValue->type = LWM2M_TYPE_UNDEFINED;
                }
                else
                {
                    if (!json_convertNumeric(buffer+index+valueStart, valueLen, baseValue))
                        return -1;
                    /* Convert explicit 0 to implicit 0 */
                    switch (baseValue->type)
                    {
                    case LWM2M_TYPE_INTEGER:
                        if (baseValue->value.asInteger == 0)
                        {
                            baseValue->type = LWM2M_TYPE_UNDEFINED;
                        }
                        break;
                    case LWM2M_TYPE_UNSIGNED_INTEGER:
                        if (baseValue->value.asUnsigned == 0)
                        {
                            baseValue->type = LWM2M_TYPE_UNDEFINED;
                        }
                        break;
                    case LWM2M_TYPE_FLOAT:
                        if (baseValue->value.asFloat == 0.0)
                        {
                            baseValue->type = LWM2M_TYPE_UNDEFINED;
                        }
                        break;
                    default:
                        return -1;
                    }
                }
            }
            else if (tokenLen == 4
                  && buffer[index+tokenStart+1] == 'v'
                  && buffer[index+tokenStart+2] == 'e'
                  && buffer[index+tokenStart+3] == 'r')
            {
                int64_t value;
                int res;
                if (bverSeen) return -1;
                bverSeen = true;
                res = utils_textToInt(buffer+index+valueStart, valueLen, &value);
                /* Only the default version (10) is supported */
                if (!res || value != 10)
                {
                    return -1;
                }
            }
            else if (buffer[index+tokenStart+tokenLen-1] == '_')
            {
                /* Label ending in _ must be supported or generate error. */
                return -1;
            }
            break;
        case 'n':
        {
            if (tokenLen == 1)
            {
                if (name) return -1;

                /* Check for " around URI */
                if (valueLen < 2
                        || buffer[index+valueStart] != '"'
                                || buffer[index+valueStart+valueLen-1] != '"')
                {
                    return -1;
                }
                name = buffer + index + valueStart + 1;
                nameLength = valueLen - 2;
            }
            else if (buffer[index+tokenStart+tokenLen-1] == '_')
            {
                /* Label ending in _ must be supported or generate error. */
                return -1;
            }
            break;
        }
        case 't':
            if (tokenLen == 1)
            {
                if (timeSeen) return -1;
                timeSeen = true;
                if (!json_convertTime(buffer+index+valueStart, valueLen, &recordP->time))
                    return -1;
            }
            else if (buffer[index+tokenStart+tokenLen-1] == '_')
            {
                /* Label ending in _ must be supported or generate error. */
                return -1;
            }
            break;
        case 'v':
            if (tokenLen == 1)
            {
                if (recordP->value.type != LWM2M_TYPE_UNDEFINED) return -1;
                if (!json_convertNumeric(buffer+index+valueStart, valueLen, &recordP->value))
                    return -1;
            }
            else if (tokenLen == 2 && buffer[index+tokenStart+1] == 'b')
            {
                if (recordP->value.type != LWM2M_TYPE_UNDEFINED) return -1;
                if (0 == lwm2m_strncmp(JSON_TRUE_STRING,
                                       (char *)buffer + index + valueStart,
                                       valueLen))
                {
                    lwm2m_data_encode_bool(true, &recordP->value);
                }
                else if (0 == lwm2m_strncmp(JSON_FALSE_STRING,
                                            (char *)buffer + index + valueStart,
                                            valueLen))
                {
                    lwm2m_data_encode_bool(false, &recordP->value);
                }
                else
                {
                    return -1;
                }
            }
            else if (tokenLen == 2
                  && (buffer[index+tokenStart+1] == 'd'
                   || buffer[index+tokenStart+1] == 's'))
            {
                if (recordP->value.type != LWM2M_TYPE_UNDEFINED) return -1;
                /* Check for " around value */
                if (valueLen < 2
                 || buffer[index+valueStart] != '"'
                 || buffer[index+valueStart+valueLen-1] != '"')
                {
                    return -1;
                }
                if (buffer[index+tokenStart+1] == 'd')
                {
                    /* Don't use lwm2m_data_encode_opaque here. It would copy the buffer */
                    recordP->value.type = LWM2M_TYPE_OPAQUE;
                }
                else
                {
                    /* Don't use lwm2m_data_encode_nstring here. It would copy the buffer */
                    recordP->value.type = LWM2M_TYPE_STRING;
                }
                recordP->value.value.asBuffer.buffer = (uint8_t *)buffer + index + valueStart + 1;
                recordP->value.value.asBuffer.length = valueLen - 2;
            }
            else if (tokenLen == 3 && buffer[index+tokenStart+1] == 'l' && buffer[index+tokenStart+2] == 'o')
            {
                if (recordP->value.type != LWM2M_TYPE_UNDEFINED) return -1;
                /* Check for " around value */
                if (valueLen < 2
                 || buffer[index+valueStart] != '"'
                 || buffer[index+valueStart+valueLen-1] != '"')
                {
                    return -1;
                }
                if (!utils_textToObjLink(buffer + index + valueStart + 1,
                                         valueLen - 2,
                                         &recordP->value.value.asObjLink.objectId,
                                         &recordP->value.value.asObjLink.objectInstanceId))
                {
                    return -1;
                }
                recordP->value.type = LWM2M_TYPE_OBJECT_LINK;
            }
            else if (buffer[index+tokenStart+tokenLen-1] == '_')
            {
                /* Label ending in _ must be supported or generate error. */
                return -1;
            }
            break;
        default:
            if (buffer[index+tokenStart+tokenLen-1] == '_')
            {
                /* Label ending in _ must be supported or generate error. */
                return -1;
            }
            break;
        }

        index += next + 1;
    } while (index < bufferLen);

    /* Combine with base values */
    recordP->time += *baseTime;
    if (baseUri[0] || name)
    {
        lwm2m_uri_t uri;
        size_t length = strlen(baseUri);
        char uriStr[URI_MAX_STRING_LEN];
        if (length > sizeof(uriStr)) return -1;
        memcpy(uriStr, baseUri, length);
        if (nameLength)
        {
            if (nameLength + length > sizeof(uriStr)) return -1;
            memcpy(uriStr + length, name, nameLength);
            length += nameLength;
        }
        if (!lwm2m_stringToUri(uriStr, length, &uri)) return -1;
        if (LWM2M_URI_IS_SET_OBJECT(&uri))
        {
            recordP->ids[0] = uri.objectId;
        }
        if (LWM2M_URI_IS_SET_INSTANCE(&uri))
        {
            recordP->ids[1] = uri.instanceId;
        }
        if (LWM2M_URI_IS_SET_RESOURCE(&uri))
        {
            recordP->ids[2] = uri.resourceId;
        }
        if (LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri))
        {
            recordP->ids[3] = uri.resourceInstanceId;
        }
    }
    if (baseValue->type != LWM2M_TYPE_UNDEFINED)
    {
        if (recordP->value.type == LWM2M_TYPE_UNDEFINED)
        {
            memcpy(&recordP->value, baseValue, sizeof(*baseValue));
        }
        else
        {
            switch (recordP->value.type)
            {
            case LWM2M_TYPE_INTEGER:
                switch(baseValue->type)
                {
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
                switch(baseValue->type)
                {
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
                switch(baseValue->type)
                {
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

    return 0;
}

static bool prv_convertValue(const _record_t * recordP,
                             lwm2m_data_t * targetP)
{
    switch (recordP->value.type)
    {
    case LWM2M_TYPE_STRING:
        if (0 != recordP->value.value.asBuffer.length)
        {
            size_t stringLen;
            uint8_t *string = (uint8_t *)lwm2m_malloc(recordP->value.value.asBuffer.length);
            if (!string) return false;
            stringLen = json_unescapeString(string,
                                            recordP->value.value.asBuffer.buffer,
                                            recordP->value.value.asBuffer.length);
            if (stringLen)
            {
                lwm2m_data_encode_nstring((char *)string, stringLen, targetP);
                lwm2m_free(string);
            }
            else
            {
                lwm2m_free(string);
                return false;
            }
        }
        else
        {
            lwm2m_data_encode_nstring(NULL, 0, targetP);
        }
        break;
    case LWM2M_TYPE_OPAQUE:
        if (0 != recordP->value.value.asBuffer.length)
        {
            size_t dataLength;
            uint8_t *data;
            dataLength = utils_base64GetDecodedSize((const char *)recordP->value.value.asBuffer.buffer,
                                                    recordP->value.value.asBuffer.length);
            data = lwm2m_malloc(dataLength);
            if (!data) return false;
            dataLength = utils_base64Decode((const char *)recordP->value.value.asBuffer.buffer,
                                   recordP->value.value.asBuffer.length,
                                   data,
                                   dataLength);
            if (dataLength)
            {
                lwm2m_data_encode_opaque(data, dataLength, targetP);
                lwm2m_free(data);
            }
            else
            {
                lwm2m_free(data);
                return false;
            }
        }
        else
        {
            lwm2m_data_encode_opaque(NULL, 0, targetP);
        }
        break;
    default:
        targetP->type = recordP->value.type;
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

static int prv_convertRecord(const _record_t * recordArray,
                             int count,
                             lwm2m_data_t ** dataP)
{
    int index;
    int freeIndex;
    lwm2m_data_t * rootP;

    rootP = lwm2m_data_new(count);
    if (NULL == rootP)
    {
        *dataP = NULL;
        return -1;
    }

    freeIndex = 0;
    for (index = 0 ; index < count ; index++)
    {
        lwm2m_data_t * targetP;
        int i;

        targetP = json_findDataItem(rootP, count, recordArray[index].ids[0]);
        if (targetP == NULL)
        {
            targetP = rootP + freeIndex;
            freeIndex++;
            targetP->id = recordArray[index].ids[0];
            targetP->type = LWM2M_TYPE_OBJECT;
        }
        if (recordArray[index].ids[1] != LWM2M_MAX_ID)
        {
            lwm2m_data_t * parentP;
            uri_depth_t level;

            parentP = targetP;
            level = URI_DEPTH_OBJECT_INSTANCE;
            for (i = 1 ; i <= 2 ; i++)
            {
                if (recordArray[index].ids[i] == LWM2M_MAX_ID) break;
                targetP = json_findDataItem(parentP->value.asChildren.array,
                                           parentP->value.asChildren.count,
                                           recordArray[index].ids[i]);
                if (targetP == NULL)
                {
                    targetP = json_extendData(parentP);
                    if (targetP == NULL) goto error;
                    targetP->id = recordArray[index].ids[i];
                    targetP->type = utils_depthToDatatype(level);
                }
                level = json_decreaseLevel(level);
                parentP = targetP;
            }
            if (recordArray[index].ids[3] != LWM2M_MAX_ID)
            {
                targetP->type = LWM2M_TYPE_MULTIPLE_RESOURCE;
                targetP = json_extendData(targetP);
                if (targetP == NULL) goto error;
                targetP->id = recordArray[index].ids[3];
                targetP->type = LWM2M_TYPE_UNDEFINED;
            }
        }

        if (!prv_convertValue(recordArray + index, targetP)) goto error;
    }

    if (freeIndex < count)
    {
        *dataP = lwm2m_data_new(freeIndex);
        if (*dataP == NULL) goto error;
        memcpy(*dataP, rootP, freeIndex * sizeof(lwm2m_data_t));
        lwm2m_free(rootP);     /* do not use lwm2m_data_free() to keep pointed values */
    }
    else
    {
        *dataP = rootP;
    }

    return freeIndex;

error:
    lwm2m_data_free(count, rootP);
    *dataP = NULL;

    return -1;
}

int senml_json_parse(const lwm2m_uri_t * uriP,
                     const uint8_t * buffer,
                     size_t bufferLen,
                     lwm2m_data_t ** dataP)
{
    size_t index;
    int count = 0;
    _record_t * recordArray;
    lwm2m_data_t * parsedP;
    int recordIndex;
    char baseUri[URI_MAX_STRING_LEN + 1];
    time_t baseTime;
    lwm2m_data_t baseValue;

    LOG_ARG("bufferLen: %d, buffer: \"%s\"", bufferLen, (char *)buffer);
    LOG_URI(uriP);
    *dataP = NULL;
    recordArray = NULL;
    parsedP = NULL;

    index = json_skipSpace(buffer, bufferLen);
    if (index == bufferLen) return -1;

    if (buffer[index] != JSON_HEADER) return -1;

    _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
    count = json_countItems(buffer + index, bufferLen - index);
    if (count <= 0) goto error;
    recordArray = (_record_t*)lwm2m_malloc(count * sizeof(_record_t));
    if (recordArray == NULL) goto error;
    /* at this point we are sure buffer[index] is '{' and all { and } are matching */
    recordIndex = 0;
    baseUri[0] = '\0';
    baseTime = 0;
    memset(&baseValue, 0, sizeof(baseValue));
    while (recordIndex < count)
    {
        int itemLen = json_itemLength(buffer + index, bufferLen - index);
        if (itemLen < 0) goto error;
        if (prv_parseItem(buffer + index + 1,
                          itemLen - 2,
                          recordArray + recordIndex,
                          baseUri,
                          &baseTime,
                          &baseValue))
        {
            goto error;
        }
        recordIndex++;
        index += itemLen - 1;
        _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
        switch (buffer[index])
        {
        case JSON_SEPARATOR:
            _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
            break;
        case JSON_FOOTER:
            if (recordIndex != count) goto error;
            break;
        default:
            goto error;
        }
    }

    if (buffer[index] != JSON_FOOTER) goto error;

    lwm2m_data_t * resultP;
    int size;

    count = prv_convertRecord(recordArray, count, &parsedP);
    lwm2m_free(recordArray);
    recordArray = NULL;

    if (count > 0 && uriP != NULL && LWM2M_URI_IS_SET_OBJECT(uriP))
    {
        if (parsedP->type != LWM2M_TYPE_OBJECT) goto error;
        if (parsedP->id != uriP->objectId) goto error;
        if (!LWM2M_URI_IS_SET_INSTANCE(uriP))
        {
            size = parsedP->value.asChildren.count;
            resultP = parsedP->value.asChildren.array;
        }
        else
        {
            int i;

            resultP = NULL;
            /* be permissive and allow full object JSON when requesting for a single instance */
            for (i = 0 ;
                 i < (int)parsedP->value.asChildren.count && resultP == NULL;
                 i++)
            {
                lwm2m_data_t * targetP;

                targetP = parsedP->value.asChildren.array + i;
                if (targetP->id == uriP->instanceId)
                {
                    resultP = targetP->value.asChildren.array;
                    size = targetP->value.asChildren.count;
                }
            }
            if (resultP == NULL) goto error;
            if (LWM2M_URI_IS_SET_RESOURCE(uriP))
            {
                lwm2m_data_t * resP;

                resP = NULL;
                for (i = 0 ; i < size && resP == NULL; i++)
                {
                    lwm2m_data_t * targetP;

                    targetP = resultP + i;
                    if (targetP->id == uriP->resourceId)
                    {
                        if (targetP->type == LWM2M_TYPE_MULTIPLE_RESOURCE
                         && LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP))
                        {
                            resP = targetP->value.asChildren.array;
                            size = targetP->value.asChildren.count;
                        }
                        else
                        {
                            size = json_dataStrip(1, targetP, &resP);
                            if (size <= 0) goto error;
                            lwm2m_data_free(count, parsedP);
                            parsedP = NULL;
                        }
                    }
                }
                if (resP == NULL) goto error;
                resultP = resP;
            }
            if (LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP))
            {
                lwm2m_data_t * resP;

                resP = NULL;
                for (i = 0 ; i < size && resP == NULL; i++)
                {
                    lwm2m_data_t * targetP;

                    targetP = resultP + i;
                    if (targetP->id == uriP->resourceInstanceId)
                    {
                        size = json_dataStrip(1, targetP, &resP);
                        if (size <= 0) goto error;
                        lwm2m_data_free(count, parsedP);
                        parsedP = NULL;
                    }
                }
                if (resP == NULL) goto error;
                resultP = resP;
            }
        }
    }
    else
    {
        resultP = parsedP;
        size = count;
    }

    if (parsedP != NULL)
    {
        lwm2m_data_t * tempP;

        size = json_dataStrip(size, resultP, &tempP);
        if (size <= 0) goto error;
        lwm2m_data_free(count, parsedP);
        resultP = tempP;
    }
    count = size;
    *dataP = resultP;

    LOG_ARG("Parsing successful. count: %d", count);
    return count;

error:
    LOG("Parsing failed");
    if (parsedP != NULL)
    {
        lwm2m_data_free(count, parsedP);
        parsedP = NULL;
    }
    if (recordArray != NULL)
    {
        lwm2m_free(recordArray);
    }
    return -1;
}

static int prv_serializeValue(const lwm2m_data_t * tlvP,
                              uint8_t * buffer,
                              size_t bufferLen)
{
    size_t res;
    size_t head;

    switch (tlvP->type)
    {
    case LWM2M_TYPE_STRING:
    case LWM2M_TYPE_CORE_LINK:
        if (bufferLen < JSON_ITEM_STRING_BEGIN_SIZE) return -1;
        memcpy(buffer, JSON_ITEM_STRING_BEGIN, JSON_ITEM_STRING_BEGIN_SIZE);
        head = JSON_ITEM_STRING_BEGIN_SIZE;

        res = json_escapeString(buffer + head,
                                bufferLen - head,
                                tlvP->value.asBuffer.buffer,
                                tlvP->value.asBuffer.length);
        if (!res) return -1;
        head += res;

        if (bufferLen - head < 1) return -1;
        buffer[head++] = JSON_ITEM_STRING_END;

        break;

    case LWM2M_TYPE_INTEGER:
    {
        int64_t value;

        if (0 == lwm2m_data_decode_int(tlvP, &value)) return -1;

        if (bufferLen < JSON_ITEM_NUM_SIZE) return -1;
        memcpy(buffer, JSON_ITEM_NUM, JSON_ITEM_NUM_SIZE);
        head = JSON_ITEM_NUM_SIZE;

        res = utils_intToText(value, buffer + head, bufferLen - head);
        if (!res) return -1;
        head += res;
    }
    break;

    case LWM2M_TYPE_UNSIGNED_INTEGER:
    {
        uint64_t value;

        if (0 == lwm2m_data_decode_uint(tlvP, &value)) return -1;

        if (bufferLen < JSON_ITEM_NUM_SIZE) return -1;
        memcpy(buffer, JSON_ITEM_NUM, JSON_ITEM_NUM_SIZE);
        head = JSON_ITEM_NUM_SIZE;

        res = utils_uintToText(value, buffer + head, bufferLen - head);
        if (!res) return -1;
        head += res;
    }
    break;

    case LWM2M_TYPE_FLOAT:
    {
        double value;

        if (0 == lwm2m_data_decode_float(tlvP, &value)) return -1;

        if (bufferLen < JSON_ITEM_NUM_SIZE) return -1;
        memcpy(buffer, JSON_ITEM_NUM, JSON_ITEM_NUM_SIZE);
        head = JSON_ITEM_NUM_SIZE;

        res = utils_floatToText(value, buffer + head, bufferLen - head);
        if (!res) return -1;
        head += res;
    }
    break;

    case LWM2M_TYPE_BOOLEAN:
    {
        bool value;

        if (0 == lwm2m_data_decode_bool(tlvP, &value)) return -1;

        if (value)
        {
            if (bufferLen < JSON_ITEM_BOOL_SIZE + JSON_TRUE_STRING_SIZE) return -1;
            memcpy(buffer,
                   JSON_ITEM_BOOL JSON_TRUE_STRING,
                   JSON_ITEM_BOOL_SIZE + JSON_TRUE_STRING_SIZE);
            head = JSON_ITEM_BOOL_SIZE + JSON_TRUE_STRING_SIZE;
        }
        else
        {
            if (bufferLen < JSON_ITEM_BOOL_SIZE + JSON_FALSE_STRING_SIZE) return -1;
            memcpy(buffer,
                   JSON_ITEM_BOOL JSON_FALSE_STRING,
                   JSON_ITEM_BOOL_SIZE + JSON_FALSE_STRING_SIZE);
            head = JSON_ITEM_BOOL_SIZE + JSON_FALSE_STRING_SIZE;
        }
    }
    break;

    case LWM2M_TYPE_OPAQUE:
        if (bufferLen < JSON_ITEM_OPAQUE_BEGIN_SIZE) return -1;
        memcpy(buffer, JSON_ITEM_OPAQUE_BEGIN, JSON_ITEM_OPAQUE_BEGIN_SIZE);
        head = JSON_ITEM_OPAQUE_BEGIN_SIZE;

        if (tlvP->value.asBuffer.length > 0)
        {
            res = utils_base64Encode(tlvP->value.asBuffer.buffer,
                                     tlvP->value.asBuffer.length,
                                     buffer+head,
                                     bufferLen - head);
            if (!res) return -1;
            head += res;
        }

        if (bufferLen - head < 1) return -1;
        buffer[head++] = JSON_ITEM_OPAQUE_END;
        break;

    case LWM2M_TYPE_OBJECT_LINK:
        if (bufferLen < JSON_ITEM_OBJECT_LINK_BEGIN_SIZE) return -1;
        memcpy(buffer,
               JSON_ITEM_OBJECT_LINK_BEGIN,
               JSON_ITEM_OBJECT_LINK_BEGIN_SIZE);
        head = JSON_ITEM_OBJECT_LINK_BEGIN_SIZE;

        res = utils_objLinkToText(tlvP->value.asObjLink.objectId,
                                  tlvP->value.asObjLink.objectInstanceId,
                                  buffer + head,
                                  bufferLen - head);
        if (!res) return -1;
        head += res;

        if (bufferLen - head < 1) return -1;
        buffer[head++] = JSON_ITEM_OBJECT_LINK_END;
        break;

    default:
        return -1;
    }

    return (int)head;
}

static int prv_serializeData(const lwm2m_data_t * tlvP,
                             const uint8_t * baseUriStr,
                             size_t baseUriLen,
                             uri_depth_t baseLevel,
                             const uint8_t * parentUriStr,
                             size_t parentUriLen,
                             uri_depth_t level,
                             bool *baseNameOutput,
                             uint8_t * buffer,
                             size_t bufferLen)
{
    size_t head;
    int res;

    head = 0;

    /* Check to override passed in level */
    switch (tlvP->type)
    {
    case LWM2M_TYPE_MULTIPLE_RESOURCE:
        level = URI_DEPTH_RESOURCE;
        break;
    case LWM2M_TYPE_OBJECT:
        level = URI_DEPTH_OBJECT;
        break;
    case LWM2M_TYPE_OBJECT_INSTANCE:
        level = URI_DEPTH_OBJECT_INSTANCE;
        break;
    default:
        break;
    }

    switch (tlvP->type)
    {
    case LWM2M_TYPE_MULTIPLE_RESOURCE:
    case LWM2M_TYPE_OBJECT:
    case LWM2M_TYPE_OBJECT_INSTANCE:
    {
        uint8_t uriStr[URI_MAX_STRING_LEN];
        size_t uriLen;
        size_t index;

        if (parentUriLen > 0)
        {
            if (URI_MAX_STRING_LEN < parentUriLen) return -1;
            memcpy(uriStr, parentUriStr, parentUriLen);
            uriLen = parentUriLen;
        }
        else
        {
            uriLen = 0;
        }
        res = utils_intToText(tlvP->id,
                              uriStr + uriLen,
                              URI_MAX_STRING_LEN - uriLen);
        if (res <= 0) return -1;
        uriLen += res;
        uriStr[uriLen] = '/';
        uriLen++;

        head = 0;
        for (index = 0 ; index < tlvP->value.asChildren.count; index++)
        {
            if (index != 0)
            {
                if (head + 1 > bufferLen) return 0;
                buffer[head++] = JSON_SEPARATOR;
            }

            res = prv_serializeData(tlvP->value.asChildren.array + index,
                                    baseUriStr,
                                    baseUriLen,
                                    baseLevel,
                                    uriStr,
                                    uriLen,
                                    level,
                                    baseNameOutput,
                                    buffer + head,
                                    bufferLen - head);
            if (res < 0) return -1;
            head += res;
        }
    }
    break;

    default:
        head = 0;
        if (bufferLen < 1) return -1;
        buffer[head++] = JSON_ITEM_BEGIN;

        if (!*baseNameOutput && baseUriLen > 0)
        {
            if (bufferLen - head < baseUriLen + JSON_BN_HEADER_SIZE + 2) return -1;
            memcpy(buffer + head, JSON_BN_HEADER, JSON_BN_HEADER_SIZE);
            head += JSON_BN_HEADER_SIZE;
            memcpy(buffer + head, baseUriStr, baseUriLen);
            head += baseUriLen;
            buffer[head++] = JSON_ITEM_STRING_END;
            buffer[head++] = JSON_SEPARATOR;
            *baseNameOutput = true;
        }

        /* TODO: support base time */

        if (!baseUriLen || level > baseLevel)
        {
            if (bufferLen - head < JSON_ITEM_URI_SIZE) return -1;
            memcpy(buffer + head, JSON_ITEM_URI, JSON_ITEM_URI_SIZE);
            head += JSON_ITEM_URI_SIZE;

            if (parentUriLen > 0)
            {
                if (bufferLen - head < parentUriLen) return -1;
                memcpy(buffer + head, parentUriStr, parentUriLen);
                head += parentUriLen;
            }

            res = utils_intToText(tlvP->id, buffer + head, bufferLen - head);
            if (res <= 0) return -1;
            head += res;

            if (bufferLen - head < 2) return -1;
            buffer[head++] = JSON_ITEM_URI_END;
            if (tlvP->type != LWM2M_TYPE_UNDEFINED)
            {
                buffer[head++] = JSON_SEPARATOR;
            }
        }

        if (tlvP->type != LWM2M_TYPE_UNDEFINED)
        {
            res = prv_serializeValue(tlvP, buffer + head, bufferLen - head);
            if (res < 0) return -1;
            head += res;
        }

        /* TODO: support time */

        if (bufferLen - head < 1) return -1;
        buffer[head++] = JSON_ITEM_END;

        break;
    }

    return (int)head;
}

int senml_json_serialize(const lwm2m_uri_t * uriP,
                         int size,
                         const lwm2m_data_t * tlvP,
                         uint8_t ** bufferP)
{
    int index;
    size_t head;
    uint8_t bufferJSON[PRV_JSON_BUFFER_SIZE];
    uint8_t baseUriStr[URI_MAX_STRING_LEN];
    int baseUriLen;
    uri_depth_t rootLevel;
    uri_depth_t baseLevel;
    int num;
    lwm2m_data_t * targetP;
    const uint8_t *parentUriStr = NULL;
    size_t parentUriLen = 0;

    LOG_ARG("size: %d", size);
    LOG_URI(uriP);
    if (size != 0 && tlvP == NULL) return -1;

    baseUriLen = uri_toString(uriP, baseUriStr, URI_MAX_STRING_LEN, &baseLevel);
    if (baseUriLen < 0) return -1;
    if (baseUriLen > 1
     && baseLevel != URI_DEPTH_RESOURCE
     && baseLevel != URI_DEPTH_RESOURCE_INSTANCE)
    {
        if (baseUriLen >= URI_MAX_STRING_LEN -1) return 0;
        baseUriStr[baseUriLen++] = '/';
    }

    num = json_findAndCheckData(uriP, json_decreaseLevel(baseLevel), size, tlvP, &targetP);
    if (num < 0) return -1;

    switch (tlvP->type)
    {
    case LWM2M_TYPE_OBJECT:
        rootLevel = URI_DEPTH_OBJECT;
        break;
    case LWM2M_TYPE_OBJECT_INSTANCE:
        rootLevel = URI_DEPTH_OBJECT_INSTANCE;
        break;
    case LWM2M_TYPE_MULTIPLE_RESOURCE:
        if (baseUriLen > 1 && baseUriStr[baseUriLen - 1] != '/')
        {
            if (baseUriLen >= URI_MAX_STRING_LEN -1) return 0;
            baseUriStr[baseUriLen++] = '/';
        }
        rootLevel = URI_DEPTH_RESOURCE_INSTANCE;
        break;
    default:
        if (baseLevel == URI_DEPTH_RESOURCE_INSTANCE)
        {
            rootLevel = URI_DEPTH_RESOURCE_INSTANCE;
        }
        else
        {
            rootLevel = URI_DEPTH_RESOURCE;
        }
        break;
    }

    if (!baseUriLen || baseUriStr[baseUriLen - 1] != '/')
    {
        parentUriStr = (const uint8_t *)"/";
        parentUriLen = 1;
    }

    head = 0;
    bufferJSON[head++] = JSON_HEADER;

    bool baseNameOutput = false;
    for (index = 0 ; index < num && head < PRV_JSON_BUFFER_SIZE ; index++)
    {
        int res;

        if (index != 0)
        {
            if (head + 1 > PRV_JSON_BUFFER_SIZE) return 0;
            bufferJSON[head++] = JSON_SEPARATOR;
        }

        res = prv_serializeData(targetP + index,
                                baseUriStr,
                                baseUriLen,
                                baseLevel,
                                parentUriStr,
                                parentUriLen,
                                rootLevel,
                                &baseNameOutput,
                                bufferJSON + head,
                                PRV_JSON_BUFFER_SIZE - head);
        if (res < 0) return res;
        head += res;
    }

    if (head + 1 > PRV_JSON_BUFFER_SIZE) return 0;
    bufferJSON[head++] = JSON_FOOTER;

    *bufferP = (uint8_t *)lwm2m_malloc(head);
    if (*bufferP == NULL) return -1;
    memcpy(*bufferP, bufferJSON, head);

    return head;
}

#endif

