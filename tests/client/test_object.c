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
 *    domedambrosio - Please refer to git log
 *    Fabien Fleutot - Please refer to git log
 *    Axel Lorente - Please refer to git log
 *    Bosch Software Innovatios GmbH - Please refer to git log
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
 *                 Multiple
 * Object |  ID  | Instances | Mandatoty |
 *  Test  | 1024 |    Yes    |    No     |
 *
 *  Ressources:
 *              Supported    Multiple
 *  Name   | ID | Operations | Instances | Mandatory |  Type   | Range | Units | Description        |
 *  test   |  1 |    R/W     |    No     |    Yes    | Integer | 0-255 |       |                    |
 *  exec   |  2 |     E      |    No     |    Yes    |         |       |       |  execute test res. |
 *  integer|  3 |    R/W     |    No     |    Yes    | Integer |       |       |  integer test res. |
 *  float  |  4 |    R/W     |    No     |    Yes    | Float   |       |       |  float   test res. |
 *  string |  5 |    R/W     |    No     |    Yes    | String  |       |       |  String  test res. |
 *  time   |  6 |    R/W     |    No     |    Yes    | Time    |       | sec   |  time    test res. |
 *  bool   |  7 |    R/W     |    No     |    Yes    | boolean |       |       |  bool    test res. |
 *  opaque |  8 |    R/W     |    No     |    Yes    | opaque  |       |       |  opaque  test res. |
 *
 */

#include "liblwm2m.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#define PRV_TLV_BUFFER_SIZE 64

/*
 * Ressource IDs for the Test Object
 */
#define TEST_TEST_ID    1
#define TEST_EXEC_ID    2

#define TEST_INTEGER_ID 3
#define TEST_FLOAT_ID   4
#define TEST_STRING_ID  5
#define TEST_TIME_ID    6
#define TEST_BOOLEAN_ID 7
#define TEST_OPAQUE_ID  8

/*
 * Multiple instance objects can use userdata to store data that will be shared between the different instances.
 * The lwm2m_object_t object structure - which represent every object of the liblwm2m as seen in the single instance
 * object - contain a chained list called instanceList with the object specific structure prv_instance_t:
 */
typedef struct _prv_instance_
{
    /*
     * The first two are mandatories and represent the pointer to the next instance and the ID of this one. The rest
     * is the instance scope user data (uint8_t test in this case)
     */
    struct _prv_instance_ * next;   // matches lwm2m_list_t::next
    uint16_t shortID;               // matches lwm2m_list_t::id
    uint8_t  test;
    // new resources:
    int32_t  rInteger;
    float    rFloat;
    char*    rString;
    time_t   rTime;
    bool     rBoolean;
    uint8_t* rOpaque;
    uint16_t lOpaque;               // opaque length!
} prv_instance_t;

static void prv_output_buffer(uint8_t * buffer,
                              int length)
{
    int i;
    uint8_t array[16];

    i = 0;
    while (i < length)
    {
        int j;
        fprintf(stderr, "  ");

        memcpy(array, buffer+i, 16);

        for (j = 0 ; j < 16 && i+j < length; j++)
        {
            fprintf(stderr, "%02X ", array[j]);
        }
        while (j < 16)
        {
            fprintf(stderr, "   ");
            j++;
        }
        fprintf(stderr, "  ");
        for (j = 0 ; j < 16 && i+j < length; j++)
        {
            if (isprint(array[j]))
                fprintf(stderr, "%c ", array[j]);
            else
                fprintf(stderr, ". ");
        }
        fprintf(stderr, "\n");

        i += 16;
    }
}

static uint8_t prv_set_tlv_value(lwm2m_tlv_t * tlvP, prv_instance_t * targetP) {
    //-------------------------------------------------------------------- JH --
    uint8_t ret = COAP_205_CONTENT;
    // a simple switch structure is used to respond at the specified resource asked
    switch (tlvP->id) {
    case TEST_TEST_ID:
        lwm2m_tlv_encode_int(targetP->test, tlvP);
        if (tlvP->length==0) ret = COAP_500_INTERNAL_SERVER_ERROR;
        break;
    case TEST_INTEGER_ID:
        lwm2m_tlv_encode_int(targetP->rInteger, tlvP);
        if (tlvP->length==0) ret = COAP_500_INTERNAL_SERVER_ERROR;
        break;
    case TEST_FLOAT_ID:
        //TODO lwm2m_tlv_encode_float(targetP->rFloat, tlvP);
        if (tlvP->length==0) ret = COAP_500_INTERNAL_SERVER_ERROR;
        break;
    case TEST_STRING_ID:
        //TODO lwm2m_tlv_encode_string(targetP->rString, tlvP);
        if (tlvP->length==0) ret = COAP_500_INTERNAL_SERVER_ERROR;
        break;
    case TEST_TIME_ID:
        lwm2m_tlv_encode_int(targetP->rTime, tlvP);
        if (tlvP->length==0) ret = COAP_500_INTERNAL_SERVER_ERROR;
        break;
    case TEST_BOOLEAN_ID:
        lwm2m_tlv_encode_bool(targetP->rBoolean, tlvP);
        if (tlvP->length==0) ret = COAP_500_INTERNAL_SERVER_ERROR;
        break;
    case TEST_OPAQUE_ID:
        //TODO lwm2m_tlv_encode_opaque(targetP->rOpaque, tlvP);
        if (tlvP->length==0) ret = COAP_500_INTERNAL_SERVER_ERROR;
        break;
    default:
        ret = COAP_404_NOT_FOUND;
        break;
    }
    return ret;
}

