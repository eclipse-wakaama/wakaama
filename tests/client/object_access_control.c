/*******************************************************************************
 *
 * Copyright (c) 2015 Bosch Software Innovations GmbH Germany.
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
 * This "Access Control" object is optional and multiple instantiated
 * 
 *  Resources:
 *
 *          Name         | ID | Oper. | Inst. | Mand.|  Type   | Range | Units |
 *  Object ID            |  0 |   R   | Single|  Yes | Integer |1-65534|       |
 *  Object instance ID   |  1 |   R   | Single|  Yes | Integer |0-65535|       |
 *  ACL                  |  2 |   RW  | Multi.|  No  | Integer | 16bit |       |
 *  Access Control Owner |  3 |   RW  | Single|  Yes | Integer |0-65535|       |
 */

#include "liblwm2m.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Resource Id's:
#define RES_M_OBJECT_ID             0
#define RES_M_OBJECT_INSTANCE_ID    1
#define RES_O_ACL                   2
#define RES_M_ACCESS_CONTROL_OWNER  3

typedef struct acc_ctrl_ri_s
{   // linked list:
    struct acc_ctrl_ri_s*   next;       // matches lwm2m_list_t::next
    uint16_t                resInstId;  // matches lwm2m_list_t::id, ..serverID
    // resource data:
    uint16_t                accCtrlValue;
} acc_ctrl_ri_t;

typedef struct acc_ctrl_oi_s
{   //linked list:
    struct acc_ctrl_oi_s*   next;       // matches lwm2m_list_t::next
    uint16_t                objInstId;  // matches lwm2m_list_t::id
    // resources
    uint16_t                objectId;
    uint16_t                objectInstId;
    uint16_t                accCtrlOwner;
    acc_ctrl_ri_t*          accCtrlValList;
} acc_ctrl_oi_t;


static uint8_t prv_set_tlv(lwm2m_tlv_t* tlvP, acc_ctrl_oi_t* accCtrlOiP)
{
    switch (tlvP->id) {
    case RES_M_OBJECT_ID:
        lwm2m_tlv_encode_int(accCtrlOiP->objectId, tlvP);
        tlvP->type = LWM2M_TYPE_RESSOURCE;
        return (0 != tlvP->length) ? COAP_205_CONTENT
                                   : COAP_500_INTERNAL_SERVER_ERROR;
        break;
    case RES_M_OBJECT_INSTANCE_ID:
        lwm2m_tlv_encode_int(accCtrlOiP->objectInstId, tlvP);
        tlvP->type = LWM2M_TYPE_RESSOURCE;
        return (0 != tlvP->length) ? COAP_205_CONTENT
                                   : COAP_500_INTERNAL_SERVER_ERROR;
        break;
    case RES_O_ACL:
    {
        int ri;
        acc_ctrl_ri_t* accCtrlRiP;
        for (accCtrlRiP =accCtrlOiP->accCtrlValList, ri=0;
             accCtrlRiP!=NULL;
             accCtrlRiP = accCtrlRiP->next, ri++);

        lwm2m_tlv_t* subTlvP = NULL;
        if (ri > 0)
        {
            subTlvP = lwm2m_tlv_new(ri);
            for (accCtrlRiP = accCtrlOiP->accCtrlValList, ri = 0;
                 accCtrlRiP!= NULL;
                 accCtrlRiP = accCtrlRiP->next, ri++)
            {
                lwm2m_tlv_encode_int(accCtrlRiP->accCtrlValue, &subTlvP[ri]);
                subTlvP[ri].type = LWM2M_TYPE_RESSOURCE_INSTANCE;
                if (subTlvP[ri].length == 0)
                {
                    lwm2m_free(subTlvP);
                    return COAP_500_INTERNAL_SERVER_ERROR ;
                }
            }
        }
        tlvP->flags  = 0;
        tlvP->type   = LWM2M_TYPE_MULTIPLE_RESSOURCE;
        tlvP->length = ri;
        tlvP->value  = (uint8_t*)subTlvP;
        return COAP_205_CONTENT;
    }   break;
    case RES_M_ACCESS_CONTROL_OWNER:
        lwm2m_tlv_encode_int(accCtrlOiP->accCtrlOwner, tlvP);
        tlvP->type = LWM2M_TYPE_RESSOURCE;
        return (0 != tlvP->length) ? COAP_205_CONTENT
                                   : COAP_500_INTERNAL_SERVER_ERROR;
        break;
    default:
        return COAP_404_NOT_FOUND ;
    }
}

