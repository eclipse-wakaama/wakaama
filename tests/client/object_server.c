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
 *    Julien Vermillard, Sierra Wireless
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    
 *******************************************************************************/

/*
 *  Resources:
 *
 *          Name         | ID | Operations | Instances | Mandatory |  Type   |  Range  | Units |
 *  Short ID             |  0 |     R      |  Single   |    Yes    | Integer | 1-65535 |       |
 *  Lifetime             |  1 |    R/W     |  Single   |    Yes    | Integer |         |   s   |
 *  Default Min Period   |  2 |    R/W     |  Single   |    No     | Integer |         |   s   |
 *  Default Max Period   |  3 |    R/W     |  Single   |    No     | Integer |         |   s   |
 *  Disable              |  4 |     E      |  Single   |    No     |         |         |       |
 *  Disable Timeout      |  5 |    R/W     |  Single   |    No     | Integer |         |   s   |
 *  Notification Storing |  6 |    R/W     |  Single   |    Yes    | Boolean |         |       |
 *  Binding              |  7 |    R/W     |  Single   |    Yes    | String  |         |       |
 *  Registration Update  |  8 |     E      |  Single   |    Yes    |         |         |       |
 *
 */

#include "liblwm2m.h"
#include "lwm2mclient.h"

#include <stdlib.h>
#include <string.h>


typedef struct _server_instance_
{
    struct _server_instance_ * next;   // matches lwm2m_list_t::next
    uint16_t    instanceId;            // matches lwm2m_list_t::id
    uint16_t    shortServerId;
    uint32_t    lifetime;
    uint32_t    defMinPeriod;
    uint32_t    defMaxPeriod;
    uint32_t    disableTimeout;
    bool        storing;
    char        binding[4];
} server_instance_t;

static uint8_t prv_get_value(lwm2m_tlv_t * tlvP,
                             server_instance_t * targetP)
{
    // There are no multiple instance resources
    tlvP->type = LWM2M_TYPE_RESOURCE;

    switch (tlvP->id)
    {
    case LWM2M_SERVER_SHORT_ID_ID:
        lwm2m_tlv_encode_int(targetP->shortServerId, tlvP);
        if (0 != tlvP->length) return COAP_205_CONTENT;
        else return COAP_500_INTERNAL_SERVER_ERROR;

    case LWM2M_SERVER_LIFETIME_ID:
        lwm2m_tlv_encode_int(targetP->lifetime, tlvP);
        if (0 != tlvP->length) return COAP_205_CONTENT;
        else return COAP_500_INTERNAL_SERVER_ERROR;

    case LWM2M_SERVER_MIN_PERIOD_ID:
        lwm2m_tlv_encode_int(targetP->defMinPeriod, tlvP);
        if (0 != tlvP->length) return COAP_205_CONTENT;
        else return COAP_500_INTERNAL_SERVER_ERROR;

    case LWM2M_SERVER_MAX_PERIOD_ID:
        lwm2m_tlv_encode_int(targetP->defMaxPeriod, tlvP);
        if (0 != tlvP->length) return COAP_205_CONTENT;
        else return COAP_500_INTERNAL_SERVER_ERROR;

    case LWM2M_SERVER_DISABLE_ID:
        return COAP_405_METHOD_NOT_ALLOWED;

    case LWM2M_SERVER_TIMEOUT_ID:
        lwm2m_tlv_encode_int(targetP->disableTimeout, tlvP);
        if (0 != tlvP->length) return COAP_205_CONTENT;
        else return COAP_500_INTERNAL_SERVER_ERROR;

    case LWM2M_SERVER_STORING_ID:
        lwm2m_tlv_encode_bool(targetP->storing, tlvP);
        if (0 != tlvP->length) return COAP_205_CONTENT;
        else return COAP_500_INTERNAL_SERVER_ERROR;

    case LWM2M_SERVER_BINDING_ID:
        tlvP->value = (uint8_t*)targetP->binding;
        tlvP->length = strlen(targetP->binding);
        tlvP->flags = LWM2M_TLV_FLAG_STATIC_DATA;
        tlvP->dataType = LWM2M_TYPE_STRING;
        return COAP_205_CONTENT;

    case LWM2M_SERVER_UPDATE_ID:
        return COAP_405_METHOD_NOT_ALLOWED;

    default:
        return COAP_404_NOT_FOUND;
    }
}

