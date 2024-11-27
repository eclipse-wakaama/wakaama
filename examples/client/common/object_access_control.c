/*******************************************************************************
 *
 * Copyright (c) 2015 Bosch Software Innovations GmbH Germany.
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
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - please refer to git log
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
 *
 ******************************************************************************/

/*
 * This "Access Control" object is optional and multiple instantiated
 *
 *  Resources:
 *
 *          Name         | ID | Oper. | Inst. | Mand.|  Type   | Range | Units |
 *  ---------------------+----+-------+-------+------+---------+-------+-------+
 *  Object ID            |  0 |   R   | Single|  Yes | Integer |1-65534|       |
 *  Object instance ID   |  1 |   R   | Single|  Yes | Integer |0-65535|       |
 *  ACL                  |  2 |   RW  | Multi.|  No  | Integer | 16bit |       |
 *  Access Control Owner |  3 |   RW  | Single|  Yes | Integer |0-65535|       |
 */

#include "liblwm2m.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Resource Id's:
#define RES_M_OBJECT_ID 0
#define RES_M_OBJECT_INSTANCE_ID 1
#define RES_O_ACL 2
#define RES_M_ACCESS_CONTROL_OWNER 3

typedef struct acc_ctrl_ri_s {  // linked list:
    struct acc_ctrl_ri_s *next; // matches lwm2m_list_t::next
    uint16_t resInstId;         // matches lwm2m_list_t::id, ..serverID
    // resource data:
    uint16_t accCtrlValue;
} acc_ctrl_ri_t;

typedef struct acc_ctrl_oi_s {  // linked list:
    struct acc_ctrl_oi_s *next; // matches lwm2m_list_t::next
    uint16_t objInstId;         // matches lwm2m_list_t::id
    // resources
    uint16_t objectId;
    uint16_t objectInstId;
    uint16_t accCtrlOwner;
    acc_ctrl_ri_t *accCtrlValList;
} acc_ctrl_oi_t;

static uint8_t prv_delete(lwm2m_context_t *contextP, uint16_t id, lwm2m_object_t *objectP);
static uint8_t prv_create(lwm2m_context_t *contextP, uint16_t objInstId, int numData, lwm2m_data_t *tlvArray,
                          lwm2m_object_t *objectP);

static uint8_t prv_set_tlv(lwm2m_data_t *dataP, acc_ctrl_oi_t *accCtrlOiP) {
    switch (dataP->id) {
    case RES_M_OBJECT_ID:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_int(accCtrlOiP->objectId, dataP);
        return COAP_205_CONTENT;
    case RES_M_OBJECT_INSTANCE_ID:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_int(accCtrlOiP->objectInstId, dataP);
        return COAP_205_CONTENT;
    case RES_O_ACL: {
        size_t count;
        acc_ctrl_ri_t *accCtrlRiP;
        for (accCtrlRiP = accCtrlOiP->accCtrlValList, count = 0; accCtrlRiP != NULL;
             accCtrlRiP = accCtrlRiP->next, count++)
            ;

        if (count == 0) // no values!
        {
            return COAP_404_NOT_FOUND;
        } else {
            lwm2m_data_t *subTlvP;
            size_t i;
            if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
                count = dataP->value.asChildren.count;
                subTlvP = dataP->value.asChildren.array;
            } else {
                subTlvP = lwm2m_data_new(count);
                for (accCtrlRiP = accCtrlOiP->accCtrlValList, i = 0; accCtrlRiP != NULL;
                     accCtrlRiP = accCtrlRiP->next, i++) {
                    subTlvP[i].id = accCtrlRiP->resInstId;
                }
                lwm2m_data_encode_instances(subTlvP, count, dataP);
            }

            for (i = 0; i < count; i++) {
                for (accCtrlRiP = accCtrlOiP->accCtrlValList; accCtrlRiP != NULL; accCtrlRiP = accCtrlRiP->next) {
                    if (subTlvP[i].id == accCtrlRiP->resInstId) {
                        lwm2m_data_encode_int(accCtrlRiP->accCtrlValue, subTlvP + i);
                        break;
                    }
                }
                if (accCtrlRiP == NULL)
                    return COAP_404_NOT_FOUND;
            }
            return COAP_205_CONTENT;
        }
    }
    case RES_M_ACCESS_CONTROL_OWNER:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_int(accCtrlOiP->accCtrlOwner, dataP);
        return COAP_205_CONTENT;
    default:
        return COAP_404_NOT_FOUND;
    }
}