static uint8_t prv_read(uint16_t instanceId, int * numDataP,
                        lwm2m_tlv_t** dataArrayP, lwm2m_object_t * objectP)
{
    uint8_t result;
    int i;

    // multi-instance object: search instance
    acc_ctrl_oi_t* accCtrlOiP =
        (acc_ctrl_oi_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (accCtrlOiP == NULL)
    {
        return COAP_404_NOT_FOUND ;
    }

    // is the server asking for the full object ?
    if (*numDataP == 0)
    {
        uint16_t resList[] = {
                RES_M_OBJECT_ID,
                RES_M_OBJECT_INSTANCE_ID,
                RES_O_ACL,
                RES_M_ACCESS_CONTROL_OWNER
        };
        int nbRes = sizeof(resList) / sizeof(uint16_t);

        *dataArrayP = lwm2m_tlv_new(nbRes);
        if (*dataArrayP == NULL)
            return COAP_500_INTERNAL_SERVER_ERROR ;
        *numDataP = nbRes;
        for (i = 0; i < nbRes; i++)
        {
            (*dataArrayP)[i].id = resList[i];
        }
    }

    i = 0;
    do
    {
        result = prv_set_tlv((*dataArrayP) + i, accCtrlOiP);
        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT );

    return result;
}

static uint8_t prv_write(uint16_t instanceId, int numData,
                         lwm2m_tlv_t* dataArray, lwm2m_object_t* objectP)
{
    int i;
    uint8_t result;

    acc_ctrl_oi_t* accCtrlDataP = (acc_ctrl_oi_t *)
            lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == accCtrlDataP)
        return COAP_404_NOT_FOUND ;

    i = 0;
    do
    {
        switch (dataArray[i].id) {
        case RES_M_OBJECT_ID:
        case RES_M_OBJECT_INSTANCE_ID:
            result = COAP_405_METHOD_NOT_ALLOWED;
            break;
        case RES_O_ACL: //ToDo!
        {
            int64_t value;
            if (1 == lwm2m_tlv_decode_int(dataArray + i, &value))
            {
                if (value >= 0 && value <= 0xFFFF)
                {
                    //ToDo accCtrlDataP->accCtrlValList = value;
                    result = COAP_204_CHANGED;
                }
                else
                {
                    result = COAP_406_NOT_ACCEPTABLE;
                }
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
        }
            break;
        case RES_M_ACCESS_CONTROL_OWNER: {
            int64_t value;
            if (1 == lwm2m_tlv_decode_int(dataArray + i, &value))
            {
                if (value >= 0 && value <= 0xFFFF)
                {
                    accCtrlDataP->accCtrlOwner = value;
                    result = COAP_204_CHANGED;
                }
                else
                {
                    result = COAP_406_NOT_ACCEPTABLE;
                }
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
        }
            break;
        default:
            return COAP_404_NOT_FOUND ;
        }
        i++;
    } while (i < numData && result == COAP_204_CHANGED );

    return result;
}

static void prv_free_oi_mem(acc_ctrl_oi_t* accCtrlOiP)
{
    while(accCtrlOiP->accCtrlValList !=NULL)
    {
        acc_ctrl_ri_t*
        accCtrlRiP = accCtrlOiP->accCtrlValList;
        accCtrlOiP->accCtrlValList = accCtrlRiP->next;
        lwm2m_free(accCtrlRiP);
    }
}

static void prv_close(lwm2m_object_t * objectP)
{
    acc_ctrl_oi_t* accCtrlOiP = (acc_ctrl_oi_t*)objectP->instanceList;
    while (accCtrlOiP != NULL)
    {
        objectP->instanceList = (lwm2m_list_t*)accCtrlOiP->next;
        // first free multiple resources:
        prv_free_oi_mem(accCtrlOiP);
        lwm2m_free(accCtrlOiP);
    }
}

static uint8_t prv_delete(uint16_t id, lwm2m_object_t * objectP)
{
    acc_ctrl_oi_t* targetP;

    objectP->instanceList = lwm2m_list_remove(objectP->instanceList, id,
                                              (lwm2m_list_t**)&targetP);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    prv_free_oi_mem(targetP);
    lwm2m_free(targetP);

    return COAP_202_DELETED;
}

static uint8_t prv_create(uint16_t objInstId, int numData,
                       lwm2m_tlv_t * dataArray, lwm2m_object_t * objectP)
{
    acc_ctrl_oi_t * targetP;
    uint8_t result;

    targetP = (acc_ctrl_oi_t *)lwm2m_malloc(sizeof(acc_ctrl_oi_t));
    if (NULL == targetP) return COAP_500_INTERNAL_SERVER_ERROR;
    memset(targetP, 0, sizeof(acc_ctrl_oi_t));

    targetP->objInstId = objInstId;
    objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, targetP);

    result = prv_write(objInstId, numData, dataArray, objectP);

    if (result != COAP_204_CHANGED)
    {
        prv_delete(objInstId, objectP);
    }
    else
    {
        result = COAP_201_CREATED;
    }
    return result;
}

/*
 * Create an empty multiple instance LWM2M Object: Access Control
 */
lwm2m_object_t * acc_ctrl_create_object()
{
    /*
     * The access_control_create_object() function creates an empty object
     * and returns a pointer to the structure that represent it.
     */
    lwm2m_object_t* accCtrlObj = NULL;

    accCtrlObj = (lwm2m_object_t *) lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != accCtrlObj)
    {
        memset(accCtrlObj, 0, sizeof(lwm2m_object_t));
        /*
         * It assign his unique ID
         * The 2 is the standard ID for the optional object "Access Control".
         */
        accCtrlObj->objID = 2;
        // Create an access control object with empty instanceList
        accCtrlObj->readFunc    = prv_read;
        accCtrlObj->writeFunc   = prv_write;
        accCtrlObj->closeFunc   = prv_close;
        accCtrlObj->createFunc  = prv_create;
        accCtrlObj->deleteFunc  = prv_delete;
    }
    return accCtrlObj;
}

