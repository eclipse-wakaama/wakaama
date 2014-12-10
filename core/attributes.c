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
/** \file
  handles the attributes of objects, instances and resources

  \author Carsten Merkle, Tobias Kurzweg
  \author Joerg Hubschneider

  Copyright (c) 2014 Bosch Software Innovations GmbH, Germany. All rights reserved.

*/

#ifdef LWM2M_CLIENT_MODE

#include <string.h>
#include <liblwm2m.h>
#include "internals.h"

/**
 * 
 * @param serverP
 * @param uriP
 * @param tv
 */
void lwm2m_updateTransmissionAttributes(lwm2m_server_t *serverP, lwm2m_uri_t *uriP, 
                                        struct timeval *tv) {
    //-------------------------------------------------------------------- JH --
    lwm2m_attribute_data_t * attributeP;

    attributeP = lwm2m_getAttributes(serverP, uriP);
    if(NULL != attributeP) {
      attributeP->lastTransmission = tv->tv_sec;
      if(attributeP->maxPeriod != NULL ) {
        attributeP->nextTransmission = tv->tv_sec + strtol(attributeP->maxPeriod, NULL, 10);
      } else {
        attributeP->nextTransmission = 0;
      }
    }
}
/**
 * 
 * @param serverP
 * @param uriP
 * @return 
 */
lwm2m_attribute_data_t * lwm2m_getAttributes(lwm2m_server_t *serverP, lwm2m_uri_t *uriP) {
    //-------------------------------------------------------------------- JH --

    lwm2m_attribute_data_t * attributeP;

    if(LWM2M_URI_IS_SET_RESOURCE(uriP)) {
	for(attributeP = serverP->attributeData; attributeP != NULL; attributeP = attributeP->next){
            if(memcmp(&attributeP->uri,uriP,sizeof(lwm2m_uri_t)) == 0) {
		return(attributeP);
	    }
	}
    }
    else {
	for(attributeP = serverP->attributeData; attributeP != NULL; attributeP = attributeP->next){
	    if(attributeP->uri.objectId == uriP->objectId && attributeP->uri.instanceId == uriP->instanceId) {
		return(attributeP);
	    }
      }
    }
    return(attributeP);
}

/**
 * Method to validate and set server sent attribute values!
 * @param contextP LWM2M root context, for to reach other functionality
 * @param uriP     URI to define related resource
 * @param attrType type of attribute to set (lt,gt,st,pmin,pmax)
 * @param value    single attribute value as char*
 * @param objectP  related object
 * @param serverP  erver data
 * @return 
 */
