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
 *    David Navarro, Intel Corporation - initial implementation
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
 *
 *******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <ctype.h> // isspace

#include "bootstrap_info.h"

typedef struct
{
    uint16_t    id;
    char *      uri;
    bool        isBootstrap;
    uint32_t    lifetime;
    uint8_t     securityMode;
    uint8_t *   publicKey;
    size_t      publicKeyLen;
    uint8_t *   secretKey;
    size_t      secretKeyLen;
    uint8_t *   serverKey;
    size_t      serverKeyLen;
#ifndef LWM2M_VERSION_1_0
    int         registrationPriorityOrder; // <0 when it doesn't exist
    int         initialRegistrationDelayTimer; // <0 when it doesn't exist
    int8_t      registrationFailureBlock; // <0 when it doesn't exist, 0 for false, > 0 for true
    int8_t      bootstrapOnRegistrationFailure; // <0 when it doesn't exist, 0 for false, > 0 for true
    int         communicationRetryCount; // <0 when it doesn't exist
    int         communicationRetryTimer; // <0 when it doesn't exist
    int         communicationSequenceDelayTimer; // <0 when it doesn't exist
    int         communicationSequenceRetryCount; // <0 when it doesn't exist
#endif
} read_server_t;

static int prv_find_next_section(FILE * fd,
                                 char * tag)
{
    char * line;
    size_t length;
    int found;

    line = NULL;
    length = 0;
    found = 0;
    while (found == 0
        && getline(&line, &length, fd) != -1)
    {
        if (line[0] == '[')
        {
            size_t i;

            length = strlen(line);
            i = 1;
            while (line[i] != ']') i++;

            if (i < length)
            {
                line[i] = 0;
                if (lwm2m_strcasecmp(line + 1, tag) == 0)
                {
                    found = 1;
                }
            }
        }
        lwm2m_free(line);
        line = NULL;
        length = 0;
    }

    return found;
}

// returns -1 for error, 0 if not found (end of section or file)
// and 1 if found
static int prv_read_key_value(FILE * fd,
                              char ** keyP,
                              char ** valueP)
{
    char * line;
    fpos_t prevPos;
    ssize_t res;
    size_t length;
    size_t start;
    size_t middle;
    size_t end;

    *keyP = NULL;
    *valueP = NULL;

    line = NULL;
    if (fgetpos(fd, &prevPos) != 0) return -1;
    while ((res = getline(&line, &length, fd)) != -1)
    {
        length = strlen(line);

        start = 0;
        while (start < length && isspace(line[start]&0xff)) start++;
        // ignore empty and commented lines
        if (start != length
         && line[start] != ';'
         && line[start] != '#')
        {
            break;
        }

        lwm2m_free(line);
        line = NULL;
        length = 0;
        if (fgetpos(fd, &prevPos) != 0) return -1;
    }

    if (res == -1) return -1;

    // end of section
    if (line[start] == '[')
    {
        lwm2m_free(line);
        fsetpos(fd, &prevPos);
        return 0;
    }

    middle = start;
    while (middle < length && line[middle] != '=') middle++;
    // invalid lines
    if (middle == start
     || middle == length)
    {
        lwm2m_free(line);
        return -1;
    }

    end = length - 2;
    while (end > middle && isspace(line[end]&0xff)) end--;
    // invalid lines
    if (end == middle)
    {
        lwm2m_free(line);
        return -1;
    }
    end += 1;

    line[middle] = 0;
    *keyP = strdup(line + start);
    if (*keyP == NULL)
    {
        lwm2m_free(line);
        return -1;
    }

    middle +=1 ;
    while (middle < end && isspace(line[middle]&0xff)) middle++;
    line[end] = 0;
    *valueP = strdup(line + middle);
    if (*valueP == NULL)
    {
        lwm2m_free(*keyP);
        *keyP = NULL;
        lwm2m_free(line);
        return -1;
    }

    lwm2m_free(line);

    return 1;
}

static int prv_readDigit(char digit)
{
    if (digit >= '0' && digit <= '9')
    {
        return (digit - '0');
    }
    if (digit >= 'a' && digit <= 'f')
    {
        return (10 + digit - 'a');
    }
    if (digit >= 'A' && digit <= 'F')
    {
        return (10 + digit - 'A');
    }

    return -1;
}

