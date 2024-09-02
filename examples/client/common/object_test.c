/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
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
 *    David Navarro, Intel Corporation - initial API and implementation
 *    domedambrosio - Please refer to git log
 *    Fabien Fleutot - Please refer to git log
 *    Axel Lorente - Please refer to git log
 *    Achim Kraus, Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Ville Skyttä - Please refer to git log
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
 *    Tuve Nordius, Husqvarna Group - Please refer to git log
 *
 *******************************************************************************/

/*
 Copyright (c) 2013, 2014 Intel Corporation

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

 David Navarro <david.navarro@intel.com>

*/

/*
 * Implements an object for testing purpose
 *
 *                  Multiple
 * Object |  ID   | Instances | Mandatory |
 *  Test  | 31024 |    Yes    |    No     |
 *
 *  Resources:
 *              Supported    Multiple
 *  Name | ID | Operations | Instances | Mandatory |  Type   | Range | Units | Description |
 *  test |  1 |    R/W     |    No     |    Yes    | Integer | 0-255 |       |             |
 *  exec |  2 |     E      |    No     |    Yes    |         |       |       |             |
 *  dec  |  3 |    R/W     |    No     |    Yes    |  Float  |       |       |             |
 *  str  |  5 |    R/W     |    No     |    No     | String  |       |       |             |
 */

#include "liblwm2m.h"
#include "lwm2mclient.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRV_TLV_BUFFER_SIZE 64

/*
 * Multiple instance objects can use userdata to store data that will be shared between the different instances.
 * The lwm2m_object_t object structure - which represent every object of the liblwm2m as seen in the single instance
 * object - contain a chained list called instanceList with the object specific structure prv_instance_t:
 */
typedef struct _prv_instance_ {
    /*
     * The first two are mandatories and represent the pointer to the next instance and the ID of this one. The rest
     * is the instance scope user data (uint8_t test in this case)
     */
    struct _prv_instance_ *next; // matches lwm2m_list_t::next
    uint16_t shortID;            // matches lwm2m_list_t::id
    uint8_t test;
    double dec;
    char *str;
#ifdef LWM2M_RAW_BLOCK1_REQUESTS
    uint8_t *block_buffer;
    size_t value_offset;
    size_t value_len;
#endif
} prv_instance_t;

static uint8_t prv_delete(lwm2m_context_t *contextP, uint16_t id, lwm2m_object_t *objectP);
static uint8_t prv_create(lwm2m_context_t *contextP, uint16_t instanceId, int numData, lwm2m_data_t *dataArray,
                          lwm2m_object_t *objectP);

static void prv_output_buffer(uint8_t *buffer, int length) {
    int i;
    uint8_t array[16];

    i = 0;
    while (i < length) {
        int j;
        fprintf(stderr, "  ");

        memcpy(array, buffer + i, 16);

        for (j = 0; j < 16 && i + j < length; j++) {
            fprintf(stderr, "%02X ", array[j]);
        }
        while (j < 16) {
            fprintf(stderr, "   ");
            j++;
        }
        fprintf(stderr, "  ");
        for (j = 0; j < 16 && i + j < length; j++) {
            if (isprint(array[j]))
                fprintf(stderr, "%c ", array[j]);
            else
                fprintf(stderr, ". ");
        }
        fprintf(stderr, "\n");

        i += 16;
    }
}

