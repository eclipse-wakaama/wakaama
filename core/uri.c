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
 *    
 *******************************************************************************/

#include "internals.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


static int prv_parse_number(const char * uriString,
                            size_t uriLength,
                            int * headP)
{
    int result = 0;

    if (uriString[*headP] == '/')
    {
        // empty Object Instance ID with resource ID is not allowed
        return -1;
    }
    while (*headP < uriLength && uriString[*headP] != '/')
    {
        if ('0' <= uriString[*headP] && uriString[*headP] <= '9')
        {
            result += uriString[*headP] - '0';
            result *= 10;
        }
        else
        {
            return -1;
        }
        *headP += 1;
    }

    result /= 10;
    return result;
}


int prv_get_number(const char * uriString,
                   size_t uriLength)
{
    int index = 0;

    return prv_parse_number(uriString, uriLength, &index);
}


lwm2m_uri_t * lwm2m_decode_uri(multi_option_t *uriPath)
{
    lwm2m_uri_t * uriP;
    int readNum;

    if (NULL == uriPath) return NULL;

    uriP = (lwm2m_uri_t *)lwm2m_malloc(sizeof(lwm2m_uri_t));
    if (NULL == uriP) return NULL;

    memset(uriP, 0, sizeof(lwm2m_uri_t));

    // Read object ID
    if (URI_REGISTRATION_SEGMENT_LEN == uriPath->len
     && 0 == strncmp(URI_REGISTRATION_SEGMENT, uriPath->data, uriPath->len))
    {
        uriP->flag |= LWM2M_URI_FLAG_REGISTRATION;
        uriPath = uriPath->next;
        if (uriPath == NULL) return uriP;
    }
    else if (URI_BOOTSTRAP_SEGMENT_LEN == uriPath->len
     && 0 == strncmp(URI_BOOTSTRAP_SEGMENT, uriPath->data, uriPath->len))
    {
        uriP->flag |= LWM2M_URI_FLAG_BOOTSTRAP;
        uriPath = uriPath->next;
        if (uriPath != NULL) goto error;
        return uriP;
    }

    readNum = prv_get_number(uriPath->data, uriPath->len);
    if (readNum < 0 || readNum > LWM2M_MAX_ID) goto error;
    uriP->objectId = (uint16_t)readNum;
    uriP->flag |= LWM2M_URI_FLAG_OBJECT_ID;
    uriPath = uriPath->next;

    if ((uriP->flag & LWM2M_URI_MASK_TYPE) == LWM2M_URI_FLAG_REGISTRATION)
    {
        if (uriPath != NULL) goto error;
        return uriP;
    }
    uriP->flag |= LWM2M_URI_FLAG_DM;

    if (uriPath == NULL) return uriP;

    // Read object instance
    if (uriPath->len != 0)
    {
        readNum = prv_get_number(uriPath->data, uriPath->len);
        if (readNum < 0 || readNum >= LWM2M_MAX_ID) goto error;
        uriP->instanceId = (uint16_t)readNum;
        uriP->flag |= LWM2M_URI_FLAG_INSTANCE_ID;
    }
    uriPath = uriPath->next;

    if (uriPath == NULL) return uriP;

    // Read resource ID
    if (uriPath->len != 0)
    {
        // resource ID without an instance ID is not allowed
        if ((uriP->flag & LWM2M_URI_FLAG_INSTANCE_ID) == 0) goto error;

        readNum = prv_get_number(uriPath->data, uriPath->len);
        if (readNum < 0 || readNum > LWM2M_MAX_ID) goto error;
        uriP->resourceId = (uint16_t)readNum;
        uriP->flag |= LWM2M_URI_FLAG_RESOURCE_ID;
    }

    // must be the last segment
    if (NULL == uriPath->next) return uriP;

error:
    lwm2m_free(uriP);
    return NULL;
}

int lwm2m_stringToUri(char * buffer,
                      size_t buffer_len,
                      lwm2m_uri_t * uriP)
{
    int head;
    int readNum;

    if (buffer == NULL || buffer_len == 0 || uriP == NULL) return 0;

    memset(uriP, 0, sizeof(lwm2m_uri_t));

    // Skip any white space
    head = 0;
    while (head < buffer_len && isspace(buffer[head]))
    {
        head++;
    }
    if (head == buffer_len) return 0;

    // Check the URI start with a '/'
    if (buffer[head] != '/') return 0;
    head++;
    if (head == buffer_len) return 0;

    // Read object ID
    readNum = prv_parse_number(buffer, buffer_len, &head);
    if (readNum < 0 || readNum > LWM2M_MAX_ID) return 0;
    uriP->objectId = (uint16_t)readNum;

    if (buffer[head] == '/') head += 1;
    if (head >= buffer_len) return head;

    readNum = prv_parse_number(buffer, buffer_len, &head);
    if (readNum < 0 || readNum >= LWM2M_MAX_ID) return 0;
    uriP->instanceId = (uint16_t)readNum;
    uriP->flag |= LWM2M_URI_FLAG_INSTANCE_ID;

    if (buffer[head] == '/') head += 1;
    if (head >= buffer_len) return head;

    readNum = prv_parse_number(buffer, buffer_len, &head);
    if (readNum < 0 || readNum >= LWM2M_MAX_ID) return 0;
    uriP->resourceId = (uint16_t)readNum;
    uriP->flag |= LWM2M_URI_FLAG_RESOURCE_ID;

    if (head != buffer_len) return 0;

    return head;
}

