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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>


#ifdef LWM2M_SUPPORT_JSON

#define PRV_JSON_BUFFER_SIZE 1024

#define JSON_MIN_ARRAY_LEN      21      // e":[{"n":"N","v":X}]}
#define JSON_MIN_BASE_LEN        7      // n":"N",
#define JSON_ITEM_MAX_SIZE      36      // with ten characters for value
#define JSON_MIN_BX_LEN          5      // bt":1

#define JSON_FALSE_STRING  "false"
#define JSON_TRUE_STRING   "true"

#define JSON_RES_ITEM_URI                 "{\"n\":\""
#define JSON_RES_ITEM_URI_SIZE            6
#define JSON_ITEM_BOOL_TRUE               "\",\"bv\":true},"
#define JSON_ITEM_BOOL_TRUE_SIZE          13
#define JSON_ITEM_BOOL_FALSE              "\",\"bv\":false},"
#define JSON_ITEM_BOOL_FALSE_SIZE         14
#define JSON_ITEM_NUM                     "\",\"v\":"
#define JSON_ITEM_NUM_SIZE                6
#define JSON_ITEM_NUM_END                 "},"
#define JSON_ITEM_NUM_END_SIZE            2
#define JSON_ITEM_STRING_BEGIN            "\",\"sv\":\""
#define JSON_ITEM_STRING_BEGIN_SIZE       8
#define JSON_ITEM_STRING_END              "\"},"
#define JSON_ITEM_STRING_END_SIZE         3
#define JSON_ITEM_OBJECT_LINK_BEGIN       "\",\"ov\":\""
#define JSON_ITEM_OBJECT_LINK_BEGIN_SIZE  8
#define JSON_ITEM_OBJECT_LINK_END         "\"},"
#define JSON_ITEM_OBJECT_LINK_END_SIZE    3

#define JSON_BN_HEADER_1        "{\"bn\":\""
#define JSON_BN_HEADER_1_SIZE   7
#define JSON_BN_HEADER_2        "\",\"e\":["
#define JSON_BN_HEADER_2_SIZE   7
#define JSON_HEADER             "{\"e\":["
#define JSON_HEADER_SIZE        6
#define JSON_FOOTER             "]}"
#define JSON_FOOTER_SIZE        2


#define _GO_TO_NEXT_CHAR(I,B,L)         \
    {                                   \
        I++;                            \
        I += json_skipSpace(B+I, L-I);   \
        if (I == L) goto error;         \
    }

typedef enum
{
    _TYPE_UNSET,
    _TYPE_FALSE,
    _TYPE_TRUE,
    _TYPE_FLOAT,
    _TYPE_STRING,
    _TYPE_OBJECT_LINK
} _type;

typedef struct
{
    uint16_t        ids[4];
    _type           type;
    const uint8_t * value;
    size_t          valueLen;
} _record_t;