uint8_t lwm2m_setAttributes(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, 
    lwm2m_attribute_type_t attrType, const char* value, lwm2m_object_t* objectP, 
    lwm2m_server_t * serverP) {
    //-------------------------------------------------------------------- JH --

    char valStr[50];    //TODO: size enough ? ... malloc?
    if(LWM2M_URI_IS_SET_RESOURCE(uriP)) {
        // resource attributes are set
        lwm2m_attribute_data_t * attributeP;

        // find uri in attribute list
        attributeP = serverP->attributeData;
        while(attributeP != NULL) {
            if(memcmp(&attributeP->uri,uriP,sizeof(lwm2m_uri_t)) == 0) {
                break;
            } else {
                attributeP = attributeP->next;
            }
        }
        lwm2m_data_type_t resDataType = LWM2M_DATATYPE_UNKNOWN;
        switch(attrType) {
        case ATTRIBUTE_GREATER_THEN:
        case ATTRIBUTE_LESS_THEN:
        case ATTRIBUTE_STEP:
            // TODO: test if dataTypeFunc() is supported for later on compare
            //printf ("DEBUG Test if dataTypeFunc is supported\n"); 
            if (objectP==NULL || objectP->datatypeFunc==NULL) {
                return COAP_501_NOT_IMPLEMENTED;
            }
            if (COAP_NO_ERROR != objectP->datatypeFunc(objectP, uriP->resourceId, &resDataType)) {
                return COAP_501_NOT_IMPLEMENTED;
            }                                                        
            break;
    default:
        break;
    }

    // check/validate attribute value format
    switch(attrType) {
    case ATTRIBUTE_MIN_PERIOD:
    case ATTRIBUTE_MAX_PERIOD: {
        long v;
        if(sscanf(value,"%d", &v)==0) { // format mismatch
            return COAP_400_BAD_REQUEST; 
        } else {    // pot. clean up
            sprintf (valStr, "%ld", v); // TODO check memsize???
        }
    }   break;
    case ATTRIBUTE_GREATER_THEN:
    case ATTRIBUTE_LESS_THEN:
    case ATTRIBUTE_STEP: 
        switch (resDataType) {
        case LWM2M_DATATYPE_STRING:
            strcpy (valStr, value);
            break;
        case LWM2M_DATATYPE_INTEGER: {
            long v;
            if(sscanf(value,"%d", &v)==0) { // format mismatch
                return COAP_400_BAD_REQUEST; 
            } else {    // pot. clean up
              sprintf (valStr, "%ld", v); // TODO check memsize???
            }
        }   break;
        case LWM2M_DATATYPE_FLOAT:    {
            double v;
            if(sscanf(value, "%f", &v)==0) { // format mismatch
                return COAP_400_BAD_REQUEST; 
            } else {    // pot. clean up!
              sprintf (valStr, "%f", v); // TODO check memsize???
            }
        }   break;
        default:
            break;
        }
      break;
    default:
        break;
    }

    if(attributeP == NULL) {    // add new attribute data to list
      attributeP = lwm2m_malloc(sizeof(lwm2m_attribute_data_t));
      if(NULL == attributeP) {
        return COAP_500_INTERNAL_SERVER_ERROR;
      }
      memset(attributeP,0,sizeof(lwm2m_attribute_data_t));
      serverP->attributeData =  (lwm2m_attribute_data_t*) lwm2m_list_add((lwm2m_list_t*)serverP->attributeData, (lwm2m_list_t*)attributeP);
      memcpy(&attributeP->uri,uriP,sizeof(lwm2m_uri_t));
    }

    // change resource attribute
    switch(attrType) {
    case ATTRIBUTE_MIN_PERIOD:
        if (attributeP->minPeriod!=NULL) lwm2m_free(attributeP->minPeriod);
        attributeP->minPeriod = lwm2m_malloc(strlen(valStr)+1);
        strcpy (attributeP->minPeriod, valStr);
        break;
    case ATTRIBUTE_MAX_PERIOD: {
        if (attributeP->maxPeriod!=NULL) lwm2m_free(attributeP->maxPeriod);
        attributeP->maxPeriod = lwm2m_malloc(strlen(valStr)+1);
        strcpy (attributeP->maxPeriod, valStr);
        struct timeval tv;
        lwm2m_gettimeofday(&tv, NULL);
        attributeP->nextTransmission = tv.tv_sec + strtol(value, NULL, 10);
    }   break;
    case ATTRIBUTE_GREATER_THEN:
        if (attributeP->greaterThan!=NULL) lwm2m_free(attributeP->greaterThan);
        attributeP->greaterThan = lwm2m_malloc(strlen(valStr)+1);
        attributeP->resDataType = resDataType;
        strcpy (attributeP->greaterThan, valStr);
        break;
    case ATTRIBUTE_LESS_THEN:
        if (attributeP->lessThan!=NULL) lwm2m_free(attributeP->lessThan);
        attributeP->lessThan    = lwm2m_malloc(strlen(valStr)+1);
        attributeP->resDataType = resDataType;
        strcpy (attributeP->lessThan, valStr);
        break;
    case ATTRIBUTE_STEP: {
        if (attributeP->step!=NULL) lwm2m_free(attributeP->step);
        attributeP->step        = lwm2m_malloc(strlen(valStr)+1);
        attributeP->resDataType = resDataType;
        strcpy (attributeP->step, valStr);
        
        // TODO tbd: take over current value to attributeP->oldValue?
        // read object instance resource value to buffer!
        char *buffer = NULL; 
        int bufLen;
        coap_status_t coapResult = object_read(contextP, uriP, &buffer, &bufLen);
        if (coapResult != CONTENT_2_05) {
            //TODO: Error Handling
        } else {       
            if (attributeP->oldValue!=NULL) lwm2m_free(attributeP->oldValue);
            attributeP->oldValue = lwm2m_malloc(bufLen+1);
            strncpy(attributeP->oldValue, buffer, bufLen); 
            attributeP->oldValue[bufLen] = 0;
        }
        if (buffer!=NULL) lwm2m_free(buffer); //allocated by object_read()!
    }  break;
    case ATTRIBUTE_CANCEL:
        // TODO: move this to a list handling function
        if(attributeP == serverP->attributeData) {
            serverP->attributeData = attributeP->next;
        } else {
            lwm2m_attribute_data_t* attributePrev  = serverP->attributeData;
            lwm2m_attribute_data_t* attributePNext = serverP->attributeData->next;
            while(attributePNext != NULL) {
                if(attributePNext == attributeP) {
                    attributePrev->next = attributePNext->next;
                    break;
                } else {
                    attributePrev  = attributePNext;
                    attributePNext = attributePNext->next;
                }
            }
        }
        if (attributeP->minPeriod!=NULL)   lwm2m_free(attributeP->minPeriod);
        if (attributeP->maxPeriod!=NULL)   lwm2m_free(attributeP->maxPeriod);
        if (attributeP->greaterThan!=NULL) lwm2m_free(attributeP->greaterThan);
        if (attributeP->lessThan!=NULL)    lwm2m_free(attributeP->lessThan);
        if (attributeP->step!=NULL)        lwm2m_free(attributeP->step);
        if (attributeP->oldValue!=NULL)    lwm2m_free(attributeP->oldValue);
        lwm2m_free(attributeP);
        cancel_observe(contextP, serverP->mid, serverP->sessionH);
        break;
    default:
        return COAP_204_CHANGED;
        break;
    }
    return COAP_NO_ERROR;
  }
  return COAP_406_NOT_ACCEPTABLE;
}