static uint8_t prv_read(lwm2m_context_t *contextP, uint16_t instanceId, int *numDataP, lwm2m_data_t **dataArrayP,
                        lwm2m_object_t *objectP) {
    prv_instance_t *targetP;
    int i;

    /* unused parameter */
    (void)contextP;

    targetP = (prv_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP)
        return COAP_404_NOT_FOUND;

    if (*numDataP == 0) {
        *dataArrayP = lwm2m_data_new(3);
        if (*dataArrayP == NULL)
            return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = 3;
        (*dataArrayP)[0].id = 1;
        (*dataArrayP)[1].id = 3;
        (*dataArrayP)[2].id = 5;
    }

    for (i = 0; i < *numDataP; i++) {
        if ((*dataArrayP)[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
            return COAP_404_NOT_FOUND;
        }

        switch ((*dataArrayP)[i].id) {
        case 1:
            lwm2m_data_encode_int(targetP->test, *dataArrayP + i);
            break;
        case 2:
            return COAP_405_METHOD_NOT_ALLOWED;
        case 3:
            lwm2m_data_encode_float(targetP->dec, *dataArrayP + i);
            break;
        case 5:
            lwm2m_data_encode_string(targetP->str, *dataArrayP + i);
            break;
        default:
            return COAP_404_NOT_FOUND;
        }
    }

    return COAP_205_CONTENT;
}

static uint8_t prv_discover(lwm2m_context_t *contextP, uint16_t instanceId, int *numDataP, lwm2m_data_t **dataArrayP,
                            lwm2m_object_t *objectP) {
    int i;

    /* unused parameter */
    (void)contextP;

    // is the server asking for the full object ?
    if (*numDataP == 0) {
        *dataArrayP = lwm2m_data_new(4);
        if (*dataArrayP == NULL)
            return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = 3;
        (*dataArrayP)[0].id = 1;
        (*dataArrayP)[1].id = 2;
        (*dataArrayP)[2].id = 3;
        (*dataArrayP)[3].id = 5;
    } else {
        for (i = 0; i < *numDataP; i++) {
            switch ((*dataArrayP)[i].id) {
            case 1:
            case 2:
            case 3:
            case 5:
                break;
            default:
                return COAP_404_NOT_FOUND;
            }
        }
    }

    return COAP_205_CONTENT;
}

static uint8_t prv_write(lwm2m_context_t *contextP, uint16_t instanceId, int numData, lwm2m_data_t *dataArray,
                         lwm2m_object_t *objectP, lwm2m_write_type_t writeType) {
    prv_instance_t *targetP;
    int i;
    char *tmp;

    targetP = (prv_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP)
        return COAP_404_NOT_FOUND;

    if (writeType == LWM2M_WRITE_REPLACE_INSTANCE) {
        uint8_t result = prv_delete(contextP, instanceId, objectP);
        if (result == COAP_202_DELETED) {
            result = prv_create(contextP, instanceId, numData, dataArray, objectP);
            if (result == COAP_201_CREATED) {
                result = COAP_204_CHANGED;
            }
        }
        return result;
    }

    for (i = 0; i < numData; i++) {
        /* No multiple instance resources */
        if (dataArray[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;

        switch (dataArray[i].id) {
        case 1: {
            int64_t value;

            if (1 != lwm2m_data_decode_int(dataArray + i, &value) || value < 0 || value > 0xFF) {
                return COAP_400_BAD_REQUEST;
            }
            targetP->test = (uint8_t)value;
        } break;
        case 2:
            return COAP_405_METHOD_NOT_ALLOWED;
        case 3:
            if (1 != lwm2m_data_decode_float(dataArray + i, &(targetP->dec))) {
                return COAP_400_BAD_REQUEST;
            }
            break;
        case 5:
            if (dataArray[i].type == LWM2M_TYPE_STRING || dataArray[i].type == LWM2M_TYPE_OPAQUE) {
                tmp = targetP->str;
                targetP->str = lwm2m_malloc(dataArray[i].value.asBuffer.length + 1);
                strncpy(targetP->str, (char *)dataArray[i].value.asBuffer.buffer, // NOSONAR
                        dataArray[i].value.asBuffer.length);
                lwm2m_free(tmp);
                break;
            } else {
                return COAP_400_BAD_REQUEST;
            }
        default:
            return COAP_404_NOT_FOUND;
        }
    }

    return COAP_204_CHANGED;
}

#ifdef LWM2M_RAW_BLOCK1_REQUESTS

static void prv_block_buffer_free(prv_instance_t *targetP) {
    if (targetP->block_buffer != NULL) {
        lwm2m_free(targetP->block_buffer);
        targetP->block_buffer = NULL;
        targetP->value_len = 0;
        targetP->value_offset = 0;
    }
}

static uint8_t prv_raw_block1_write(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, lwm2m_media_type_t format,
                                    uint8_t *payload, int length, lwm2m_object_t *objectP, uint32_t block_num,
                                    uint8_t block_more) {
    prv_instance_t *targetP;

    targetP = (prv_instance_t *)lwm2m_list_find(objectP->instanceList, uriP->instanceId);
    if (NULL == targetP)
        return COAP_404_NOT_FOUND;
    if (uriP->resourceId < 5)
        return COAP_400_BAD_REQUEST;
    if (uriP->resourceId > 5)
        return COAP_404_NOT_FOUND;

    // Only accept single attribute value
    if (block_num == 0 && format == LWM2M_CONTENT_TLV && length > 0 &&
        ((payload[0] & 0xC0) == 0xC0 || (payload[0] & 0xC0) == 0x40)) {
        prv_block_buffer_free(targetP);

        int offset; // length of id field

        if ((payload[0] & 0x20) == 0x20) {
            offset = 2;
        } else {
            offset = 1;
        }
        switch (payload[0] & 0x18) {
        case 0x00:
            // no length field
            targetP->value_len = payload[0] & 0x07;
            break;
        case 0x08:
            // length field is 8 bits long
            targetP->value_len = payload[offset];
            offset += 1;
            break;
        case 0x10:
            // length field is 16 bits long
            targetP->value_len = (payload[offset] << 8) + payload[offset + 1];
            offset += 2;
            break;
        case 0x18:
            // length field is 24 bits long
            targetP->value_len = (payload[offset] << 16) + (payload[offset + 1] << 8) + payload[offset + 2];
            offset += 3;
            break;
        default:
            // this should never occur, so return Bad Request
            return COAP_400_BAD_REQUEST;
        }
        targetP->value_offset = offset;
        targetP->block_buffer = lwm2m_malloc(targetP->value_offset + targetP->value_offset);
    } else if (length > 0 && (format == LWM2M_CONTENT_OPAQUE || format == LWM2M_CONTENT_TEXT)) {
        size_t len = targetP->value_len + length;
        uint8_t *tmp = (uint8_t *)lwm2m_malloc(len);
        if (targetP->block_buffer != NULL) {
            memcpy(tmp, targetP->block_buffer, targetP->value_len);
            prv_block_buffer_free(targetP);
        }
        targetP->block_buffer = tmp;
        targetP->value_len = len;
    } else {
        return COAP_400_BAD_REQUEST;
    }

    if ((block_num + 1) * length > targetP->value_offset + targetP->value_len) {
        prv_block_buffer_free(targetP);
        return COAP_400_BAD_REQUEST;
    } else {
        memcpy(targetP->block_buffer + (block_num * length), payload, length);
    }

    if (block_more == 0) {
        char *old_str = targetP->str;
        targetP->str = lwm2m_malloc(targetP->value_len + 1);
        strncpy(targetP->str, (char *)targetP->block_buffer + targetP->value_offset, targetP->value_len); // NOSONAR
        targetP->str[targetP->value_len] = 0;
        lwm2m_free(old_str);
        prv_block_buffer_free(targetP);
        return COAP_204_CHANGED;
    } else {
        return COAP_231_CONTINUE;
    }
}
#endif

static uint8_t prv_delete(lwm2m_context_t *contextP, uint16_t id, lwm2m_object_t *objectP) {
    prv_instance_t *targetP;

    /* unused parameter */
    (void)contextP;

    objectP->instanceList = lwm2m_list_remove(objectP->instanceList, id, (lwm2m_list_t **)&targetP);
    if (NULL == targetP)
        return COAP_404_NOT_FOUND;

#ifdef LWM2M_RAW_BLOCK1_REQUESTS
    lwm2m_free(targetP->block_buffer);
    targetP->block_buffer = NULL;
#endif
    lwm2m_free(targetP->str);
    targetP->str = NULL;
    lwm2m_free(targetP);

    return COAP_202_DELETED;
}

static uint8_t prv_create(lwm2m_context_t *contextP, uint16_t instanceId, int numData, lwm2m_data_t *dataArray,
                          lwm2m_object_t *objectP) {
    prv_instance_t *targetP;
    uint8_t result;

    targetP = (prv_instance_t *)lwm2m_malloc(sizeof(prv_instance_t));
    if (NULL == targetP)
        return COAP_500_INTERNAL_SERVER_ERROR;
    memset(targetP, 0, sizeof(prv_instance_t));

    targetP->shortID = instanceId;
    objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, targetP);

    result = prv_write(contextP, instanceId, numData, dataArray, objectP, LWM2M_WRITE_REPLACE_RESOURCES);

    if (result != COAP_204_CHANGED) {
        (void)prv_delete(contextP, instanceId, objectP);
    } else {
        result = COAP_201_CREATED;
    }

    return result;
}

static uint8_t prv_exec(lwm2m_context_t *contextP, uint16_t instanceId, uint16_t resourceId, uint8_t *buffer,
                        int length, lwm2m_object_t *objectP) {
    /* unused parameter */
    (void)contextP;

    if (NULL == lwm2m_list_find(objectP->instanceList, instanceId))
        return COAP_404_NOT_FOUND;

    switch (resourceId) {
    case 1:
        return COAP_405_METHOD_NOT_ALLOWED;
    case 2:
        fprintf(stdout,
                "\r\n-----------------\r\n"
                "Execute on %hu/%d/%d\r\n"
                " Parameter (%d bytes):\r\n",
                objectP->objID, instanceId, resourceId, length);
        prv_output_buffer((uint8_t *)buffer, length);
        fprintf(stdout, "-----------------\r\n\r\n");
        return COAP_204_CHANGED;
    case 3:
        return COAP_405_METHOD_NOT_ALLOWED;
    case 5:
        return COAP_405_METHOD_NOT_ALLOWED;
    default:
        return COAP_404_NOT_FOUND;
    }
}

void display_test_object(lwm2m_object_t *object) {
    fprintf(stdout, "  /%u: Test object, instances:\r\n", object->objID);
    prv_instance_t *instance = (prv_instance_t *)object->instanceList;
    while (instance != NULL) {
        fprintf(stdout, "    /%u/%u: shortId: %u, test: %u\r\n", object->objID, instance->shortID, instance->shortID,
                instance->test);
        instance = (prv_instance_t *)instance->next;
    }
}

lwm2m_object_t *get_test_object(void) {
    lwm2m_object_t *testObj;

    testObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != testObj) {
        int i;
        prv_instance_t *targetP;

        memset(testObj, 0, sizeof(lwm2m_object_t));

        testObj->objID = TEST_OBJECT_ID;
        // Not required, but useful for testing.
        testObj->versionMajor = 1;
        testObj->versionMinor = 0;
        for (i = 0; i < 3; i++) {
            targetP = (prv_instance_t *)lwm2m_malloc(sizeof(prv_instance_t));
            if (NULL == targetP)
                return NULL;
            memset(targetP, 0, sizeof(prv_instance_t));
            targetP->shortID = 10 + i;
            targetP->test = 20 + i;
            targetP->dec = -30 + i + (double)i / 100.0;
            char *str = lwm2m_malloc(i + 1);
            str[0] = 0;
            for (int j = 0; j < i; j++) {
                strcat(str, "I"); // NOSONAR
            }
            targetP->str = str;
#ifdef LWM2M_RAW_BLOCK1_REQUESTS
            targetP->block_buffer = NULL;
            targetP->value_len = 0;
            targetP->value_offset = 0;
#endif
            testObj->instanceList = LWM2M_LIST_ADD(testObj->instanceList, targetP);
        }
        /*
         * From a single instance object, two more functions are available.
         * - The first one (createFunc) create a new instance and filled it with the provided informations. If an ID is
         *   provided a check is done for verifying his disponibility, or a new one is generated.
         * - The other one (deleteFunc) delete an instance by removing it from the instance list (and freeing the memory
         *   allocated to it)
         */
        testObj->readFunc = prv_read;
        testObj->discoverFunc = prv_discover;
        testObj->writeFunc = prv_write;
        testObj->executeFunc = prv_exec;
        testObj->createFunc = prv_create;
        testObj->deleteFunc = prv_delete;
#ifdef LWM2M_RAW_BLOCK1_REQUESTS
        testObj->rawBlock1WriteFunc = prv_raw_block1_write;
#endif
    }

    return testObj;
}

void free_test_object(lwm2m_object_t *object) {
    prv_instance_t *targetP = (prv_instance_t *)object->instanceList;
    while (targetP != NULL) {
        prv_instance_t *next = targetP->next;
        if (targetP != NULL) {
            lwm2m_free(targetP->str);
#ifdef LWM2M_RAW_BLOCK1_REQUESTS
            prv_block_buffer_free(targetP);
#endif
        }
        targetP = next;
    }
    LWM2M_LIST_FREE(object->instanceList);
    if (object->userData != NULL) {
        lwm2m_free(object->userData);
        object->userData = NULL;
    }
    lwm2m_free(object);
}