static int prv_parseItem(const uint8_t * buffer,
                         size_t bufferLen,
                         _record_t * recordP)
{
    size_t index;

    memset(recordP->ids, 0xFF, 4*sizeof(uint16_t));
    recordP->type = _TYPE_UNSET;
    recordP->value = NULL;
    recordP->valueLen = 0;

    index = 0;
    do
    {
        size_t tokenStart;
        size_t tokenLen;
        size_t valueStart;
        size_t valueLen;
        int next;

        next = json_split(buffer+index, bufferLen-index, &tokenStart, &tokenLen, &valueStart, &valueLen);
        if (next < 0) return -1;

        switch (tokenLen)
        {
        case 1:
        {
            switch (buffer[index+tokenStart])
            {
            case 'n':
            {
                size_t i;
                size_t j;

                if (recordP->ids[0] != LWM2M_MAX_ID) return -1;

                // Check for " around URI
                if (valueLen < 2
                 || buffer[index+valueStart] != '"'
                 || buffer[index+valueStart+valueLen-1] != '"')
                {
                    return -1;
                }
                // Ignore starting /
                if (buffer[index + valueStart + 1] == '/')
                {
                    if (valueLen < 4)
                    {
                        return -1;
                    }
                    valueStart += 1;
                    valueLen -= 1;
                }
                i = 0;
                j = 0;
                if (valueLen > 1)
                {
                    do {
                        uint32_t readId;

                        readId = 0;
                        i++;
                        while (i < valueLen-1 && buffer[index+valueStart+i] != '/')
                        {
                            if (buffer[index+valueStart+i] < '0'
                             || buffer[index+valueStart+i] > '9')
                            {
                                return -1;
                            }
                            readId *= 10;
                            readId += buffer[index+valueStart+i] - '0';
                            if (readId > LWM2M_MAX_ID) return -1;
                            i++;
                        }
                        recordP->ids[j] = readId;
                        j++;
                    } while (i < valueLen-1 && j < 4 && buffer[index+valueStart+i] == '/');
                    if (i < valueLen-1 ) return -1;
                }
            }
            break;

            case 'v':
                if (recordP->type != _TYPE_UNSET) return -1;
                recordP->type = _TYPE_FLOAT;
                recordP->value = buffer + index + valueStart;
                recordP->valueLen = valueLen;
                break;

            case 't':
                // TODO: support time
                break;

            default:
                return -1;
            }
        }
        break;

        case 2:
        {
            // "bv", "ov", or "sv"
            if (buffer[index+tokenStart+1] != 'v') return -1;
            switch (buffer[index+tokenStart])
            {
            case 'b':
                if (recordP->type != _TYPE_UNSET) return -1;
                if (0 == lwm2m_strncmp(JSON_TRUE_STRING, (char *)buffer + index + valueStart, valueLen))
                {
                    recordP->type = _TYPE_TRUE;
                }
                else if (0 == lwm2m_strncmp(JSON_FALSE_STRING, (char *)buffer + index + valueStart, valueLen))
                {
                    recordP->type = _TYPE_FALSE;
                }
                else
                {
                    return -1;
                }
                break;

            case 'o':
                if (recordP->type != _TYPE_UNSET) return -1;
                // Check for " around value
                if (valueLen < 2
                 || buffer[index+valueStart] != '"'
                 || buffer[index+valueStart+valueLen-1] != '"')
                {
                    return -1;
                }
                recordP->type = _TYPE_OBJECT_LINK;
                recordP->value = buffer + index + valueStart + 1;
                recordP->valueLen = valueLen - 2;
                break;

            case 's':
                if (recordP->type != _TYPE_UNSET) return -1;
                // Check for " around value
                if (valueLen < 2
                 || buffer[index+valueStart] != '"'
                 || buffer[index+valueStart+valueLen-1] != '"')
                {
                    return -1;
                }
                recordP->type = _TYPE_STRING;
                recordP->value = buffer + index + valueStart + 1;
                recordP->valueLen = valueLen - 2;
                break;

            default:
                return -1;
            }
        }
        break;

        default:
            return -1;
        }

        index += next + 1;
    } while (index < bufferLen);

    return 0;
}

