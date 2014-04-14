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

#ifndef _LWM2M_CLIENT_H_
#define _LWM2M_CLIENT_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/time.h>

#ifndef LWM2M_EMBEDDED_MODE
#define lwm2m_gettimeofday gettimeofday
#define lwm2m_malloc malloc
#define lwm2m_free free
#else
int lwm2m_gettimeofday(struct timeval *tv, void *p);
void *lwm2m_malloc(size_t s);
void lwm2m_free(void *p);
#endif

/*
 * Error code
 */

#define COAP_NO_ERROR                   (uint8_t)0x00

#define COAP_201_CREATED                (uint8_t)0x41
#define COAP_202_DELETED                (uint8_t)0x42
#define COAP_204_CHANGED                (uint8_t)0x44
#define COAP_205_CONTENT                (uint8_t)0x45
#define COAP_400_BAD_REQUEST            (uint8_t)0x80
#define COAP_401_UNAUTHORIZED           (uint8_t)0x81
#define COAP_404_NOT_FOUND              (uint8_t)0x84
#define COAP_405_METHOD_NOT_ALLOWED     (uint8_t)0x85
#define COAP_406_NOT_ACCEPTABLE         (uint8_t)0x86
#define COAP_500_INTERNAL_SERVER_ERROR  (uint8_t)0xA0
#define COAP_501_NOT_IMPLEMENTED        (uint8_t)0xA1
#define COAP_503_SERVICE_UNAVAILABLE    (uint8_t)0xA3


/*
 * Utility functions for sorted linked list
 */

typedef struct _lwm2m_list_t
{
    struct _lwm2m_list_t * next;
    uint16_t    id;
} lwm2m_list_t;

// defined in list.c
// Add 'node' to the list 'head' and return the new list
lwm2m_list_t * lwm2m_list_add(lwm2m_list_t * head, lwm2m_list_t * node);
// Return the node with ID 'id' from the list 'head' or NULL if not found
lwm2m_list_t * lwm2m_list_find(lwm2m_list_t * head, uint16_t id);
// Remove the node with ID 'id' from the list 'head' and return the new list
lwm2m_list_t * lwm2m_list_remove(lwm2m_list_t * head, uint16_t id, lwm2m_list_t ** nodeP);
// Return the lowest unused ID in the list 'head'
uint16_t lwm2m_list_newId(lwm2m_list_t * head);

#define LWM2M_LIST_ADD(H,N) lwm2m_list_add((lwm2m_list_t *)H, (lwm2m_list_t *)N);
#define LWM2M_LIST_RM(H,I,N) lwm2m_list_remove((lwm2m_list_t *)H, I, (lwm2m_list_t **)N);


/*
 *  Resource values
 */

// defined in utils.c
int lwm2m_PlainTextToInt64(char * buffer, int length, int64_t * dataP);

/*
 * These utility functions allocate a new buffer storing the plain text
 * representation of data. They return the size in bytes of the buffer
 * or 0 in case of error.
 * There is no trailing '\0' character in the buffer.
 */
int lwm2m_int8ToPlainText(int8_t data, char ** bufferP);
int lwm2m_int16ToPlainText(int16_t data, char ** bufferP);
int lwm2m_int32ToPlainText(int32_t data, char ** bufferP);
int lwm2m_int64ToPlainText(int64_t data, char ** bufferP);
int lwm2m_float32ToPlainText(float data, char ** bufferP);
int lwm2m_float64ToPlainText(double data, char ** bufferP);
int lwm2m_boolToPlainText(bool data, char ** bufferP);


/*
 * TLV
 */

#define LWM2M_TLV_HEADER_MAX_LENGTH 6

typedef enum
{
    TLV_OBJECT_INSTANCE,
    TLV_RESSOURCE_INSTANCE,
    TLV_MULTIPLE_INSTANCE,
    TLV_RESSOURCE
} lwm2m_tlv_type_t;

/*
 * These utility functions fill the buffer with a TLV record containing
 * the data. They return the size in bytes of the TLV record, 0 in case
 * of error.
 */
