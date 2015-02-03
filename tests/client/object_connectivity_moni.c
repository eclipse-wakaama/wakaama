/*******************************************************************************
 *
 * Copyright (c) 2014 Bosch Software Innovations GmbH Germany. 
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
 * This connectivity monitoring object is single instance only
 * 
 *  Resources:
 *
 *          Name             | ID | Operations | Instances | Mandatory |  Type   |  Range  | Units |
 *  Network Bearer           |  0 |     R      |  Single   |    Yes    | Integer |         |       |
 *  Available Network Bearer |  1 |     R      |  Multi    |    Yes    | Integer |         |       |
 *  Radio Signal Strength    |  2 |     R      |  Single   |    Yes    | Integer |         | dBm   |
 *  Link Quality             |  3 |     R      |  Single   |    No     | Integer | 0-100   |   %   |
 *  IP Addresses             |  4 |     R      |  Multi    |    Yes    | String  |         |       |
 *  Router IP Addresses      |  5 |     R      |  Multi    |    No     | String  |         |       |
 *  Link Utilization         |  6 |     R      |  Single   |    No     | Integer | 0-100   |   %   |
 *  APN                      |  7 |     R      |  Multi    |    No     | String  |         |       |
 *  Cell ID                  |  8 |     R      |  Single   |    No     | Integer |         |       |
 *  SMNC                     |  9 |     R      |  Single   |    No     | Integer | 0-999   |   %   |
 *  SMCC                     | 10 |     R      |  Single   |    No     | Integer | 0-999   |       |
 *
 */

#include "liblwm2m.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// related to TS RC 20131210-C (ATTENTION changes in -D!)
// Object Id
#define OBJ_DEVICE_ID                   4
// Resource Id's:
#define RES_M_NETWORK_BEARER            0
#define RES_M_AVL_NETWORK_BEARER        1
#define RES_M_RADIO_SIGNAL_STRENGTH     2
#define RES_O_LINK_QUALITY              3
#define RES_M_IP_ADDRESSES              4
#define RES_O_ROUTER_IP_ADDRESS         5
#define RES_O_LINK_UTILIZATION          6
#define RES_O_APN                       7
#define RES_O_CELL_ID                   8
#define RES_O_SMNC                      9
#define RES_O_SMCC                      10

#define VALUE_NETWORK_BEARER_GSM    0   //GSM see 
#define VALUE_AVL_NETWORK_BEARER_1  0   //GSM
#define VALUE_AVL_NETWORK_BEARER_2  21  //WLAN
#define VALUE_AVL_NETWORK_BEARER_3  41  //Ethernet
#define VALUE_AVL_NETWORK_BEARER_4  42  //DSL
#define VALUE_AVL_NETWORK_BEARER_5  43  //PLC
#define VALUE_IP_ADDRESS_1              "192.168.178.101"
#define VALUE_IP_ADDRESS_2              "192.168.178.102"
#define VALUE_ROUTER_IP_ADDRESS_1       "192.168.178.001"
#define VALUE_ROUTER_IP_ADDRESS_2       "192.168.178.002"
#define VALUE_APN_1                     "web.vodafone.de"
#define VALUE_APN_2                     "cda.vodafone.de"
#define VALUE_CELL_ID                   69696969
#define VALUE_RADIO_SIGNAL_STRENGTH     80                  //dBm
#define VALUE_LINK_QUALITY              98     
#define VALUE_LINK_UTILIZATION          666
#define VALUE_SMNC                      33
#define VALUE_SMCC                      44

//#define PRV_TLV_BUFFER_SIZE             128

typedef struct
{
    char ipAddresses[2][16];         // limited to 2!
    char routerIpAddresses[2][16];  // limited to 2!
    long cellId;
    int signalStrength;
    int linkQuality;
    int linkUtilization;
} conn_m_data_t;