bool  acc_ctrl_obj_add_inst (lwm2m_object_t* accCtrlObjP, uint16_t instId,
                uint16_t acObjectId, uint16_t acObjInstId, uint16_t acOwner)
{
    bool ret = false;

    if (NULL == accCtrlObjP)
    {
        return ret;
    }
    else
    {
        // create an access control object instance
        acc_ctrl_oi_t* accCtrlOiP;
        accCtrlOiP = (acc_ctrl_oi_t *)lwm2m_malloc(sizeof(acc_ctrl_oi_t));
        if (NULL == accCtrlOiP)
        {
            return ret;
        }
        else
        {
            memset(accCtrlOiP, 0, sizeof(acc_ctrl_oi_t));
            // list: key
            accCtrlOiP->objInstId    = instId;
            // object instance data:
            accCtrlOiP->objectId     = acObjectId;
            accCtrlOiP->objectInstId = acObjInstId;
            accCtrlOiP->accCtrlOwner = acOwner;

            accCtrlObjP->instanceList =
                    LWM2M_LIST_ADD(accCtrlObjP->instanceList, accCtrlOiP);
            ret = true;
        }
    }
    return ret;
}

bool acc_ctrl_oi_add_ac_val (lwm2m_object_t* accCtrlObjP, uint16_t instId,
                             uint16_t acResId, uint16_t acValue)
{
    bool ret = false;

    acc_ctrl_oi_t* accCtrlOiP = (acc_ctrl_oi_t *)
            lwm2m_list_find(accCtrlObjP->instanceList, instId);
    if (NULL == accCtrlOiP)
        return ret;

    acc_ctrl_ri_t *accCtrlRiP;
    accCtrlRiP = (acc_ctrl_ri_t *)lwm2m_malloc(sizeof(acc_ctrl_ri_t));
    if (accCtrlRiP==NULL)
    {
        return ret;
    }
    else
    {
        memset(accCtrlRiP, 0, sizeof(acc_ctrl_ri_t));
        accCtrlRiP->resInstId      = acResId;
        accCtrlRiP->accCtrlValue   = acValue;

        accCtrlOiP->accCtrlValList = (acc_ctrl_ri_t*)
                LWM2M_LIST_ADD(accCtrlOiP->accCtrlValList, accCtrlRiP);
        ret = true;
    }
    return ret;
}
