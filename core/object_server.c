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
 *  Notification Storing |  6 |    R/W     |  Single   |    Yes    | String  |         |       |
 *  Binding              |  7 |    R/W     |  Single   |    Yes    |         |         |       |
 *  Registration Update  |  8 |     E      |  Single   |    Yes    |         |         |       |
 *
 */

#ifdef LWM2M_CLIENT_MODE

#include "internals.h"

#define RESOURCE_SHORTID_ID     0
#define RESOURCE_LIFETIME_ID    1
#define RESOURCE_MINPERIOD_ID   2
#define RESOURCE_MAXPERIOD_ID   3
#define RESOURCE_DISABLE_ID     4
#define RESOURCE_TIMEOUT_ID     5
#define RESOURCE_STORING_ID     6
#define RESOURCE_BINDING_ID     7
#define RESOURCE_UPDATE_ID      8

#define LIFETIME_DEFAULT        86400

coap_status_t object_server_read(lwm2m_context_t * contextP,
                                 lwm2m_uri_t * uriP,
                                 char ** bufferP,
                                 int * lengthP)
{
    if (!LWM2M_URI_IS_SET_INSTANCE(uriP))
    {
        return COAP_501_NOT_IMPLEMENTED;
    }
    else
    {
        lwm2m_server_t * serverP;

        serverP = (lwm2m_server_t *)lwm2m_list_find((lwm2m_list_t *)contextP->serverList, uriP->instanceId);
        if (serverP == NULL) return COAP_404_NOT_FOUND;

        if (!LWM2M_URI_IS_SET_RESOURCE(uriP))
        {
            return COAP_501_NOT_IMPLEMENTED;
        }
        else
        {
            switch (uriP->resourceId)
            {
            case RESOURCE_SHORTID_ID:
                *lengthP = lwm2m_int32ToPlainText(serverP->shortID, bufferP);
                if (0 != *lengthP)
                {
                    return COAP_205_CONTENT;
                }
                else
                {
                    return COAP_500_INTERNAL_SERVER_ERROR;
                }
                break;

            case RESOURCE_LIFETIME_ID:
                {
                    int lifetime = serverP->lifetime;
                    if (lifetime == 0) lifetime = LIFETIME_DEFAULT;

                    *lengthP = lwm2m_int32ToPlainText(lifetime,bufferP);

                    if (0 != *lengthP)
                    {
                        return COAP_205_CONTENT;
                    }
                    else
                    {
                        return COAP_500_INTERNAL_SERVER_ERROR;
                    }
                }
                break;

            case RESOURCE_MINPERIOD_ID:
                return COAP_404_NOT_FOUND;
            case RESOURCE_MAXPERIOD_ID:
                return COAP_404_NOT_FOUND;
            case RESOURCE_TIMEOUT_ID:
                return COAP_404_NOT_FOUND;
            case RESOURCE_STORING_ID:
                return COAP_501_NOT_IMPLEMENTED;
            case RESOURCE_BINDING_ID:
                {
                    char * value;
                    int len;
                    switch (serverP->binding) {
                    case BINDING_U:
                        value = "U";
                        *lengthP = 1;
                        break;
                    case BINDING_UQ:
                        value = "UQ";
                        *lengthP = 2;
                        break;
                    case BINDING_S:
                        value="S";
                        *lengthP = 1;
                        break;
                    case BINDING_SQ:
                        value = "SQ";
                        *lengthP = 2;
                        break;
                    case BINDING_US:
                        value = "US";
                        *lengthP = 2;
                        break;
                    case BINDING_UQS:
                        value = "USQ";
                        *lengthP = 3;
                        break;
                    default:
                        return COAP_500_INTERNAL_SERVER_ERROR;
                    }
                    *bufferP = lwm2m_malloc(*lengthP);
                    if (NULL == *bufferP) return COAP_500_INTERNAL_SERVER_ERROR;
                    memcpy(*bufferP, value, *lengthP);
                    return COAP_205_CONTENT;
                }
            default:
                return COAP_405_METHOD_NOT_ALLOWED;
            }
        }
    }
}