static uint8_t prv_set_value(lwm2m_tlv_t * tlvP, conn_m_data_t * connDataP)
{   //------------------------------------------------------------------- JH --
    switch (tlvP->id) {
    case RES_M_NETWORK_BEARER: //s-int
        lwm2m_tlv_encode_int(VALUE_NETWORK_BEARER_GSM, tlvP);
        tlvP->type = LWM2M_TYPE_RESSOURCE;
        if (0 != tlvP->length)
            return COAP_205_CONTENT ;
        else
            return COAP_500_INTERNAL_SERVER_ERROR ;
        break;
    case RES_M_AVL_NETWORK_BEARER: { //m-int
        lwm2m_tlv_t * subTlvP;
        subTlvP = lwm2m_tlv_new(2);
        subTlvP[0].flags = 0;
        subTlvP[0].id = 0;
        subTlvP[0].type = LWM2M_TYPE_RESSOURCE_INSTANCE;
        lwm2m_tlv_encode_int(VALUE_AVL_NETWORK_BEARER_1, subTlvP);
        if (0 == subTlvP[0].length)
        {
            lwm2m_tlv_free(2, subTlvP);
            return COAP_500_INTERNAL_SERVER_ERROR ;
        }
        subTlvP[1].flags = 0;
        subTlvP[1].id = 1;
        subTlvP[1].type = LWM2M_TYPE_RESSOURCE_INSTANCE;
        lwm2m_tlv_encode_int(VALUE_AVL_NETWORK_BEARER_2, subTlvP + 1);
        if (0 == subTlvP[1].length)
        {
            lwm2m_tlv_free(2, subTlvP);
            return COAP_500_INTERNAL_SERVER_ERROR ;
        }
        tlvP->flags = 0;
        tlvP->type = LWM2M_TYPE_MULTIPLE_RESSOURCE;
        tlvP->length = 2;
        tlvP->value = (uint8_t *) subTlvP;
        return COAP_205_CONTENT ;
    }
        break;
    case RES_M_RADIO_SIGNAL_STRENGTH: //s-int
        lwm2m_tlv_encode_int(connDataP->signalStrength, tlvP);
        tlvP->type = LWM2M_TYPE_RESSOURCE;
        if (0 != tlvP->length)
            return COAP_205_CONTENT ;
        else
            return COAP_500_INTERNAL_SERVER_ERROR ;
        break;
    case RES_O_LINK_QUALITY: //s-int
        lwm2m_tlv_encode_int(connDataP->linkQuality, tlvP);
        tlvP->type = LWM2M_TYPE_RESSOURCE;
        if (0 != tlvP->length)
            return COAP_205_CONTENT ;
        else
            return COAP_500_INTERNAL_SERVER_ERROR ;
        break;
    case RES_M_IP_ADDRESSES: { //m-string
        lwm2m_tlv_t * subTlvP;
        subTlvP = lwm2m_tlv_new(2);
        subTlvP[0].flags = LWM2M_TLV_FLAG_STATIC_DATA;
        subTlvP[0].id = 0;
        subTlvP[0].type = LWM2M_TYPE_RESSOURCE_INSTANCE;
        subTlvP[0].value = (uint8_t*) connDataP->ipAddresses[0];
        subTlvP[0].length = strlen(connDataP->ipAddresses[0]);
        if (0 == subTlvP[0].length)
        {
            lwm2m_tlv_free(2, subTlvP);
            return COAP_500_INTERNAL_SERVER_ERROR ;
        }
        subTlvP[1].flags = LWM2M_TLV_FLAG_STATIC_DATA;
        subTlvP[1].id = 1;
        subTlvP[1].type = LWM2M_TYPE_RESSOURCE_INSTANCE;
        subTlvP[1].value = (uint8_t*) connDataP->ipAddresses[1];
        subTlvP[1].length = strlen(connDataP->ipAddresses[1]);
        if (0 == subTlvP[1].length)
        {
            lwm2m_tlv_free(2, subTlvP);
            return COAP_500_INTERNAL_SERVER_ERROR ;
        }
        tlvP->flags = 0;
        tlvP->type = LWM2M_TYPE_MULTIPLE_RESSOURCE;
        tlvP->length = 2;
        tlvP->value = (uint8_t *) subTlvP;
        return COAP_205_CONTENT ;
    }
        break;
    case RES_O_ROUTER_IP_ADDRESS: { //m-string
        lwm2m_tlv_t * subTlvP;
        subTlvP = lwm2m_tlv_new(2);
        subTlvP[0].flags = LWM2M_TLV_FLAG_STATIC_DATA;
        subTlvP[0].id = 0;
        subTlvP[0].type = LWM2M_TYPE_RESSOURCE_INSTANCE;
        subTlvP[0].value = (uint8_t*) connDataP->routerIpAddresses[0];
        subTlvP[0].length = strlen(connDataP->routerIpAddresses[0]);
        if (0 == subTlvP[0].length)
        {
            lwm2m_tlv_free(2, subTlvP);
            return COAP_500_INTERNAL_SERVER_ERROR ;
        }
        subTlvP[1].flags = LWM2M_TLV_FLAG_STATIC_DATA;
        subTlvP[1].id = 1;
        subTlvP[1].type = LWM2M_TYPE_RESSOURCE_INSTANCE;
        subTlvP[1].value = (uint8_t*) connDataP->routerIpAddresses[1];
        subTlvP[1].length = strlen(connDataP->routerIpAddresses[1]);
        if (0 == subTlvP[1].length)
        {
            lwm2m_tlv_free(2, subTlvP);
            return COAP_500_INTERNAL_SERVER_ERROR ;
        }
        tlvP->flags = 0;
        tlvP->type = LWM2M_TYPE_MULTIPLE_RESSOURCE;
        tlvP->length = 2;
        tlvP->value = (uint8_t *) subTlvP;
        return COAP_205_CONTENT ;
    }
        break;
    case RES_O_LINK_UTILIZATION: //s-int
        lwm2m_tlv_encode_int(connDataP->linkUtilization, tlvP);
        tlvP->type = LWM2M_TYPE_RESSOURCE;
        if (0 != tlvP->length)
            return COAP_205_CONTENT ;
        else
            return COAP_500_INTERNAL_SERVER_ERROR ;
        break;
    case RES_O_APN: //m-string
    {
        lwm2m_tlv_t * subTlvP;
        subTlvP = lwm2m_tlv_new(2);
        subTlvP[0].flags = LWM2M_TLV_FLAG_STATIC_DATA;
        subTlvP[0].id = 0;
        subTlvP[0].type = LWM2M_TYPE_RESSOURCE_INSTANCE;
        subTlvP[0].value = (uint8_t*) VALUE_APN_1;
        subTlvP[0].length = strlen(VALUE_APN_1);
        if (0 == subTlvP[0].length)
        {
            lwm2m_tlv_free(2, subTlvP);
            return COAP_500_INTERNAL_SERVER_ERROR ;
        }
        subTlvP[1].flags = LWM2M_TLV_FLAG_STATIC_DATA;
        subTlvP[1].id = 1;
        subTlvP[1].type = LWM2M_TYPE_RESSOURCE_INSTANCE;
        subTlvP[1].value = (uint8_t*) VALUE_APN_2;
        subTlvP[1].length = strlen(VALUE_APN_2);
        if (0 == subTlvP[1].length)
        {
            lwm2m_tlv_free(2, subTlvP);
            return COAP_500_INTERNAL_SERVER_ERROR ;
        }
        tlvP->flags = 0;
        tlvP->type = LWM2M_TYPE_MULTIPLE_RESSOURCE;
        tlvP->length = 2;
        tlvP->value = (uint8_t *) subTlvP;
        return COAP_205_CONTENT ;
    }
        break;
    case RES_O_CELL_ID: //s-int
        lwm2m_tlv_encode_int(connDataP->cellId, tlvP);
        tlvP->type = LWM2M_TYPE_RESSOURCE;
        if (0 != tlvP->length)
            return COAP_205_CONTENT ;
        else
            return COAP_500_INTERNAL_SERVER_ERROR ;
        break;
    case RES_O_SMNC: //s-int
        lwm2m_tlv_encode_int(VALUE_SMNC, tlvP);
        tlvP->type = LWM2M_TYPE_RESSOURCE;
        if (0 != tlvP->length)
            return COAP_205_CONTENT ;
        else
            return COAP_500_INTERNAL_SERVER_ERROR ;
        break;
    case RES_O_SMCC: //s-int
        lwm2m_tlv_encode_int(VALUE_SMCC, tlvP);
        tlvP->type = LWM2M_TYPE_RESSOURCE;
        if (0 != tlvP->length)
            return COAP_205_CONTENT ;
        else
            return COAP_500_INTERNAL_SERVER_ERROR ;
        break;
    default:
        return COAP_404_NOT_FOUND ;
    }
}

