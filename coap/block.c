/*******************************************************************************
 *
 * Copyright (c) 2016 Intel Corporation and others.
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
 *    Simon Bernard - initial API and implementation
 *    Tuve Nordius, Husqvarna Group - Please refer to git log
 *
 *******************************************************************************/
/*
 Copyright (c) 2016 Intel Corporation

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
*/
#include "internals.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// the maximum payload transferred by block we accumulate per transfer
#ifndef MAX_BLOCK_SIZE
#define MAX_BLOCK_SIZE 4096
#endif

    
bool prv_matchBlock1 (block_data_identifier_t identifier, lwm2m_block_data_t * blockData)
    {
    if (blockData->identifier.uri == NULL || identifier.uri == NULL)
    {
        return false;
    }
    return strcmp(identifier.uri, blockData->identifier.uri) == 0;
}
    
bool prv_matchBlock2 (block_data_identifier_t identifier, lwm2m_block_data_t * blockData)
{
    return identifier.mid == blockData->identifier.mid;
}

bool (* prv_get_matcher(block_type_t blockType)) (block_data_identifier_t, lwm2m_block_data_t *) {
    if (blockType == BLOCK_1)
{
        return &prv_matchBlock1;
    }
    else
    {
        return &prv_matchBlock2;
    }
}

lwm2m_block_data_t * find_block_data(lwm2m_block_data_t * blockDataHead,
                                     block_data_identifier_t identifier,
                                     block_type_t blockType)
{
    bool (* match) (block_data_identifier_t, lwm2m_block_data_t *) = prv_get_matcher(blockType);
    lwm2m_block_data_t * blockData = blockDataHead;
    
    while(blockData != NULL && !match(identifier, blockData))
    {
        blockData = blockData->next;
}

    return blockData;
}

/*
 * creates and inserts blockData entry
 * returns the newly created entry or NULL if creation fails
 */
static
lwm2m_block_data_t * prv_block_insert(lwm2m_block_data_t ** pBlockDataHead, block_data_identifier_t identifier, block_type_t blockType)
{
    lwm2m_block_data_t * blockData = (lwm2m_block_data_t *) lwm2m_malloc(sizeof(lwm2m_block_data_t));
    if (NULL == blockData) return NULL;
    blockData->next = *pBlockDataHead;
    blockData->blockType = blockType;
    blockData->identifier = identifier;
    if (blockType == BLOCK_1) {
        blockData->identifier.uri = lwm2m_strdup(identifier.uri);
    }
    *pBlockDataHead = blockData;
    return blockData;
}

static
void prv_block_data_delete(lwm2m_block_data_t ** pBlockDataHead,
                                           block_data_identifier_t identifier,
                                           block_type_t blockType
                                           )
{
    lwm2m_block_data_t * removed = find_block_data(*pBlockDataHead, identifier, blockType);
    
    if (removed == NULL) {
        return;
    }

    if (removed == *pBlockDataHead) {
        *pBlockDataHead = (*pBlockDataHead)->next;
    } else {
        lwm2m_block_data_t * target = *pBlockDataHead;

        while(NULL != target->next && target->next != removed){
           target = target->next;
        }

        if (NULL != target->next && target->next == removed)
        {
           target->next = target->next->next;
        }
    }
    free_block_data(removed);
}

#ifdef LWM2M_RAW_BLOCK1_REQUESTS
static
uint8_t prv_coap_raw_block_handler(lwm2m_block_data_t ** pBlockDataHead,
                               block_data_identifier_t identifier,
                               uint16_t mid,
                               block_type_t blockType,
                               uint8_t * buffer,
                               size_t length,
                               uint16_t blockSize,
                               uint32_t blockNum,
                               bool blockMore)
{
    lwm2m_block_data_t * blockData = find_block_data(*pBlockDataHead, identifier, blockType);
    
    // manage new block transfer
    if (blockNum == 0)
    {
        if (blockData == NULL)
        {
            blockData = prv_block_insert(pBlockDataHead, identifier, blockType);
        }
        else if (blockData->mid == mid)
        {
            return COAP_IGNORE;
        }
    }
    // manage already started block1 transfer
    else
    {
        if (blockData == NULL)
        {
            // we never receive the first block or block is out of order
            return COAP_408_REQ_ENTITY_INCOMPLETE;
        }

        if (blockNum <= blockData->blockNum){
            // this is a retransmissiion, ignore
            return COAP_IGNORE;
        }
    }

    blockData->blockNum = blockNum;
    blockData->mid = mid;

    if (blockMore)
    {
        return COAP_231_CONTINUE;
    }
    else
    {
        prv_block_data_delete(pBlockDataHead, identifier, blockType);

        return NO_ERROR;
    }
}
#endif