coap_status_t object_server_write(lwm2m_context_t * contextP,
                                  lwm2m_uri_t * uriP,
                                  char * buffer,
                                  int length)
{
    if (!LWM2M_URI_IS_SET_INSTANCE(uriP))
    {
        return COAP_501_NOT_IMPLEMENTED;
    }
    else
    {
        lwm2m_server_t * serverP;

        serverP = (lwm2m_server_t *)lwm2m_list_find((lwm2m_list_t *)contextP->serverList, uriP->instanceId);
        if (serverP == NULL) return COAP_404_NOT_FOUND;

        if (!LWM2M_URI_IS_SET_RESOURCE(uriP))
        {
            return COAP_501_NOT_IMPLEMENTED;
        }
        else
        {
            switch (uriP->resourceId)
            {
            case RESOURCE_LIFETIME_ID:
                return COAP_501_NOT_IMPLEMENTED;
            case RESOURCE_MINPERIOD_ID:
                return COAP_404_NOT_FOUND;
            case RESOURCE_MAXPERIOD_ID:
                return COAP_404_NOT_FOUND;
            case RESOURCE_TIMEOUT_ID:
                return COAP_404_NOT_FOUND;
            case RESOURCE_STORING_ID:
                return COAP_501_NOT_IMPLEMENTED;
            case RESOURCE_BINDING_ID:
                return COAP_501_NOT_IMPLEMENTED;
            default:
                return COAP_405_METHOD_NOT_ALLOWED;
            }
        }
    }
}

coap_status_t object_server_execute(lwm2m_context_t * contextP,
                                    lwm2m_uri_t * uriP,
                                    char * buffer,
                                    int length)
{
    lwm2m_server_t * serverP;

    serverP = (lwm2m_server_t *)lwm2m_list_find((lwm2m_list_t *)contextP->serverList, uriP->instanceId);
    if (serverP == NULL) return COAP_404_NOT_FOUND;

    switch (uriP->resourceId)
    {
    case RESOURCE_DISABLE_ID:
        return COAP_404_NOT_FOUND;
    case RESOURCE_UPDATE_ID:
        return COAP_501_NOT_IMPLEMENTED;
    default:
        return COAP_405_METHOD_NOT_ALLOWED;
    }
}

coap_status_t object_server_create(lwm2m_context_t * contextP,
                                   lwm2m_uri_t * uriP,
                                   char * buffer,
                                   int length)
{
    lwm2m_server_t * serverP;

    serverP = (lwm2m_server_t *)lwm2m_list_find((lwm2m_list_t *)contextP->serverList, uriP->instanceId);
    if (serverP == NULL) return COAP_404_NOT_FOUND;

    return COAP_501_NOT_IMPLEMENTED;
}

coap_status_t object_server_delete(lwm2m_context_t * contextP,
                                   lwm2m_uri_t * uriP)
{
    lwm2m_server_t * serverP;

    serverP = (lwm2m_server_t *)lwm2m_list_find((lwm2m_list_t *)contextP->serverList, uriP->instanceId);
    if (serverP == NULL) return COAP_404_NOT_FOUND;

    return COAP_501_NOT_IMPLEMENTED;
}

coap_status_t object_security_create(lwm2m_context_t * contextP,
                                     lwm2m_uri_t * uriP,
                                     char * buffer,
                                     int length)
{
    lwm2m_server_t * serverP;

    serverP = (lwm2m_server_t *)lwm2m_list_find((lwm2m_list_t *)contextP->serverList, uriP->instanceId);
    if (serverP == NULL) return COAP_404_NOT_FOUND;

    return COAP_501_NOT_IMPLEMENTED;
}

coap_status_t object_security_delete(lwm2m_context_t * contextP,
                                     lwm2m_uri_t * uriP)
{
    lwm2m_server_t * serverP;

    serverP = (lwm2m_server_t *)lwm2m_list_find((lwm2m_list_t *)contextP->serverList, uriP->instanceId);
    if (serverP == NULL) return COAP_404_NOT_FOUND;

    return COAP_501_NOT_IMPLEMENTED;
}

#endif
