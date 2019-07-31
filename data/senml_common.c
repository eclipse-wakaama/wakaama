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

#if defined(LWM2M_SUPPORT_JSON) || defined(LWM2M_SUPPORT_SENML_JSON) || defined(LWM2M_SUPPORT_SENML_CBOR)

static int prv_convertRecord(const senml_record_t * recordArray,
                             int count,
                             lwm2m_data_t ** dataP,
                             senml_convertValue convertValue)
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

        targetP = senml_findDataItem(rootP, count, recordArray[index].ids[0]);
        if (targetP == NULL)
        {
            targetP = rootP + freeIndex;
            freeIndex++;
            targetP->id = recordArray[index].ids[0];
            if (targetP->id != LWM2M_MAX_ID)
            {
                targetP->type = LWM2M_TYPE_OBJECT;
            }
            else
            {
                targetP->type = LWM2M_TYPE_UNDEFINED;
            }
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
                targetP = senml_findDataItem(parentP->value.asChildren.array,
                                           parentP->value.asChildren.count,
                                           recordArray[index].ids[i]);
                if (targetP == NULL)
                {
                    targetP = senml_extendData(parentP,
                                               utils_depthToDatatype(level),
                                               recordArray[index].ids[i]);
                    if (targetP == NULL) goto error;
                }
                level = senml_decreaseLevel(level);
                parentP = targetP;
            }
            if (recordArray[index].ids[3] != LWM2M_MAX_ID)
            {
                targetP->type = LWM2M_TYPE_MULTIPLE_RESOURCE;
                targetP = senml_extendData(targetP,
                                           LWM2M_TYPE_UNDEFINED,
                                           recordArray[index].ids[3]);
                if (targetP == NULL) goto error;
            }
        }

        if (!convertValue(recordArray + index, targetP)) goto error;
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

int senml_convert_records(const lwm2m_uri_t* uriP,
                          senml_record_t * recordArray,
                          int numRecords,
                          senml_convertValue convertValue,
                          lwm2m_data_t ** dataP)
{
    lwm2m_data_t * resultP;
    lwm2m_data_t * parsedP = NULL;
    int size;
    int count;

    count = prv_convertRecord(recordArray, numRecords, &parsedP, convertValue);
    lwm2m_free(recordArray);

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
            /* be permissive and allow full object when requesting for a single instance */
            for (i = 0;
                 i < (int)parsedP->value.asChildren.count && resultP == NULL;
                 i++)
            {
                lwm2m_data_t* targetP;
                targetP = parsedP->value.asChildren.array + i;
                if (targetP->id == uriP->instanceId)
                {
                    resultP = targetP->value.asChildren.array;
                    size = targetP->value.asChildren.count;
                }
            }
            if (resultP == NULL)
                goto error;

            if (LWM2M_URI_IS_SET_RESOURCE(uriP))
            {
                lwm2m_data_t* resP;
                resP = NULL;
                for (i = 0; i < size && resP == NULL; i++)
                {
                    lwm2m_data_t* targetP;
                    targetP = resultP + i;
                    if (targetP->id == uriP->resourceId)
                    {
#ifndef LWM2M_VERSION_1_0
                        if (targetP->type == LWM2M_TYPE_MULTIPLE_RESOURCE
                         && LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP))
                        {
                            resP = targetP->value.asChildren.array;
                            size = targetP->value.asChildren.count;
                        }
                        else
#endif
                        {
                            size = senml_dataStrip(1, targetP, &resP);
                            if (size <= 0) goto error;

                            lwm2m_data_free(count, parsedP);
                            parsedP = NULL;
                        }
                    }
                }
                if (resP == NULL) goto error;

                resultP = resP;
            }
#ifndef LWM2M_VERSION_1_0
            if (LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP))
            {
                lwm2m_data_t* resP;
                resP = NULL;
                for (i = 0; i < size && resP == NULL; i++)
                {
                    lwm2m_data_t* targetP;
                    targetP = resultP + i;
                    if (targetP->id == uriP->resourceInstanceId)
                    {
                        size = senml_dataStrip(1, targetP, &resP);
                        if (size <= 0) goto error;

                        lwm2m_data_free(count, parsedP);
                        parsedP = NULL;
                    }
                }
                if (resP == NULL) goto error;

                resultP = resP;
            }
#endif
        }
    }
    else
    {
        resultP = parsedP;
        size = count;
    }
    if (parsedP != NULL)
    {
        lwm2m_data_t* tempP;
        size = senml_dataStrip(size, resultP, &tempP);
        if (size <= 0) goto error;

        lwm2m_data_free(count, parsedP);
        resultP = tempP;
    }
    count = size;
    *dataP = resultP;
    return count;