static size_t prv_readSecurityKey(char * value,
                                  uint8_t ** resultP)
{
    size_t length;
    size_t charIndex;
    size_t resIndex;

    if (strlen(value)%2 != 0) return 0;

    length = strlen(value) / 2;
    *resultP = (uint8_t *)lwm2m_malloc(length);
    if (*resultP == NULL) return 0;

    resIndex = 0;
    charIndex = 0;
    while (resIndex < length)
    {
        int tmp;

        tmp = prv_readDigit(value[charIndex]);
        if (tmp == -1) goto error;
        (*resultP)[resIndex] = (tmp & 0xFF);

        charIndex++;
        tmp = prv_readDigit(value[charIndex]);
        if (tmp == -1) goto error;
        (*resultP)[resIndex] = ((*resultP)[resIndex] << 4) + (tmp & 0xFF);

        charIndex++;
        resIndex++;
    }

    return length;

error:
    lwm2m_free(*resultP);
    *resultP = NULL;

    return 0;
}

static read_server_t * prv_read_next_server(FILE * fd)
{
    char * key;
    char * value;
    read_server_t * readSrvP;
    int res;

    if (prv_find_next_section(fd, "Server") == 0) return NULL;

    readSrvP = (read_server_t *)lwm2m_malloc(sizeof(read_server_t));
    if (readSrvP == NULL) return NULL;
    memset(readSrvP, 0, sizeof(read_server_t));
#ifndef LWM2M_VERSION_1_0
    readSrvP->registrationPriorityOrder = -1;
    readSrvP->initialRegistrationDelayTimer = -1;
    readSrvP->registrationFailureBlock = -1;
    readSrvP->bootstrapOnRegistrationFailure = -1;
    readSrvP->communicationRetryCount = -1;
    readSrvP->communicationRetryTimer = -1;
    readSrvP->communicationSequenceDelayTimer = -1;
    readSrvP->communicationSequenceRetryCount = -1;
#endif

    while((res = prv_read_key_value(fd, &key, &value)) == 1)
    {
        if (lwm2m_strcasecmp(key, "id") == 0)
        {
            int num;

            if (sscanf(value, "%d", &num) != 1) goto error;
            if (num <= 0 || num > LWM2M_MAX_ID) goto error;
            readSrvP->id = num;
            lwm2m_free(value);
        }
        else if (lwm2m_strcasecmp(key, "uri") == 0)
        {
            readSrvP->uri = value;
        }
        else if (lwm2m_strcasecmp(key, "bootstrap") == 0)
        {
            if (lwm2m_strcasecmp(value, "yes") == 0)
            {
                readSrvP->isBootstrap = true;
            }
            else if (lwm2m_strcasecmp(value, "no") == 0)
            {
                readSrvP->isBootstrap = false;
            }
            else goto error;
            lwm2m_free(value);
        }
        else if (lwm2m_strcasecmp(key, "lifetime") == 0)
        {
            int num;

            if (sscanf(value, "%d", &num) != 1) goto error;
            if (num <= 0) goto error;
            readSrvP->lifetime = num;
            lwm2m_free(value);
        }
        else if (lwm2m_strcasecmp(key, "security") == 0)
        {
            if (lwm2m_strcasecmp(value, "nosec") == 0)
            {
                readSrvP->securityMode = LWM2M_SECURITY_MODE_NONE;
            }
            else if (lwm2m_strcasecmp(value, "PSK") == 0)
            {
                readSrvP->securityMode = LWM2M_SECURITY_MODE_PRE_SHARED_KEY;
            }
            else if (lwm2m_strcasecmp(value, "RPK") == 0)
            {
                readSrvP->securityMode = LWM2M_SECURITY_MODE_RAW_PUBLIC_KEY;
            }
            else if (lwm2m_strcasecmp(value, "certificate") == 0)
            {
                readSrvP->securityMode = LWM2M_SECURITY_MODE_CERTIFICATE;
            }
            else goto error;
            lwm2m_free(value);
        }
        else if (lwm2m_strcasecmp(key, "public") == 0)
        {
            readSrvP->publicKeyLen = prv_readSecurityKey(value, &(readSrvP->publicKey));
            if (readSrvP->publicKeyLen == 0) goto error;
            lwm2m_free(value);
        }
        else if (lwm2m_strcasecmp(key, "server") == 0)
        {
            readSrvP->serverKeyLen = prv_readSecurityKey(value, &(readSrvP->serverKey));
            if (readSrvP->serverKeyLen == 0) goto error;
            lwm2m_free(value);
        }
        else if (lwm2m_strcasecmp(key, "secret") == 0)
        {
            readSrvP->secretKeyLen = prv_readSecurityKey(value, &(readSrvP->secretKey));
            if (readSrvP->secretKeyLen == 0) goto error;
            lwm2m_free(value);
        }
#ifndef LWM2M_VERSION_1_0
        else if (lwm2m_strcasecmp(key, "registrationPriorityOrder") == 0)
        {
            int num;

            if (sscanf(value, "%d", &num) != 1) goto error;
            if (num < 0) goto error;
            readSrvP->registrationPriorityOrder = num;
            lwm2m_free(value);
        }
        else if (lwm2m_strcasecmp(key, "initialRegistrationDelay") == 0)
        {
            int num;

            if (sscanf(value, "%d", &num) != 1) goto error;
            if (num < 0) goto error;
            readSrvP->initialRegistrationDelayTimer = num;
            lwm2m_free(value);
        }
        else if (lwm2m_strcasecmp(key, "registrationFailureBlock") == 0)
        {
            if (lwm2m_strcasecmp(value, "yes") == 0)
            {
                readSrvP->registrationFailureBlock = true;
            }
            else if (lwm2m_strcasecmp(value, "no") == 0)
            {
                readSrvP->registrationFailureBlock = false;
            }
            else goto error;
            lwm2m_free(value);
        }
        else if (lwm2m_strcasecmp(key, "bootstrapOnRegistrationFailure") == 0)
        {
            if (lwm2m_strcasecmp(value, "yes") == 0)
            {
                readSrvP->bootstrapOnRegistrationFailure = true;
            }
            else if (lwm2m_strcasecmp(value, "no") == 0)
            {
                readSrvP->bootstrapOnRegistrationFailure = false;
            }
            else goto error;
            lwm2m_free(value);
        }
        else if (lwm2m_strcasecmp(key, "communicationRetryCount") == 0)
        {
            int num;

            if (sscanf(value, "%d", &num) != 1) goto error;
            if (num < 0) goto error;
            readSrvP->communicationRetryCount = num;
            lwm2m_free(value);
        }
        else if (lwm2m_strcasecmp(key, "communicationRetryTimer") == 0)
        {
            int num;

            if (sscanf(value, "%d", &num) != 1) goto error;
            if (num < 0) goto error;
            readSrvP->communicationRetryTimer = num;
            lwm2m_free(value);
        }
        else if (lwm2m_strcasecmp(key, "communicationSequenceDelayTimer") == 0)
        {
            int num;

            if (sscanf(value, "%d", &num) != 1) goto error;
            if (num < 0) goto error;
            readSrvP->communicationSequenceDelayTimer = num;
            lwm2m_free(value);
        }
        else if (lwm2m_strcasecmp(key, "communicationSequenceRetryCount") == 0)
        {
            int num;

            if (sscanf(value, "%d", &num) != 1) goto error;
            if (num < 0) goto error;
            readSrvP->communicationSequenceRetryCount = num;
            lwm2m_free(value);
        }
#endif
        else
        {
            // ignore key for now
            lwm2m_free(value);
        }
        lwm2m_free(key);
    }

    if (res == -1) goto error;
    if (readSrvP->id == 0
     || readSrvP->uri == 0
     || (readSrvP->securityMode != LWM2M_SECURITY_MODE_NONE
          && (readSrvP->publicKey == NULL || readSrvP->secretKey == NULL))
     || (readSrvP->serverKey == NULL
          && (readSrvP->securityMode == LWM2M_SECURITY_MODE_RAW_PUBLIC_KEY
           || readSrvP->securityMode == LWM2M_SECURITY_MODE_CERTIFICATE)))
    {
        goto error;
    }

    return readSrvP;

error:
    if (readSrvP != NULL)
    {
        if (readSrvP->uri != NULL) lwm2m_free(readSrvP->uri);
        if (readSrvP->publicKey != NULL) lwm2m_free(readSrvP->publicKey);
        if (readSrvP->secretKey != NULL) lwm2m_free(readSrvP->secretKey);
        if (readSrvP->serverKey != NULL) lwm2m_free(readSrvP->serverKey);
        lwm2m_free(readSrvP);
    }
    if (key != NULL) lwm2m_free(key);
    if (value != NULL) lwm2m_free(value);

    return NULL;
}

