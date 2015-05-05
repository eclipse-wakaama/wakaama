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
} read_data_t;


static int prv_add_server(bs_info_t * infoP,
                          read_data_t * dataP)
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

    serverP->next = infoP->serverList;
    infoP->serverList = serverP;

    return 0;

error:
    if (tlvP != NULL) lwm2m_tlv_free(size, tlvP);
    if (serverP->securityData != NULL) free(serverP->securityData);
    if (serverP->serverData != NULL) free(serverP->serverData);
    free(serverP);

    return -1;
}

bs_info_t *  bs_get_info(char * filename)
{
    bs_info_t * infoP;
    read_data_t data;
    bs_endpoint_info_t * cltInfoP;
    bs_command_t * cmdP;

    infoP = (bs_info_t *)malloc(sizeof(bs_info_t));
    if (infoP == NULL) return NULL;

memset(&data, 0, sizeof(server_info_t));
data.id = 1;
data.uri = "coap://localhost:5683";
data.isBootstrap = false;
data.lifetime = 300;
data.securityMode = LWM2M_SECURITY_MODE_NONE;

    if (prv_add_server(infoP, &data) != 0) goto error;

    cltInfoP = (bs_endpoint_info_t *)malloc(sizeof(bs_endpoint_info_t));
    if (cltInfoP == NULL) goto error;
    memset(cltInfoP, 0, sizeof(bs_endpoint_info_t));
    infoP->endpointList = cltInfoP;

    cmdP = (bs_command_t *)malloc(sizeof(bs_command_t));
    if (cmdP == NULL) goto error;
    memset(cmdP, 0, sizeof(bs_command_t));
    cltInfoP->commandList = cmdP;

cmdP->operation = BS_DELETE;

cmdP = (bs_command_t *)malloc(sizeof(bs_command_t));
if (cmdP == NULL) goto error;
memset(cmdP, 0, sizeof(bs_command_t));
cltInfoP->commandList->next = cmdP;

cmdP->operation = BS_WRITE_SECURITY;
cmdP->serverId = 1;

cmdP = (bs_command_t *)malloc(sizeof(bs_command_t));
if (cmdP == NULL) goto error;
memset(cmdP, 0, sizeof(bs_command_t));
cltInfoP->commandList->next->next = cmdP;

cmdP->operation = BS_WRITE_SERVER;
cmdP->serverId = 1;

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