static uint8_t prv_read(uint16_t instanceId,
                               int * numDataP,
                               lwm2m_tlv_t ** dataArrayP,
                               lwm2m_object_t * objectP) {
    //-------------------------------------------------------------------- JH --
    uint8_t         result;
    prv_instance_t  *targetP;
    int             i;
    
    targetP = (prv_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);

    if (NULL == targetP) return COAP_404_NOT_FOUND;

    // is the server asking for the full object ?
    if (*numDataP == 0) {
        uint16_t resList[] = {TEST_TEST_ID,  TEST_INTEGER_ID, 
                              TEST_FLOAT_ID, TEST_STRING_ID, TEST_TIME_ID};
        int nbRes = sizeof(resList)/sizeof(uint16_t);

        *dataArrayP = lwm2m_tlv_new(nbRes);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0 ; i < nbRes ; i++) {
            (*dataArrayP)[i].id = resList[i];
        }
    }
    i = 0;
    do {
        result = prv_set_tlv_value((*dataArrayP) + i, targetP);
        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);

    return result;
}

/* JH
static uint8_t prv_read(uint16_t instanceId,
                        int * numDataP,
                        lwm2m_tlv_t ** dataArrayP,
                        lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;
    int i;
    targetP = (prv_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;
    if (*numDataP == 0) {
        *dataArrayP = lwm2m_tlv_new(1);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = 1;
        (*dataArrayP)->id = 1;
    }
    if (*numDataP != 1) return COAP_404_NOT_FOUND;
    if ((*dataArrayP)->id != 1) return COAP_404_NOT_FOUND;

    (*dataArrayP)->type = LWM2M_TYPE_RESSOURCE;
    lwm2m_tlv_encode_int(targetP->test, *dataArrayP);

    if ((*dataArrayP)->length == 0) return COAP_500_INTERNAL_SERVER_ERROR;
 * 
    return COAP_205_CONTENT;
}
 JH */

// TODO remove object param! (const lwm2m_object_t * objectP, ))
static uint8_t prv_datatype(int resourceId, lwm2m_data_type_t *resDataType) {
    //-------------------------------------------------------------------- JH --
    uint8_t ret = COAP_NO_ERROR;
    switch (resourceId) {
    case TEST_TEST_ID:      *resDataType = LWM2M_DATATYPE_INTEGER;  break;
    
    case TEST_INTEGER_ID:   *resDataType = LWM2M_DATATYPE_INTEGER;  break;
    case TEST_FLOAT_ID:     *resDataType = LWM2M_DATATYPE_FLOAT;    break;
    case TEST_STRING_ID:    *resDataType = LWM2M_DATATYPE_STRING;   break;
    case TEST_TIME_ID:      *resDataType = LWM2M_DATATYPE_TIME;     break;
    case TEST_BOOLEAN_ID:   *resDataType = LWM2M_DATATYPE_BOOLEAN;  break;
    case TEST_OPAQUE_ID:    *resDataType = LWM2M_DATATYPE_OPAQUE;   break;
    default:                ret =  COAP_405_METHOD_NOT_ALLOWED;     break;
    }
    return ret;
}