static uint8_t prv_read(uint16_t instanceId, int * numDataP, lwm2m_tlv_t ** dataArrayP, lwm2m_object_t * objectP)
{   //------------------------------------------------------------------- JH --
    uint8_t result;
    int i;

    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND ;
    }

    // is the server asking for the full object ?
    if (*numDataP == 0)
    {
        uint16_t resList[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
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
        result = prv_set_value((*dataArrayP) + i, (conn_m_data_t*) (objectP->userData));
        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT );

    return result;
}

static uint8_t prv_datatype(int resourceId, lwm2m_data_type_t *rDataType)
{   //------------------------------------------------------------------ JH --
    uint8_t ret = COAP_NO_ERROR;
    switch (resourceId) {
    case RES_M_NETWORK_BEARER:
    case RES_M_AVL_NETWORK_BEARER:
    case RES_M_RADIO_SIGNAL_STRENGTH:
    case RES_O_LINK_QUALITY:
    case RES_O_LINK_UTILIZATION:
    case RES_O_CELL_ID:
    case RES_O_SMNC:
    case RES_O_SMCC:
        *rDataType = LWM2M_DATATYPE_INTEGER;
        break;
    default:
        ret = COAP_405_METHOD_NOT_ALLOWED;
        break;
    }
    return ret;
}

