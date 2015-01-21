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

 \author Carsten Merkle, Tobias Kurzweg, Joerg Hubschneider, Achim Kraus
 Copyright (c) 2014 Bosch Software Innovations GmbH, Germany. All rights reserved.

 */

#include "internals.h"

#ifdef LWM2M_CLIENT_MODE

#include <string.h>
#include <liblwm2m.h>

/**
 * 
 * @param serverP
 * @param uriP
 * @param tv
 */
void lwm2m_updateTransmissionAttributes(lwm2m_attribute_data_t * attributeP, struct timeval *tv)
{
	//-------------------------------------------------------------------- JH --
	if (NULL != attributeP)
	{
		attributeP->lastTransmission = tv->tv_sec;
		if (attributeP->maxPeriod != NULL)
		{
			attributeP->nextTransmission = tv->tv_sec + strtol(attributeP->maxPeriod, NULL, 10);
		}
		else
		{
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
lwm2m_attribute_data_t * lwm2m_getAttributes(lwm2m_server_t *serverP, lwm2m_uri_t *uriP)
{
	//-------------------------------------------------------------------- JH --

	lwm2m_attribute_data_t * attributeP;

	if (LWM2M_URI_IS_SET_RESOURCE(uriP))
	{
		for (attributeP = serverP->attributeData; attributeP != NULL; attributeP = attributeP->next)
		{
			if (memcmp(&attributeP->uri, uriP, sizeof(lwm2m_uri_t)) == 0)
			{
				return (attributeP);
			}
		}
	}
	else
	{
		for (attributeP = serverP->attributeData; attributeP != NULL; attributeP = attributeP->next)
		{
			if (attributeP->uri.objectId == uriP->objectId && attributeP->uri.instanceId == uriP->instanceId)
			{
				return (attributeP);
			}
		}
	}
	return (attributeP);
}

static uint8_t prv_setAttribute(char** attribute, const void* value, size_t length)
{
	if (NULL != *attribute)
		lwm2m_free(*attribute);
	*attribute = lwm2m_malloc(length + 1);
	if (NULL == *attribute)
		return COAP_500_INTERNAL_SERVER_ERROR ;
	memcpy(*attribute, value, length);
	(*attribute)[length] = 0;
	return COAP_204_CHANGED ;
}

static uint8_t prv_setAttributeFromString(char** attribute, const char* value)
{
	return prv_setAttribute(attribute, value, strlen(value));
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
uint8_t lwm2m_setAttributes(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, lwm2m_attribute_type_t attrType,
		const char* value, int length, lwm2m_object_t* objectP, lwm2m_server_t * serverP)
{
	//-------------------------------------------------------------------- JH --

	char valStr[50];    //TODO: size enough ? ... malloc?
	long valLong = 0;
	// resource attributes are set
	lwm2m_attribute_data_t * attributeP = NULL;

	if (!LWM2M_URI_IS_SET_RESOURCE(uriP))
		return COAP_501_NOT_IMPLEMENTED ;

	if (sizeof(valStr) <= length)
		return COAP_500_INTERNAL_SERVER_ERROR ;

	// make string \0 termination
	memcpy(valStr, value, length);
	valStr[length] = 0;

	lwm2m_data_type_t resDataType = LWM2M_DATATYPE_UNKNOWN;
	switch (attrType) {
	case ATTRIBUTE_GREATER_THEN:
	case ATTRIBUTE_LESS_THEN:
	case ATTRIBUTE_STEP:
		if (NULL == objectP || NULL == objectP->datatypeFunc)
		{
			return COAP_400_BAD_REQUEST ;
		}
		if (COAP_NO_ERROR != objectP->datatypeFunc(uriP->resourceId, &resDataType))
		{
			return COAP_404_NOT_FOUND ;
		}
		break;
	default:
		break;
	}

	// check/validate attribute value format
	switch (attrType) {
	case ATTRIBUTE_MIN_PERIOD:
	case ATTRIBUTE_MAX_PERIOD:
		if (sscanf(valStr, "%ld", &valLong) != 1)
		{ // format mismatch
			return COAP_400_BAD_REQUEST ;
		}
		else
		{    // pot. clean up
			sprintf(valStr, "%ld", valLong); // TODO check memsize???
		}
		break;
	case ATTRIBUTE_GREATER_THEN:
	case ATTRIBUTE_LESS_THEN:
	case ATTRIBUTE_STEP:
		switch (resDataType) {
		case LWM2M_DATATYPE_INTEGER:
			if (sscanf(valStr, "%ld", &valLong) != 1)
			{ // format mismatch
				return COAP_400_BAD_REQUEST ;
			}
			else
			{    // pot. clean up
				sprintf(valStr, "%ld", valLong); // TODO check memsize???
			}
			break;
		case LWM2M_DATATYPE_FLOAT: {
			double v;
			if (sscanf(valStr, "%lf", &v) != 1)
			{ // format mismatch
				return COAP_400_BAD_REQUEST ;
			}
			else
			{    // pot. clean up!
				sprintf(valStr, "%lf", v); // TODO check memsize???
			}
		}
			break;
		default:
			return COAP_400_BAD_REQUEST ;
			break;
		}
		break;
	default:
		break;

	}

	// find uri in attribute list
	attributeP = lwm2m_getAttributes(serverP, uriP);

	if (NULL == attributeP && attrType != ATTRIBUTE_CANCEL)
	{    // add new attribute data to list
		attributeP = lwm2m_malloc(sizeof(lwm2m_attribute_data_t));
		if (NULL == attributeP)
		{
			return COAP_500_INTERNAL_SERVER_ERROR ;
		}
		memset(attributeP, 0, sizeof(lwm2m_attribute_data_t));
		serverP->attributeData = (lwm2m_attribute_data_t*) lwm2m_list_add((lwm2m_list_t*) serverP->attributeData,
				(lwm2m_list_t*) attributeP);
		memcpy(&attributeP->uri, uriP, sizeof(lwm2m_uri_t));
	}

	// change resource attribute
	switch (attrType) {
	case ATTRIBUTE_MIN_PERIOD:
		prv_setAttributeFromString(&attributeP->minPeriod, valStr);
		break;
	case ATTRIBUTE_MAX_PERIOD: {
		struct timeval tv;
		prv_setAttributeFromString(&attributeP->maxPeriod, valStr);
		lwm2m_gettimeofday(&tv, NULL);
		attributeP->nextTransmission = tv.tv_sec + valLong;
	}
		break;
	case ATTRIBUTE_GREATER_THEN:
		prv_setAttributeFromString(&attributeP->greaterThan, valStr);
		attributeP->resDataType = resDataType;
		break;
	case ATTRIBUTE_LESS_THEN:
		prv_setAttributeFromString(&attributeP->lessThan, valStr);
		attributeP->resDataType = resDataType;
		break;
	case ATTRIBUTE_STEP: {
		prv_setAttributeFromString(&attributeP->step, valStr);
		attributeP->resDataType = resDataType;

		// TODO tbd: take over current value to attributeP->oldValue?
		// read object instance resource value to buffer!
		char *buffer = NULL;
		int bufLen = 0;
		coap_status_t coapResult = object_read(contextP, uriP, &buffer, &bufLen);
		if (coapResult == CONTENT_2_05)
			prv_setAttribute(&attributeP->oldValue, buffer, bufLen);
		if (buffer != NULL)
			lwm2m_free(buffer); //allocated by object_read()!
		if (coapResult != CONTENT_2_05)
			return COAP_500_INTERNAL_SERVER_ERROR ;
	}
		break;
	case ATTRIBUTE_CANCEL:
		// TODO: move this to a list handling function
		if (NULL != attributeP)
		{
			if (attributeP == serverP->attributeData)
			{
				serverP->attributeData = attributeP->next;
			}
			else
			{
				lwm2m_attribute_data_t* attributePrev = serverP->attributeData;
				lwm2m_attribute_data_t* attributePNext = serverP->attributeData->next;
				while (attributePNext != NULL)
				{
					if (attributePNext == attributeP)
					{
						attributePrev->next = attributePNext->next;
						break;
					}
					else
					{
						attributePrev = attributePNext;
						attributePNext = attributePNext->next;
					}
				}
			}
			if (attributeP->minPeriod != NULL)
				lwm2m_free(attributeP->minPeriod);
			if (attributeP->maxPeriod != NULL)
				lwm2m_free(attributeP->maxPeriod);
			if (attributeP->greaterThan != NULL)
				lwm2m_free(attributeP->greaterThan);
			if (attributeP->lessThan != NULL)
				lwm2m_free(attributeP->lessThan);
			if (attributeP->step != NULL)
				lwm2m_free(attributeP->step);
			if (attributeP->oldValue != NULL)
				lwm2m_free(attributeP->oldValue);
			lwm2m_free(attributeP);
		}
		cancel_observe(contextP, -1, serverP->sessionH);
		break;
	default:
		return COAP_400_BAD_REQUEST ;
		break;
	}
	return COAP_204_CHANGED ;
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
uint8_t lwm2m_evalAttributes(lwm2m_attribute_data_t* attrData, const char* resValBuf, int bufLen, struct timeval tv,
bool *notifyResult)
{
	//-------------------------------------------------------------------- JH --
	uint8_t ret = COAP_500_INTERNAL_SERVER_ERROR;

	if (resValBuf == NULL || bufLen == 0)
		return ret;

	char *resValStr = (char*) lwm2m_malloc(bufLen + 1);
	memcpy(resValStr, resValBuf, bufLen);      // TODO!! multi inst res->TLV!!
	resValStr[bufLen] = 0;                      // terminate char string
	*notifyResult = false;

	if (attrData->maxPeriod != NULL)
	{ // check if max period is elapsed
		long maxPer;
		if (sscanf(attrData->maxPeriod, "%ld", &maxPer) == 1)
		{
			if ((attrData->lastTransmission + maxPer) < tv.tv_sec)
			{
				LOG("max period %ld expired!\n", maxPer);
				// schedule transmission after min period is elapsed
				attrData->nextTransmission = attrData->lastTransmission + maxPer;
				ret = COAP_NO_ERROR;
				*notifyResult = true;   // notify in any case
			}
		}
	}

	if (!(*notifyResult))
	{
		bool cmp = false;
		switch (attrData->resDataType) {
		case LWM2M_DATATYPE_INTEGER: {
			long value, gt, lt, st, ov;
			if (sscanf(resValStr, "%ld", &value) != 1)
				break;
			if (attrData->greaterThan != NULL)
			{
				if (sscanf(attrData->greaterThan, "%ld", &gt) != 1)
					break;
			}
			if (attrData->lessThan != NULL)
			{
				if (sscanf(attrData->lessThan, "%ld", &lt) != 1)
					break;
			}
			if (attrData->step != NULL && attrData->oldValue != NULL)
			{
				if (sscanf(attrData->step, "%ld", &st) != 1)
					break;
				if (sscanf(attrData->oldValue, "%ld", &ov) != 1)
					break;
			}
			// TODO: check, if both attributes are evaluate as OR or as AND!
			if (attrData->lessThan != NULL && attrData->greaterThan != NULL)
			{
				cmp = (value < lt) && (value > gt);
			}
			else if (attrData->greaterThan != NULL && attrData->lessThan == NULL)
			{
				cmp = (value > gt);
			}
			else if (attrData->lessThan != NULL && attrData->greaterThan == NULL)
			{
				cmp = (value < lt);
			}
			if (cmp == false &&   // or step condition...
					attrData->step != NULL && attrData->oldValue != NULL)
			{
				cmp = (value < ov - st) || (value > ov + st); // value outside ov +/-st
			}
			*notifyResult = cmp;
			ret = COAP_NO_ERROR;
		}
			break;
		case LWM2M_DATATYPE_FLOAT: {
			double value, gt, lt, st, ov;
			if (sscanf(resValStr, "%lf", &value) != 1)
				break;
			if (attrData->greaterThan != NULL)
			{
				if (sscanf(attrData->greaterThan, "%lf", &gt) != 1)
					break;
			}
			if (attrData->lessThan != NULL)
			{
				if (sscanf(attrData->lessThan, "%lf", &lt) != 1)
					break;
			}
			if (attrData->step != NULL && attrData->oldValue != NULL)
			{
				if (sscanf(attrData->step, "%lf", &st) != 1)
					break;
				if (sscanf(attrData->oldValue, "%lf", &ov) != 1)
					break;
			}
			// TODO: check, if both attributes are evaluate as OR or as AND!
			if (attrData->lessThan != NULL && attrData->greaterThan != NULL)
			{
				cmp = (value < lt) && (value > gt);
			}
			else if (attrData->greaterThan != NULL && attrData->lessThan == NULL)
			{
				cmp = (value > gt);
			}
			else if (attrData->lessThan != NULL && attrData->greaterThan == NULL)
			{
				cmp = (value < lt);
			}
			if (cmp == false &&   // or step condition ...
					attrData->step != NULL && attrData->oldValue != NULL)
			{
				cmp = (value < ov - st) || (value > ov + st); // value outside ov +/-st
			}
			*notifyResult = cmp;
			ret = COAP_NO_ERROR;
		}
			break;
		default:
			break;
		}
	}
	if (ret != COAP_NO_ERROR)
	{
		lwm2m_free(resValStr);
	}
	else
	{
		if (*notifyResult == true)
		{
			if (attrData->minPeriod != NULL)
			{ // check if min period is elapsed
				long minPer;
				if (sscanf(attrData->minPeriod, "%ld", &minPer) == 1)
				{
					if ((attrData->lastTransmission + minPer) > tv.tv_sec)
					{
						// schedule transmission after min period is elapsed
						attrData->nextTransmission = attrData->lastTransmission + minPer;
						*notifyResult = false;
					}
				}
			}
		}
		//update attribute oldValue!
		if (attrData->oldValue != NULL)
		{
			lwm2m_free(attrData->oldValue);
		}
		attrData->oldValue = resValStr;
	}
	return ret;
}

#endif // LWM2M_CLIENT_MODE
