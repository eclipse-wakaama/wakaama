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

#ifdef LWM2M_CLIENT_MODE

#include "internals.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


static lwm2m_object_t * prv_find_object(lwm2m_context_t * contextP,
                                        uint16_t Id)
{
    int i;

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
                          char ** bufferP,
                          int * lengthP)
{
    coap_status_t result;

    switch (uriP->objectId)
    {
    case LWM2M_SECURITY_OBJECT_ID:
        result = NOT_FOUND_4_04;
        break;

    case LWM2M_SERVER_OBJECT_ID:
        result = object_server_read(contextP, uriP, bufferP, lengthP);
        break;

    default:
        {
            lwm2m_object_t * targetP;
            lwm2m_tlv_t * tlvP = NULL;
            int size = 0;

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

                tlvP->type = LWM2M_TYPE_RESSOURCE;
                tlvP->flags = LWM2M_TLV_FLAG_TEXT_FORMAT;
                tlvP->id = uriP->resourceId;
            }
            result = targetP->readFunc(uriP->instanceId, &size, &tlvP, targetP);
            if (result == COAP_205_CONTENT)
            {
                if (size == 1
                 && tlvP->type == LWM2M_TYPE_RESSOURCE
                 && (tlvP->flags && LWM2M_TLV_FLAG_TEXT_FORMAT) != 0 )
                {
                    *bufferP = (char *)malloc(tlvP->length);
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
        }
    }

    return result;
}

coap_status_t object_write(lwm2m_context_t * contextP,
                           lwm2m_uri_t * uriP,
                           char * buffer,
                           int length)
{
    coap_status_t result;

    switch (uriP->objectId)
    {
    case LWM2M_SECURITY_OBJECT_ID:
        result = NOT_FOUND_4_04;
        break;

    case LWM2M_SERVER_OBJECT_ID:
        result = object_server_write(contextP, uriP, buffer, length);
        break;

    default:
        {
            lwm2m_object_t * targetP;
            lwm2m_tlv_t * tlvP = NULL;
            int size = 0;

            targetP = prv_find_object(contextP, uriP->objectId);
            if (NULL == targetP) return NOT_FOUND_4_04;
            if (NULL == targetP->writeFunc) return METHOD_NOT_ALLOWED_4_05;

            if (LWM2M_URI_IS_SET_RESOURCE(uriP))
            {
                size = 1;
                tlvP = lwm2m_tlv_new(size);
                if (tlvP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

                tlvP->flags = LWM2M_TLV_FLAG_TEXT_FORMAT | LWM2M_TLV_FLAG_STATIC_DATA;
                tlvP->type = LWM2M_TYPE_RESSOURCE;
                tlvP->id = uriP->resourceId;
                tlvP->length = length;
                tlvP->value = buffer;
            }
            else
            {
                size = lwm2m_tlv_parse(buffer, length, &tlvP);
                if (size == 0) return COAP_500_INTERNAL_SERVER_ERROR;
            }
            result = targetP->writeFunc(uriP->instanceId, size, tlvP, targetP);
            lwm2m_tlv_free(size, tlvP);
        }
    }

    return result;
}

coap_status_t object_execute(lwm2m_context_t * contextP,
                             lwm2m_uri_t * uriP,
                             char * buffer,
                             int length)
{
    switch (uriP->objectId)
    {
    case LWM2M_SECURITY_OBJECT_ID:
        return NOT_FOUND_4_04;

    case LWM2M_SERVER_OBJECT_ID:
        return object_server_execute(contextP, uriP, buffer, length);

    default:
        {
            lwm2m_object_t * targetP;

            targetP = prv_find_object(contextP, uriP->objectId);
            if (NULL == targetP) return NOT_FOUND_4_04;
            if (NULL == targetP->executeFunc) return METHOD_NOT_ALLOWED_4_05;

            return targetP->executeFunc(uriP->instanceId, uriP->resourceId, buffer, length, targetP);
        }
    }
}

coap_status_t object_create(lwm2m_context_t * contextP,
                            lwm2m_uri_t * uriP,
                            char * buffer,
                            int length)
{
    if (length == 0 || buffer == 0)
    {
        return BAD_REQUEST_4_00;
    }

    switch (uriP->objectId)
    {
    case LWM2M_SECURITY_OBJECT_ID:
        return object_security_create(contextP, uriP, buffer, length);

    case LWM2M_SERVER_OBJECT_ID:
        return object_server_create(contextP, uriP, buffer, length);

    default:
        {
            lwm2m_object_t * targetP;
            lwm2m_tlv_t * tlvP = NULL;
            int size = 0;
            uint8_t result;

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

            targetP = prv_find_object(contextP, uriP->objectId);
            if (NULL == targetP) return NOT_FOUND_4_04;
            if (NULL == targetP->writeFunc) return METHOD_NOT_ALLOWED_4_05;

            size = lwm2m_tlv_parse(buffer, length, &tlvP);
            if (size == 0) return COAP_500_INTERNAL_SERVER_ERROR;

            result = targetP->createFunc(uriP->instanceId, size, tlvP, targetP);
            lwm2m_tlv_free(size, tlvP);

            return result;
        }
    }
}

coap_status_t object_delete(lwm2m_context_t * contextP,
                            lwm2m_uri_t * uriP)
{
    switch (uriP->objectId)
    {
    case LWM2M_SECURITY_OBJECT_ID:
        return object_security_delete(contextP, uriP);

    case LWM2M_SERVER_OBJECT_ID:
        return object_server_delete(contextP, uriP);

    default:
        {
            lwm2m_object_t * targetP;

            targetP = prv_find_object(contextP, uriP->objectId);
            if (NULL == targetP) return NOT_FOUND_4_04;
            if (NULL == targetP->deleteFunc) return METHOD_NOT_ALLOWED_4_05;

            return targetP->deleteFunc(uriP->instanceId, targetP);
        }
    }
}

bool object_isInstanceNew(lwm2m_context_t * contextP,
                          uint16_t objectId,
                          uint16_t instanceId)
{
    switch (objectId)
    {
    case LWM2M_SECURITY_OBJECT_ID:
    case LWM2M_SERVER_OBJECT_ID:
        if (NULL != lwm2m_list_find((lwm2m_list_t *)contextP->serverList, instanceId))
        {
            return false;
        }
        break;

    default:
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
        }
        break;
    }

    return true;
}

int prv_getRegisterPayload(lwm2m_context_t * contextP,
                           char * buffer,
                           size_t length)
{
    int index;
    int i;
    int result;

    lwm2m_server_t * serverP;

    // index can not be greater than length
    index = 0;
    for (serverP = contextP->serverList;
         serverP != NULL;
         serverP = serverP->next)
    {
        result = snprintf(buffer + index, length - index, "</%hu/%hu>,", LWM2M_SERVER_OBJECT_ID, serverP->shortID);
        if (result > 0 && result <= length - index)
        {
            index += result;
        }
        else
        {
            return 0;
        }
    }

    for (i = 0 ; i < contextP->numObject ; i++)
    {
        if (contextP->objectList[i]->instanceList == NULL)
        {
            result = snprintf(buffer + index, length - index, "</%hu>,", contextP->objectList[i]->objID);
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
                int result;

                result = snprintf(buffer + index, length - index, "</%hu/%hu>,", contextP->objectList[i]->objID, targetP->id);
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
#endif
