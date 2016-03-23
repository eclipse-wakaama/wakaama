/*******************************************************************************
*
* Copyright (c) 2013, 2014 Intel Corporation and others.
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* and Eclipse Distribution License v1.0 which accompany this distribution.
*
* The Eclipse Public License is available at
*    http://www.eclipse.org/legal/epl-v10.html
* The Eclipse Distribution License is available at
*    http://www.eclipse.org/org/documents/edl-v10.php.
*
* Contributors:
*    David Navarro, Intel Corporation - initial API and implementation
*    Fabien Fleutot - Please refer to git log
*    Bosch Software Innovations GmbH - Please refer to git log
*
*******************************************************************************/

#include "internals.h"
#include <float.h>


lwm2m_data_t * lwm2m_data_new(int size)
{
    lwm2m_data_t * dataP;

    if (size <= 0) return NULL;

    dataP = (lwm2m_data_t *)lwm2m_malloc(size * sizeof(lwm2m_data_t));

    if (dataP != NULL)
    {
        memset(dataP, 0, size * sizeof(lwm2m_data_t));
    }

    return dataP;
}

void lwm2m_data_free(int size,
                     lwm2m_data_t * dataP)
{
    int i;

    if (size == 0 || dataP == NULL) return;

    for (i = 0; i < size; i++)
    {
        if ((dataP[i].flags & LWM2M_TLV_FLAG_STATIC_DATA) == 0)
        {
            if (dataP[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE
                || dataP[i].type == LWM2M_TYPE_OBJECT_INSTANCE
                || dataP[i].type == LWM2M_TYPE_OBJECT)
            {
                lwm2m_data_free(dataP[i].length, (lwm2m_data_t *)(dataP[i].value));
            }
            else if (dataP[i].value != NULL)
            {
                lwm2m_free(dataP[i].value);
            }
        }
    }
    lwm2m_free(dataP);
}

void lwm2m_data_encode_int(int64_t value,
                           lwm2m_data_t * dataP)
{
    dataP->length = 0;
    dataP->dataType = LWM2M_TYPE_INTEGER;

    if ((dataP->flags & LWM2M_TLV_FLAG_TEXT_FORMAT) != 0)
    {
        dataP->flags &= ~LWM2M_TLV_FLAG_STATIC_DATA;
        dataP->length = lwm2m_int64ToPlainText(value, &dataP->value);
    }
    else
    {
        uint8_t buffer[_PRV_64BIT_BUFFER_SIZE];
        size_t length = 0;

        prv_encodeInt(value, buffer, &length);

        dataP->value = (uint8_t *)lwm2m_malloc(length);
        if (dataP->value != NULL)
        {
            memcpy(dataP->value,
                   buffer,
                   length);
            dataP->flags &= ~LWM2M_TLV_FLAG_STATIC_DATA;
            dataP->length = length;
        }
    }
}

int lwm2m_data_decode_int(const lwm2m_data_t * dataP,
                          int64_t * valueP)
{
    int result;

    if (dataP->length == 0) return 0;

    if ((dataP->flags & LWM2M_TLV_FLAG_TEXT_FORMAT) != 0)
    {
        result = lwm2m_PlainTextToInt64(dataP->value, dataP->length, valueP);
    }
    else
    {
        result = lwm2m_opaqueToInt(dataP->value, dataP->length, valueP);
        if (result == dataP->length)
        {
            result = 1;
        }
        else
        {
            result = 0;
        }
    }

    return result;
}

void lwm2m_data_encode_float(double value,
                             lwm2m_data_t * dataP)
{
    dataP->length = 0;
    dataP->dataType = LWM2M_TYPE_FLOAT;

    if ((dataP->flags & LWM2M_TLV_FLAG_TEXT_FORMAT) != 0)
    {
        dataP->flags &= ~LWM2M_TLV_FLAG_STATIC_DATA;
        dataP->length = lwm2m_float64ToPlainText(value, &dataP->value);
    }
    else
    {
        size_t length = 0;

        if (value > FLT_MAX || value < (0 - FLT_MAX))
        {
            length = 8;
        }
        else
        {
            length = 4;
        }

        dataP->value = (uint8_t *)lwm2m_malloc(length);
        if (dataP->value != NULL)
        {
            if (length == 4)
            {
                float temp;

                temp = value;

                utils_copyValue(dataP->value, &temp, length);
            }
            else
            {
                utils_copyValue(dataP->value, &value, length);
            }

            dataP->flags &= ~LWM2M_TLV_FLAG_STATIC_DATA;
            dataP->length = length;
        }
    }
}

int lwm2m_data_decode_float(const lwm2m_data_t * dataP,
                            double * valueP)
{
    int result;

    if (dataP->length == 0) return 0;

    if ((dataP->flags & LWM2M_TLV_FLAG_TEXT_FORMAT) != 0)
    {
        result = lwm2m_PlainTextToFloat64(dataP->value, dataP->length, valueP);
    }
    else
    {
        result = lwm2m_opaqueToFloat(dataP->value, dataP->length, valueP);
        if (result == dataP->length)
        {
            result = 1;
        }
        else
        {
            result = 0;
        }
    }

    return result;
}

void lwm2m_data_encode_bool(bool value,
                            lwm2m_data_t * dataP)
{
    dataP->length = 0;
    dataP->dataType = LWM2M_TYPE_BOOLEAN;

    dataP->value = (uint8_t *)lwm2m_malloc(1);
    if (dataP->value != NULL)
    {
        if (value == true)
        {
            if ((dataP->flags & LWM2M_TLV_FLAG_TEXT_FORMAT) != 0)
            {
                dataP->value[0] = '1';
            }
            else
            {
                dataP->value[0] = 1;
            }
        }
        else
        {
            if ((dataP->flags & LWM2M_TLV_FLAG_TEXT_FORMAT) != 0)
            {
                dataP->value[0] = '0';
            }
            else
            {
                dataP->value[0] = 0;
            }
        }
        dataP->flags &= ~LWM2M_TLV_FLAG_STATIC_DATA;
        dataP->length = 1;
    }
}

int lwm2m_data_decode_bool(const lwm2m_data_t * dataP,
                           bool * valueP)
{
    if (dataP->length != 1) return 0;

    if ((dataP->flags & LWM2M_TLV_FLAG_TEXT_FORMAT) != 0)
    {
        switch (dataP->value[0])
        {
        case '0':
            *valueP = false;
            break;
        case '1':
            *valueP = true;
            break;
        default:
            return 0;
        }
    }
    else
    {
        switch (dataP->value[0])
        {
        case 0:
            *valueP = false;
            break;
        case 1:
            *valueP = true;
            break;
        default:
            return 0;
        }
    }

    return 1;
}

void lwm2m_data_include(lwm2m_data_t * subDataP,
                        size_t count,
                        lwm2m_data_t * dataP)
{
    if (subDataP == NULL || count == 0) return;

    switch (subDataP[0].type)
    {
    case LWM2M_TYPE_RESOURCE:
    case LWM2M_TYPE_MULTIPLE_RESOURCE:
        dataP->type = LWM2M_TYPE_OBJECT_INSTANCE;
        break;
    case LWM2M_TYPE_RESOURCE_INSTANCE:
        dataP->type = LWM2M_TYPE_MULTIPLE_RESOURCE;
        break;
    default:
        break;
    }
    dataP->flags = 0;
    dataP->dataType = LWM2M_TYPE_UNDEFINED;
    dataP->length = count;
    dataP->value = (uint8_t *)subDataP;
}

int lwm2m_data_parse(lwm2m_uri_t * uriP,
                     uint8_t * buffer,
                     size_t bufferLen,
                     lwm2m_media_type_t format,
                     lwm2m_data_t ** dataP)
{
    switch (format)
    {
    case LWM2M_CONTENT_TEXT:
    case LWM2M_CONTENT_OPAQUE:
        *dataP = lwm2m_data_new(1);
        if (*dataP == NULL) return 0;
        (*dataP)->length = bufferLen;
        (*dataP)->value = buffer;
        (*dataP)->flags = LWM2M_TLV_FLAG_STATIC_DATA;
        return 1;

    case LWM2M_CONTENT_TLV:
        return lwm2m_tlv_parse(buffer, bufferLen, dataP);

#ifdef LWM2M_SUPPORT_JSON
    case LWM2M_CONTENT_JSON:
        return json_parse(uriP, buffer, bufferLen, dataP);
#endif

    default:
        return 0;
    }
}

int lwm2m_data_serialize(lwm2m_uri_t * uriP,
                         int size,
                         lwm2m_data_t * dataP,
                         lwm2m_media_type_t * formatP,
                         uint8_t ** bufferP)
{

    // Check format
    if (*formatP == LWM2M_CONTENT_TEXT
        || *formatP == LWM2M_CONTENT_OPAQUE)
    {
        if ((size != 1)
            || (dataP->type != LWM2M_TYPE_RESOURCE))
        {
#ifdef LWM2M_SUPPORT_JSON
            *formatP = LWM2M_CONTENT_JSON;
#else
            *formatP = LWM2M_CONTENT_TLV;
#endif
        }
    }

    switch (*formatP)
    {
    case LWM2M_CONTENT_TEXT:
    case LWM2M_CONTENT_OPAQUE:
        *bufferP = (uint8_t *)lwm2m_malloc(dataP->length);
        if (*bufferP == NULL) return 0;
        memcpy(*bufferP, dataP->value, dataP->length);
        return dataP->length;

    case LWM2M_CONTENT_TLV:
        return lwm2m_tlv_serialize(size, dataP, bufferP);

#ifdef LWM2M_CLIENT_MODE
    case LWM2M_CONTENT_LINK:
        return discover_serialize(NULL, uriP, size, dataP, bufferP);
#endif
#ifdef LWM2M_SUPPORT_JSON
    case LWM2M_CONTENT_JSON:
        return json_serialize(uriP, size, dataP, bufferP);
#endif

    default:
        return 0;
    }
}

