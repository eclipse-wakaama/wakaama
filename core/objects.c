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
 *    Toby Jaffey - Please refer to git log
 *    Benjamin Cabé - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    
 *******************************************************************************/

/*
 Copyright (c) 2013, 2014 Intel Corporation

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
     * Neither the name of Intel Corporation nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.

 David Navarro <david.navarro@intel.com>

*/
#include "internals.h"

#ifdef LWM2M_CLIENT_MODE


#include <stdlib.h>
#include <string.h>
#include <stdio.h>


static lwm2m_object_t * prv_find_object(lwm2m_context_t * contextP,
                                        uint16_t Id)
{
    int i;

    if (
#ifdef LWM2M_BOOTSTRAP
        (contextP->bsState != BOOTSTRAP_PENDING) &&
#endif
        (Id == LWM2M_SECURITY_OBJECT_ID))
    {
        return NULL;
    }

    for (i = 0 ; i < contextP->numObject ; i++)
    {
        if (contextP->objectList[i]->objID == Id)
        {
            return contextP->objectList[i];
        }
    }

    return NULL;
}

coap_status_t object_read(lwm2m_context_t * contextP,
                          lwm2m_uri_t * uriP,
                          uint8_t ** bufferP,
                          size_t * lengthP)
{
    coap_status_t result;
    lwm2m_object_t * targetP;
    lwm2m_tlv_t * tlvP = NULL;
    int size = 0;

#ifdef LWM2M_BOOTSTRAP
    if (contextP->bsState == BOOTSTRAP_PENDING) return METHOD_NOT_ALLOWED_4_05;
#endif

    targetP = prv_find_object(contextP, uriP->objectId);
    if (NULL == targetP) return NOT_FOUND_4_04;
    if (NULL == targetP->readFunc) return METHOD_NOT_ALLOWED_4_05;
    if (targetP->instanceList == NULL)
    {
        // this is a single instance object
        if (LWM2M_URI_IS_SET_INSTANCE(uriP) && (uriP->instanceId != 0))
        {
            return COAP_404_NOT_FOUND;
        }
    }
    else
    {
        if (LWM2M_URI_IS_SET_INSTANCE(uriP))
        {
            if (NULL == lwm2m_list_find(targetP->instanceList, uriP->instanceId))
            {
                return COAP_404_NOT_FOUND;
            }
        }
        else
        {
            // multiple object instances read
            lwm2m_list_t * instanceP;
            int i;

            size = 0;
            for (instanceP = targetP->instanceList; instanceP != NULL ; instanceP = instanceP->next)
            {
                size++;
            }

            tlvP = lwm2m_tlv_new(size);
            if (tlvP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

            result = COAP_205_CONTENT;
            instanceP = targetP->instanceList;
            i = 0;
            while (instanceP != NULL && result == COAP_205_CONTENT)
            {
                result = targetP->readFunc(instanceP->id, (int*)&(tlvP[i].length), (lwm2m_tlv_t **)&(tlvP[i].value), targetP);
                tlvP[i].type = LWM2M_TYPE_OBJECT_INSTANCE;
                tlvP[i].id = instanceP->id;
                i++;
                instanceP = instanceP->next;
            }

            if (result == COAP_205_CONTENT)
            {
                *lengthP = lwm2m_tlv_serialize(size, tlvP, bufferP);
                if (*lengthP == 0) result = COAP_500_INTERNAL_SERVER_ERROR;
            }
            lwm2m_tlv_free(size, tlvP);

            return result;
        }
    }

    // single instance read
    if (LWM2M_URI_IS_SET_RESOURCE(uriP))
    {
        size = 1;
        tlvP = lwm2m_tlv_new(size);
        if (tlvP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

        tlvP->type = LWM2M_TYPE_RESOURCE;
        tlvP->flags = LWM2M_TLV_FLAG_TEXT_FORMAT;
        tlvP->id = uriP->resourceId;
    }
    result = targetP->readFunc(uriP->instanceId, &size, &tlvP, targetP);
    if (result == COAP_205_CONTENT)
    {
        if (size == 1
         && tlvP->type == LWM2M_TYPE_RESOURCE
         && (tlvP->flags && LWM2M_TLV_FLAG_TEXT_FORMAT) != 0 )
        {
            *bufferP = (uint8_t *)malloc(tlvP->length);
            if (*bufferP == NULL)
            {
                result = COAP_500_INTERNAL_SERVER_ERROR;
            }
            else
            {
                memcpy(*bufferP, tlvP->value, tlvP->length);
                *lengthP = tlvP->length;
            }
        }
        else
        {
            *lengthP = lwm2m_tlv_serialize(size, tlvP, bufferP);
            if (*lengthP == 0) result = COAP_500_INTERNAL_SERVER_ERROR;
        }
    }
    lwm2m_tlv_free(size, tlvP);

    return result;
}

coap_status_t object_write(lwm2m_context_t * contextP,
                           lwm2m_uri_t * uriP,
                           uint8_t * buffer,
                           size_t length)
{
    coap_status_t result = NO_ERROR;
    lwm2m_object_t * targetP;
    lwm2m_tlv_t * tlvP = NULL;
    int size = 0;

    targetP = prv_find_object(contextP, uriP->objectId);
    if (NULL == targetP)
    {
        result = NOT_FOUND_4_04;
    }
    else if (NULL == targetP->writeFunc)
    {
        result = METHOD_NOT_ALLOWED_4_05;
    }
    else
    {
        if (LWM2M_URI_IS_SET_RESOURCE(uriP))
        {
            size = 1;
            tlvP = lwm2m_tlv_new(size);
            if (tlvP == NULL)
            {
                return COAP_500_INTERNAL_SERVER_ERROR;
            }

            tlvP->flags = LWM2M_TLV_FLAG_TEXT_FORMAT | LWM2M_TLV_FLAG_STATIC_DATA;
            tlvP->type = LWM2M_TYPE_RESOURCE;
            tlvP->id = uriP->resourceId;
            tlvP->length = length;
            tlvP->value = (uint8_t *)buffer;
        }
        else
        {
            size = lwm2m_tlv_parse(buffer, length, &tlvP);
            if (size == 0)
            {
                result = COAP_500_INTERNAL_SERVER_ERROR;
            }
        }
    }
    if (result == NO_ERROR)
    {
#ifdef LWM2M_BOOTSTRAP
        if (contextP->bsState == BOOTSTRAP_PENDING)
        {
            tlvP->flags |= LWM2M_TLV_FLAG_BOOTSTRAPPING;
        }
#endif
        result = targetP->writeFunc(uriP->instanceId, size, tlvP, targetP);
        lwm2m_tlv_free(size, tlvP);
    }
#ifdef LWM2M_BOOTSTRAP
    if (contextP->bsState == BOOTSTRAP_PENDING)
    {
        if (result == COAP_204_CHANGED)
        {
            reset_bootstrap_timer(contextP);
        }
        else
        {
            bootstrap_failed(contextP);
        }
    }
#endif
    return result;
}

coap_status_t object_execute(lwm2m_context_t * contextP,
                             lwm2m_uri_t * uriP,
                             uint8_t * buffer,
                             size_t length)
{
    lwm2m_object_t * targetP;

#ifdef LWM2M_BOOTSTRAP
    if (contextP->bsState == BOOTSTRAP_PENDING) return METHOD_NOT_ALLOWED_4_05;
#endif

    targetP = prv_find_object(contextP, uriP->objectId);
    if (NULL == targetP) return NOT_FOUND_4_04;
    if (NULL == targetP->executeFunc) return METHOD_NOT_ALLOWED_4_05;

    return targetP->executeFunc(uriP->instanceId, uriP->resourceId, buffer, length, targetP);
}

coap_status_t object_create(lwm2m_context_t * contextP,
                            lwm2m_uri_t * uriP,
                            uint8_t * buffer,
                            size_t length)
{
    lwm2m_object_t * targetP;
    lwm2m_tlv_t * tlvP = NULL;
    int size = 0;
    uint8_t result;

    if (length == 0 || buffer == 0)
    {
        return BAD_REQUEST_4_00;
    }

    targetP = prv_find_object(contextP, uriP->objectId);
    if (NULL == targetP) return NOT_FOUND_4_04;
    if (NULL == targetP->createFunc) return METHOD_NOT_ALLOWED_4_05;

    if (LWM2M_URI_IS_SET_INSTANCE(uriP))
    {
        if (NULL != lwm2m_list_find(targetP->instanceList, uriP->instanceId))
        {
            // Instance already exists
            return COAP_406_NOT_ACCEPTABLE;
        }
    }
    else
    {
        uriP->instanceId = lwm2m_list_newId(targetP->instanceList);
        uriP->flag |= LWM2M_URI_FLAG_INSTANCE_ID;
    }

    size = lwm2m_tlv_parse(buffer, length, &tlvP);
    if (size == 0) return COAP_500_INTERNAL_SERVER_ERROR;
#ifdef LWM2M_BOOTSTRAP
    if (contextP->bsState == BOOTSTRAP_PENDING)
    {
        tlvP->flags |= LWM2M_TLV_FLAG_BOOTSTRAPPING;
    }
#endif
    result = targetP->createFunc(uriP->instanceId, size, tlvP, targetP);
    lwm2m_tlv_free(size, tlvP);

    return result;
}

coap_status_t object_delete(lwm2m_context_t * contextP,
                            lwm2m_uri_t * uriP)
{
    lwm2m_object_t * targetP;

    targetP = prv_find_object(contextP, uriP->objectId);
    if (NULL == targetP) return NOT_FOUND_4_04;
    if (NULL == targetP->deleteFunc) return METHOD_NOT_ALLOWED_4_05;

    LOG("    Call to object_delete\r\n");

    return targetP->deleteFunc(uriP->instanceId, targetP);
}

bool object_isInstanceNew(lwm2m_context_t * contextP,
                          uint16_t objectId,
                          uint16_t instanceId)
{
    lwm2m_object_t * targetP;

    targetP = prv_find_object(contextP, objectId);
    if (targetP != NULL)
    {
        if (NULL != lwm2m_list_find(targetP->instanceList, instanceId))
        {
            return false;
        }
    }

    return true;
}

int prv_getRegisterPayload(lwm2m_context_t * contextP,
                           uint8_t * buffer,
                           size_t length)
{
    int index;
    int i;
    int result;

    // index can not be greater than length
    index = 0;

    if ((contextP->altPath != NULL)
     && (contextP->altPath[0] != 0))
    {
        result = snprintf((char *)buffer, length, REG_ALT_PATH_LINK, contextP->altPath);
        if (result > 0 && result <= length)
        {
            index = result;
        }
        else
        {
            return 0;
        }
    }
    for (i = 0 ; i < contextP->numObject ; i++)
    {
        if (contextP->objectList[i]->objID == LWM2M_SECURITY_OBJECT_ID) continue;

        if (contextP->objectList[i]->instanceList == NULL)
        {
            result = snprintf((char *)buffer + index, length - index,
                              REG_OBJECT_PATH,
                              contextP->altPath?contextP->altPath:"", contextP->objectList[i]->objID);
            if (result > 0 && result <= length - index)
            {
                index += result;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            lwm2m_list_t * targetP;
            for (targetP = contextP->objectList[i]->instanceList ; targetP != NULL ; targetP = targetP->next)
            {
                result = snprintf((char *)buffer + index, length - index,
                                  REG_OBJECT_INSTANCE_PATH,
                                  contextP->altPath?contextP->altPath:"", contextP->objectList[i]->objID, targetP->id);
                if (result > 0 && result <= length - index)
                {
                    index += result;
                }
                else
                {
                    return 0;
                }
            }
        }
    }

    if (index > 0)
    {
        index = index - 1;  // remove trailing ','
    }

    buffer[index] = 0;

    return index;
}

static lwm2m_list_t * prv_findServerInstance(lwm2m_object_t * objectP,
                                             uint16_t shortID)
{
    lwm2m_list_t * instanceP;

    instanceP = objectP->instanceList;
    while (NULL != instanceP)
    {
        int64_t value;
        lwm2m_tlv_t * tlvP;
        int size;

        size = 1;
        tlvP = lwm2m_tlv_new(size);
        if (tlvP == NULL) return NULL;
        tlvP->id = LWM2M_SERVER_SHORT_ID_ID;

        if (objectP->readFunc(instanceP->id, &size, &tlvP, objectP) != COAP_205_CONTENT)
        {
            lwm2m_tlv_free(size, tlvP);
            return NULL;
        }

        if (1 == lwm2m_tlv_decode_int(tlvP, &value))
        {
            if (value == shortID)
            {
                lwm2m_tlv_free(size, tlvP);
                break;
            }
        }
        lwm2m_tlv_free(size, tlvP);
        instanceP = instanceP->next;
    }

    return instanceP;
}

static int prv_getMandatoryInfo(lwm2m_object_t * objectP,
                                uint16_t instanceID,
                                lwm2m_server_t * targetP)
{
    lwm2m_tlv_t * tlvP;
    int size;
    int64_t value;

    size = 2;
    tlvP = lwm2m_tlv_new(size);
    if (tlvP == NULL) return -1;
    tlvP[0].id = LWM2M_SERVER_LIFETIME_ID;
    tlvP[1].id = LWM2M_SERVER_BINDING_ID;

    if (objectP->readFunc(instanceID, &size, &tlvP, objectP) != COAP_205_CONTENT)
    {
        lwm2m_tlv_free(size, tlvP);
        return -1;
    }

    if (0 == lwm2m_tlv_decode_int(tlvP, &value)
     || value < 0 || value >0xFFFFFFFF)             // This is an implementation limit
    {
        lwm2m_tlv_free(size, tlvP);
        return -1;
    }
    targetP->lifetime = value;

    targetP->binding = lwm2m_stringToBinding(tlvP[1].value, tlvP[1].length);

    lwm2m_tlv_free(size, tlvP);

    if (targetP->binding == BINDING_UNKNOWN)
    {
        return -1;
    }

    return 0;
}

int object_getServers(lwm2m_context_t * contextP)
{
    lwm2m_object_t * securityObjP = NULL;
    lwm2m_object_t * serverObjP = NULL;
    lwm2m_list_t * securityInstP;   // instanceID of the server in the LWM2M Security Object
    int i;

    for (i = 0 ; i < contextP->numObject ; i++)
    {
        if (contextP->objectList[i]->objID == LWM2M_SECURITY_OBJECT_ID)
        {
            securityObjP = contextP->objectList[i];
        }
        else if (contextP->objectList[i]->objID == LWM2M_SERVER_OBJECT_ID)
        {
            serverObjP = contextP->objectList[i];
        }
    }

    if (NULL == securityObjP) return -1;

    securityInstP = securityObjP->instanceList;
    while (securityInstP != NULL)
    {
        lwm2m_tlv_t * tlvP;
        int size;
        lwm2m_server_t * targetP;
        bool isBootstrap;
        int64_t value = 0;

        size = 3;
        tlvP = lwm2m_tlv_new(size);
        if (tlvP == NULL) return -1;
        tlvP[0].id = LWM2M_SECURITY_BOOTSTRAP_ID;
        tlvP[1].id = LWM2M_SECURITY_SHORT_SERVER_ID;
        tlvP[2].id = LWM2M_SECURITY_HOLD_OFF_ID;

        if (securityObjP->readFunc(securityInstP->id, &size, &tlvP, securityObjP) != COAP_205_CONTENT)
        {
            lwm2m_tlv_free(size, tlvP);
            return -1;
        }

        targetP = (lwm2m_server_t *)lwm2m_malloc(sizeof(lwm2m_server_t));
        if (targetP == NULL) {
            lwm2m_tlv_free(size, tlvP);
            return -1;
        }
        memset(targetP, 0, sizeof(lwm2m_server_t));
        targetP->secObjInstID = securityInstP->id;

        if (0 == lwm2m_tlv_decode_bool(tlvP + 0, &isBootstrap))
        {
            lwm2m_free(targetP);
            lwm2m_tlv_free(size, tlvP);
            return -1;
        }

        if (0 == lwm2m_tlv_decode_int(tlvP + 1, &value)
         || value < (isBootstrap ? 0 : 1) || value > 0xFFFF)                // 0 is forbidden as a Short Server ID
        {
            lwm2m_free(targetP);
            lwm2m_tlv_free(size, tlvP);
            return -1;
        }
        targetP->shortID = value;

        if (isBootstrap == true)
        {
            if (0 == lwm2m_tlv_decode_int(tlvP + 2, &value)
             || value < 0 || value > 0xFFFFFFFF)             // This is an implementation limit
            {
                lwm2m_free(targetP);
                lwm2m_tlv_free(size, tlvP);
                return -1;
            }
            // lifetime of a bootstrap server is set to ClientHoldOffTime
            targetP->lifetime = value;

            contextP->bootstrapServerList = (lwm2m_server_t*)LWM2M_LIST_ADD(contextP->bootstrapServerList, targetP);
        }
        else
        {
            lwm2m_list_t * serverInstP;     // instanceID of the server in the LWM2M Server Object

            serverInstP = prv_findServerInstance(serverObjP, targetP->shortID);
            if (serverInstP == NULL)
            {
                lwm2m_free(targetP);
                lwm2m_tlv_free(size, tlvP);
                return -1;
            }
            if (0 != prv_getMandatoryInfo(serverObjP, serverInstP->id, targetP))
            {
                lwm2m_free(targetP);
                lwm2m_tlv_free(size, tlvP);
                return -1;
            }
            targetP->status = STATE_DEREGISTERED;
            contextP->serverList = (lwm2m_server_t*)LWM2M_LIST_ADD(contextP->serverList, targetP);
        }
        lwm2m_tlv_free(size, tlvP);
        securityInstP = securityInstP->next;
    }

    return 0;
}

#endif