int lwm2m_intToTLV(lwm2m_tlv_type_t type, int64_t data, uint16_t id, char * buffer, size_t buffer_len);
int lwm2m_boolToTLV(lwm2m_tlv_type_t type, bool value, uint16_t id, char * buffer, size_t buffer_len);
int lwm2m_opaqueToTLV(lwm2m_tlv_type_t type, uint8_t * dataP, size_t data_len, uint16_t id, char * buffer, size_t buffer_len);
int lwm2m_decodeTLV(char * buffer, size_t buffer_len, lwm2m_tlv_type_t * oType, uint16_t * oID, size_t * oDataIndex, size_t * oDataLen);
int lwm2m_opaqueToInt(char * buffer, size_t buffer_len, int64_t * dataP);

/*
 * URI
 *
 * objectId is always set
 * if instanceId or resourceId is greater than LWM2M_URI_MAX_ID, it means it is not specified
 *
 */

#define LWM2M_MAX_ID   ((uint16_t)0xFFFF)

#define LWM2M_URI_FLAG_OBJECT_ID    (uint8_t)0x04
#define LWM2M_URI_FLAG_INSTANCE_ID  (uint8_t)0x02
#define LWM2M_URI_FLAG_RESOURCE_ID  (uint8_t)0x01

#define LWM2M_URI_IS_SET_INSTANCE(uri) ((uri->flag & LWM2M_URI_FLAG_INSTANCE_ID) != 0)
#define LWM2M_URI_IS_SET_RESOURCE(uri) ((uri->flag & LWM2M_URI_FLAG_RESOURCE_ID) != 0)

typedef struct
{
    uint8_t     flag;           // indicates which segments are set
    uint16_t    objectId;
    uint16_t    instanceId;
    uint16_t    resourceId;
} lwm2m_uri_t;


#define LWM2M_STRING_ID_MAX_LEN 6

// Parse an URI in LWM2M format and fill the lwm2m_uri_t.
// Return the number of characters read from buffer or 0 in case of error.
// Valid URIs: /1, /1/, /1/2, /1/2/, /1/2/3
// Invalid URIs: /, //, //2, /1//, /1//3, /1/2/3/, /1/2/3/4
int lwm2m_stringToUri(char * buffer, size_t buffer_len, lwm2m_uri_t * uriP);


/*
 * LWM2M Objects
 */

typedef struct _lwm2m_object_t lwm2m_object_t;

typedef uint8_t (*lwm2m_read_callback_t) (lwm2m_uri_t * uriP, char ** bufferP, int * lengthP, lwm2m_object_t * objectP);
typedef uint8_t (*lwm2m_write_callback_t) (lwm2m_uri_t * uriP, char * buffer, int length, lwm2m_object_t * objectP);
typedef uint8_t (*lwm2m_execute_callback_t) (lwm2m_uri_t * uriP, char * rBuffer, int rLength,char ** wBuffer, int *wLength, lwm2m_object_t * objectP);
typedef uint8_t (*lwm2m_create_callback_t) (lwm2m_uri_t * uriP, char * buffer, int length, lwm2m_object_t * objectP);
typedef uint8_t (*lwm2m_delete_callback_t) (uint16_t id, lwm2m_object_t * objectP);
typedef void (*lwm2m_close_callback_t) (lwm2m_object_t * objectP);


struct _lwm2m_object_t
{
    uint16_t                 objID;
    lwm2m_list_t *           instanceList;
    lwm2m_read_callback_t    readFunc;
    lwm2m_write_callback_t   writeFunc;
    lwm2m_execute_callback_t executeFunc;
    lwm2m_create_callback_t  createFunc;
    lwm2m_delete_callback_t  deleteFunc;
    lwm2m_close_callback_t   closeFunc;
    void *                   userData;
};

/*
 * LWM2M Servers
 *
 * Since LWM2M Server Object instances are not accessible to LWM2M servers,
 * there is no need to store them as lwm2m_objects_t
 */

typedef enum
{
    SEC_NONE = 0,
    SEC_PRE_SHARED_KEY,
    SEC_RAW_PUBLIC_KEY,
    SEC_CERTIFICATE
} lwm2m_security_mode_t;

typedef struct
{
    lwm2m_security_mode_t mode;
    size_t     publicKeyLength;
    uint8_t *  publicKey;
    size_t     privateKeyLength;
    uint8_t *  privateKey;
} lwm2m_security_t;

