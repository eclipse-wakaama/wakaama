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
    switch (uriP->objectId)
    {
    case LWM2M_SECURITY_OBJECT_ID:
        return NOT_FOUND_4_04;

    case LWM2M_SERVER_OBJECT_ID:
        return object_server_read(contextP, uriP, bufferP, lengthP);

    default:
        {
            lwm2m_object_t * targetP;

            targetP = prv_find_object(contextP, uriP->objectId);

            if (NULL == targetP)
            {
                return NOT_FOUND_4_04;
            }
            if (NULL == targetP->readFunc)
            {
                return METHOD_NOT_ALLOWED_4_05;
            }

            return targetP->readFunc(uriP, bufferP, lengthP, targetP);
        }
    }
}

coap_status_t object_write(lwm2m_context_t * contextP,
                           lwm2m_uri_t * uriP,
                           char * buffer,
                           int length)
{
    switch (uriP->objectId)
    {
    case LWM2M_SECURITY_OBJECT_ID:
        return NOT_FOUND_4_04;

    case LWM2M_SERVER_OBJECT_ID:
        return object_server_write(contextP, uriP, buffer, length);

    default:
        {
            lwm2m_object_t * targetP;

            targetP = prv_find_object(contextP, uriP->objectId);

            if (NULL == targetP)
            {
                return NOT_FOUND_4_04;
            }
            if (NULL == targetP->writeFunc)
            {
                return METHOD_NOT_ALLOWED_4_05;
            }

            return targetP->writeFunc(uriP, buffer, length, targetP);
        }
    }
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

            if (NULL == targetP)
            {
                return NOT_FOUND_4_04;
            }

            if (NULL == targetP->executeFunc)
            {
                return METHOD_NOT_ALLOWED_4_05;
            }

            return targetP->executeFunc(uriP, buffer, length, targetP);
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

            targetP = prv_find_object(contextP, uriP->objectId);

            if (NULL == targetP)
            {
                return NOT_FOUND_4_04;
            }

            if (NULL == targetP->createFunc)
            {
                return METHOD_NOT_ALLOWED_4_05;
            }

            return targetP->createFunc(uriP, buffer, length, targetP);
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

            if (NULL == targetP)
            {
                return NOT_FOUND_4_04;
            }
            if (NULL == targetP->deleteFunc)
            {
                return METHOD_NOT_ALLOWED_4_05;
            }

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
