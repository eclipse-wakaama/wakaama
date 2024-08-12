/*******************************************************************************
 *
 * Copyright (c) 2024 GARDENA GmbH
 *
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
 *   Lukas Woodtli, GARDENA GmbH - Please refer to git log
 *
 *******************************************************************************/
#ifndef WAKAAMA_OBJECTS_H
#define WAKAAMA_OBJECTS_H
#include "internals.h"

uint8_t object_readData(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, int *sizeP, lwm2m_data_t **dataP);
uint8_t object_read(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, const uint16_t *accept, uint8_t acceptNum,
                    lwm2m_media_type_t *formatP, uint8_t **bufferP, size_t *lengthP);
uint8_t object_write(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, lwm2m_media_type_t format, uint8_t *buffer,
                     size_t length, bool partial);
uint8_t object_create(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, lwm2m_media_type_t format, uint8_t *buffer,
                      size_t length);
uint8_t object_execute(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, uint8_t *buffer, size_t length);
#ifdef LWM2M_RAW_BLOCK1_REQUESTS
uint8_t object_raw_block1_write(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, lwm2m_media_type_t format,
                                uint8_t *buffer, size_t length, uint32_t block_num, uint8_t block_more);
uint8_t object_raw_block1_create(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, lwm2m_media_type_t format,
                                 uint8_t *buffer, size_t length, uint32_t block_num, uint8_t block_more);
uint8_t object_raw_block1_execute(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, uint8_t *buffer, size_t length,
                                  uint32_t block_num, uint8_t block_more);
#endif
uint8_t object_delete(lwm2m_context_t *contextP, lwm2m_uri_t *uriP);
uint8_t object_discover(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, lwm2m_server_t *serverP, uint8_t **bufferP,
                        size_t *lengthP);
uint8_t object_checkReadable(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, lwm2m_attributes_t *attrP);
bool object_isInstanceNew(lwm2m_context_t *contextP, uint16_t objectId, uint16_t instanceId);
int object_getRegisterPayloadBufferLength(lwm2m_context_t *contextP);
int object_getRegisterPayload(lwm2m_context_t *contextP, uint8_t *buffer, size_t length);
int object_getServers(lwm2m_context_t *contextP, bool checkOnly);
uint8_t object_createInstance(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, lwm2m_data_t *dataP);
uint8_t object_writeInstance(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, lwm2m_data_t *dataP);
#ifndef LWM2M_VERSION_1_0
uint8_t object_readCompositeData(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, size_t numUris, int *sizeP,
                                 lwm2m_data_t **dataP);
#endif

#endif /* WAKAAMA_OBJECTS_H */
