/*******************************************************************************
 *
 * Copyright (c) 2014 Bosch Software Innovations GmbH, Germany.
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
 *    Bosch Software Innovations GmbH - Please refer to git log
 *
 *******************************************************************************/
/*
 *  blockwise.c
 *  basic functions for blockwise data transfer.
 *
 *  Created on: 19.12.2014
 *  Author: Achim Kraus
 *  Copyright (c) 2014 Bosch Software Innovations GmbH, Germany. All rights reserved.
 */

#include <stdlib.h>
#include "liblwm2m.h"
#include "internals.h"

//#define USE_ETAG
//#define TEST_ETAG

#ifdef WITH_LOGS
static void prv_logUri(const char* desc, const lwm2m_uri_t * uriP)
{
    uint8_t flag = uriP->flag & LWM2M_URI_MASK_ID;
    LOG("%s flag 0x%x %d/%d/%d\n", desc, flag, uriP->objectId, uriP->instanceId, uriP->resourceId);
}

#define LOG_URI(D, U) prv_logUri(D, U)
#else
#define LOG_URI(D, U)
#endif

static int prv_blockwise_compare_uri(const lwm2m_uri_t * uri1P, const lwm2m_uri_t * uri2P)
{
    uint8_t flag = uri1P->flag & LWM2M_URI_MASK_ID;
#if 0
    LOG_URI("URI1:", uri1P);
    LOG_URI("URI2:", uri2P);
#endif
    if (flag != (uri2P->flag & LWM2M_URI_MASK_ID ))
        return 1;
    if ((flag & LWM2M_URI_FLAG_OBJECT_ID ) && (uri1P->objectId != uri2P->objectId))
        return 2;
    if ((flag & LWM2M_URI_FLAG_INSTANCE_ID ) && (uri1P->instanceId != uri2P->instanceId))
        return 3;
    if ((flag & LWM2M_URI_FLAG_RESOURCE_ID ) && (uri1P->resourceId != uri2P->resourceId))
        return 4;
    return 0;
}

static void prv_blockwise_set_time(lwm2m_blockwise_t* current)
{
    struct timeval tv;
    if (0 == lwm2m_gettimeofday(&tv, NULL))
    {
        current->time = tv.tv_sec;
    }
}

#ifdef USE_ETAG
static void prv_blockwise_etag(lwm2m_blockwise_t* current)
{
    static uint16_t counter = 0;
    uint16_t tag;
    uint32_t secs;
    int pos;
    struct timeval tv;

    if (sizeof(secs) <= sizeof(current->etag) && 0 == lwm2m_gettimeofday(&tv, NULL))
    {
        uint32_t secs = tv.tv_sec;
        memcpy(current->etag, &secs, sizeof(secs));
        current->etag_len = sizeof(secs);
    }
    pos = MIN(sizeof(current->etag) - sizeof(tag), current->etag_len);
    if (0 <= pos)
    {
        tag = ++counter;
        memcpy(current->etag + pos, &tag, sizeof(tag));
        current->etag_len += sizeof(tag);
    }
}
#endif

lwm2m_blockwise_t* blockwise_get(lwm2m_context_t * contextP, const lwm2m_uri_t * uriP)
{
    lwm2m_blockwise_t* current = contextP->blockwiseList;
    while (NULL != current)
    {
        if (0 == prv_blockwise_compare_uri(&current->uri, uriP))
        {
            LOG("Found blockwise %d bytes for %d/%d/%d\n", current->length, current->uri.objectId,
                    current->uri.instanceId, current->uri.resourceId);
            return current;
        }
        current = current->next;
    }
    return NULL;
}

lwm2m_blockwise_t * blockwise_new(lwm2m_context_t * contextP, const lwm2m_uri_t * uriP, coap_packet_t * responseP,
bool detach)
{
    lwm2m_blockwise_t* result = (lwm2m_blockwise_t *) lwm2m_malloc(sizeof(lwm2m_blockwise_t));
    if (NULL == result)
        return NULL;
    memset(result, 0, sizeof(lwm2m_blockwise_t));

    if (detach)
    {
        result->size = REST_MAX_CHUNK_SIZE * 4;
        result->data = malloc(result->size);
        if (NULL == result->data)
        {
            free(result);
            return NULL;
        }
        result->length = responseP->payload_len;
        memcpy(result->data, responseP->payload, result->length);
        memset(result->data + result->length, 0, result->size - result->length);
    }
    else
    {
        result->size = responseP->payload_len;
        result->length = responseP->payload_len;
        result->data = responseP->payload;
    }

    result->next = contextP->blockwiseList;
    contextP->blockwiseList = result;

    result->uri = *uriP;
    result->etag_len = responseP->etag_len;
    if (0 < result->etag_len)
    {
        memcpy(result->etag, responseP->etag, responseP->etag_len);
    }
#ifdef USE_ETAG
    else
    {
        prv_blockwise_etag(result);
    }
#endif

    LOG_URI("URI:", uriP);
    LOG("New blockwise %d bytes for %d/%d/%d\n", result->length, result->uri.objectId, result->uri.instanceId,
            result->uri.resourceId);
    return result;
}