static uint8_t prv_server_read(uint16_t instanceId,
                               int * numDataP,
                               lwm2m_tlv_t ** dataArrayP,
                               lwm2m_object_t * objectP)
{
    server_instance_t * targetP;
    uint8_t result;
    int i;

    targetP = (server_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    // is the server asking for the full instance ?
    if (*numDataP == 0)
    {
        uint16_t resList[] = {
                LWM2M_SERVER_SHORT_ID_ID,
                LWM2M_SERVER_LIFETIME_ID,
                LWM2M_SERVER_MIN_PERIOD_ID,
                LWM2M_SERVER_MAX_PERIOD_ID,
                LWM2M_SERVER_TIMEOUT_ID,
                LWM2M_SERVER_STORING_ID,
                LWM2M_SERVER_BINDING_ID
        };
        int nbRes = sizeof(resList)/sizeof(uint16_t);

        *dataArrayP = lwm2m_tlv_new(nbRes);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0 ; i < nbRes ; i++)
        {
            (*dataArrayP)[i].id = resList[i];
        }
    }

    i = 0;
    do
    {
        result = prv_get_value((*dataArrayP) + i, targetP);
        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);

    return result;
}

static uint8_t prv_server_write(uint16_t instanceId,
                                int numData,
                                lwm2m_tlv_t * dataArray,
                                lwm2m_object_t * objectP)
{
    server_instance_t * targetP;
    int i;
    uint8_t result;

    targetP = (server_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    i = 0;
    do
    {
        switch (dataArray[i].id)
        {
        case LWM2M_SERVER_SHORT_ID_ID:
            result = COAP_405_METHOD_NOT_ALLOWED;
            break;

        case LWM2M_SERVER_LIFETIME_ID:
        {
            int64_t value;

            if (1 == lwm2m_tlv_decode_int(dataArray + i, &value))
            {
                if (value >= 0 && value <= 0xFFFFFFFF)
                {
                    targetP->lifetime = value;
                    result = COAP_204_CHANGED;
                }
                else
                {
                    result = COAP_406_NOT_ACCEPTABLE;
                }
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
        }
        break;

        case LWM2M_SERVER_MIN_PERIOD_ID:
        {
            int64_t value;

            if (1 == lwm2m_tlv_decode_int(dataArray + i, &value))
            {
                if (value >= 0 && value <= 0xFFFFFFFF)
                {
                    targetP->defMinPeriod = value;
                    result = COAP_204_CHANGED;
                }
                else
                {
                    result = COAP_406_NOT_ACCEPTABLE;
                }
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
        }
        break;

        case LWM2M_SERVER_MAX_PERIOD_ID:
        {
            int64_t value;

            if (1 == lwm2m_tlv_decode_int(dataArray + i, &value))
            {
                if (value >= 0 && value <= 0xFFFFFFFF)
                {
                    targetP->defMaxPeriod = value;
                    result = COAP_204_CHANGED;
                }
                else
                {
                    result = COAP_406_NOT_ACCEPTABLE;
                }
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
        }
        break;

        case LWM2M_SERVER_DISABLE_ID:
            result = COAP_405_METHOD_NOT_ALLOWED;
            break;

        case LWM2M_SERVER_TIMEOUT_ID:
        {
            int64_t value;

            if (1 == lwm2m_tlv_decode_int(dataArray + i, &value))
            {
                if (value >= 0 && value <= 0xFFFFFFFF)
                {
                    targetP->disableTimeout = value;
                    result = COAP_204_CHANGED;
                }
                else
                {
                    result = COAP_406_NOT_ACCEPTABLE;
                }
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
        }
        break;

        case LWM2M_SERVER_STORING_ID:
        {
            bool value;

            if (1 == lwm2m_tlv_decode_bool(dataArray + i, &value))
            {
                targetP->storing = value;
                result = COAP_204_CHANGED;
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
        }
        break;

        case LWM2M_SERVER_BINDING_ID:
            if ((dataArray[i].length > 0 && dataArray[i].length <= 3)
             && (strncmp((char*)dataArray[i].value, "U",   dataArray[i].length) == 0
              || strncmp((char*)dataArray[i].value, "UQ",  dataArray[i].length) == 0
              || strncmp((char*)dataArray[i].value, "S",   dataArray[i].length) == 0
              || strncmp((char*)dataArray[i].value, "SQ",  dataArray[i].length) == 0
              || strncmp((char*)dataArray[i].value, "US",  dataArray[i].length) == 0
              || strncmp((char*)dataArray[i].value, "UQS", dataArray[i].length) == 0))
            {
                strncpy(targetP->binding, (char*)dataArray[i].value, dataArray[i].length);
                result = COAP_204_CHANGED;
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
            break;

        case LWM2M_SERVER_UPDATE_ID:
            result = COAP_405_METHOD_NOT_ALLOWED;
            break;

        default:
            return COAP_404_NOT_FOUND;
        }
        i++;
    } while (i < numData && result == COAP_204_CHANGED);

    return result;
}

static uint8_t prv_server_execute(uint16_t instanceId,
                                  uint16_t resourceId,
                                  char * buffer,
                                  int length,
                                  lwm2m_object_t * objectP)

{
    server_instance_t * targetP;

    targetP = (server_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    switch (resourceId)
    {
    case LWM2M_SERVER_DISABLE_ID:
        // executed in core, if COAP_204_CHANGED is returned
        if (0 < targetP->disableTimeout) return COAP_204_CHANGED;
        else return COAP_405_METHOD_NOT_ALLOWED;
    case LWM2M_SERVER_UPDATE_ID:
        // executed in core, if COAP_204_CHANGED is returned
        return COAP_204_CHANGED;
    default:
        return COAP_405_METHOD_NOT_ALLOWED;
    }
}

static uint8_t prv_server_delete(uint16_t id,
                                 lwm2m_object_t * objectP)
{
    server_instance_t * targetP;

    objectP->instanceList = lwm2m_list_remove(objectP->instanceList, id, (lwm2m_list_t **)&targetP);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    lwm2m_free(targetP);

    return COAP_202_DELETED;
}

static uint8_t prv_server_create(uint16_t instanceId,
                                 int numData,
                                 lwm2m_tlv_t * dataArray,
                                 lwm2m_object_t * objectP)
{
    server_instance_t * targetP;
    uint8_t result;

    targetP = (server_instance_t *)lwm2m_malloc(sizeof(server_instance_t));
    if (NULL == targetP) return COAP_500_INTERNAL_SERVER_ERROR;
    memset(targetP, 0, sizeof(server_instance_t));

    targetP->instanceId = instanceId;
    objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, targetP);

    result = prv_server_write(instanceId, numData, dataArray, objectP);

    if (result != COAP_204_CHANGED)
    {
        (void)prv_server_delete(instanceId, objectP);
    }
    else
    {
        result = COAP_201_CREATED;
    }

    return result;
}

static void prv_server_close(lwm2m_object_t * objectP)
{
    while (objectP->instanceList != NULL)
    {
        server_instance_t * targetP;

        targetP = (server_instance_t *)objectP->instanceList;
        objectP->instanceList = objectP->instanceList->next;

        lwm2m_free(targetP);
    }
}

lwm2m_object_t * get_server_object(int serverId, const char* binding, 
                                   int lifetime, bool storing)
{
    lwm2m_object_t * serverObj;

    serverObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != serverObj)
    {
        server_instance_t * targetP;

        memset(serverObj, 0, sizeof(lwm2m_object_t));

        serverObj->objID = 1;

        // Manually create an hardcoded server
        targetP = (server_instance_t *)lwm2m_malloc(sizeof(server_instance_t));
        if (NULL == targetP)
        {
            lwm2m_free(serverObj);
            return NULL;
        }

        memset(targetP, 0, sizeof(server_instance_t));
        targetP->instanceId = 0;
        targetP->shortServerId = serverId;
        targetP->lifetime = lifetime;
        targetP->storing = storing;
        memcpy (targetP->binding, binding, strlen(binding)+1);
        serverObj->instanceList = LWM2M_LIST_ADD(serverObj->instanceList, targetP);

        serverObj->readFunc = prv_server_read;
        serverObj->writeFunc = prv_server_write;
        serverObj->createFunc = prv_server_create;
        serverObj->deleteFunc = prv_server_delete;
        serverObj->executeFunc = prv_server_execute;
        serverObj->closeFunc = prv_server_close;
    }

    return serverObj;
}