static int prv_add_server(bs_info_t * infoP,
                          read_server_t * dataP)
{
    bs_server_data_t * serverP;

    serverP = (bs_server_data_t *)lwm2m_malloc(sizeof(bs_server_data_t));
    if (serverP == NULL) return -1;
    memset(serverP, 0, sizeof(bs_server_data_t));

    serverP->id = dataP->id;

    switch (dataP->securityMode)
    {
    case LWM2M_SECURITY_MODE_NONE:
        serverP->securitySize = 4;
        break;
    case LWM2M_SECURITY_MODE_PRE_SHARED_KEY:
        serverP->securitySize = 6;
        break;
    case LWM2M_SECURITY_MODE_RAW_PUBLIC_KEY:
    case LWM2M_SECURITY_MODE_CERTIFICATE:
        serverP->securitySize = 7;
        break;
    default:
        goto error;
    }

    serverP->securityData = lwm2m_data_new(serverP->securitySize);
    if (serverP->securityData == NULL) goto error;

    // LWM2M Server URI
    serverP->securityData[0].id = LWM2M_SECURITY_URI_ID;
    lwm2m_data_encode_string(dataP->uri, serverP->securityData);

    // Bootstrap Server
    serverP->securityData[1].id = LWM2M_SECURITY_BOOTSTRAP_ID;
    lwm2m_data_encode_bool(dataP->isBootstrap, serverP->securityData + 1);

    // Short Server ID
    serverP->securityData[2].id = LWM2M_SECURITY_SHORT_SERVER_ID;
    lwm2m_data_encode_int(dataP->id, serverP->securityData + 2);

    // Security Mode
    serverP->securityData[3].id = LWM2M_SECURITY_SECURITY_ID;
    lwm2m_data_encode_int(dataP->securityMode, serverP->securityData + 3);

    if (serverP->securitySize > 4)
    {
        serverP->securityData[4].id = LWM2M_SECURITY_PUBLIC_KEY_ID;
        lwm2m_data_encode_opaque(dataP->publicKey, dataP->publicKeyLen, serverP->securityData + 4);

        serverP->securityData[5].id = LWM2M_SECURITY_SECRET_KEY_ID;
        lwm2m_data_encode_opaque(dataP->secretKey, dataP->secretKeyLen, serverP->securityData + 5);

        if (serverP->securitySize == 7)
        {
            serverP->securityData[6].id = LWM2M_SECURITY_SERVER_PUBLIC_KEY_ID;
            lwm2m_data_encode_opaque(dataP->serverKey, dataP->serverKeyLen, serverP->securityData + 6);
        }
    }

    if (dataP->isBootstrap == false)
    {
#ifndef LWM2M_VERSION_1_0
        int i;
#endif
        serverP->serverSize = 4;
#ifndef LWM2M_VERSION_1_0
        if (dataP->registrationPriorityOrder >= 0) serverP->serverSize++;
        if (dataP->initialRegistrationDelayTimer >= 0) serverP->serverSize++;
        if (dataP->registrationFailureBlock >= 0) serverP->serverSize++;
        if (dataP->bootstrapOnRegistrationFailure >= 0) serverP->serverSize++;
        if (dataP->communicationRetryCount >= 0) serverP->serverSize++;
        if (dataP->communicationRetryTimer >= 0) serverP->serverSize++;
        if (dataP->communicationSequenceDelayTimer >= 0) serverP->serverSize++;
        if (dataP->communicationSequenceRetryCount >= 0) serverP->serverSize++;
#endif

        serverP->serverData = lwm2m_data_new(serverP->serverSize);
        if (serverP->serverData == NULL) goto error;

        // Short Server ID
        serverP->serverData[0].id = LWM2M_SERVER_SHORT_ID_ID;
        lwm2m_data_encode_int(dataP->id, serverP->serverData);

        // Lifetime
        serverP->serverData[1].id = LWM2M_SERVER_LIFETIME_ID;
        lwm2m_data_encode_int(dataP->lifetime, serverP->serverData + 1);

        // Notification Storing
        serverP->serverData[2].id = LWM2M_SERVER_STORING_ID;
        lwm2m_data_encode_bool(false, serverP->serverData + 2);

        // Binding
        serverP->serverData[3].id = LWM2M_SERVER_BINDING_ID;
        lwm2m_data_encode_string("U", serverP->serverData + 3);

#ifndef LWM2M_VERSION_1_0
        i = 3;
        if (dataP->registrationPriorityOrder >= 0)
        {
            serverP->serverData[++i].id = LWM2M_SERVER_REG_ORDER_ID;
            lwm2m_data_encode_uint(dataP->registrationPriorityOrder, serverP->serverData + i);
        }
        if (dataP->initialRegistrationDelayTimer >= 0)
        {
            serverP->serverData[++i].id = LWM2M_SERVER_INITIAL_REG_DELAY_ID;
            lwm2m_data_encode_uint(dataP->initialRegistrationDelayTimer, serverP->serverData + i);
        }
        if (dataP->registrationFailureBlock >= 0)
        {
            serverP->serverData[++i].id = LWM2M_SERVER_REG_FAIL_BLOCK_ID;
            lwm2m_data_encode_bool(dataP->registrationFailureBlock > 0, serverP->serverData + i);
        }
        if (dataP->bootstrapOnRegistrationFailure >= 0)
        {
            serverP->serverData[++i].id = LWM2M_SERVER_REG_FAIL_BOOTSTRAP_ID;
            lwm2m_data_encode_bool(dataP->bootstrapOnRegistrationFailure > 0, serverP->serverData + i);
        }
        if (dataP->communicationRetryCount >= 0)
        {
            serverP->serverData[++i].id = LWM2M_SERVER_COMM_RETRY_COUNT_ID;
            lwm2m_data_encode_uint(dataP->communicationRetryCount, serverP->serverData + i);
        }
        if (dataP->communicationRetryTimer >= 0)
        {
            serverP->serverData[++i].id = LWM2M_SERVER_COMM_RETRY_TIMER_ID;
            lwm2m_data_encode_uint(dataP->communicationRetryTimer, serverP->serverData + i);
        }
        if (dataP->communicationSequenceDelayTimer >= 0)
        {
            serverP->serverData[++i].id = LWM2M_SERVER_SEQ_DELAY_TIMER_ID;
            lwm2m_data_encode_uint(dataP->communicationSequenceDelayTimer, serverP->serverData + i);
        }
        if (dataP->communicationSequenceRetryCount >= 0)
        {
            serverP->serverData[++i].id = LWM2M_SERVER_SEQ_RETRY_COUNT_ID;
            lwm2m_data_encode_uint(dataP->communicationSequenceRetryCount, serverP->serverData + i);
        }
#endif
    }

    infoP->serverList = (bs_server_data_t *)LWM2M_LIST_ADD(infoP->serverList, serverP);

    return 0;

error:
    if (serverP->securityData != NULL) lwm2m_data_free(serverP->securitySize, serverP->securityData);
    if (serverP->serverData != NULL) lwm2m_data_free(serverP->serverSize, serverP->serverData);
    lwm2m_free(serverP);

    return -1;
}

