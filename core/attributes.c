/** \file
  handles the attributes of objects, instances and resources

  \author Carsten Merkle

  Copyright (c) 2014 Bosch Software Innovations GmbH, Germany. All rights reserved.

*/

#ifdef LWM2M_CLIENT_MODE

#include <string.h>
#include <liblwm2m.h>
#include "internals.h"

void lwm2m_updateTransmissionAttributes(lwm2m_server_t * serverP, lwm2m_uri_t * uriP, struct timeval *tv) {
    lwm2m_attribute_data_t * attributeP;

    attributeP = lwm2m_getAttributes(serverP, uriP);
    if(NULL != attributeP) {
      attributeP->lastTransmission = tv->tv_sec;
      if(attributeP->maxPeriod > 0 ) {
        attributeP->nextTransmission = tv->tv_sec + attributeP->maxPeriod;
      } else {
        attributeP->nextTransmission = 0;
      }
    }
}

lwm2m_attribute_data_t * lwm2m_getAttributes(lwm2m_server_t * serverP, lwm2m_uri_t * uriP) {

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

int lwm2m_setAttributes(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, lwm2m_attribute_type_t type, uint32_t value, lwm2m_object_t * objectP,  lwm2m_server_t * serverP) {

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

    if(attributeP == NULL) {
      // add new attribute data to list
      attributeP = lwm2m_malloc(sizeof(lwm2m_attribute_data_t));
      if(NULL == attributeP) {
        // TODO: error
        return(0);
      }
      memset(attributeP,0,sizeof(lwm2m_attribute_data_t));
      serverP->attributeData =  (lwm2m_attribute_data_t *) lwm2m_list_add((lwm2m_list_t *) serverP->attributeData, (lwm2m_list_t *) attributeP);
      memcpy(&attributeP->uri,uriP,sizeof(lwm2m_uri_t));
    }

    // change resource attribute
    switch(type) {
      case ATTRIBUTE_MIN_PERIOD:
          attributeP->minPeriod = value;
        break;
      case ATTRIBUTE_MAX_PERIOD: {
          struct timeval tv;
          attributeP->maxPeriod = value;
           lwm2m_gettimeofday(&tv, NULL);
          attributeP->nextTransmission = tv.tv_sec + value;
        break;
      }
      case ATTRIBUTE_GREATER_THEN:
          attributeP->greaterThan = value;
        break;
      case ATTRIBUTE_LESS_THEN:
          attributeP->lessThan = value;
        break;
      case ATTRIBUTE_STEP:
          attributeP->step = value;
        break;
      case ATTRIBUTE_CANCEL:
        // TODO: move this to a list handling function
        if(attributeP == serverP->attributeData) {
          serverP->attributeData = attributeP->next;
        } else {
          lwm2m_attribute_data_t * attributePrev = serverP->attributeData;
          lwm2m_attribute_data_t * attributePNext = serverP->attributeData->next;
          while(attributePNext != NULL) {
            if(attributePNext == attributeP) {
              attributePrev->next = attributePNext->next;
              break;
            } else {
              attributePrev = attributePNext;
              attributePNext = attributePNext->next;
            }
          }
        }
        lwm2m_free(attributeP);
        cancel_observe(contextP, serverP->mid, serverP->sessionH);
        break;
      default:
        return(0);
        break;
    }

    return(1);
  }

  return(0);
}
#endif // LWM2M_CLIENT_MODE