/**
 * evaluates all optional attribute contions for to notify the LWM2M server
 * @param attrData  given attribute data.
 * @param resValBuf buffer containing the value of  resource value buffer.
 * @param bufLen    buffer length of valid resource value bytes.
 * @param tv        current timeval structure value.
 * @param notifyResult notify result as return as value by reference.
 * @return potential coap error or COAP_NO_ERROR.
 */
uint8_t lwm2m_evalAttributes(lwm2m_attribute_data_t* attrData, 
                             uint8_t* resValBuf, int bufLen,
                             struct timeval tv, bool *notifyResult) {
    //-------------------------------------------------------------------- JH --
    uint8_t ret = COAP_400_BAD_REQUEST;

    if (resValBuf == NULL || bufLen==0) return ret;

    char *resValStr = (char*)lwm2m_malloc(bufLen+1);
    strncpy(resValStr, resValBuf, bufLen);      // TODO!! multi inst res->TLV!!
    resValStr[bufLen] = 0;                      // terminate char string
        
  if (attrData->maxPeriod != NULL) { // check if max period is elapsed
        int maxPer;
        if (sscanf(attrData->maxPeriod, "%ld", &maxPer)!=0) {
            if((attrData->lastTransmission + maxPer) < tv.tv_sec) {
                // schedule transmission after min period is elapsed
                attrData->nextTransmission = attrData->lastTransmission + maxPer;
                *notifyResult = true;   // notify in any case
                ret = COAP_NO_ERROR;
            }
        }
  } else {
    
    bool cmp = false;
    switch(attrData->resDataType) {
    case LWM2M_DATATYPE_STRING: { // lexigraphic order less & greater !
        // step: does make no sense!
        if (attrData->lessThan!=NULL    && attrData->greaterThan!=NULL){
            cmp = (strcmp(resValStr, attrData->lessThan)    <0 && 
                   strcmp(resValStr, attrData->greaterThan) >0   );
        } else 
        if (attrData->greaterThan!=NULL && attrData->lessThan==NULL){
            cmp = (strcmp(resValStr, attrData->greaterThan) >0 );
        } else 
        if (attrData->lessThan!=NULL    && attrData->greaterThan==NULL){
            cmp = (strcmp(resValStr, attrData->lessThan)    <0 );
        }
        *notifyResult = cmp;
        ret = COAP_NO_ERROR;
    }   break;
    case LWM2M_DATATYPE_INTEGER: {
        long value, gt, lt, st, ov;
        if (sscanf(resValStr, "%ld", &value)==0)             break;
        if (attrData->greaterThan != NULL) {
            if (sscanf(attrData->greaterThan,"%ld", &gt)==0) break;
        }
        if (attrData->lessThan != NULL) {
            if (sscanf(attrData->lessThan,"%ld", &lt)==0)    break;
        }
        if (attrData->step != NULL && attrData->oldValue!=NULL) {
            if (sscanf(attrData->step ,   "%ld", &st)==0)    break;
            if (sscanf(attrData->oldValue,"%ld", &ov)==0)    break;
        } 
        if (attrData->lessThan!=NULL    && attrData->greaterThan!=NULL){
            cmp = (value < lt) && (value > gt);
        } else 
        if (attrData->greaterThan!=NULL && attrData->lessThan==NULL){
            cmp = (value > gt);
        } else 
        if (attrData->lessThan!=NULL    && attrData->greaterThan==NULL){
            cmp = (value < lt);
        } 
        if (cmp==false &&   // or step condition...
            attrData->step!=NULL        && attrData->oldValue!=NULL){
            cmp = (value < ov-st) || (value > ov+st); // value outside ov +/-st
        }
        *notifyResult = cmp;
        ret = COAP_NO_ERROR;
    }   break;
    case LWM2M_DATATYPE_FLOAT: {
        double value, gt, lt, st, ov;   
        if (sscanf(resValStr , "%f", &value)==0)            break;
        if (attrData->greaterThan != NULL) {
          if (sscanf(attrData->greaterThan, "%f", &gt)==0)  break;
        }
        if (attrData->lessThan != NULL) {
          if (sscanf(attrData->lessThan, "%f", &lt)==0)     break;
        }
        if (attrData->step != NULL && attrData->oldValue!=NULL) {
          if (sscanf(attrData->step,    "%f", &st)==0)      break;
          if (sscanf(attrData->oldValue,"%f", &ov)==0)      break;
        }       
        if (attrData->lessThan!=NULL    && attrData->greaterThan!=NULL){
            cmp = (value < lt) && (value > gt);
        } else 
        if (attrData->greaterThan!=NULL && attrData->lessThan==NULL){
            cmp = (value > gt);
        } else 
        if (attrData->lessThan!=NULL    && attrData->greaterThan==NULL){
            cmp = (value < lt);
        }
        if (cmp==false &&   // or step condition ...
            attrData->step!=NULL        && attrData->oldValue!=NULL){
            cmp = (value < ov-st) || (value > ov+st); // value outside ov +/-st
        }
        *notifyResult = cmp;
        ret = COAP_NO_ERROR;
    }   break;
    case LWM2M_DATATYPE_OPAQUE:
    case LWM2M_DATATYPE_BOOLEAN:
    case LWM2M_DATATYPE_TIME:               //TODO!!
    default:
        ret = COAP_501_NOT_IMPLEMENTED;     //.. yet!
        break;
    }
  }
    if (ret != COAP_NO_ERROR) {
        lwm2m_free(resValStr);
    } else {
        if (*notifyResult==true) {
            if (attrData->minPeriod != NULL) { // check if min period is elapsed
                int minPer = strtol(attrData->minPeriod, NULL, 10);
                if((attrData->lastTransmission + minPer) > tv.tv_sec) {
                    // schedule transmission after min period is elapsed
                    attrData->nextTransmission = attrData->lastTransmission + minPer;
                    *notifyResult = false;
                }
            }
        } 
        //update attribute oldValue!
        if (attrData->oldValue!=NULL)  {
            lwm2m_free(attrData->oldValue);
        }
        attrData->oldValue = resValStr;
    }
    return ret;
}

#endif // LWM2M_CLIENT_MODE


