/*******************************************************************************
 *
 * Copyright (c) 2015 Intel Corporation.
 * All rights reserved.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *
 *******************************************************************************/

#include <stdlib.h>
#include <string.h>

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
    uint8_t *   privateKey;
    size_t      privateKeyLen;
    uint8_t *   serverKey;
    size_t      serverKeyLen;
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
            int i;

            length = strlen(line);
            i = 1;
            while (line[i] != ']') i++;

            if (i < length)
            {
                line[i] = 0;
                if (strcasecmp(line + 1, tag) == 0)
                {
                    found = 1;
                }
            }
        }
        free(line);
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

        free(line);
        line = NULL;
        length = 0;
        if (fgetpos(fd, &prevPos) != 0) return -1;
    }

    if (res == -1) return -1;

    // end of section
    if (line[start] == '[')
    {
        free(line);
        fsetpos(fd, &prevPos);
        return 0;
    }

    middle = start;
    while (middle < length && line[middle] != '=') middle++;
    // invalid lines
    if (middle == start
     || middle == length)
    {
        free(line);
        return -1;
    }

    end = length - 2;
    while (end > middle && isspace(line[end]&0xff)) end--;
    // invalid lines
    if (end == middle)
    {
        free(line);
        return -1;
    }
    end += 1;

    line[middle] = 0;
    *keyP = strdup(line + start);
    if (*keyP == NULL)
    {
        free(line);
        return -1;
    }

    middle +=1 ;
    while (middle < end && isspace(line[middle]&0xff)) middle++;
    line[end] = 0;
    *valueP = strdup(line + middle);
    if (*valueP == NULL)
    {
        free(*keyP);
        *keyP = NULL;
        free(line);
        return -1;
    }

    free(line);

    return 1;
}

static read_server_t * prv_read_next_server(FILE * fd)
{
    char * key;
    char * value;
    read_server_t * readSrvP;
    int res;

    if (prv_find_next_section(fd, "Server") == 0) return NULL;

    readSrvP = (read_server_t *)malloc(sizeof(read_server_t));
    if (readSrvP == NULL) return NULL;
    memset(readSrvP, 0, sizeof(read_server_t));

    while((res = prv_read_key_value(fd, &key, &value)) == 1)
    {
        if (strcasecmp(key, "id") == 0)
        {
            int num;

            if (sscanf(value, "%d", &num) != 1) goto error;
            if (num <= 0 || num > LWM2M_MAX_ID) goto error;
            readSrvP->id = num;
            free(value);
        }
        else if (strcasecmp(key, "uri") == 0)
        {
            readSrvP->uri = value;
        }
        else if (strcasecmp(key, "bootstrap") == 0)
        {
            if (strcasecmp(value, "yes") == 0)
            {
                readSrvP->isBootstrap = true;
            }
            else if (strcasecmp(value, "no") == 0)
            {
                readSrvP->isBootstrap = false;
            }
            else goto error;
            free(value);
        }
        else if (strcasecmp(key, "lifetime") == 0)
        {
            int num;

            if (sscanf(value, "%d", &num) != 1) goto error;
            if (num <= 0) goto error;
            readSrvP->lifetime = num;
            free(value);
        }
        else if (strcasecmp(key, "security") == 0)
        {
            if (strcasecmp(value, "nosec") == 0)
            {
                readSrvP->securityMode = LWM2M_SECURITY_MODE_NONE;
            }
            else if (strcasecmp(value, "PSK") == 0)
            {
                readSrvP->securityMode = LWM2M_SECURITY_MODE_PRE_SHARED_KEY;
            }
            else if (strcasecmp(value, "RSK") == 0)
            {
                readSrvP->securityMode = LWM2M_SECURITY_MODE_RAW_PUBLIC_KEY;
            }
            else if (strcasecmp(value, "certificate") == 0)
            {
                readSrvP->securityMode = LWM2M_SECURITY_MODE_CERTIFICATE;
            }
            else goto error;
            free(value);
        }
        else
        {
            // ignore key for now
            free(value);
        }
        free(key);
    }

    if (res == -1) goto error;
    if (readSrvP->id == 0
     || readSrvP->uri == 0
     || (readSrvP->securityMode != LWM2M_SECURITY_MODE_NONE
          && (readSrvP->publicKey == NULL || readSrvP->serverKey == NULL))
     || (readSrvP->privateKey == NULL
          && (readSrvP->securityMode == LWM2M_SECURITY_MODE_RAW_PUBLIC_KEY
           || readSrvP->securityMode == LWM2M_SECURITY_MODE_CERTIFICATE)))
    {
        goto error;
    }

    return readSrvP;

error:
    if (readSrvP != NULL)
    {
        if (readSrvP->uri != NULL) free(readSrvP->uri);
        if (readSrvP->publicKey != NULL) free(readSrvP->publicKey);
        if (readSrvP->privateKey != NULL) free(readSrvP->privateKey);
        if (readSrvP->serverKey != NULL) free(readSrvP->serverKey);
        free(readSrvP);
    }
    if (key != NULL) free(key);
    if (value != NULL) free(value);

    return NULL;
}