error:
    if (parsedP != NULL)
    {
        lwm2m_data_free(count, parsedP);
        parsedP = NULL;
    }
    return -1;
}

lwm2m_data_t * senml_extendData(lwm2m_data_t * parentP,
                                lwm2m_data_type_t type,
                                uint16_t id)
{
    int count = parentP->value.asChildren.count;
    if (lwm2m_data_append_one(&count,
                              &parentP->value.asChildren.array,
                              type,
                              id) != 0)
    {
        parentP->value.asChildren.count = count;
        return parentP->value.asChildren.array + count - 1;
    }
    return NULL;
}

int senml_dataStrip(int size, lwm2m_data_t * dataP, lwm2m_data_t ** resultP)
{
    int i;
    int j;

    *resultP = lwm2m_data_new(size);
    if (*resultP == NULL) return -1;

    j = 0;
    for (i = 0 ; i < size ; i++)
    {
        memcpy((*resultP) + j, dataP + i, sizeof(lwm2m_data_t));

        switch (dataP[i].type)
        {
        case LWM2M_TYPE_OBJECT:
        case LWM2M_TYPE_OBJECT_INSTANCE:
        case LWM2M_TYPE_MULTIPLE_RESOURCE:
        {
            if (dataP[i].value.asChildren.count != 0)
            {
                int childLen;

                childLen = senml_dataStrip(dataP[i].value.asChildren.count,
                                          dataP[i].value.asChildren.array,
                                          &((*resultP)[j].value.asChildren.array));
                if (childLen <= 0)
                {
                    /* skip this one */
                    j--;
                }
                else
                {
                    (*resultP)[j].value.asChildren.count = childLen;
                }
            }
            break;
        }
        case LWM2M_TYPE_STRING:
        case LWM2M_TYPE_OPAQUE:
        case LWM2M_TYPE_CORE_LINK:
            dataP[i].value.asBuffer.length = 0;
            dataP[i].value.asBuffer.buffer = NULL;
            break;
        default:
            /* do nothing */
            break;
        }

        j++;
    }

    return size;
}

lwm2m_data_t * senml_findDataItem(lwm2m_data_t * listP, size_t count, uint16_t id)
{
    size_t i;

    // TODO: handle times

    i = 0;
    while (i < count)
    {
        if (listP[i].type != LWM2M_TYPE_UNDEFINED && listP[i].id == id)
        {
            return listP + i;
        }
        i++;
    }

    return NULL;
}

uri_depth_t senml_decreaseLevel(uri_depth_t level)
{
    switch(level)
    {
    case URI_DEPTH_NONE:
        return URI_DEPTH_OBJECT;
    case URI_DEPTH_OBJECT:
        return URI_DEPTH_OBJECT_INSTANCE;
    case URI_DEPTH_OBJECT_INSTANCE:
        return URI_DEPTH_RESOURCE;
    case URI_DEPTH_RESOURCE:
        return URI_DEPTH_RESOURCE_INSTANCE;
    case URI_DEPTH_RESOURCE_INSTANCE:
        return URI_DEPTH_RESOURCE_INSTANCE;
    default:
        return URI_DEPTH_RESOURCE;
    }
}