typedef enum
{
    STATE_UNKNOWN = 0,
    STATE_REG_PENDING,
    STATE_REGISTERED
} lwm2m_status_t;

typedef struct _lwm2m_server_
{
    struct _lwm2m_server_ * next;   // matches lwm2m_list_t::next
    uint16_t          shortID;      // matches lwm2m_list_t::id
    lwm2m_security_t  security;
    void *            sessionH;
    lwm2m_status_t    status;
    char *            location;
    uint16_t          mid;
} lwm2m_server_t;

typedef struct
{
    char *           uri;
    lwm2m_security_t security;
    uint32_t         holdOffTime;
} lwm2m_bootstrap_server_t;

/*
 * LWM2M result callback
 *
 * When used with an observe, if 'data' is not nil, 'status' holds the observe counter.
 */
typedef void (*lwm2m_result_callback_t) (uint16_t clientID, lwm2m_uri_t * uriP, int status, uint8_t * data, int dataLength, void * userData);

/*
 * LWM2M Observations
 *
 * Used to store observation of remote clients resources.
 * status STATE_REG_PENDING means the observe request was sent to the client but not yet answered.
 * status STATE_REGISTERED means the client acknowledged the observe request.
 */

typedef struct _lwm2m_observation_
{
    struct _lwm2m_observation_ * next;  // matches lwm2m_list_t::next
    uint16_t                     id;    // matches lwm2m_list_t::id
    struct _lwm2m_client_ * clientP;
    lwm2m_uri_t             uri;
    lwm2m_result_callback_t callback;
    void *                  userData;
} lwm2m_observation_t;

/*
 * LWM2M Clients
 *
 * Be careful not to mix lwm2m_client_object_t used to store list of objects of remote clients
 * and lwm2m_object_t describing objects exposed to remote servers.
 *
 */

typedef struct _lwm2m_client_object_
{
    struct _lwm2m_client_object_ * next; // matches lwm2m_list_t::next
    uint16_t                 id;         // matches lwm2m_list_t::id
    lwm2m_list_t *           instanceList;
} lwm2m_client_object_t;

typedef struct _lwm2m_client_
{
    struct _lwm2m_client_ * next;       // matches lwm2m_list_t::next
    uint16_t                internalID; // matches lwm2m_list_t::id
    char *                  name;
    void *                  sessionH;
    lwm2m_client_object_t * objectList;
    lwm2m_observation_t *   observationList;
} lwm2m_client_t;


/*
 * LWM2M transaction
 *
 * Adaptation of Erbium's coap_transaction_t
 */

typedef enum
{
    ENDPOINT_UNKNOWN = 0,
    ENDPOINT_CLIENT,
    ENDPOINT_SERVER,
    ENDPOINT_BOOTSTRAP
} lwm2m_endpoint_type_t;

typedef struct _lwm2m_transaction_ lwm2m_transaction_t;

typedef void (*lwm2m_transaction_callback_t) (lwm2m_transaction_t * transacP, void * message);

struct _lwm2m_transaction_
{
    lwm2m_transaction_t * next;  // matches lwm2m_list_t::next
    uint16_t              mID;   // matches lwm2m_list_t::id
    lwm2m_endpoint_type_t peerType;
    void *                peerP;
    uint8_t  retrans_counter;
    time_t   retrans_time;
    char objStringID[LWM2M_STRING_ID_MAX_LEN];
    char instanceStringID[LWM2M_STRING_ID_MAX_LEN];
    char resourceStringID[LWM2M_STRING_ID_MAX_LEN];
    void * message;
    uint16_t buffer_len;
    uint8_t * buffer;
    lwm2m_transaction_callback_t callback;
    void * userData;
};

/*
 * LWM2M observed resources
 */
typedef struct _lwm2m_watcher_
{
    struct _lwm2m_watcher_ * next;

    lwm2m_server_t * server;
    uint8_t token[8];
    size_t tokenLen;
    uint32_t counter;
    uint16_t lastMid;
} lwm2m_watcher_t;

typedef struct _lwm2m_observed_
{
    struct _lwm2m_observed_ * next;

    lwm2m_uri_t uri;
    lwm2m_watcher_t * watcherList;
} lwm2m_observed_t;


/*
 * LWM2M Context
 */