static int prv_add_server(bs_info_t * infoP,
                          read_server_t * dataP)
{
    lwm2m_tlv_t * tlvP;
    int size;
    bs_server_tlv_t * serverP;

    switch (dataP->securityMode)
    {
    case LWM2M_SECURITY_MODE_NONE:
        size = 4;
        break;
    case LWM2M_SECURITY_MODE_PRE_SHARED_KEY:
        size = 6;
        break;
    case LWM2M_SECURITY_MODE_RAW_PUBLIC_KEY:
    case LWM2M_SECURITY_MODE_CERTIFICATE:
        size = 7;
        break;
    default:
        return -1;
    }

    serverP = (bs_server_tlv_t *)malloc(sizeof(bs_server_tlv_t));
    if (serverP == NULL) return -1;
    memset(serverP, 0, sizeof(bs_server_tlv_t));

    serverP->id = dataP->id;

    tlvP = lwm2m_tlv_new(size);
    if (tlvP == NULL) goto error;

    // LWM2M Server URI
    tlvP[0].type = LWM2M_TYPE_RESOURCE;
    tlvP[0].id = LWM2M_SECURITY_URI_ID;
    tlvP[0].dataType = LWM2M_TYPE_STRING;
    tlvP[0].flags = LWM2M_TLV_FLAG_STATIC_DATA;
    tlvP[0].value = (uint8_t *)dataP->uri;
    tlvP[0].length = strlen(dataP->uri);

    // Bootstrap Server
    tlvP[1].type = LWM2M_TYPE_RESOURCE;
    tlvP[1].id = LWM2M_SECURITY_BOOTSTRAP_ID;
    lwm2m_tlv_encode_bool(dataP->isBootstrap, tlvP + 1);

    // Short Server ID
    tlvP[2].type = LWM2M_TYPE_RESOURCE;
    tlvP[2].id = LWM2M_SECURITY_SHORT_SERVER_ID;
    lwm2m_tlv_encode_int(dataP->id, tlvP + 2);

    // Security Mode
    tlvP[3].type = LWM2M_TYPE_RESOURCE;
    tlvP[3].id = LWM2M_SECURITY_SECURITY_ID;
    lwm2m_tlv_encode_int(dataP->securityMode, tlvP + 3);

    if (size > 4)
    {
        tlvP[4].type = LWM2M_TYPE_RESOURCE;
        tlvP[4].id = LWM2M_SECURITY_PUBLIC_KEY_ID;
        tlvP[4].dataType = LWM2M_TYPE_OPAQUE;
        tlvP[4].flags = LWM2M_TLV_FLAG_STATIC_DATA;
        tlvP[4].value = dataP->publicKey;
        tlvP[4].length = dataP->publicKeyLen;

        tlvP[5].type = LWM2M_TYPE_RESOURCE;
        tlvP[5].id = LWM2M_SECURITY_SERVER_PUBLIC_KEY_ID;
        tlvP[5].dataType = LWM2M_TYPE_OPAQUE;
        tlvP[5].flags = LWM2M_TLV_FLAG_STATIC_DATA;
        tlvP[5].value = dataP->serverKey;
        tlvP[5].length = dataP->serverKeyLen;

        if (size = 7)
        {
            tlvP[6].type = LWM2M_TYPE_RESOURCE;
            tlvP[6].id = LWM2M_SECURITY_SERVER_PUBLIC_KEY_ID;
            tlvP[6].dataType = LWM2M_TYPE_OPAQUE;
            tlvP[6].flags = LWM2M_TLV_FLAG_STATIC_DATA;
            tlvP[6].value = dataP->privateKey;
            tlvP[6].length = dataP->privateKeyLen;
        }
    }

    serverP->securityLen = lwm2m_tlv_serialize(size, tlvP, &(serverP->securityData));
    if (serverP->securityLen <= 0) goto error;
    lwm2m_tlv_free(size, tlvP);

    if (dataP->isBootstrap == false)
    {
        size = 4;
        tlvP = lwm2m_tlv_new(size);
        if (tlvP == NULL) goto error;

        // Short Server ID
        tlvP[0].type = LWM2M_TYPE_RESOURCE;
        tlvP[0].id = LWM2M_SERVER_SHORT_ID_ID;
        lwm2m_tlv_encode_int(dataP->id, tlvP);

        // Lifetime
        tlvP[1].type = LWM2M_TYPE_RESOURCE;
        tlvP[1].id = LWM2M_SERVER_LIFETIME_ID;
        lwm2m_tlv_encode_int(dataP->lifetime, tlvP + 1);

        // Notification Storing
        tlvP[2].type = LWM2M_TYPE_RESOURCE;
        tlvP[2].id = LWM2M_SERVER_STORING_ID;
        lwm2m_tlv_encode_bool(false, tlvP + 2);

        // Binding
        tlvP[3].type = LWM2M_TYPE_RESOURCE;
        tlvP[3].id = LWM2M_SERVER_BINDING_ID;
        tlvP[3].dataType = LWM2M_TYPE_STRING;
        tlvP[3].flags = LWM2M_TLV_FLAG_STATIC_DATA;
        tlvP[3].value = "U";
        tlvP[3].length = 1;

        serverP->serverLen = lwm2m_tlv_serialize(size, tlvP, &(serverP->serverData));
        if (serverP->serverLen <= 0) goto error;
        lwm2m_tlv_free(size, tlvP);
    }

    infoP->serverList = (bs_server_tlv_t *)LWM2M_LIST_ADD(infoP->serverList, serverP);

    return 0;

error:
    if (tlvP != NULL) lwm2m_tlv_free(size, tlvP);
    if (serverP->securityData != NULL) free(serverP->securityData);
    if (serverP->serverData != NULL) free(serverP->serverData);
    free(serverP);

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

    endptP = (bs_endpoint_info_t *)malloc(sizeof(bs_endpoint_info_t));
    if (endptP == NULL) return NULL;
    memset(endptP, 0, sizeof(bs_endpoint_info_t));

    cmdP = NULL;

    while((res = prv_read_key_value(fd, &key, &value)) == 1)
    {
        if (strcasecmp(key, "Name") == 0)
        {
            endptP->name = value;
        }
        else if (strcasecmp(key, "Delete") == 0)
        {
            lwm2m_uri_t uri;


            if (lwm2m_stringToUri(value, strlen(value), &uri) == 0)
            {
                if (value[0] == '/'
                 && value[1] == 0)
                {
                    uri.flag = 0;
                }
                else goto error;
            }

            cmdP = (bs_command_t *)malloc(sizeof(bs_command_t));
            if (cmdP == NULL) goto error;
            memset(cmdP, 0, sizeof(bs_command_t));

            cmdP->operation = BS_DELETE;
            if (uri.flag != 0)
            {
                cmdP->uri = (lwm2m_uri_t *)malloc(sizeof(lwm2m_uri_t));
                if (cmdP->uri == NULL) goto error;
                memcpy(cmdP->uri, &uri, sizeof(lwm2m_uri_t));
            }

            free(value);
        }
        else if (strcasecmp(key, "Server") == 0)
        {
            int num;

            if (sscanf(value, "%d", &num) != 1) goto error;
            if (num <= 0 || num > LWM2M_MAX_ID) goto error;

            cmdP = (bs_command_t *)malloc(sizeof(bs_command_t));
            if (cmdP == NULL) goto error;
            memset(cmdP, 0, sizeof(bs_command_t));
            cmdP->next = (bs_command_t *)malloc(sizeof(bs_command_t));
            if (cmdP->next == NULL) goto error;
            memset(cmdP->next, 0, sizeof(bs_command_t));

            cmdP->operation = BS_WRITE_SECURITY;
            cmdP->serverId = num;
            cmdP->next->operation = BS_WRITE_SERVER;
            cmdP->next->serverId = num;

            free(value);
        }
        else
        {
            // ignore key for now
            free(value);
        }
        free(key);

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

    return endptP;

error:
    if (key != NULL) free(key);
    if (value != NULL) free(value);
    while (cmdP != NULL)
    {
        bs_command_t * tempP;

        if (cmdP->uri != NULL) free(cmdP->uri);
        tempP = cmdP;
        cmdP = cmdP->next;
        free(tempP);
    }
    if (endptP != NULL)
    {
        if (endptP->name != NULL) free(endptP->name);
        while (endptP->commandList != NULL)
        {
            cmdP = endptP->commandList;
            endptP->commandList =endptP->commandList->next;

            if (cmdP->uri != NULL) free(cmdP->uri);
            free(cmdP);
        }
        free(endptP);
    }

    return NULL;
}

bs_info_t *  bs_get_info(FILE * fd)
{
    bs_info_t * infoP;
    read_server_t * readSrvP;
    bs_endpoint_info_t * cltInfoP;
    bs_command_t * cmdP;

    infoP = (bs_info_t *)malloc(sizeof(bs_info_t));
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
                bs_server_tlv_t * serverP;

                serverP = (bs_server_tlv_t *)LWM2M_LIST_FIND(infoP->serverList, cmdP->serverId);
                if (serverP == NULL) goto error;
                if (serverP->serverData == NULL)
                {
                    // this is a Bootstrap server, remove this command
                    if (parentP == NULL)
                    {
                        cltInfoP->commandList = cmdP->next;
                        free(cmdP);
                        cmdP = cltInfoP->commandList;
                    }
                    else
                    {
                        parentP->next = cmdP->next;
                        free(cmdP);
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
        bs_server_tlv_t * targetP;

        targetP = infoP->serverList;
        infoP->serverList = infoP->serverList->next;

        if (targetP->securityData != NULL) free(targetP->securityData);
        if (targetP->serverData != NULL) free(targetP->serverData);

        free(targetP);
    }

    while (infoP->endpointList != NULL)
    {
        bs_endpoint_info_t * targetP;

        targetP = infoP->endpointList;
        infoP->endpointList = infoP->endpointList->next;

        if (targetP->name != NULL) free(targetP->name);
        while (targetP->commandList != NULL)
        {
            bs_command_t * cmdP;

            cmdP = targetP->commandList;
            targetP->commandList =targetP->commandList->next;

            if (cmdP->uri != NULL) free(cmdP->uri);
            free(cmdP);
        }

        free(targetP);
    }

    free(infoP);
}