static bs_endpoint_info_t * prv_read_next_endpoint(FILE * fd)
{
    char * key;
    char * value;
    bs_endpoint_info_t * endptP;
    int res;
    bs_command_t * cmdP;

    if (prv_find_next_section(fd, "Endpoint") == 0) return NULL;

    endptP = (bs_endpoint_info_t *)lwm2m_malloc(sizeof(bs_endpoint_info_t));
    if (endptP == NULL) return NULL;
    memset(endptP, 0, sizeof(bs_endpoint_info_t));

    cmdP = NULL;

    while((res = prv_read_key_value(fd, &key, &value)) == 1)
    {
        if (lwm2m_strcasecmp(key, "Name") == 0)
        {
            endptP->name = value;
        }
        else if (lwm2m_strcasecmp(key, "Discover") == 0)
        {
            lwm2m_uri_t uri;

            if (lwm2m_stringToUri(value, strlen(value), &uri) == 0) goto error;

            cmdP = (bs_command_t *)lwm2m_malloc(sizeof(bs_command_t));
            if (cmdP == NULL) goto error;
            memset(cmdP, 0, sizeof(bs_command_t));

            cmdP->operation = BS_DISCOVER;
            if (LWM2M_URI_IS_SET_OBJECT(&uri))
            {
                cmdP->uri = (lwm2m_uri_t *)lwm2m_malloc(sizeof(lwm2m_uri_t));
                if (cmdP->uri == NULL) goto error;
                memcpy(cmdP->uri, &uri, sizeof(lwm2m_uri_t));
            }

            lwm2m_free(value);
        }
        else if (lwm2m_strcasecmp(key, "Read") == 0)
        {
#ifndef LWM2M_VERSION_1_0
            lwm2m_uri_t uri;

            if (lwm2m_stringToUri(value, strlen(value), &uri) == 0) goto error;

            cmdP = (bs_command_t *)lwm2m_malloc(sizeof(bs_command_t));
            if (cmdP == NULL) goto error;
            memset(cmdP, 0, sizeof(bs_command_t));

            cmdP->operation = BS_READ;
            if (LWM2M_URI_IS_SET_OBJECT(&uri))
            {
                cmdP->uri = (lwm2m_uri_t *)lwm2m_malloc(sizeof(lwm2m_uri_t));
                if (cmdP->uri == NULL) goto error;
                memcpy(cmdP->uri, &uri, sizeof(lwm2m_uri_t));
            }
#endif
            lwm2m_free(value);
        }
        else if (lwm2m_strcasecmp(key, "Delete") == 0)
        {
            lwm2m_uri_t uri;

            if (lwm2m_stringToUri(value, strlen(value), &uri) == 0) goto error;

            cmdP = (bs_command_t *)lwm2m_malloc(sizeof(bs_command_t));
            if (cmdP == NULL) goto error;
            memset(cmdP, 0, sizeof(bs_command_t));

            cmdP->operation = BS_DELETE;
            if (LWM2M_URI_IS_SET_OBJECT(&uri))
            {
                cmdP->uri = (lwm2m_uri_t *)lwm2m_malloc(sizeof(lwm2m_uri_t));
                if (cmdP->uri == NULL) goto error;
                memcpy(cmdP->uri, &uri, sizeof(lwm2m_uri_t));
            }

            lwm2m_free(value);
        }
        else if (lwm2m_strcasecmp(key, "Server") == 0)
        {
            int num;

            if (sscanf(value, "%d", &num) != 1) goto error;
            if (num <= 0 || num > LWM2M_MAX_ID) goto error;

            cmdP = (bs_command_t *)lwm2m_malloc(sizeof(bs_command_t));
            if (cmdP == NULL) goto error;
            memset(cmdP, 0, sizeof(bs_command_t));
            cmdP->next = (bs_command_t *)lwm2m_malloc(sizeof(bs_command_t));
            if (cmdP->next == NULL) goto error;
            memset(cmdP->next, 0, sizeof(bs_command_t));

            cmdP->operation = BS_WRITE_SECURITY;
            cmdP->serverId = num;
            cmdP->next->operation = BS_WRITE_SERVER;
            cmdP->next->serverId = num;

            lwm2m_free(value);
        }
        else
        {
            // ignore key for now
            lwm2m_free(value);
        }
        lwm2m_free(key);

        if (cmdP != NULL)
        {
            if (endptP->commandList == NULL)
            {
                endptP->commandList = cmdP;
            }
            else
            {
                bs_command_t * parentP;

                parentP = endptP->commandList;
                while (parentP->next != NULL)
                {
                    parentP = parentP->next;
                }
                parentP->next = cmdP;
            }
            cmdP = NULL;
        }
    }

    if (res != 0) goto error;

    if (endptP->commandList != NULL)
    {
        bs_command_t * parentP;

        cmdP = (bs_command_t *)lwm2m_malloc(sizeof(bs_command_t));
        if (cmdP == NULL) goto error;
        memset(cmdP, 0, sizeof(bs_command_t));

        cmdP->operation = BS_FINISH;

        parentP = endptP->commandList;
        while (parentP->next != NULL)
        {
            parentP = parentP->next;
        }
        parentP->next = cmdP;
    }

    return endptP;

error:
    if (key != NULL) lwm2m_free(key);
    if (value != NULL) lwm2m_free(value);
    while (cmdP != NULL)
    {
        bs_command_t * tempP;

        if (cmdP->uri != NULL) lwm2m_free(cmdP->uri);
        tempP = cmdP;
        cmdP = cmdP->next;
        lwm2m_free(tempP);
    }
    if (endptP != NULL)
    {
        if (endptP->name != NULL) lwm2m_free(endptP->name);
        while (endptP->commandList != NULL)
        {
            cmdP = endptP->commandList;
            endptP->commandList =endptP->commandList->next;

            if (cmdP->uri != NULL) lwm2m_free(cmdP->uri);
            lwm2m_free(cmdP);
        }
        lwm2m_free(endptP);
    }

    return NULL;
}

