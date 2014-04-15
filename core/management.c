/*
Copyright (c) 2013, Intel Corporation

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
coap_status_t handle_dm_request(lwm2m_context_t * contextP,
                                lwm2m_uri_t * uriP,
                                void * fromSessionH,
                                coap_packet_t * message,
                                coap_packet_t * response)
{
    coap_status_t result;

    switch (message->code)
    {
    case COAP_GET:
        {
            char * buffer = NULL;
            int length = 0;

            result = object_read(contextP, uriP, &buffer, &length);
            if (result == COAP_205_CONTENT)
            {
                if (IS_OPTION(message, COAP_OPTION_OBSERVE))
                {
                    result = handle_observe_request(contextP, uriP, fromSessionH, message, response);
                }
                if (result == COAP_205_CONTENT)
                {
                    coap_set_payload(response, buffer, length);
                    // lwm2m_handle_packet will free buffer
                }
            }
        }
        break;
    case COAP_POST:
        {
            //is the instanceId set in the requested uri?
        	bool isInstanceSet=uriP->flag & LWM2M_URI_FLAG_INSTANCE_ID;
        	
            result = object_create_execute(contextP, uriP, message->payload, message->payload_len);
            if (result == COAP_201_CREATED || result == COAP_204_CHANGED)
		    {
            	//create & instanceId not in the uri before processing the request
            	//so we have assigned an Id for the new Instance & must send it back
            	if((result == COAP_201_CREATED) && !isInstanceSet)
            	{
            		//longest uri is /65535/65535 =12 + 1 (null) chars
            		char location_path[12]="";
            		//instanceId expected
            		if((uriP->flag & LWM2M_URI_FLAG_INSTANCE_ID)==0)
            		{
            			result = COAP_500_INTERNAL_SERVER_ERROR;
            			break;
            		}

					if(sprintf(location_path,"/%d/%d",uriP->objectId,uriP->instanceId) < 0){
						result = COAP_500_INTERNAL_SERVER_ERROR;
						break;
					}
					coap_set_header_location_path(response,location_path);

            	}
		    }
        }
        break;
    case COAP_PUT:
        {
            result = object_write(contextP, uriP, message->payload, message->payload_len);
        }
        break;
    case COAP_DELETE:
        {
            result = object_delete(contextP, uriP);
        }
        break;
    default:
        result = BAD_REQUEST_4_00;
        break;
    }

    return result;
}
#endif

#ifdef LWM2M_SERVER_MODE

#define ID_AS_STRING_MAX_LEN 8

static void dm_result_callback(lwm2m_transaction_t * transacP,
                               void * message)
{
    dm_data_t * dataP = (dm_data_t *)transacP->userData;

    if (message == NULL)
    {
        dataP->callback(((lwm2m_client_t*)transacP->peerP)->internalID,
                        &dataP->uri,
                        COAP_503_SERVICE_UNAVAILABLE,
                        NULL, 0,
                        dataP->userData);
    }
    else
    {
        coap_packet_t * packet = (coap_packet_t *)message;
        char* location_path = NULL;
		int location_path_len =0;

		location_path_len = coap_get_header_location_path(packet,&location_path);

        //if packet is a CREATE response and the instanceId was assigned by the client
        if ((packet->code == COAP_201_CREATED) && (location_path_len > 0))
        {
            int result =0;
            char locationString[12]="";

            //longest uri is /65535/65535 =12 + 1 (null) chars
			snprintf(locationString,location_path_len+2,"/%s",location_path);

            lwm2m_uri_t *transactionUri=NULL;
            lwm2m_uri_t *locationUri=(lwm2m_uri_t*)lwm2m_malloc(sizeof(lwm2m_uri_t));
            if (locationUri == NULL)
			 {
				 fprintf(stderr,"Error: lwm2m_malloc failed in dm_result_callback()\n");
				 return;
			 }
            memset(locationUri,0,sizeof(locationUri));

            result= lwm2m_stringToUri(locationString,strlen(locationString),locationUri);
            if (result == 0)
            {
                fprintf(stderr,"Error: lwm2m_stringToUri() failed for Location_path option in dm_result_callback()\n");
                return;
            }

            transactionUri = &((dm_data_t*)transacP->userData)->uri;
            transactionUri->instanceId = locationUri->instanceId;
            transactionUri->flag =locationUri->flag;
            lwm2m_free(locationUri);
        }

        dataP->callback(((lwm2m_client_t*)transacP->peerP)->internalID,
                        &dataP->uri,
                        packet->code,
                        packet->payload,
                        packet->payload_len,
                        dataP->userData);
    }
    lwm2m_free(dataP);
}

static int prv_make_operation(lwm2m_context_t * contextP,
                              uint16_t clientID,
                              lwm2m_uri_t * uriP,
                              coap_method_t method,
                              char * buffer,
                              int length,
                              lwm2m_result_callback_t callback,
                              void * userData)
{
    lwm2m_client_t * clientP;
    lwm2m_transaction_t * transaction;
    dm_data_t * dataP;

    clientP = (lwm2m_client_t *)lwm2m_list_find((lwm2m_list_t *)contextP->clientList, clientID);
    if (clientP == NULL) return COAP_404_NOT_FOUND;

    transaction = transaction_new(method, uriP, contextP->nextMID++, ENDPOINT_CLIENT, (void *)clientP);
    if (transaction == NULL) return INTERNAL_SERVER_ERROR_5_00;

    if (buffer != NULL)
    {
        // TODO: Take care of fragmentation
        coap_set_payload(transaction->message, buffer, length);
    }

    if (callback != NULL)
    {
        dataP = (dm_data_t *)lwm2m_malloc(sizeof(dm_data_t));
        if (dataP == NULL)
        {
            transaction_free(transaction);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
        memcpy(&dataP->uri, uriP, sizeof(lwm2m_uri_t));
        dataP->callback = callback;
        dataP->userData = userData;

        transaction->callback = dm_result_callback;
        transaction->userData = (void *)dataP;
    }

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);

    return transaction_send(contextP, transaction);
}

int lwm2m_dm_read(lwm2m_context_t * contextP,
                  uint16_t clientID,
                  lwm2m_uri_t * uriP,
                  lwm2m_result_callback_t callback,
                  void * userData)
{
    return prv_make_operation(contextP, clientID, uriP,
                              COAP_GET, NULL, 0,
                              callback, userData);
}

int lwm2m_dm_write(lwm2m_context_t * contextP,
                   uint16_t clientID,
                   lwm2m_uri_t * uriP,
                   char * buffer,
                   int length,
                   lwm2m_result_callback_t callback,
                   void * userData)
{
    if (!LWM2M_URI_IS_SET_INSTANCE(uriP)
     || length == 0)
    {
        return COAP_400_BAD_REQUEST;
    }

    return prv_make_operation(contextP, clientID, uriP,
                              COAP_PUT, buffer, length,
                              callback, userData);
}

int lwm2m_dm_execute(lwm2m_context_t * contextP,
                     uint16_t clientID,
                     lwm2m_uri_t * uriP,
                     char * buffer,
                     int length,
                     lwm2m_result_callback_t callback,
                     void * userData)
{
    if (!LWM2M_URI_IS_SET_RESOURCE(uriP))
    {
        return COAP_400_BAD_REQUEST;
    }

    return prv_make_operation(contextP, clientID, uriP,
                              COAP_POST, buffer, length,
                              callback, userData);
}

int lwm2m_dm_create(lwm2m_context_t * contextP,
                    uint16_t clientID,
                    lwm2m_uri_t * uriP,
                    char * buffer,
                    int length,
                    lwm2m_result_callback_t callback,
                    void * userData)
{
    if (LWM2M_URI_IS_SET_RESOURCE(uriP)
     || length == 0)
    {
        return COAP_400_BAD_REQUEST;
    }

    return prv_make_operation(contextP, clientID, uriP,
                              COAP_POST, buffer, length,
                              callback, userData);
}

int lwm2m_dm_delete(lwm2m_context_t * contextP,
                    uint16_t clientID,
                    lwm2m_uri_t * uriP,
                    lwm2m_result_callback_t callback,
                    void * userData)
{
    if (!LWM2M_URI_IS_SET_INSTANCE(uriP)
     || LWM2M_URI_IS_SET_RESOURCE(uriP))
    {
        return COAP_400_BAD_REQUEST;
    }

    return prv_make_operation(contextP, clientID, uriP,
                              COAP_DELETE, NULL, 0,
                              callback, userData);
}
#endif