static int prv_findAndCheckData(const lwm2m_uri_t * uriP,
                                uri_depth_t desiredLevel,
                                size_t size,
                                const lwm2m_data_t * tlvP,
                                lwm2m_data_t ** targetP,
                                uri_depth_t *targetLevelP)
{
    size_t index;
    int result;

    if (size == 0) return 0;

    if (size > 1)
    {
        if (tlvP[0].type == LWM2M_TYPE_OBJECT
         || tlvP[0].type == LWM2M_TYPE_OBJECT_INSTANCE)
        {
            for (index = 0; index < size; index++)
            {
                if (tlvP[index].type != tlvP[0].type)
                {
                    *targetP = NULL;
                    return -1;
                }
            }
        }
        else
        {
            for (index = 0; index < size; index++)
            {
                if (tlvP[index].type == LWM2M_TYPE_OBJECT
                 || tlvP[index].type == LWM2M_TYPE_OBJECT_INSTANCE)
                {
                    *targetP = NULL;
                    return -1;
                }
            }
        }
    }

    *targetP = NULL;
    result = -1;
    switch (desiredLevel)
    {
    case URI_DEPTH_OBJECT:
        if (tlvP[0].type == LWM2M_TYPE_OBJECT)
        {
            *targetP = (lwm2m_data_t*)tlvP;
            *targetLevelP = URI_DEPTH_OBJECT;
            result = (int)size;
        }
        break;

    case URI_DEPTH_OBJECT_INSTANCE:
        switch (tlvP[0].type)
        {
        case LWM2M_TYPE_OBJECT:
            for (index = 0; index < size; index++)
            {
                if (tlvP[index].id == uriP->objectId)
                {
                    *targetLevelP = URI_DEPTH_OBJECT_INSTANCE;
                    return prv_findAndCheckData(uriP,
                                                desiredLevel,
                                                tlvP[index].value.asChildren.count,
                                                tlvP[index].value.asChildren.array,
                                                targetP,
                                                targetLevelP);
                }
            }
            break;
        case LWM2M_TYPE_OBJECT_INSTANCE:
            *targetP = (lwm2m_data_t*)tlvP;
            result = (int)size;
            break;
        default:
            break;
        }
        break;

    case URI_DEPTH_RESOURCE:
        switch (tlvP[0].type)
        {
        case LWM2M_TYPE_OBJECT:
            for (index = 0; index < size; index++)
            {
                if (tlvP[index].id == uriP->objectId)
                {
                    *targetLevelP = URI_DEPTH_OBJECT_INSTANCE;
                    return prv_findAndCheckData(uriP,
                                                desiredLevel,
                                                tlvP[index].value.asChildren.count,
                                                tlvP[index].value.asChildren.array,
                                                targetP,
                                                targetLevelP);
                }
            }
            break;
        case LWM2M_TYPE_OBJECT_INSTANCE:
            for (index = 0; index < size; index++)
            {
                if (tlvP[index].id == uriP->instanceId)
                {
                    *targetLevelP = URI_DEPTH_RESOURCE;
                    return prv_findAndCheckData(uriP,
                                                desiredLevel,
                                                tlvP[index].value.asChildren.count,
                                                tlvP[index].value.asChildren.array,
                                                targetP, targetLevelP);
                }
            }
            break;
        default:
            *targetP = (lwm2m_data_t*)tlvP;
            result = (int)size;
            break;
        }
        break;

    case URI_DEPTH_RESOURCE_INSTANCE:
        switch (tlvP[0].type)
        {
        case LWM2M_TYPE_OBJECT:
            for (index = 0; index < size; index++)
            {
                if (tlvP[index].id == uriP->objectId)
                {
                    *targetLevelP = URI_DEPTH_OBJECT_INSTANCE;
                    return prv_findAndCheckData(uriP,
                                                desiredLevel,
                                                tlvP[index].value.asChildren.count,
                                                tlvP[index].value.asChildren.array,
                                                targetP,
                                                targetLevelP);
                }
            }
            break;
        case LWM2M_TYPE_OBJECT_INSTANCE:
            for (index = 0; index < size; index++)
            {
                if (tlvP[index].id == uriP->instanceId)
                {
                    *targetLevelP = URI_DEPTH_RESOURCE;
                    return prv_findAndCheckData(uriP,
                                                desiredLevel,
                                                tlvP[index].value.asChildren.count,
                                                tlvP[index].value.asChildren.array,
                                                targetP,
                                                targetLevelP);
                }
            }
            break;
        case LWM2M_TYPE_MULTIPLE_RESOURCE:
            for (index = 0; index < size; index++)
            {
                if (tlvP[index].id == uriP->resourceId)
                {
                    *targetLevelP = URI_DEPTH_RESOURCE_INSTANCE;
                    return prv_findAndCheckData(uriP,
                                                desiredLevel,
                                                tlvP[index].value.asChildren.count,
                                                tlvP[index].value.asChildren.array,
                                                targetP,
                                                targetLevelP);
                }
            }
            break;
        default:
            *targetP = (lwm2m_data_t*)tlvP;
            result = (int)size;
            break;
        }
        break;

    default:
        break;
    }

    return result;
}

int senml_findAndCheckData(const lwm2m_uri_t * uriP,
                           uri_depth_t baseLevel,
                           size_t size,
                           const lwm2m_data_t * tlvP,
                           lwm2m_data_t ** targetP,
                           uri_depth_t *targetLevelP)
{
    uri_depth_t desiredLevel = senml_decreaseLevel(baseLevel);
    if (baseLevel < URI_DEPTH_RESOURCE)
    {
        *targetLevelP = desiredLevel;
    }
    else
    {
        *targetLevelP = baseLevel;
    }
    return prv_findAndCheckData(uriP,
                                desiredLevel,
                                size,
                                tlvP,
                                targetP,
                                targetLevelP);
}

#endif