static bool prv_convertValue(_record_t * recordP,
                             lwm2m_data_t * targetP)
{
    switch (recordP->type)
    {
    case _TYPE_FALSE:
        lwm2m_data_encode_bool(false, targetP);
        break;
    case _TYPE_TRUE:
        lwm2m_data_encode_bool(true, targetP);
        break;
    case _TYPE_FLOAT:
        if(1 != json_convertNumeric(recordP->value, recordP->valueLen, targetP)) return false;
    break;

    case _TYPE_STRING:
        if (0 != recordP->valueLen)
        {
            size_t stringLen;
            uint8_t *string = (uint8_t *)lwm2m_malloc(recordP->valueLen);
            if (string == NULL) return false;
            stringLen = json_unescapeString(string, recordP->value, recordP->valueLen);
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

    case _TYPE_OBJECT_LINK:
        if (1 != utils_textToObjLink(recordP->value,
                                     recordP->valueLen,
                                     &targetP->value.asObjLink.objectId,
                                     &targetP->value.asObjLink.objectInstanceId))
        {
            return false;
        }
        targetP->type = LWM2M_TYPE_OBJECT_LINK;
        break;

    case _TYPE_UNSET:
    default:
        return false;
    }

    return true;
}

static int prv_convertRecord(lwm2m_uri_t * uriP,
                             _record_t * recordArray,
                             int count,
                             lwm2m_data_t ** dataP)
{
    int index;
    int freeIndex;
    lwm2m_data_t * rootP;
    int size;
    uri_depth_t rootLevel;

    if (uriP == NULL)
    {
        size = count;
        *dataP = lwm2m_data_new(count);
        if (NULL == *dataP) return -1;
        rootLevel = URI_DEPTH_OBJECT;
        rootP = *dataP;
    }
    else
    {
        lwm2m_data_t * parentP;
        size = 1;

        *dataP = lwm2m_data_new(1);
        if (NULL == *dataP) return -1;
        (*dataP)->type = LWM2M_TYPE_OBJECT;
        (*dataP)->id = uriP->objectId;
        rootLevel = URI_DEPTH_OBJECT_INSTANCE;
        parentP = *dataP;
        if (LWM2M_URI_IS_SET_INSTANCE(uriP))
        {
            parentP->value.asChildren.count = 1;
            parentP->value.asChildren.array = lwm2m_data_new(1);
            if (NULL == parentP->value.asChildren.array) goto error;
            parentP = parentP->value.asChildren.array;
            parentP->type = LWM2M_TYPE_OBJECT_INSTANCE;
            parentP->id = uriP->instanceId;
            rootLevel = URI_DEPTH_RESOURCE;
            if (LWM2M_URI_IS_SET_RESOURCE(uriP))
            {
                parentP->value.asChildren.count = 1;
                parentP->value.asChildren.array = lwm2m_data_new(1);
                if (NULL == parentP->value.asChildren.array) goto error;
                parentP = parentP->value.asChildren.array;
                parentP->type = LWM2M_TYPE_MULTIPLE_RESOURCE;
                parentP->id = uriP->resourceId;
                rootLevel = URI_DEPTH_RESOURCE_INSTANCE;
            }
        }
        parentP->value.asChildren.count = count;
        parentP->value.asChildren.array = lwm2m_data_new(count);
        if (NULL == parentP->value.asChildren.array) goto error;
        rootP = parentP->value.asChildren.array;
    }

    freeIndex = 0;
    for (index = 0 ; index < count ; index++)
    {
        lwm2m_data_t * targetP;
        int resSegmentIndex;
        int i;

        // check URI depth
        // resSegmentIndex is set to the resource segment position
        switch(rootLevel)
        {
        case URI_DEPTH_OBJECT:
            resSegmentIndex = 2;
            break;
        case URI_DEPTH_OBJECT_INSTANCE:
            resSegmentIndex = 1;
            break;
        case URI_DEPTH_RESOURCE:
            resSegmentIndex = 0;
            break;
        case URI_DEPTH_RESOURCE_INSTANCE:
            resSegmentIndex = -1;
            break;
        default:
            goto error;
        }
        for (i = 0 ; i <= resSegmentIndex ; i++)
        {
            if (recordArray[index].ids[i] == LWM2M_MAX_ID) goto error;
        }
        if (resSegmentIndex < 2)
        {
            if (recordArray[index].ids[resSegmentIndex + 2] != LWM2M_MAX_ID) goto error;
        }

        targetP = json_findDataItem(rootP, count, recordArray[index].ids[0]);
        if (targetP == NULL)
        {
            targetP = rootP + freeIndex;
            freeIndex++;
            targetP->id = recordArray[index].ids[0];
            targetP->type = utils_depthToDatatype(rootLevel);
        }
        if (recordArray[index].ids[1] != LWM2M_MAX_ID)
        {
            lwm2m_data_t * parentP;
            uri_depth_t level;

            parentP = targetP;
            level = json_decreaseLevel(rootLevel);
            for (i = 1 ; i <= resSegmentIndex ; i++)
            {
                targetP = json_findDataItem(parentP->value.asChildren.array, parentP->value.asChildren.count, recordArray[index].ids[i]);
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
            if (recordArray[index].ids[resSegmentIndex + 1] != LWM2M_MAX_ID)
            {
                targetP->type = LWM2M_TYPE_MULTIPLE_RESOURCE;
                targetP = json_extendData(targetP);
                if (targetP == NULL) goto error;
                targetP->id = recordArray[index].ids[resSegmentIndex + 1];
                targetP->type = LWM2M_TYPE_UNDEFINED;
            }
        }

        if (true != prv_convertValue(recordArray + index, targetP)) goto error;
    }


    if (freeIndex < count)
    {
        lwm2m_data_t * newRootP = lwm2m_data_new(freeIndex);
        if (newRootP == NULL) goto error;
        memcpy(newRootP, rootP, freeIndex * sizeof(lwm2m_data_t));
        if (uriP == NULL)
        {
            *dataP = newRootP;
            size = freeIndex;
        }
        else
        {
            lwm2m_data_t * parentP = *dataP;
            while (parentP->value.asChildren.array != rootP)
            {
                parentP = parentP->value.asChildren.array;
            }
            parentP->value.asChildren.array = newRootP;
            parentP->value.asChildren.count = freeIndex;
        }
        lwm2m_free(rootP);     /* do not use lwm2m_data_free() to keep pointed values */
    }

    return size;

error:
    lwm2m_data_free(size, *dataP);
    *dataP = NULL;

    return -1;
}

int json_parse(lwm2m_uri_t * uriP,
               const uint8_t * buffer,
               size_t bufferLen,
               lwm2m_data_t ** dataP)
{
    size_t index;
    int count = 0;
    bool eFound = false;
    bool bnFound = false;
    bool btFound = false;
    size_t bnStart;
    size_t bnLen;
    _record_t * recordArray;
    lwm2m_data_t * parsedP;

    LOG_ARG("bufferLen: %d, buffer: \"%.*s\"", bufferLen, bufferLen, STR_NULL2EMPTY((char *)buffer));
    LOG_URI(uriP);
    *dataP = NULL;
    recordArray = NULL;
    parsedP = NULL;

    index = json_skipSpace(buffer, bufferLen);
    if (index == bufferLen) return -1;

    if (buffer[index] != '{') return -1;
    do
    {
        _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
        if (buffer[index] != '"') goto error;
        if (index++ >= bufferLen) goto error;
        switch (buffer[index])
        {
        case 'e':
        {
            int recordIndex;

            if (bufferLen-index < JSON_MIN_ARRAY_LEN) goto error;
            index++;
            if (buffer[index] != '"') goto error;
            if (eFound == true) goto error;
            eFound = true;

            _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
            if (buffer[index] != ':') goto error;
            _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
            if (buffer[index] != '[') goto error;
            _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
            count = json_countItems(buffer + index, bufferLen - index);
            if (count <= 0) goto error;
            recordArray = (_record_t*)lwm2m_malloc(count * sizeof(_record_t));
            if (recordArray == NULL) goto error;
            // at this point we are sure buffer[index] is '{' and all { and } are matching
            recordIndex = 0;
            while (recordIndex < count)
            {
                int itemLen = json_itemLength(buffer + index, bufferLen - index);
                if (itemLen < 0) goto error;
                if (0 != prv_parseItem(buffer + index + 1, itemLen - 2, recordArray + recordIndex))
                {
                    goto error;
                }
                recordIndex++;
                index += itemLen - 1;
                _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
                switch (buffer[index])
                {
                case ',':
                    _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
                    break;
                case ']':
                    if (recordIndex == count) break;
                    // else this is an error
                default:
                    goto error;
                }
            }
            if (buffer[index] != ']') goto error;
        }
        break;

        case 'b':
            if (bufferLen-index < JSON_MIN_BX_LEN) goto error;
            index++;
            switch (buffer[index])
            {
            case 't':
                index++;
                if (buffer[index] != '"') goto error;
                if (btFound == true) goto error;
                btFound = true;

                // TODO: handle timed values
                // temp: skip this token
                while(index < bufferLen && buffer[index] != ',' && buffer[index] != '}') index++;
                if (index == bufferLen) goto error;
                index--;
                // end temp
                break;
            case 'n':
                {
                    int next;
                    size_t tokenStart;
                    size_t tokenLen;
                    int itemLen;

                    index++;
                    if (buffer[index] != '"') goto error;
                    if (bnFound == true) goto error;
                    bnFound = true;
                    index -= 3;
                    itemLen = 0;
                    while (buffer[index + itemLen] != '}'
                        && buffer[index + itemLen] != ','
                        && index + itemLen < bufferLen)
                    {
                        itemLen++;
                    }
                    if (index + itemLen == bufferLen) goto error;
                    next = json_split(buffer+index, itemLen, &tokenStart, &tokenLen, &bnStart, &bnLen);
                    if (next < 0) goto error;
                    bnStart += index;
                    index += next - 1;
                }
                break;
            default:
                goto error;
            }
            break;

        default:
            goto error;
        }

        _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
    } while (buffer[index] == ',');

    if (buffer[index] != '}') goto error;

    if (eFound == true)
    {
        lwm2m_uri_t baseURI;
        lwm2m_uri_t * baseUriP;
        lwm2m_data_t * resultP;
        int size;

        LWM2M_URI_RESET(&baseURI);
        if (bnFound == false)
        {
            baseUriP = uriP;
        }
        else
        {
            int res;

            // we ignore the request URI and use the bn one.

            // Check for " around URI
            if (bnLen < 3
             || buffer[bnStart] != '"'
             || buffer[bnStart+bnLen-1] != '"')
            {
                goto error;
            }
            bnStart += 1;
            bnLen -= 2;

            if (bnLen == 1)
            {
                if (buffer[bnStart] != '/') goto error;
                baseUriP = NULL;
            }
            else
            {
                /* Base name may have a trailing "/" on a multiple instance
                 * resource. This isn't valid for a URI string in LWM2M 1.0.
                 * Strip off any trailing "/" to avoid an error. */
                if (buffer[bnStart + bnLen - 1] == '/') bnLen -= 1;
                res = lwm2m_stringToUri((char *)buffer + bnStart, bnLen, &baseURI);
                if (res < 0 || (size_t)res != bnLen) goto error;
                baseUriP = &baseURI;
            }
        }

        count = prv_convertRecord(baseUriP, recordArray, count, &parsedP);
        lwm2m_free(recordArray);
        recordArray = NULL;

        if (count > 0 && uriP != NULL)
        {
            if (parsedP->type != LWM2M_TYPE_OBJECT || parsedP->id != uriP->objectId) goto error;
            if (!LWM2M_URI_IS_SET_INSTANCE(uriP))
            {
                size = parsedP->value.asChildren.count;
                resultP = parsedP->value.asChildren.array;
            }
            else
            {
                int i;

                resultP = NULL;
                // be permissive and allow full object JSON when requesting for a single instance
                for (i = 0 ; i < (int)parsedP->value.asChildren.count && resultP == NULL; i++)
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
    }

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

static int prv_serializeValue(lwm2m_data_t * tlvP,
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
        if (tlvP->value.asBuffer.length != 0 && res == 0) return -1;
        head += res;

        if (bufferLen - head < JSON_ITEM_STRING_END_SIZE) return -1;
        memcpy(buffer + head, JSON_ITEM_STRING_END, JSON_ITEM_STRING_END_SIZE);
        head += JSON_ITEM_STRING_END_SIZE;

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

        if (bufferLen - head < JSON_ITEM_NUM_END_SIZE) return -1;
        memcpy(buffer + head, JSON_ITEM_NUM_END, JSON_ITEM_NUM_END_SIZE);
        head += JSON_ITEM_NUM_END_SIZE;
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

        if (bufferLen - head < JSON_ITEM_NUM_END_SIZE) return -1;
        memcpy(buffer + head, JSON_ITEM_NUM_END, JSON_ITEM_NUM_END_SIZE);
        head += JSON_ITEM_NUM_END_SIZE;
    }
    break;

    case LWM2M_TYPE_FLOAT:
    {
        double value;

        if (0 == lwm2m_data_decode_float(tlvP, &value)) return -1;

        if (bufferLen < JSON_ITEM_NUM_SIZE) return -1;
        memcpy(buffer, JSON_ITEM_NUM, JSON_ITEM_NUM_SIZE);
        head = JSON_ITEM_NUM_SIZE;

        res = utils_floatToText(value, buffer + head, bufferLen - head, true);
        if (!res) return -1;
        /* Error if inf or nan */
        if (buffer[head] != '-' && !isdigit(buffer[head])) return -1;
        if (res > 1 && buffer[head] == '-' && !isdigit(buffer[head+1])) return -1;
        head += res;

        if (bufferLen - head < JSON_ITEM_NUM_END_SIZE) return -1;
        memcpy(buffer + head, JSON_ITEM_NUM_END, JSON_ITEM_NUM_END_SIZE);
        head += JSON_ITEM_NUM_END_SIZE;
    }
    break;

    case LWM2M_TYPE_BOOLEAN:
    {
        bool value;

        if (0 == lwm2m_data_decode_bool(tlvP, &value)) return -1;

        if (value == true)
        {
            if (bufferLen < JSON_ITEM_BOOL_TRUE_SIZE) return -1;
            memcpy(buffer, JSON_ITEM_BOOL_TRUE, JSON_ITEM_BOOL_TRUE_SIZE);
            head = JSON_ITEM_BOOL_TRUE_SIZE;
        }
        else
        {
            if (bufferLen < JSON_ITEM_BOOL_FALSE_SIZE) return -1;
            memcpy(buffer, JSON_ITEM_BOOL_FALSE, JSON_ITEM_BOOL_FALSE_SIZE);
            head = JSON_ITEM_BOOL_FALSE_SIZE;
        }
    }
    break;

    case LWM2M_TYPE_OPAQUE:
        if (bufferLen < JSON_ITEM_STRING_BEGIN_SIZE) return -1;
        memcpy(buffer, JSON_ITEM_STRING_BEGIN, JSON_ITEM_STRING_BEGIN_SIZE);
        head = JSON_ITEM_STRING_BEGIN_SIZE;

        res = utils_base64Encode(tlvP->value.asBuffer.buffer, tlvP->value.asBuffer.length, buffer+head, bufferLen - head);
        if (tlvP->value.asBuffer.length != 0 && res == 0) return -1;
        head += res;

        if (bufferLen - head < JSON_ITEM_STRING_END_SIZE) return -1;
        memcpy(buffer + head, JSON_ITEM_STRING_END, JSON_ITEM_STRING_END_SIZE);
        head += JSON_ITEM_STRING_END_SIZE;
        break;

    case LWM2M_TYPE_OBJECT_LINK:
        if (bufferLen < JSON_ITEM_OBJECT_LINK_BEGIN_SIZE) return -1;
        memcpy(buffer, JSON_ITEM_OBJECT_LINK_BEGIN, JSON_ITEM_OBJECT_LINK_BEGIN_SIZE);
        head = JSON_ITEM_OBJECT_LINK_BEGIN_SIZE;

        res = utils_objLinkToText(tlvP->value.asObjLink.objectId,
                                  tlvP->value.asObjLink.objectInstanceId,
                                  buffer + head,
                                  bufferLen - head);
        if (!res) return -1;
        head += res;

        if (bufferLen - head < JSON_ITEM_OBJECT_LINK_END_SIZE) return -1;
        memcpy(buffer + head, JSON_ITEM_OBJECT_LINK_END, JSON_ITEM_OBJECT_LINK_END_SIZE);
        head += JSON_ITEM_OBJECT_LINK_END_SIZE;
        break;

    default:
        return -1;
    }

    return (int)head;
}

static int prv_serializeData(lwm2m_data_t * tlvP,
                             const uint8_t * parentUriStr,
                             size_t parentUriLen,
                             uint8_t * buffer,
                             size_t bufferLen)
{
    int head;
    int res;

    head = 0;

    switch (tlvP->type)
    {
    case LWM2M_TYPE_OBJECT:
    case LWM2M_TYPE_OBJECT_INSTANCE:
    case LWM2M_TYPE_MULTIPLE_RESOURCE:
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
        res = utils_intToText(tlvP->id, uriStr + uriLen, URI_MAX_STRING_LEN - uriLen);
        if (res <= 0) return -1;
        uriLen += res;
        uriStr[uriLen] = '/';
        uriLen++;

        head = 0;
        for (index = 0 ; index < tlvP->value.asChildren.count; index++)
        {
            res = prv_serializeData(tlvP->value.asChildren.array + index, uriStr, uriLen, buffer + head, bufferLen - head);
            if (res < 0) return -1;
            head += res;
        }
    }
    break;

    default:
        if (bufferLen < JSON_RES_ITEM_URI_SIZE) return -1;
        memcpy(buffer, JSON_RES_ITEM_URI, JSON_RES_ITEM_URI_SIZE);
        head = JSON_RES_ITEM_URI_SIZE;

        if (parentUriLen > 0)
        {
            if (bufferLen - head < parentUriLen) return -1;
            memcpy(buffer + head, parentUriStr, parentUriLen);
            head += parentUriLen;
        }

        res = utils_intToText(tlvP->id, buffer + head, bufferLen - head);
        if (res <= 0) return -1;
        head += res;

        res = prv_serializeValue(tlvP, buffer + head, bufferLen - head);
        if (res < 0) return -1;
        head += res;
        break;
    }

    return head;
}

int json_serialize(lwm2m_uri_t * uriP,
                   int size,
                   lwm2m_data_t * tlvP,
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
#ifndef LWM2M_VERSION_1_0
    lwm2m_uri_t uri;
#endif

    LOG_ARG("size: %d", size);
    LOG_URI(uriP);
    if (size != 0 && tlvP == NULL) return -1;

#ifndef LWM2M_VERSION_1_0
    if (uriP && LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP))
    {
        /* The resource instance doesn't get serialized as part of the base URI.
         * Strip it out. */
        memcpy(&uri, uriP, sizeof(lwm2m_uri_t));
        uri.resourceInstanceId = LWM2M_MAX_ID;
        uriP = &uri;
    }
#endif

    baseUriLen = uri_toString(uriP, baseUriStr, URI_MAX_STRING_LEN, &baseLevel);
    if (baseUriLen < 0) return -1;

    num = json_findAndCheckData(uriP, baseLevel, size, tlvP, &targetP, &rootLevel);
    if (num < 0) return -1;

    if (baseLevel >= URI_DEPTH_RESOURCE
     && rootLevel == baseLevel
     && baseUriLen > 1)
    {
        /* Remove the ID from the base name */
        while (baseUriLen > 1 && baseUriStr[baseUriLen - 1] != '/') baseUriLen--;
        if (baseUriLen > 1 && baseUriStr[baseUriLen - 1] == '/') baseUriLen--;
    }

    while (num == 1
        && (targetP->type == LWM2M_TYPE_OBJECT
         || targetP->type == LWM2M_TYPE_OBJECT_INSTANCE
         || targetP->type == LWM2M_TYPE_MULTIPLE_RESOURCE))
    {
        int res;

        if (baseUriLen >= URI_MAX_STRING_LEN -1) return 0;
        baseUriStr[baseUriLen++] = '/';
        res = utils_intToText(targetP->id, baseUriStr + baseUriLen, URI_MAX_STRING_LEN - baseUriLen);
        if (res <= 0) return 0;
        baseUriLen += res;
        num = targetP->value.asChildren.count;
        targetP = targetP->value.asChildren.array;
    }

    if (baseUriLen > 0)
    {
        if (baseUriLen >= URI_MAX_STRING_LEN -1) return 0;
        baseUriStr[baseUriLen++] = '/';
        memcpy(bufferJSON, JSON_BN_HEADER_1, JSON_BN_HEADER_1_SIZE);
        head = JSON_BN_HEADER_1_SIZE;
        memcpy(bufferJSON + head, baseUriStr, baseUriLen);
        head += baseUriLen;
        memcpy(bufferJSON + head, JSON_BN_HEADER_2, JSON_BN_HEADER_2_SIZE);
        head += JSON_BN_HEADER_2_SIZE;
    }
    else
    {
        memcpy(bufferJSON, JSON_HEADER, JSON_HEADER_SIZE);
        head = JSON_HEADER_SIZE;
        parentUriStr = (const uint8_t *)"/";
        parentUriLen = 1;
    }

    for (index = 0 ; index < num && head < PRV_JSON_BUFFER_SIZE ; index++)
    {
        int res;

        res = prv_serializeData(targetP + index,
                                parentUriStr,
                                parentUriLen,
                                bufferJSON + head,
                                PRV_JSON_BUFFER_SIZE - head);
        if (res < 0) return res;
        head += res;
    }

    if (head + JSON_FOOTER_SIZE - 1 > PRV_JSON_BUFFER_SIZE) return 0;

    if (num > 0) head = head - 1;

    memcpy(bufferJSON + head, JSON_FOOTER, JSON_FOOTER_SIZE);
    head = head + JSON_FOOTER_SIZE;

    *bufferP = (uint8_t *)lwm2m_malloc(head);
    if (*bufferP == NULL) return -1;
    memcpy(*bufferP, bufferJSON, head);

    return head;
}

#endif

