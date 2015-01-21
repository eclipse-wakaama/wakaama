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
 *    Toby Jaffey - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
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

#include "internals.h"
#include <stdio.h>

#ifdef LWM2M_CLIENT_MODE

static lwm2m_observed_t * prv_findObserved(lwm2m_context_t * contextP,
                                           lwm2m_uri_t * uriP)
{
	lwm2m_observed_t * targetP;

	targetP = contextP->observedList;
	while (targetP != NULL
        && (targetP->uri.objectId != uriP->objectId
         || targetP->uri.flag != uriP->flag
					|| (LWM2M_URI_IS_SET_INSTANCE(uriP) && targetP->uri.instanceId != uriP->instanceId)
					|| (LWM2M_URI_IS_SET_RESOURCE(uriP) && targetP->uri.resourceId != uriP->resourceId)))
	{
		targetP = targetP->next;
	}

	return targetP;
}

static obs_list_t * prv_getObservedList(lwm2m_context_t * contextP,
                                        lwm2m_uri_t * uriP)
{
	obs_list_t * resultP = NULL;
	lwm2m_observed_t * targetP = contextP->observedList;

	while (targetP != NULL)
	{
		if (targetP->uri.objectId == uriP->objectId)
		{
            if (!LWM2M_URI_IS_SET_INSTANCE(uriP)
             || (targetP->uri.flag & LWM2M_URI_FLAG_INSTANCE_ID) == 0
					|| uriP->instanceId == targetP->uri.instanceId)
			{
                if (!LWM2M_URI_IS_SET_RESOURCE(uriP)
                 || (targetP->uri.flag & LWM2M_URI_FLAG_RESOURCE_ID) == 0
						|| uriP->resourceId == targetP->uri.resourceId)
				{
					obs_list_t * newP;

					newP = (obs_list_t *) lwm2m_malloc(sizeof(obs_list_t));
					if (newP != NULL)
					{
						newP->item = targetP;
						newP->next = resultP;
						resultP = newP;
					}
				}
			}
		}
		targetP = targetP->next;
	}

	return resultP;
}

static void prv_unlinkObserved(lwm2m_context_t * contextP,
                               lwm2m_observed_t * observedP)
{
	if (contextP->observedList == observedP)
	{
		contextP->observedList = contextP->observedList->next;
    }
    else
	{
		lwm2m_observed_t * parentP;

		parentP = contextP->observedList;
        while (parentP->next != NULL
            && parentP->next != observedP)
		{
			parentP = parentP->next;
		}
		if (parentP->next != NULL)
		{
			parentP->next = parentP->next->next;
		}
	}
}

static lwm2m_server_t * prv_findServer(lwm2m_context_t * contextP,
                                       void * fromSessionH)
{
	lwm2m_server_t * targetP;

	targetP = contextP->serverList;
    while (targetP != NULL
        && targetP->sessionH != fromSessionH)
	{
		targetP = targetP->next;
	}

	return targetP;
}

static lwm2m_watcher_t * prv_findWatcher(lwm2m_observed_t * observedP,
                                         lwm2m_server_t * serverP)
{
	lwm2m_watcher_t * targetP;

	targetP = observedP->watcherList;
    while (targetP != NULL
        && targetP->server != serverP)
	{
		targetP = targetP->next;
	}

	return targetP;
}