static uint8_t prv_read(lwm2m_context_t *contextP, uint16_t instanceId, int *numDataP, lwm2m_data_t **dataArrayP,
                        lwm2m_object_t *objectP) {
    uint8_t result;
    int ri, ni;

    /* unused parameter */
    (void)contextP;

    // multi-instance object: search instance
    acc_ctrl_oi_t *accCtrlOiP = (acc_ctrl_oi_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (accCtrlOiP == NULL) {
        return COAP_404_NOT_FOUND;
    }

    // is the server asking for the full object ?
    if (*numDataP == 0) {
        uint16_t resList[] = {RES_M_OBJECT_ID, RES_M_OBJECT_INSTANCE_ID,
                              RES_O_ACL, // prv_set_tlv will return COAP_404_NOT_FOUND w/o values!
                              RES_M_ACCESS_CONTROL_OWNER};
        int nbRes = sizeof(resList) / sizeof(uint16_t);

        *dataArrayP = lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL)
            return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (ri = 0; ri < nbRes; ri++) {
            (*dataArrayP)[ri].id = resList[ri];
        }
    }

    ni = ri = 0;
    do {
        result = prv_set_tlv((*dataArrayP) + ni, accCtrlOiP);
        if (result == COAP_404_NOT_FOUND) {
            ri++;
            if (*numDataP > 1)
                result = COAP_205_CONTENT;
        } else if (ri > 0) // copy new one by ri skipped ones in front
        {
            (*dataArrayP)[ni - ri] = (*dataArrayP)[ni];
        }
        ni++;
    } while (ni < *numDataP && result == COAP_205_CONTENT);
    *numDataP = ni - ri;

    return result;
}

static bool prv_add_ac_val(acc_ctrl_oi_t *accCtrlOiP, uint16_t acResId, uint16_t acValue) {
    bool ret = false;
    acc_ctrl_ri_t *accCtrlRiP;
    accCtrlRiP = (acc_ctrl_ri_t *)lwm2m_malloc(sizeof(acc_ctrl_ri_t));
    if (accCtrlRiP == NULL) {
        return ret;
    } else {
        memset(accCtrlRiP, 0, sizeof(acc_ctrl_ri_t));
        accCtrlRiP->resInstId = acResId;
        accCtrlRiP->accCtrlValue = acValue;

        accCtrlOiP->accCtrlValList = (acc_ctrl_ri_t *)LWM2M_LIST_ADD(accCtrlOiP->accCtrlValList, accCtrlRiP);
        ret = true;
    }
    return ret;
}