void blockwise_prepare(lwm2m_blockwise_t * blockwiseP, uint32_t block_num, uint16_t block_size,
        coap_packet_t * response)
{
    uint32_t block_offset = block_num * block_size;
    int packet_payload_length = MIN(blockwiseP->length - block_offset, block_size);
    int more = block_offset + packet_payload_length < blockwiseP->length;
    coap_set_header_block2(response, block_num, more, block_size);
    if (0 < blockwiseP->etag_len)
    {
#ifdef TEST_ETAG
        // change etag to test detection by server.
        // Currently ignored by server (cal 1.0.0 / leshan 0.1.9).
        blockwiseP->etag[0]++;
#endif
        coap_set_header_etag(response, blockwiseP->etag, blockwiseP->etag_len);
    }
    coap_set_payload(response, blockwiseP->data + block_offset, packet_payload_length);
    prv_blockwise_set_time(blockwiseP);
    LOG("  Blockwise: prepare block bs %d, index %d, offset %d (%s)\r\n", block_size, block_num, block_offset,
            more ? "more..." : "last");
}

void blockwise_append(lwm2m_blockwise_t * blockwiseP, uint32_t block_offset, coap_packet_t * response)
{
    size_t length = blockwiseP->length;
    prv_blockwise_set_time(blockwiseP);
    blockwiseP->length = block_offset + response->payload_len;
    if (blockwiseP->length > blockwiseP->size)
    {
        size_t newSize = blockwiseP->size * 2;
        uint8_t* newPayload = malloc(newSize);
        if (NULL == newPayload)
            return;
        memcpy(newPayload, blockwiseP->data, length);
        memset(newPayload + length, 0, newSize - blockwiseP->length);
        free(blockwiseP->data);
        blockwiseP->size = newSize;
        blockwiseP->data = newPayload;
    }
    memcpy(blockwiseP->data + block_offset, response->payload, response->payload_len);
}

void blockwise_remove(lwm2m_context_t * contextP, const lwm2m_uri_t * uriP)
{
    lwm2m_blockwise_t* current = contextP->blockwiseList;
    lwm2m_blockwise_t* prev = NULL;
    while (NULL != current)
    {
        if (0 == prv_blockwise_compare_uri(&current->uri, uriP))
        {
            if (NULL == prev)
            {
                contextP->blockwiseList = current->next;
            }
            else
            {
                prev->next = current->next;
            }
            LOG("Remove blockwise %d bytes for %d/%d/%d\n", current->length, current->uri.objectId,
                    current->uri.instanceId, current->uri.resourceId);
            lwm2m_free(current->data);
            lwm2m_free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

void blockwise_free(lwm2m_context_t * contextP, uint32_t time)
{
    int removed = 0;
    int pending = 0;
    lwm2m_blockwise_t* current = contextP->blockwiseList;
    lwm2m_blockwise_t* rem = NULL;
    lwm2m_blockwise_t* prev = NULL;
    while (NULL != current)
    {
        if (time - current->time > COAP_DEFAULT_MAX_AGE)
        {
            rem = current;
            if (NULL == prev)
            {
                contextP->blockwiseList = current->next;
            }
            else
            {
                prev->next = current->next;
            }
            ++removed;
        }
        else
        {
            ++pending;
            prev = current;
        }
        current = current->next;
        if (NULL != rem)
        {
            LOG("Free blockwise %d bytes for %d/%d/%d\n", rem->length, rem->uri.objectId, rem->uri.instanceId,
                    rem->uri.resourceId);
            lwm2m_free(rem->data);
            lwm2m_free(rem);
            rem = NULL;
        }
    }
    if (pending || removed)
    {
        LOG("Blockwise %u time: %d pending, %d removed\n", time, pending, removed);
    }
}
