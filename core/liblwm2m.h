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
#include <netinet/in.h>

/*
 *  Ressource URI
 */

#define LWM2M_URI_NOT_DEFINED   ((uint16_t)0xFFFF)

typedef struct
{
    uint16_t    objID;
    uint16_t    objInstance;
    uint16_t    resID;
} lwm2m_uri_t;


/*
 *  Ressource values
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
 * These utility functions fill the buffer with a TLV record containig
 * the data. They return the size in bytes of the TLV record, 0 in case
 * of error.
 */
int lwm2m_intToTLV(lwm2m_tlv_type_t type, int64_t data, uint16_t id, char * buffer, size_t buffer_len);
int lwm2m_boolToTLV(lwm2m_tlv_type_t type, bool value, uint16_t id, char * buffer, size_t buffer_len);
int lwm2m_opaqueToTLV(lwm2m_tlv_type_t type, uint8_t* dataP, size_t data_len, uint16_t id, char * buffer, size_t buffer_len);


/*
 * LWM2M Objects
 */

typedef struct _lwm2m_object_t lwm2m_object_t;

typedef uint8_t (*lwm2m_read_callback_t) (lwm2m_uri_t * uriP, char ** bufferP, int * lengthP, lwm2m_object_t * objectP);
typedef uint8_t (*lwm2m_write_callback_t) (lwm2m_uri_t * uriP, char * buffer, int length, lwm2m_object_t * objectP);
typedef uint8_t (*lwm2m_execute_callback_t) (lwm2m_uri_t * uriP, lwm2m_object_t * objectP);
typedef void (*lwm2m_close_callback_t) (lwm2m_object_t * objectP);


struct _lwm2m_object_t
{
    uint16_t                 objID;
    lwm2m_read_callback_t    readFunc;
    lwm2m_write_callback_t   writeFunc;
    lwm2m_execute_callback_t executeFunc;
    lwm2m_close_callback_t   closeFunc;
    void *                   userData;
} ;


/*
 * LWM2M Servers
 *
 * Since LWM2M Server Object instances are not accessible to LWM2M servers,
 * there is no need to store them as lwm2m_objects_t
 */

typedef enum
{
    SEC_PRE_SHARED_KEY = 0,
    SEC_RAW_PUBLIC_KEY,
    SEC_CERTIFICATE,
    SEC_NONE
} lwm2m_security_mode_t;

typedef struct
{
    lwm2m_security_mode_t mode;
    size_t     publicKeyLength;
    uint8_t *  publicKey;
    size_t     privateKeyLength;
    uint8_t *  privateKey;
} lwm2m_security_t;

typedef struct _lwm2m_server_
{
    struct _lwm2m_server_ * next;
    char *           uri;
    lwm2m_security_t security;
    uint16_t         shortID;
} lwm2m_server_t;

typedef struct
{
    char *           uri;
    lwm2m_security_t security;
    uint32_t         holdOffTime;
} lwm2m_bootstrap_server_t;

/*
 * LWM2M Context
 */

typedef struct
{
    lwm2m_bootstrap_server_t * bootstrapServer;
    lwm2m_server_t *  serverList;
    uint16_t          numObject;
    lwm2m_object_t ** objectList;
} lwm2m_context_t;

lwm2m_context_t * lwm2m_init();
void lwm2m_close(lwm2m_context_t * contextP);

int lwm2m_set_bootstrap_server(lwm2m_context_t * contextP, lwm2m_bootstrap_server_t * serverP);
int lwm2m_add_server(lwm2m_context_t * contextP, lwm2m_server_t * serverP);

int lwm2m_add_object(lwm2m_context_t * contextP, lwm2m_object_t * objectP);

int lwm2m_handle_packet(lwm2m_context_t * contextP, uint8_t * buffer, int length, int socket, struct sockaddr_storage fromAddr, socklen_t fromAddrLen);

#endif