static uint8_t prv_write(uint16_t instanceId,
                         int numData,
                         lwm2m_tlv_t * dataArray,
                         lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;
    int64_t value;

    targetP = (prv_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

//    if (numData != 1 || dataArray->id != 1) return COAP_404_NOT_FOUND;
//    if (1 != lwm2m_tlv_decode_int(dataArray, &value) || value<0 || value>0xFF){
//        return COAP_400_BAD_REQUEST;
//    }
    int i = 0;
    uint8_t result;
    do {
        switch (dataArray->id) {
        case TEST_TEST_ID:
            if (lwm2m_tlv_decode_int(dataArray, &value)!=1 || value<0 || value>0xFF){
                result = COAP_400_BAD_REQUEST;
            } else {
                targetP->test = (uint8_t)value;
                result = COAP_204_CHANGED;
            }
            break;    
        case TEST_INTEGER_ID:
            if (lwm2m_tlv_decode_int(dataArray, &value)!=1 ){ //TODO limits?
                result = COAP_400_BAD_REQUEST;
            } else {
                targetP->rInteger = (int32_t)value;
                result = COAP_204_CHANGED;
            }
            break;            
        case TEST_FLOAT_ID:     
        case TEST_STRING_ID:
        case TEST_TIME_ID:
        case TEST_BOOLEAN_ID:
        case TEST_OPAQUE_ID:
            //TODOs for each missing!
            result = COAP_501_NOT_IMPLEMENTED;
            break;
        default:
            result = COAP_405_METHOD_NOT_ALLOWED;
            break;
        }
        i++;
    } while (i < numData && result == COAP_204_CHANGED);
    return COAP_204_CHANGED;
}

static uint8_t prv_delete(uint16_t id,
                          lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;

    objectP->instanceList = lwm2m_list_remove(objectP->instanceList, id, (lwm2m_list_t **)&targetP);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    if (targetP->rString!=NULL) lwm2m_free(targetP->rString);
    if (targetP->rOpaque!=NULL) lwm2m_free(targetP->rOpaque);
    free(targetP);

    return COAP_202_DELETED;
}

static uint8_t prv_create(uint16_t instanceId,
                          int numData,
                          lwm2m_tlv_t * dataArray,
                          lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;
    uint8_t result;


    targetP = (prv_instance_t *)malloc(sizeof(prv_instance_t));
    if (NULL == targetP) return COAP_500_INTERNAL_SERVER_ERROR;
    memset(targetP, 0, sizeof(prv_instance_t));

    targetP->shortID = instanceId;
    objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, targetP);

    result = prv_write(instanceId, numData, dataArray, objectP);

    if (result != COAP_204_CHANGED)
    {
        (void)prv_delete(instanceId, objectP);
    }
    else
    {
        result = COAP_201_CREATED;
    }

    return result;
}

static uint8_t prv_exec(uint16_t instanceId,
                        uint16_t resourceId,
                        char * buffer,
                        int length,
                        lwm2m_object_t * objectP)
{

    if (NULL == lwm2m_list_find(objectP->instanceList, instanceId)) return COAP_404_NOT_FOUND;

    switch (resourceId)
    {
    case TEST_TEST_ID:
    case TEST_INTEGER_ID:
    case TEST_FLOAT_ID:
    case TEST_STRING_ID:
    case TEST_TIME_ID:
    case TEST_BOOLEAN_ID:
    case TEST_OPAQUE_ID:
        return COAP_405_METHOD_NOT_ALLOWED;
    case TEST_EXEC_ID:
        fprintf(stdout, "\r\n-----------------\r\n"
                        "Execute on %hu/%d/%d\r\n"
                        " Parameter (%d bytes):\r\n",
                        objectP->objID, instanceId, resourceId, length);
        prv_output_buffer(buffer, length);
        fprintf(stdout, "-----------------\r\n\r\n");
        return COAP_204_CHANGED;
    default:
        return COAP_404_NOT_FOUND;
    }
}

lwm2m_object_t * get_test_object() {
    //-------------------------------------------------------------------- JH --
    lwm2m_object_t * testObj;

    testObj = (lwm2m_object_t *)malloc(sizeof(lwm2m_object_t));

    if (NULL != testObj)
    {
        int i, oi;
        prv_instance_t * targetP;

        memset(testObj, 0, sizeof(lwm2m_object_t));

        testObj->objID = 1024;
        for (i=0 ; i < 3 ; i++) {
            targetP = (prv_instance_t *)malloc(sizeof(prv_instance_t));
            if (NULL == targetP) return NULL;
            memset(targetP, 0, sizeof(prv_instance_t));
            targetP->shortID = 10 + i;
            targetP->test    = 20 + i;
            targetP->rInteger= 1111111 *i;
            targetP->rFloat  = 1010101.*i;
            
            char str[20]; sprintf(str,"String-%d",i);
            targetP->rString = lwm2m_malloc(strlen(str)+1);
            strcpy(targetP->rString, str);
            
            targetP->rTime   = time(NULL);
            targetP->rBoolean= (i%2==1)? true:false;
            
            targetP->lOpaque = 10;
            targetP->rOpaque = lwm2m_malloc(targetP->lOpaque);
            for (oi=0; oi<targetP->lOpaque; oi++) targetP->rOpaque[oi] = oi+i;
            
            testObj->instanceList = LWM2M_LIST_ADD(testObj->instanceList, targetP);
        }
        /*
         * From a single instance object, two more functions are available.
         * - The first one (createFunc) create a new instance and filled it with the provided informations. If an ID is
         *   provided a check is done for verifying his disponibility, or a new one is generated.
         * - The other one (deleteFunc) delete an instance by removing it from the instance list (and freeing the memory
         *   allocated to it)
         */
        testObj->readFunc     = prv_read;
        testObj->writeFunc    = prv_write;
        testObj->createFunc   = prv_create;
        testObj->deleteFunc   = prv_delete;
        testObj->executeFunc  = prv_exec;
        testObj->datatypeFunc = prv_datatype;
    }

    return testObj;
}