// The session handle MUST uniquely identify a peer.
typedef uint8_t (*lwm2m_buffer_send_callback_t)(void * sessionH, uint8_t * buffer, size_t length, void * userData);


typedef struct
{
    int    socket;
#ifdef LWM2M_CLIENT_MODE
    char * endpointName;
    lwm2m_bootstrap_server_t * bootstrapServer;
    lwm2m_server_t *  serverList;
    lwm2m_object_t ** objectList;
    uint16_t          numObject;
    lwm2m_observed_t * observedList;
#endif
#ifdef LWM2M_SERVER_MODE
    lwm2m_client_t *        clientList;
    lwm2m_result_callback_t monitorCallback;
    void *                  monitorUserData;
#endif
    uint16_t          nextMID;
    lwm2m_transaction_t * transactionList;
    // buffer send callback
    lwm2m_buffer_send_callback_t bufferSendCallback;
    void *                       bufferSendUserData;
} lwm2m_context_t;


// initialize a liblwm2m context. endpointName, numObject and objectList are ignored for pure servers.
lwm2m_context_t * lwm2m_init(char * endpointName, uint16_t numObject, lwm2m_object_t * objectList[], lwm2m_buffer_send_callback_t bufferSendCallback, void * bufferSendUserData);
// close a liblwm2m context.
void lwm2m_close(lwm2m_context_t * contextP);

// perform any required pending operation and adjust timeoutP to the maximal time interval to wait.
int lwm2m_step(lwm2m_context_t * contextP, struct timeval * timeoutP);
// dispatch received data to liblwm2m
void lwm2m_handle_packet(lwm2m_context_t * contextP, uint8_t * buffer, int length, void * fromSessionH);

#ifdef LWM2M_CLIENT_MODE
void lwm2m_set_bootstrap_server(lwm2m_context_t * contextP, lwm2m_bootstrap_server_t * serverP);
int lwm2m_add_server(lwm2m_context_t * contextP, uint16_t shortID, void * sessionH, lwm2m_security_t * securityP);

// send registration message to all known LWM2M Servers.
int lwm2m_register(lwm2m_context_t * contextP);
// inform liblwm2m that a resource value has changed.
void lwm2m_resource_value_changed(lwm2m_context_t * contextP, lwm2m_uri_t * uriP);
#endif

#ifdef LWM2M_SERVER_MODE
// Clients registration/deregistration monitoring API.
// When a LWM2M client registers, the callback is called with status CREATED_2_01.
// When a LWM2M client deregisters, the callback is called with status DELETED_2_02.
// clientID is the internal ID of the LWM2M Client.
// The callback's parameters uri, data, dataLength are always NULL.
// The lwm2m_client_t is present in the lwm2m_context_t's clientList when the callback is called. On a deregistration, it deleted when the callback returns.
void lwm2m_set_monitoring_callback(lwm2m_context_t * contextP, lwm2m_result_callback_t callback, void * userData);

// Device Management APIs
int lwm2m_dm_read(lwm2m_context_t * contextP, uint16_t clientID, lwm2m_uri_t * uriP, lwm2m_result_callback_t callback, void * userData);
int lwm2m_dm_write(lwm2m_context_t * contextP, uint16_t clientID, lwm2m_uri_t * uriP, char * buffer, int length, lwm2m_result_callback_t callback, void * userData);
int lwm2m_dm_execute(lwm2m_context_t * contextP, uint16_t clientID, lwm2m_uri_t * uriP, char * buffer, int length, lwm2m_result_callback_t callback, void * userData);
int lwm2m_dm_create(lwm2m_context_t * contextP, uint16_t clientID, lwm2m_uri_t * uriP, char * buffer, int length, lwm2m_result_callback_t callback, void * userData);
int lwm2m_dm_delete(lwm2m_context_t * contextP, uint16_t clientID, lwm2m_uri_t * uriP, lwm2m_result_callback_t callback, void * userData);

// Information Reporting APIs
int lwm2m_observe(lwm2m_context_t * contextP, uint16_t clientID, lwm2m_uri_t * uriP, lwm2m_result_callback_t callback, void * userData);
int lwm2m_observe_cancel(lwm2m_context_t * contextP, uint16_t clientID, lwm2m_uri_t * uriP, lwm2m_result_callback_t callback, void * userData);
#endif

#endif