coap_status_t handle_observe_request(lwm2m_context_t * contextP,
                                     lwm2m_uri_t * uriP,
                                     void * fromSessionH,
                                     coap_packet_t * message,
                                     coap_packet_t * response)
{
	lwm2m_observed_t * observedP;
	lwm2m_watcher_t * watcherP;
	lwm2m_server_t * serverP;
	bool checkForBlockwiseChange = !IS_OPTION(message, COAP_OPTION_OBSERVE);

	if (checkForBlockwiseChange)
	{
		LOG("check_observe_for_blockwise_change()\r\n");
	} else
	{
		LOG("handle_observe_request()\r\n");
	}
    if (!LWM2M_URI_IS_SET_INSTANCE(uriP) && LWM2M_URI_IS_SET_RESOURCE(uriP)) return COAP_400_BAD_REQUEST;
    if (message->token_len == 0) return COAP_400_BAD_REQUEST;

	serverP = prv_findServer(contextP, fromSessionH);
	if (serverP == NULL) return COAP_401_UNAUTHORIZED ;
	if (serverP->status != STATE_REGISTERED && serverP->status != STATE_REG_UPDATE_PENDING)	return COAP_401_UNAUTHORIZED ;

	observedP = prv_findObserved(contextP, uriP);
	if (observedP == NULL)
	{
		if (checkForBlockwiseChange)
			return COAP_205_CONTENT ;
		observedP = (lwm2m_observed_t *) lwm2m_malloc(sizeof(lwm2m_observed_t));
        if (observedP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
		memset(observedP, 0, sizeof(lwm2m_observed_t));
		memcpy(&(observedP->uri), uriP, sizeof(lwm2m_uri_t));
		observedP->next = contextP->observedList;
		contextP->observedList = observedP;
	}

	watcherP = prv_findWatcher(observedP, serverP);
	if (watcherP == NULL)
	{
		if (checkForBlockwiseChange)
			return COAP_205_CONTENT ;
		watcherP = (lwm2m_watcher_t *) lwm2m_malloc(sizeof(lwm2m_watcher_t));
		if (watcherP == NULL) return COAP_500_INTERNAL_SERVER_ERROR ;
		memset(watcherP, 0, sizeof(lwm2m_watcher_t));
		watcherP->server = serverP;
		watcherP->next = observedP->watcherList;
		observedP->watcherList = watcherP;
	}

	if (checkForBlockwiseChange)
	{
		if (watcherP->changed && 0 < watcherP->tokenLen && watcherP->tokenLen == message->token_len)
		{
			if (memcmp(watcherP->token, message->token, watcherP->tokenLen) == 0)
			{
				// blockwise transfer of notify for changed value
				return COAP_204_CHANGED ;
			}
		}
	} else
	{
		watcherP->changed = false;
		watcherP->tokenLen = message->token_len;
		memcpy(watcherP->token, message->token, message->token_len);
		watcherP->blockSize = REST_MAX_CHUNK_SIZE;
		coap_get_header_block2(message, NULL, NULL, &watcherP->blockSize, NULL);
		watcherP->blockSize = MIN(watcherP->blockSize, REST_MAX_CHUNK_SIZE);
		coap_set_header_observe(response, watcherP->counter++);
	}

	return COAP_205_CONTENT ;
}

void cancel_observe(lwm2m_context_t * contextP,
                    int32_t mid,
                    void * fromSessionH)
{
	lwm2m_observed_t * observedP;

	LOG("cancel_observe()\r\n");

    for (observedP = contextP->observedList;
         observedP != NULL;
         observedP = observedP->next)
	{
		lwm2m_watcher_t * targetP = NULL;

        if ((0 > mid || observedP->watcherList->lastMid == mid)
         && observedP->watcherList->server->sessionH == fromSessionH)
		{
			targetP = observedP->watcherList;
			observedP->watcherList = observedP->watcherList->next;
        }
        else
		{
			lwm2m_watcher_t * parentP;

			parentP = observedP->watcherList;
			while (parentP->next != NULL
                && ((0 > mid || parentP->next->lastMid != mid)
                 || parentP->next->server->sessionH != fromSessionH))
			{
				parentP = parentP->next;
			}
			if (parentP->next != NULL)
			{
				targetP = parentP->next;
				parentP->next = parentP->next->next;
			}
		}
		if (targetP != NULL)
		{
			lwm2m_free(targetP);
			if (observedP->watcherList == NULL)
			{
				prv_unlinkObserved(contextP, observedP);
				lwm2m_free(observedP);
			}
			return;
		}
	}
}

typedef struct {
  lwm2m_blockwise_t * blockwiseP;
  char * buffer;
  int length;
} notification_data_t;

static void prv_free_notification_data(notification_data_t* data) {
	if (NULL == data->blockwiseP)
	{
		lwm2m_free(data->buffer);
		data->buffer = NULL;
	}
}

coap_status_t lwm2m_sendNotification(lwm2m_context_t * contextP, lwm2m_watcher_t * watcherP, lwm2m_uri_t * uriP, notification_data_t* data) {
  coap_status_t result = COAP_205_CONTENT;
  if (NULL == data->buffer) {
	  result = object_read(contextP, uriP, &(data->buffer), &(data->length));
  }
  if (result == COAP_205_CONTENT) {
	coap_packet_t message;
	watcherP->changed = true;
	watcherP->lastMid = contextP->nextMID++;

    coap_init_message(&message, COAP_TYPE_NON, COAP_204_CHANGED, watcherP->lastMid);
    coap_set_payload(&message, data->buffer, data->length);

    // if (!LWM2M_URI_IS_SET_RESOURCE(uriP)) {                      //JH+ leshan awaits content-type in case of a object instance notification
    //   //ToDo: at the moment TLV is default for object instance!  //JH+
    //   coap_set_header_content_type(message, VND_OMA_LWM2M_TLV);  //JH+ new pre-IANA enums see: er_coap_13.h
    // }                                                            //JH+
    coap_set_header_token(&message, watcherP->token, watcherP->tokenLen);
    coap_set_header_observe(&message, watcherP->counter++);
    if (watcherP->blockSize < data->length)
	{
		if (NULL == data->blockwiseP)
		{
			data->blockwiseP = blockwise_new(contextP, uriP, &message, false);
		}
		blockwise_prepare(data->blockwiseP, 0, watcherP->blockSize, &message);
	}
	LOG("send notify %d %d/%d/%d mid %d\n", watcherP->server->shortID, uriP->objectId, uriP->instanceId, uriP->resourceId, message.mid);

    (void)message_send(contextP, &message, watcherP->server->sessionH);
  }
  return result;
}

time_t lwm2m_notify(lwm2m_context_t * contextP, struct timeval * tv) {
    lwm2m_observed_t * observedP;
    time_t nextTransmission = 0;
    
    observedP = contextP->observedList;

    while (observedP != NULL)
    {
    	notification_data_t data;
    	memset(&data, 0, sizeof(data));
        lwm2m_watcher_t * watcherP = observedP->watcherList;
        while (watcherP != NULL) {
       	  lwm2m_attribute_data_t * attributeData = lwm2m_getAttributes(watcherP->server, &(observedP->uri));
          if(attributeData != NULL && attributeData->nextTransmission > 0) {
              // check if update needed or schedule next transmission
              if(tv->tv_sec >= attributeData->nextTransmission) {
                // send data
  				LOG("min period %s expired!\n", attributeData->minPeriod);
                if (COAP_205_CONTENT == lwm2m_sendNotification(contextP, watcherP, &(observedP->uri), &data)) {
                	lwm2m_updateTransmissionAttributes(attributeData, tv);
                }
                else {
                	// content error -> next observed value!
                	break;
                }
              }
              if (0 < attributeData->nextTransmission) {
				  if(nextTransmission == 0) {
					nextTransmission = attributeData->nextTransmission;
				  } else if (attributeData->nextTransmission < nextTransmission) {
					nextTransmission = attributeData->nextTransmission;
				  }
              }
           }
           watcherP = watcherP->next; 
        }
        prv_free_notification_data(&data);
        observedP = observedP->next;
    }
    
    return (nextTransmission);
}


void lwm2m_resource_value_changed(lwm2m_context_t * contextP,
                                  lwm2m_uri_t * uriP)
{
	obs_list_t * listP;

	listP = prv_getObservedList(contextP, uriP);
	while (listP != NULL)
	{
    	notification_data_t data;
		obs_list_t * targetP;
 	    coap_status_t result;

    	memset(&data, 0, sizeof(data));
		blockwise_remove(contextP, &(listP->item->uri));

		result = object_read(contextP, &(listP->item->uri), &(data.buffer), &(data.length));
		if (result == COAP_205_CONTENT)
		{
	    	lwm2m_watcher_t * watcherP;
			for (watcherP = listP->item->watcherList; watcherP != NULL; watcherP = watcherP->next)
			{
		        struct timeval tv;
                bool notifyResult = true;
                lwm2m_attribute_data_t*    attrData = lwm2m_getAttributes(watcherP->server, &(listP->item->uri));
                if (NULL != attrData) {
            		lwm2m_gettimeofday(&tv, NULL);
                	result = lwm2m_evalAttributes(attrData, data.buffer, data.length, tv, &notifyResult);
                    if (result != COAP_NO_ERROR) {
                        LOG("ERROR: lwm2m_evalAttributes() failed!\n"); //TODO: Error Handling
                        continue;
                    }
                }
                if (notifyResult) {
                    if (COAP_205_CONTENT == lwm2m_sendNotification(contextP, watcherP, &(listP->item->uri), &data)) {
                        if (NULL != attrData) {
                        	lwm2m_updateTransmissionAttributes(attrData, &tv);
                        }
                    }
                    else {
                    	// content error -> next observed value!
                    	break;
                    }
				}
			}
		}
		targetP = listP;
		listP = listP->next;
		lwm2m_free(targetP);
        prv_free_notification_data(&data);
	}
}
#endif

#ifdef LWM2M_SERVER_MODE
static lwm2m_observation_t * prv_findObservationByURI(lwm2m_client_t * clientP,
		lwm2m_uri_t * uriP)
{
	lwm2m_observation_t * targetP;

	targetP = clientP->observationList;
	while (targetP != NULL)
	{
		if (targetP->uri.objectId == uriP->objectId
				&& targetP->uri.flag == uriP->flag
				&& targetP->uri.instanceId == uriP->instanceId
				&& targetP->uri.resourceId == uriP->resourceId)
		{
			return targetP;
		}

		targetP = targetP->next;
	}

	return targetP;
}

void observation_remove(lwm2m_client_t * clientP,
		lwm2m_observation_t * observationP)
{
	if (clientP->observationList == observationP)
	{
		clientP->observationList = clientP->observationList->next;
	}
	else if (clientP->observationList != NULL)
	{
		lwm2m_observation_t * parentP;

		parentP = clientP->observationList;

		while (parentP->next != NULL
				&& parentP->next != observationP)
		{
			parentP = parentP->next;
		}
		if (parentP->next != NULL)
		{
			parentP->next = parentP->next->next;
		}
	}

	lwm2m_free(observationP);
}

static void prv_obsRequestCallback(lwm2m_transaction_t * transacP,
		void * message)
{
	lwm2m_observation_t * observationP = (lwm2m_observation_t *)transacP->userData;
	coap_packet_t * packet = (coap_packet_t *)message;
	uint8_t code;

	if (message == NULL)
	{
		code = COAP_503_SERVICE_UNAVAILABLE;
	}
	else if (packet->code == COAP_205_CONTENT
			&& !IS_OPTION(packet, COAP_OPTION_OBSERVE))
	{
		code = COAP_405_METHOD_NOT_ALLOWED;
	}
	else
	{
		code = packet->code;
	}

	if (code != COAP_205_CONTENT)
	{
		observationP->callback(((lwm2m_client_t*)transacP->peerP)->internalID,
				&observationP->uri,
				code,
				NULL, 0,
				observationP->userData);
		observation_remove(((lwm2m_client_t*)transacP->peerP), observationP);
	}
	else
	{
		observationP->clientP->observationList = (lwm2m_observation_t *)LWM2M_LIST_ADD(observationP->clientP->observationList, observationP);
		observationP->callback(((lwm2m_client_t*)transacP->peerP)->internalID,
				&observationP->uri,
				0,
				packet->payload, packet->payload_len,
				observationP->userData);
	}
}

int lwm2m_observe(lwm2m_context_t * contextP,
		uint16_t clientID,
		lwm2m_uri_t * uriP,
		lwm2m_result_callback_t callback,
		void * userData)
{
	lwm2m_client_t * clientP;
	lwm2m_transaction_t * transactionP;
	lwm2m_observation_t * observationP;
	uint8_t token[4];

	if (!LWM2M_URI_IS_SET_INSTANCE(uriP) && LWM2M_URI_IS_SET_RESOURCE(uriP)) return COAP_400_BAD_REQUEST;

	clientP = (lwm2m_client_t *)lwm2m_list_find((lwm2m_list_t *)contextP->clientList, clientID);
	if (clientP == NULL) return COAP_404_NOT_FOUND;

	observationP = (lwm2m_observation_t *)lwm2m_malloc(sizeof(lwm2m_observation_t));
	if (observationP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
	memset(observationP, 0, sizeof(lwm2m_observation_t));

	transactionP = transaction_new(COAP_GET, uriP, contextP->nextMID++, ENDPOINT_CLIENT, (void *)clientP);
	if (transactionP == NULL)
	{
		lwm2m_free(observationP);
		return COAP_500_INTERNAL_SERVER_ERROR;
	}

	observationP->id = lwm2m_list_newId((lwm2m_list_t *)clientP->observationList);
	memcpy(&observationP->uri, uriP, sizeof(lwm2m_uri_t));
	observationP->clientP = clientP;
	observationP->callback = callback;
	observationP->userData = userData;

	token[0] = clientP->internalID >> 8;
	token[1] = clientP->internalID & 0xFF;
	token[2] = observationP->id >> 8;
	token[3] = observationP->id & 0xFF;

	coap_set_header_observe(transactionP->message, 0);
	coap_set_header_token(transactionP->message, token, sizeof(token));

	transactionP->callback = prv_obsRequestCallback;
	transactionP->userData = (void *)observationP;

	contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transactionP);

	return transaction_send(contextP, transactionP);
}

int lwm2m_observe_cancel(lwm2m_context_t * contextP,
		uint16_t clientID,
		lwm2m_uri_t * uriP,
		lwm2m_result_callback_t callback,
		void * userData)
{
	lwm2m_client_t * clientP;
	lwm2m_observation_t * observationP;

	clientP = (lwm2m_client_t *)lwm2m_list_find((lwm2m_list_t *)contextP->clientList, clientID);
	if (clientP == NULL) return COAP_404_NOT_FOUND;

	observationP = prv_findObservationByURI(clientP, uriP);
	if (observationP == NULL) return COAP_404_NOT_FOUND;

	observation_remove(clientP, observationP);

	return 0;
}

void handle_observe_notify(lwm2m_context_t * contextP,
		void * fromSessionH,
		coap_packet_t * message)
{
	uint8_t * tokenP;
	int token_len;
	uint16_t clientID;
	uint16_t obsID;
	lwm2m_client_t * clientP;
	lwm2m_observation_t * observationP;
	uint32_t count;

	token_len = coap_get_header_token(message, (const uint8_t **)&tokenP);
	if (token_len != sizeof(uint32_t)) return;

	if (1 != coap_get_header_observe(message, &count)) return;

	clientID = (tokenP[0] << 8) | tokenP[1];
	obsID = (tokenP[2] << 8) | tokenP[3];

	clientP = (lwm2m_client_t *)lwm2m_list_find((lwm2m_list_t *)contextP->clientList, clientID);
	if (clientP == NULL) return;

	observationP = (lwm2m_observation_t *)lwm2m_list_find((lwm2m_list_t *)clientP->observationList, obsID);
	if (observationP == NULL)
	{
		coap_packet_t resetMsg;

		coap_init_message(&resetMsg, COAP_TYPE_RST, 0, message->mid);

		message_send(contextP, &resetMsg, fromSessionH);
	}
	else
	{
		observationP->callback(clientID,
				&observationP->uri,
				(int)count,
				message->payload, message->payload_len,
				observationP->userData);
	}
}
#endif