bs_info_t *  bs_get_info(FILE * fd)
{
    bs_info_t * infoP;
    read_server_t * readSrvP;
    bs_endpoint_info_t * cltInfoP;

    infoP = (bs_info_t *)lwm2m_malloc(sizeof(bs_info_t));
    if (infoP == NULL) return NULL;
    memset(infoP, 0, sizeof(bs_info_t));

    do
    {
        readSrvP = prv_read_next_server(fd);
        if (readSrvP != NULL)
        {
            if (prv_add_server(infoP, readSrvP) != 0) goto error;
        }
    } while (readSrvP != NULL);

    rewind(fd);
    do
    {
        cltInfoP = prv_read_next_endpoint(fd);
        if (cltInfoP != NULL)
        {
            cltInfoP->next = infoP->endpointList;
            infoP->endpointList = cltInfoP;
        }
    } while (cltInfoP != NULL);

    // check validity
    if (infoP->endpointList == NULL) goto error;

    cltInfoP = infoP->endpointList;
    while (cltInfoP != NULL)
    {
        bs_endpoint_info_t * otherP;
        bs_command_t * cmdP;
        bs_command_t * parentP;

        // check names are unique
        otherP = cltInfoP->next;
        while (otherP != NULL)
        {
            if (cltInfoP->name == NULL)
            {
                if (otherP->name == NULL) goto error;
            }
            else
            {
                if (otherP->name != NULL
                 && strcmp(cltInfoP->name, otherP->name) == 0)
                {
                    goto error;
                }
            }
            otherP = otherP->next;
        }

        // check servers exist
        cmdP = cltInfoP->commandList;
        parentP = NULL;
        // be careful: iterator changes are inside the switch/case
        while (cmdP != NULL)
        {
            switch (cmdP->operation)
            {
            case BS_WRITE_SECURITY:
                if (LWM2M_LIST_FIND(infoP->serverList, cmdP->serverId) == NULL) goto error;
                parentP = cmdP;
                cmdP = cmdP->next;
                break;

            case BS_WRITE_SERVER:
            {
                bs_server_data_t * serverP;

                serverP = (bs_server_data_t *)LWM2M_LIST_FIND(infoP->serverList, cmdP->serverId);
                if (serverP == NULL) goto error;
                if (serverP->serverData == NULL)
                {
                    // this is a Bootstrap server, remove this command
                    if (parentP == NULL)
                    {
                        cltInfoP->commandList = cmdP->next;
                        lwm2m_free(cmdP);
                        cmdP = cltInfoP->commandList;
                    }
                    else
                    {
                        parentP->next = cmdP->next;
                        lwm2m_free(cmdP);
                        cmdP = parentP->next;
                    }
                }
                else
                {
                    cmdP = cmdP->next;
                }
            }
            break;

            case BS_DELETE:
            default:
                parentP = cmdP;
                cmdP = cmdP->next;
                break;
            }
        }

        cltInfoP = cltInfoP->next;
    }

    return infoP;

error:
    bs_free_info(infoP);
    return NULL;
}

void bs_free_info(bs_info_t * infoP)
{
    if (infoP == NULL) return;

    while (infoP->serverList != NULL)
    {
        bs_server_data_t * targetP;

        targetP = infoP->serverList;
        infoP->serverList = infoP->serverList->next;

        if (targetP->securityData != NULL) lwm2m_data_free(targetP->securitySize, targetP->securityData);
        if (targetP->serverData != NULL) lwm2m_data_free(targetP->serverSize, targetP->serverData);

        lwm2m_free(targetP);
    }

    while (infoP->endpointList != NULL)
    {
        bs_endpoint_info_t * targetP;

        targetP = infoP->endpointList;
        infoP->endpointList = infoP->endpointList->next;

        if (targetP->name != NULL) lwm2m_free(targetP->name);
        while (targetP->commandList != NULL)
        {
            bs_command_t * cmdP;

            cmdP = targetP->commandList;
            targetP->commandList =targetP->commandList->next;

            if (cmdP->uri != NULL) lwm2m_free(cmdP->uri);
            lwm2m_free(cmdP);
        }

        lwm2m_free(targetP);
    }

    lwm2m_free(infoP);
}
