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

#include <stdio.h>
#include "liblwm2m.h"

typedef enum
{
    BS_DISCOVER = 0,
#ifndef LWM2M_VERSION_1_0
    BS_READ,
#endif
    BS_DELETE,
    BS_WRITE_SECURITY,
    BS_WRITE_SERVER,
    BS_FINISH
} bs_operation_t;

typedef struct
{
    uint16_t    id;
    char *      uri;
    bool        isBootstrap;
    uint32_t    lifetime;
    uint8_t     securityMode;
    uint8_t *   publicKey;
    uint8_t *   privateKey;
    uint8_t *   serverKey;
} server_info_t;

typedef struct _server_data_
{
    struct _server_data_ * next;  // matches lwm2m_list_t::next
    uint16_t               id;    // matches lwm2m_list_t::id
    lwm2m_data_t *         securityData;
    int                    securitySize;
    lwm2m_data_t *         serverData;
    int                    serverSize;
} bs_server_data_t;

typedef struct _command_
{
    struct _command_ * next;
    bs_operation_t  operation;
    lwm2m_uri_t *   uri;
    uint16_t        serverId;
} bs_command_t;

typedef struct _endpoint_info_
{
    struct _endpoint_info_ * next;
    char *             name;
    bs_command_t *     commandList;
} bs_endpoint_info_t;

typedef struct
{
    bs_server_data_t *   serverList;
    bs_endpoint_info_t * endpointList;
} bs_info_t;

bs_info_t * bs_get_info(FILE * fd);
void bs_free_info(bs_info_t * infoP);