static
uint8_t prv_coap_block_handler(lwm2m_block_data_t ** pBlockDataHead,
                               block_data_identifier_t identifier,
                               block_type_t blockType,
                               uint8_t * buffer,
                               size_t length,
                               uint16_t blockSize,
                               uint32_t blockNum,
                               bool blockMore,
                               uint8_t ** outputBuffer,
                               size_t * outputLength)
{
    lwm2m_block_data_t * blockData = find_block_data(*pBlockDataHead, identifier, blockType);
    
    // manage new block transfer
    if (blockNum == 0)
    {
        if (blockData == NULL)
        {
            blockData = prv_block_insert(pBlockDataHead, identifier, blockType);
        }
        else
        {
            // there is already existing block for this resource, clear buffer
            lwm2m_free(blockData->blockBuffer);
            blockData->blockBuffer = NULL;
            blockData->blockBufferSize = 0;
        }

        uint8_t * buf = (uint8_t *) lwm2m_malloc(length);
        if(buf == NULL){
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
        blockData->blockBuffer = buf;
        blockData->blockBufferSize = length;

        // write new block in buffer
        memcpy(blockData->blockBuffer, buffer, length);
        blockData->blockNum = blockNum;
    }
    // manage already started block1 transfer
    else
    {
        if (blockData == NULL)
        {
           return COAP_408_REQ_ENTITY_INCOMPLETE;
        }

        if (blockNum <= blockData->blockNum){
            // this is a retransmission
            return COAP_RETRANSMISSION;
        }

        // If this is a retransmission, we already did that.
       if (blockNum == blockData->blockNum +1)
       {
          uint8_t * oldBuffer = blockData->blockBuffer;
          size_t oldSize = blockData->blockBufferSize;

          if (blockData->blockBufferSize != (size_t)blockSize * blockNum) {
              // we don't receive block in right order
              // TODO should we clean block1 data for this server ?
              return COAP_408_REQ_ENTITY_INCOMPLETE;
          }

          // re-alloc new buffer
          blockData->blockBufferSize = oldSize+length;
          blockData->blockBuffer = (uint8_t *) lwm2m_malloc(blockData->blockBufferSize);
          if (NULL == blockData->blockBuffer) return COAP_500_INTERNAL_SERVER_ERROR; //TODO: should we clean up
          memcpy(blockData->blockBuffer, oldBuffer, oldSize);
          lwm2m_free(oldBuffer);

          // write new block in buffer
          memcpy(blockData->blockBuffer + oldSize, buffer, length);
          blockData->blockNum = blockNum;
       }
    }

    if (blockMore)
    {
        *outputLength = -1;
        return COAP_231_CONTINUE;
    }
    else
    {
        // buffer is full, set output parameter
        // we don't free it to be able to send retransmission
        *outputLength = blockData->blockBufferSize;
        *outputBuffer = blockData->blockBuffer;

        return NO_ERROR;
    }
}

lwm2m_block_data_t * block1_create(lwm2m_block_data_t ** pBlockDataHead, char *uri)
{
    block_data_identifier_t identifier;
    identifier.uri = uri;
    return prv_block_insert(pBlockDataHead, identifier, BLOCK_1);
}

void block1_delete(lwm2m_block_data_t ** pBlockDataHead,
                   char * uri)
{
    block_data_identifier_t identifier;
    identifier.uri = uri;

    prv_block_data_delete(pBlockDataHead, identifier, BLOCK_1);
}

uint8_t coap_block1_handler (lwm2m_block_data_t ** pBlockDataHead,
 
                            const char * uri,
#ifdef LWM2M_RAW_BLOCK1_REQUESTS
                            uint16_t mid,
#endif
                            uint8_t * buffer,
                            size_t length,
                            uint16_t blockSize,
                            uint32_t blockNum,
                            bool blockMore,
                            uint8_t ** outputBuffer,
                            size_t * outputLength)
{
    block_data_identifier_t identifier;
    identifier.uri = (char *) uri;
#ifdef LWM2M_RAW_BLOCK1_REQUESTS
    return prv_coap_raw_block_handler(pBlockDataHead, identifier, mid, BLOCK_1, buffer, length, blockSize, blockNum, blockMore);
#else
    return prv_coap_block_handler(pBlockDataHead, identifier, BLOCK_1, buffer, length, blockSize, blockNum, blockMore, outputBuffer, outputLength);
#endif
}

lwm2m_block_data_t * block2_create(lwm2m_block_data_t ** pBlockDataHead, uint16_t mid)
{
    block_data_identifier_t identifier;
    identifier.mid = mid;
    return prv_block_insert(pBlockDataHead, identifier, BLOCK_2);
}

void block2_delete(lwm2m_block_data_t ** pBlockDataHead, uint16_t mid)
{
    block_data_identifier_t identifier;
    identifier.mid = mid;

    prv_block_data_delete(pBlockDataHead, identifier, BLOCK_2);
}

void coap_block2_set_expected_mid(lwm2m_block_data_t * blockDataHead, uint16_t currentMid, uint16_t expectedMid)
{
    block_data_identifier_t identifier;
    identifier.mid = currentMid;
    
    lwm2m_block_data_t * blockData = find_block_data(blockDataHead, identifier, BLOCK_2);
    if(blockData == NULL)
    {
        return;
    }
    blockData->identifier.mid = expectedMid;
}

uint8_t coap_block2_handler(lwm2m_block_data_t ** pBlockDataHead,
                            uint16_t mid,
                            uint8_t * buffer,
                            size_t length,
                            uint16_t blockSize,
                            uint32_t blockNum,
                            bool blockMore,
                            uint8_t ** outputBuffer,
                            size_t * outputLength)
{
    block_data_identifier_t identifier;
    identifier.mid = mid;

    return prv_coap_block_handler(pBlockDataHead, identifier, BLOCK_2, buffer, length, blockSize, blockNum, blockMore, outputBuffer, outputLength);
}


void free_block_data(lwm2m_block_data_t * blockData)
{
    if (blockData != NULL)
    {
#ifndef LWM2M_RAW_BLOCK1_REQUESTS
        lwm2m_free(blockData->blockBuffer);
#endif
        if (blockData->blockType == BLOCK_1)
        {
            lwm2m_free(blockData->identifier.uri);
        }
        lwm2m_free(blockData);
    }
}