static uint8_t prv_write_resources(lwm2m_context_t *contextP, uint16_t instanceId, int numData, lwm2m_data_t *tlvArray,
                                   lwm2m_object_t *objectP, bool doCreate, lwm2m_write_type_t writeType) {
    int i;
    uint8_t result = COAP_500_INTERNAL_SERVER_ERROR;
    int64_t value;

    acc_ctrl_oi_t *accCtrlOiP = (acc_ctrl_oi_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == accCtrlOiP)
        return COAP_404_NOT_FOUND;

    if (writeType == LWM2M_WRITE_REPLACE_INSTANCE) {
        result = prv_delete(contextP, instanceId, objectP);
        if (result == COAP_202_DELETED) {
            result = prv_create(contextP, instanceId, numData, tlvArray, objectP);
            if (result == COAP_201_CREATED) {
                result = COAP_204_CHANGED;
            }
        }
        return result;
    }

    i = 0;
    do {
        switch (tlvArray[i].id) {
        case RES_M_OBJECT_ID:
            if (doCreate == false) {
                result = COAP_405_METHOD_NOT_ALLOWED;
            } else if (tlvArray[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
                result = COAP_404_NOT_FOUND;
            } else {
                if (1 != lwm2m_data_decode_int(&tlvArray[i], &value)) {
                    result = COAP_400_BAD_REQUEST;
                } else if (value < 1 || value > 65534) {
                    result = COAP_406_NOT_ACCEPTABLE;
                } else {
                    accCtrlOiP->objectId = value;
                    result = COAP_204_CHANGED;
                }
            }
            break;
        case RES_M_OBJECT_INSTANCE_ID:
            if (doCreate == false) {
                result = COAP_405_METHOD_NOT_ALLOWED;
            } else if (tlvArray[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
                result = COAP_404_NOT_FOUND;
            } else {
                if (1 != lwm2m_data_decode_int(&tlvArray[i], &value)) {
                    result = COAP_400_BAD_REQUEST;
                } else if (value < 0 || value > 65535) {
                    result = COAP_406_NOT_ACCEPTABLE;
                } else {
                    accCtrlOiP->objectInstId = value;
                    result = COAP_204_CHANGED;
                }
            }
            break;
        case RES_O_ACL: {
            if (tlvArray[i].type != LWM2M_TYPE_MULTIPLE_RESOURCE) {
                result = COAP_400_BAD_REQUEST;
            } else {
                // 1st: save accValueList!
                acc_ctrl_ri_t *acValListSave = accCtrlOiP->accCtrlValList;
                accCtrlOiP->accCtrlValList = NULL;

                size_t ri;
                lwm2m_data_t *subTlvArray = tlvArray[i].value.asChildren.array;

                if (writeType == LWM2M_WRITE_PARTIAL_UPDATE) {
                    result = COAP_204_CHANGED;

                    if (tlvArray[i].value.asChildren.count == 0) {
                        acValListSave = NULL;
                        accCtrlOiP->accCtrlValList = acValListSave;
                    } else if (subTlvArray == NULL) {
                        result = COAP_400_BAD_REQUEST;
                    } else {
                        // Duplicate original list
                        acc_ctrl_ri_t *acValListElement;
                        for (acValListElement = acValListSave; acValListElement != NULL;
                             acValListElement = acValListElement->next) {
                            if (!prv_add_ac_val(accCtrlOiP, acValListElement->resInstId,
                                                acValListElement->accCtrlValue)) {
                                result = COAP_500_INTERNAL_SERVER_ERROR;
                                break;
                            }
                        }

                        if (result == COAP_204_CHANGED) {
                            // Add or replace based on new values
                            for (ri = 0; ri < tlvArray[i].value.asChildren.count; ri++) {
                                if (1 != lwm2m_data_decode_int(&subTlvArray[ri], &value)) {
                                    result = COAP_400_BAD_REQUEST;
                                    break;
                                } else if (value < 0 || value > 0xFFFF) {
                                    result = COAP_406_NOT_ACCEPTABLE;
                                    break;
                                } else {
                                    for (acValListElement = accCtrlOiP->accCtrlValList; acValListElement != NULL;
                                         acValListElement = acValListElement->next) {
                                        if (subTlvArray[ri].id == acValListElement->resInstId) {
                                            acValListElement->accCtrlValue = (uint16_t)value;
                                            break;
                                        }
                                    }

                                    if (acValListElement == NULL &&
                                        !prv_add_ac_val(accCtrlOiP, subTlvArray[ri].id, (uint16_t)value)) {
                                        result = COAP_500_INTERNAL_SERVER_ERROR;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if (tlvArray[i].value.asChildren.count == 0) {
                        result = COAP_204_CHANGED;
                    } else if (subTlvArray == NULL) {
                        result = COAP_400_BAD_REQUEST;
                    } else {
                        for (ri = 0; ri < tlvArray[i].value.asChildren.count; ri++) {
                            if (1 != lwm2m_data_decode_int(&subTlvArray[ri], &value)) {
                                result = COAP_400_BAD_REQUEST;
                                break;
                            } else if (value < 0 || value > 0xFFFF) {
                                result = COAP_406_NOT_ACCEPTABLE;
                                break;
                            } else if (!prv_add_ac_val(accCtrlOiP, subTlvArray[ri].id, (uint16_t)value)) {
                                result = COAP_500_INTERNAL_SERVER_ERROR;
                                break;
                            } else {
                                result = COAP_204_CHANGED;
                            }
                        }
                    }
                }

                if (result != COAP_204_CHANGED) {
                    // free pot. partial created new ones
                    LWM2M_LIST_FREE(accCtrlOiP->accCtrlValList);
                    // restore old values:
                    accCtrlOiP->accCtrlValList = acValListSave;
                } else {
                    // final free saved value list
                    LWM2M_LIST_FREE(acValListSave);
                }
            }
        } break;
        case RES_M_ACCESS_CONTROL_OWNER: {
            if (tlvArray[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
                result = COAP_404_NOT_FOUND;
            } else {
                if (1 == lwm2m_data_decode_int(tlvArray + i, &value)) {
                    if (value >= 0 && value <= 0xFFFF) {
                        accCtrlOiP->accCtrlOwner = value;
                        result = COAP_204_CHANGED;
                    } else {
                        result = COAP_406_NOT_ACCEPTABLE;
                    }
                } else {
                    result = COAP_400_BAD_REQUEST;
                }
            }
        } break;
        default:
            return COAP_404_NOT_FOUND;
        }
        i++;
    } while (i < numData && result == COAP_204_CHANGED);

    return result;
}

static uint8_t prv_write(lwm2m_context_t *contextP, uint16_t instanceId, int numData, lwm2m_data_t *tlvArray,
                         lwm2m_object_t *objectP, lwm2m_write_type_t writeType) {
    return prv_write_resources(contextP, instanceId, numData, tlvArray, objectP, false, writeType);
}

static uint8_t prv_delete(lwm2m_context_t *contextP, uint16_t id, lwm2m_object_t *objectP) {
    acc_ctrl_oi_t *targetP;

    /* unused parameter */
    (void)contextP;

    objectP->instanceList = lwm2m_list_remove(objectP->instanceList, id, (lwm2m_list_t **)&targetP);
    if (NULL == targetP)
        return COAP_404_NOT_FOUND;

    LWM2M_LIST_FREE(targetP->accCtrlValList);
    lwm2m_free(targetP);

    return COAP_202_DELETED;
}

static uint8_t prv_create(lwm2m_context_t *contextP, uint16_t objInstId, int numData, lwm2m_data_t *tlvArray,
                          lwm2m_object_t *objectP) {
    acc_ctrl_oi_t *targetP;
    uint8_t result;

    targetP = (acc_ctrl_oi_t *)lwm2m_malloc(sizeof(acc_ctrl_oi_t));
    if (NULL == targetP)
        return COAP_500_INTERNAL_SERVER_ERROR;
    memset(targetP, 0, sizeof(acc_ctrl_oi_t));

    targetP->objInstId = objInstId;
    objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, targetP);

    result = prv_write_resources(contextP, objInstId, numData, tlvArray, objectP, true, LWM2M_WRITE_REPLACE_RESOURCES);

    if (result != COAP_204_CHANGED) {
        prv_delete(contextP, objInstId, objectP);
    } else {
        result = COAP_201_CREATED;
    }
    return result;
}

/*
 * Create an empty multiple instance LwM2M Object: Access Control
 */
lwm2m_object_t *acc_ctrl_create_object(void) {
    /*
     * The acc_ctrl_create_object() function creates an empty object
     * and returns a pointer to the structure that represents it.
     */
    lwm2m_object_t *accCtrlObj = NULL;

    accCtrlObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != accCtrlObj) {
        memset(accCtrlObj, 0, sizeof(lwm2m_object_t));
        /*
         * It assign his unique object ID
         * The 2 is the standard ID for the optional object "Access Control".
         */
        accCtrlObj->objID = 2;
        // Init callbacks, empty instanceList!
        accCtrlObj->readFunc = prv_read;
        accCtrlObj->writeFunc = prv_write;
        accCtrlObj->createFunc = prv_create;
        accCtrlObj->deleteFunc = prv_delete;
    }
    return accCtrlObj;
}

void acl_ctrl_free_object(lwm2m_object_t *objectP) {
    acc_ctrl_oi_t *accCtrlOiT;
    acc_ctrl_oi_t *accCtrlOiP = (acc_ctrl_oi_t *)objectP->instanceList;
    while (accCtrlOiP != NULL) {
        // first free acl (multiple resource!):
        LWM2M_LIST_FREE(accCtrlOiP->accCtrlValList);
        accCtrlOiT = accCtrlOiP;
        accCtrlOiP = accCtrlOiP->next;
        lwm2m_free(accCtrlOiT);
    }
    lwm2m_free(objectP);
}

bool acc_ctrl_obj_add_inst(lwm2m_object_t *accCtrlObjP, uint16_t instId, uint16_t acObjectId, uint16_t acObjInstId,
                           uint16_t acOwner) {
    bool ret = false;

    if (NULL == accCtrlObjP) {
        return ret;
    } else {
        // create an access control object instance
        acc_ctrl_oi_t *accCtrlOiP;
        accCtrlOiP = (acc_ctrl_oi_t *)lwm2m_malloc(sizeof(acc_ctrl_oi_t));
        if (NULL == accCtrlOiP) {
            return ret;
        } else {
            memset(accCtrlOiP, 0, sizeof(acc_ctrl_oi_t));
            // list: key
            accCtrlOiP->objInstId = instId;
            // object instance data:
            accCtrlOiP->objectId = acObjectId;
            accCtrlOiP->objectInstId = acObjInstId;
            accCtrlOiP->accCtrlOwner = acOwner;

            accCtrlObjP->instanceList = LWM2M_LIST_ADD(accCtrlObjP->instanceList, accCtrlOiP);
            ret = true;
        }
    }
    return ret;
}

bool acc_ctrl_oi_add_ac_val(lwm2m_object_t *accCtrlObjP, uint16_t instId, uint16_t acResId, uint16_t acValue) {
    bool ret = false;

    acc_ctrl_oi_t *accCtrlOiP = (acc_ctrl_oi_t *)lwm2m_list_find(accCtrlObjP->instanceList, instId);
    if (NULL == accCtrlOiP)
        return ret;

    return prv_add_ac_val(accCtrlOiP, acResId, acValue);
}