static uint8_t prv_write(uint16_t instanceId, int numData, lwm2m_tlv_t * dataArray, lwm2m_object_t * objectP)
{
    int i;
    int64_t value;
    uint8_t result;
    conn_m_data_t* data;
    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND ;
    }
    if (0 == (dataArray[0].flags & LWM2M_TLV_FLAG_INTERNAL_WRITE))
    {
        return COAP_405_METHOD_NOT_ALLOWED ;
    }

    i = 0;
    data = (conn_m_data_t*) (objectP->userData);

    do
    {
        switch (dataArray[i].id) {
        case RES_M_RADIO_SIGNAL_STRENGTH:
            if (1 == lwm2m_tlv_decode_int(&dataArray[i], &value))
            {
                data->signalStrength = value;
                result = COAP_204_CHANGED;
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
            break;
        case RES_O_LINK_QUALITY:
            if (1 == lwm2m_tlv_decode_int(&dataArray[i], &value))
            {
                data->linkQuality = value;
                result = COAP_204_CHANGED;
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
            break;
        case RES_M_IP_ADDRESSES:
            if (1 == lwm2m_tlv_decode_string(&dataArray[i], data->ipAddresses[0], sizeof(data->ipAddresses[0])))
            {
                result = COAP_204_CHANGED;
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
            break;
        case RES_O_ROUTER_IP_ADDRESS:
            if (1
                    == lwm2m_tlv_decode_string(&dataArray[i], data->routerIpAddresses[0],
                            sizeof(data->routerIpAddresses[0])))
            {
                result = COAP_204_CHANGED;
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
            break;
        case RES_O_CELL_ID:
            if (1 == lwm2m_tlv_decode_int(&dataArray[i], &value))
            {
                data->cellId = value;
                result = COAP_204_CHANGED;
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
            break;
        default:
            result = COAP_405_METHOD_NOT_ALLOWED;
        }
        i++;
    } while (i < numData && result == COAP_204_CHANGED );

    return result;
}

lwm2m_object_t * get_object_conn_m()
{   //------------------------------------------------------------------- JH --
    /*
     * The get_object_conn_moni function create the object itself and return a pointer to the structure that represent it.
     */
    lwm2m_object_t * connObj;

    connObj = (lwm2m_object_t *) lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != connObj)
    {
        memset(connObj, 0, sizeof(lwm2m_object_t));

        /*
         * It assign his unique ID
         * The 3 is the standard ID for the mandatory object "Object device".
         */
        connObj->objID = OBJ_DEVICE_ID;

        /*
         * And the private function that will access the object.
         * Those function will be called when a read/write/execute query is made by the server. In fact the library don't need to
         * know the resources of the object, only the server does.
         */
        connObj->readFunc = prv_read;
        connObj->writeFunc = prv_write;
        connObj->executeFunc = NULL;
        connObj->datatypeFunc = prv_datatype;
        connObj->userData = lwm2m_malloc(sizeof(conn_m_data_t));

        /*
         * Also some user data can be stored in the object with a private structure containing the needed variables
         */
        if (NULL != connObj->userData)
        {
            conn_m_data_t *myData = (conn_m_data_t*) connObj->userData;
            myData->cellId = VALUE_CELL_ID;
            myData->signalStrength = VALUE_RADIO_SIGNAL_STRENGTH;
            myData->linkQuality = VALUE_LINK_QUALITY;
            myData->linkUtilization = VALUE_LINK_UTILIZATION;
            strcpy(myData->ipAddresses[0], VALUE_IP_ADDRESS_1);
            strcpy(myData->ipAddresses[1], VALUE_IP_ADDRESS_1);
            strcpy(myData->routerIpAddresses[0], VALUE_ROUTER_IP_ADDRESS_1);
            strcpy(myData->routerIpAddresses[1], VALUE_ROUTER_IP_ADDRESS_2);
        }
        else
        {
            lwm2m_free(connObj);
            connObj = NULL;
        }
    }
    return connObj;
}
