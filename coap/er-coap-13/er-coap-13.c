/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *      An implementation of the Constrained Application Protocol (draft 12)
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 * \author
 *    David Navarro, Intel Corporation - Adapt to usage in liblwm2m
 *    Tuve Nordius, Husqvarna Group - Please refer to git log
 *    Lukas Woodtli, Gardena AG - Please refer to git log
 */


#include <stdlib.h>

#include <string.h>
#include <stdio.h>

#include "er-coap-13.h"

#include "liblwm2m.h" /* for lwm2m_malloc() and lwm2m_free() */

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]",(lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3],(lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

#define COAP_OPTION_EXTENDED_8_BIT_OFFSET 13
#define COAP_OPTION_EXTENDED_8_BIT_MARKER 13

#define COAP_OPTION_EXTENDED_16_BIT_OFFSET 269
#define COAP_OPTION_EXTENDED_16_BIT_MARKER 14

/*-----------------------------------------------------------------------------------*/
/*- Variables -----------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------*/
static uint16_t current_mid = 0;

coap_status_t coap_error_code = NO_ERROR;
const char *coap_error_message = "";
/*-----------------------------------------------------------------------------------*/
/*- LOCAL HELP FUNCTIONS ------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------*/
static
uint16_t
coap_log_2(uint16_t value)
{
  uint16_t result = 0;
  do {
    value = value >> 1;
    result++;
  } while (value);

  return result ? result - 1 : result;
}
/*-----------------------------------------------------------------------------------*/
static
uint32_t
coap_parse_int_option(uint8_t *bytes, size_t length)
{
  uint32_t var = 0;
  size_t i = 0;
  while (i<length)
  {
    var <<= 8;
    var |= bytes[i++];
  }
  return var;
}
/*-----------------------------------------------------------------------------------*/

static inline uint8_t coap_option_nibble(unsigned int value)
{
  if (value < COAP_OPTION_EXTENDED_8_BIT_OFFSET)
  {
    return value;
  }
  else if (value <= 0xFF + COAP_OPTION_EXTENDED_8_BIT_OFFSET)
  {
    return COAP_OPTION_EXTENDED_8_BIT_MARKER;
  }
  else
  {
    return COAP_OPTION_EXTENDED_16_BIT_MARKER;
  }
}
/*-----------------------------------------------------------------------------------*/

static inline size_t coap_set_option_header_extended(const uint32_t value, uint8_t *buffer)
{
  size_t i = 0;

  if (value >= COAP_OPTION_EXTENDED_16_BIT_OFFSET)
  {
    buffer[i] = (value - COAP_OPTION_EXTENDED_16_BIT_OFFSET) >> 8;
    buffer[i + 1] = (value - COAP_OPTION_EXTENDED_16_BIT_OFFSET);
    i += 2;
  }
  else if (value >= COAP_OPTION_EXTENDED_8_BIT_OFFSET)
  {
    buffer[i] = (value - COAP_OPTION_EXTENDED_16_BIT_OFFSET);
    i++;
  }

  return i;
}

/** Write the header of an option to the buffer.
 * This also handles extended delta and length fields.
 * See CoAP spec (RFC 7252) Section: 3.1. Option Format */
static inline size_t coap_set_option_header(unsigned int delta, size_t length, uint8_t *buffer)
{
  size_t index = 0;

  buffer[index] = coap_option_nibble(delta)<<4 | coap_option_nibble(length);
  index++;

  index += coap_set_option_header_extended(delta, &buffer[index]);
  index += coap_set_option_header_extended(length, &buffer[index]);

  return index;
}

/*-----------------------------------------------------------------------------------*/
static
size_t
coap_serialize_int_option(unsigned int number, unsigned int current_number, uint8_t *buffer, uint32_t value)
{
  size_t i = 0;

  if (0xFF000000 & value) ++i;
  if (0xFFFF0000 & value) ++i;
  if (0xFFFFFF00 & value) ++i;
  if (0xFFFFFFFF & value) ++i;

  PRINTF("OPTION %u (delta %u, len %u)\n", number, number - current_number, i);

  i = coap_set_option_header(number - current_number, i, buffer);

  if (0xFF000000 & value) buffer[i++] = (uint8_t) (value>>24);
  if (0xFFFF0000 & value) buffer[i++] = (uint8_t) (value>>16);
  if (0xFFFFFF00 & value) buffer[i++] = (uint8_t) (value>>8);
  if (0xFFFFFFFF & value) buffer[i++] = (uint8_t) (value);

  return i;
}
/*-----------------------------------------------------------------------------------*/
static
size_t
coap_serialize_array_option(unsigned int number, unsigned int current_number, uint8_t *buffer, uint8_t *array, size_t length, char split_char)
{
  size_t i = 0;

  if (split_char!='\0')
  {
    size_t j;
    uint8_t *part_start = array;
    uint8_t *part_end = NULL;
    size_t temp_length;

    for (j = 0; j<=length; ++j)
    {
      if (array[j]==split_char || j==length)
      {
        part_end = array + j;
        temp_length = part_end-part_start;

        i += coap_set_option_header(number - current_number, temp_length, &buffer[i]);
        memcpy(&buffer[i], part_start, temp_length);
        i += temp_length;

        PRINTF("OPTION type %u, delta %u, len %u, part [%.*s]\n", number, number - current_number, i, temp_length, part_start);

        ++j; /* skip the splitter */
        current_number = number;
        part_start = array + j;
      }
    } /* for */
  }
  else
  {
    i += coap_set_option_header(number - current_number, length, &buffer[i]);
    memcpy(&buffer[i], array, length);
    i += length;

    PRINTF("OPTION type %u, delta %u, len %u\n", number, number - current_number, length);
  }

  return i;
}
/*-----------------------------------------------------------------------------------*/
static
size_t
coap_serialize_multi_option(unsigned int number, unsigned int current_number, uint8_t *buffer, multi_option_t *array)
{
  size_t i = 0;
  multi_option_t * j;

  for (j = array; j != NULL; j= j->next)
  {
     i += coap_set_option_header(number - current_number, j->len, &buffer[i]);
     current_number = number;
     memcpy(&buffer[i], j->data, j->len);
     i += j->len;
  } /* for */

  return i;
}
/*-----------------------------------------------------------------------------------*/
static
void
coap_merge_multi_option(uint8_t **dst, size_t *dst_len, uint8_t *option, size_t option_len, char separator)
{
  /* Merge multiple options. */
  if (*dst_len > 0)
  {
    /* dst already contains an option: concatenate */
    (*dst)[*dst_len] = separator;
    *dst_len += 1;

    /* memmove handles 2-byte option headers */
    memmove((*dst)+(*dst_len), option, option_len);

    *dst_len += option_len;
  }
  else
  {
    /* dst is empty: set to option */
    *dst = option;
    *dst_len = option_len;
  }
}

void
coap_add_multi_option(multi_option_t **dst, uint8_t *option, size_t option_len, uint8_t is_static)
{
  multi_option_t *opt = (multi_option_t *)lwm2m_malloc(sizeof(multi_option_t));

  if (opt)
  {
    opt->next = NULL;
    opt->len = (uint8_t)option_len;
    if (is_static)
    {
      opt->data = option;
      opt->is_static = 1;
    }
    else
    {
        opt->is_static = 0;
        opt->data = (uint8_t *)lwm2m_malloc(option_len);
        if (opt->data == NULL)
        {
            lwm2m_free(opt);
            return;
        }
        memcpy(opt->data, option, option_len);
    }

    if (*dst)
    {
      multi_option_t * i = *dst;
      while (i->next)
      {
        i = i->next;
      }
      i->next = opt;
    }
    else
    {
      *dst = opt;
    }
  }
}

void
free_multi_option(multi_option_t *dst)
{
  if (dst)
  {
    multi_option_t *n = dst->next;
    dst->next = NULL;
    if (dst->is_static == 0)
    {
        lwm2m_free(dst->data);
    }
    lwm2m_free(dst);
    free_multi_option(n);
  }
}

static
char *
prv_coap_get_multi_option_as_string(multi_option_t * option, char prefix, char delimiter)
{
    size_t len = 0;
    multi_option_t * opt;
    char * output;

    for (opt = option; opt != NULL; opt = opt->next)
    {
       len += opt->len + 1;     // for separator
    }

    output = (char*) lwm2m_malloc(len + 1); // for String terminator
    if (output != NULL)
    {
        size_t i = 0;

        for (opt = option; opt != NULL; opt = opt->next)
        {
            if (i == 0){
                output[i] = prefix;
            } else {
                output[i] = delimiter;
            }
            i += 1;

            memmove(output + i, opt->data, opt->len);
            i += opt->len;
        }
        output[i] = 0;
    }

    return output;
}

char * coap_get_multi_option_as_path_string(multi_option_t * option)
{
    return prv_coap_get_multi_option_as_string(option, '/', '/');
}

char * coap_get_multi_option_as_query_string(multi_option_t * option)
{
    return prv_coap_get_multi_option_as_string(option, '?', '&');
}


static
bool
is_non_std_coap_port(uint16_t port)
{
    return !(port == 0 || port == 5683 || port == 5684);
}

static
int nDigits(unsigned x) {
    if (x >= 1000000000) return 10;
    if (x >= 100000000)  return 9;
    if (x >= 10000000)   return 8;
    if (x >= 1000000)    return 7;
    if (x >= 100000)     return 6;
    if (x >= 10000)      return 5;
    if (x >= 1000)       return 4;
    if (x >= 100)        return 3;
    if (x >= 10)         return 2;
    return 1;
}
char * coap_get_packet_uri_as_string(coap_packet_t * packet)
{
    // We do not know the destination or if it is secured with dtls or not therefor
    // we omit the protocol part as well as host and port if not in the options
    //
    char * output;
    
    char * path_string = coap_get_multi_option_as_path_string(packet->uri_path);
    size_t path_len = strlen(path_string);
    char * query_string = coap_get_multi_option_as_query_string(packet->uri_query);
    size_t query_len = strlen(query_string);

    size_t len = 2 * sizeof(char); // "//"
    len += packet->uri_host_len;
    len += is_non_std_coap_port(packet->uri_port) ? (int)(nDigits(packet->uri_port)*sizeof(char)) : 0;
    len += 1 > path_len ? 1 : path_len; // "/" or path
    len += query_len;
    
    output = (char *) lwm2m_malloc(len + 1);
    if(output == NULL){
      return NULL;
    }
    
    strcpy(output, "//");
    strncat(output, (char *)packet->uri_host, packet->uri_host_len);
    if (1 > path_len)
    {
        strcat(output, "/");
    }
    else
    {
        strncat(output, path_string, path_len);
    }
    strncat(output, query_string, query_len);
    
    output[len] = 0;
    
    lwm2m_free(path_string);
    lwm2m_free(query_string);
    
    return output;
}

/*-----------------------------------------------------------------------------------*/

static
int
coap_get_variable(const uint8_t *buffer, size_t length, const char *name, const char **output)
{
  const uint8_t *start = NULL;
  const uint8_t *end = NULL;
  const uint8_t *value_end = NULL;
  size_t name_len = 0;

  /*initialize the output buffer first*/
  *output = 0;

  name_len = strlen(name);
  end = buffer + length;

  for (start = buffer; start + name_len < end; ++start){
    if ((start == buffer || start[-1] == '&') && start[name_len] == '=' &&
        strncmp(name, (char *)start, name_len)==0) {

      /* Point start to variable value */
      start += name_len + 1;

      /* Point end to the end of the value */
      value_end = (const uint8_t *) memchr(start, '&', end - start);
      if (value_end == NULL) {
        value_end = end;
      }

      *output = (char *)start;

      return (value_end - start);
    }
  }

  return 0;
}

/*-----------------------------------------------------------------------------------*/
uint16_t
coap_get_mid(void)
{
  return ++current_mid;
}
/*-----------------------------------------------------------------------------------*/
/*- MEASSAGE PROCESSING -------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------*/
void
coap_init_message(void *packet, coap_message_type_t type, uint8_t code, uint16_t mid)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  /* Important thing */
  memset(coap_pkt, 0, sizeof(coap_packet_t));

  coap_pkt->type = type;
  coap_pkt->code = code;
  coap_pkt->mid = mid;
}

void
coap_free_header(void *packet)
{
    coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

    free_multi_option(coap_pkt->uri_path);
    free_multi_option(coap_pkt->uri_query);
    free_multi_option(coap_pkt->location_path);
    coap_pkt->uri_path = NULL;
    coap_pkt->uri_query = NULL;
    coap_pkt->location_path = NULL;
}

/*-----------------------------------------------------------------------------------*/
size_t coap_serialize_get_size(void *packet)
{
    coap_packet_t *const coap_pkt = (coap_packet_t *) packet;
    size_t length = 0;

    length = COAP_HEADER_LEN + coap_pkt->payload_len + coap_pkt->token_len;

    if (IS_OPTION(coap_pkt, COAP_OPTION_IF_MATCH))
    {
        length += COAP_MAX_OPTION_HEADER_LEN + coap_pkt->if_match_len;
    }
    if (IS_OPTION(coap_pkt, COAP_OPTION_URI_HOST))
    {
        length += COAP_MAX_OPTION_HEADER_LEN + coap_pkt->uri_host_len;
    }
    if (IS_OPTION(coap_pkt, COAP_OPTION_ETAG))
    {
        length += COAP_MAX_OPTION_HEADER_LEN + coap_pkt->etag_len;
    }
    if (IS_OPTION(coap_pkt, COAP_OPTION_IF_NONE_MATCH))
    {
        // can be stored in extended fields
        length += COAP_MAX_OPTION_HEADER_LEN;
    }
    if (IS_OPTION(coap_pkt, COAP_OPTION_OBSERVE))
    {
        // can be stored in extended fields
        length += COAP_MAX_OPTION_HEADER_LEN;
    }
    if (IS_OPTION(coap_pkt, COAP_OPTION_URI_PORT))
    {
        // can be stored in extended fields
        length += COAP_MAX_OPTION_HEADER_LEN;
    }
    if (IS_OPTION(coap_pkt, COAP_OPTION_LOCATION_PATH))
    {
        multi_option_t * optP;

        for (optP = coap_pkt->location_path ; optP != NULL ; optP = optP->next)
        {
            length += COAP_MAX_OPTION_HEADER_LEN + optP->len;
        }
    }
    if (IS_OPTION(coap_pkt, COAP_OPTION_URI_PATH))
    {
        multi_option_t * optP;

        for (optP = coap_pkt->uri_path ; optP != NULL ; optP = optP->next)
        {
            length += COAP_MAX_OPTION_HEADER_LEN + optP->len;
        }
    }
    if (IS_OPTION(coap_pkt, COAP_OPTION_CONTENT_TYPE))
    {
        // can be stored in extended fields
        length += COAP_MAX_OPTION_HEADER_LEN;
    }
    if (IS_OPTION(coap_pkt, COAP_OPTION_MAX_AGE))
    {
        // can be stored in extended fields
        length += COAP_MAX_OPTION_HEADER_LEN;
    }
    if (IS_OPTION(coap_pkt, COAP_OPTION_URI_QUERY))
    {
        multi_option_t * optP;

        for (optP = coap_pkt->uri_query ; optP != NULL ; optP = optP->next)
        {
            length += COAP_MAX_OPTION_HEADER_LEN + optP->len;
        }
    }
    if (IS_OPTION(coap_pkt, COAP_OPTION_ACCEPT))
    {
        length += coap_pkt->accept_num * COAP_MAX_OPTION_HEADER_LEN;
    }
    if (IS_OPTION(coap_pkt, COAP_OPTION_LOCATION_QUERY))
    {
        length += COAP_MAX_OPTION_HEADER_LEN + coap_pkt->location_query_len;
    }
    if (IS_OPTION(coap_pkt, COAP_OPTION_BLOCK2))
    {
        // can be stored in extended fields
        length += COAP_MAX_OPTION_HEADER_LEN;
    }
    if (IS_OPTION(coap_pkt, COAP_OPTION_BLOCK1))
    {
        // can be stored in extended fields
        length += COAP_MAX_OPTION_HEADER_LEN;
    }
    if (IS_OPTION(coap_pkt, COAP_OPTION_SIZE))
    {
        // can be stored in extended fields
        length += COAP_MAX_OPTION_HEADER_LEN;
    }
    if (IS_OPTION(coap_pkt, COAP_OPTION_PROXY_URI))
    {
        length += COAP_MAX_OPTION_HEADER_LEN + coap_pkt->proxy_uri_len;
    }

    if (coap_pkt->payload_len)
    {
        // Account for the payload marker.
        length += 1;
    }

    return length;
}

/*-----------------------------------------------------------------------------------*/
size_t
coap_serialize_message(void *packet, uint8_t *buffer)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;
  uint8_t *option;
  unsigned int current_number = 0;

  /* Initialize */
  coap_pkt->buffer = buffer;
  coap_pkt->version = 1;

  PRINTF("-Serializing MID %u to %p, ", coap_pkt->mid, coap_pkt->buffer);

  /* set header fields */
  coap_pkt->buffer[0]  = 0x00;
  coap_pkt->buffer[0] |= COAP_HEADER_VERSION_MASK & (coap_pkt->version)<<COAP_HEADER_VERSION_POSITION;
  coap_pkt->buffer[0] |= COAP_HEADER_TYPE_MASK & (coap_pkt->type)<<COAP_HEADER_TYPE_POSITION;
  coap_pkt->buffer[0] |= COAP_HEADER_TOKEN_LEN_MASK & (coap_pkt->token_len)<<COAP_HEADER_TOKEN_LEN_POSITION;
  coap_pkt->buffer[1] = coap_pkt->code;
  coap_pkt->buffer[2] = (uint8_t) ((coap_pkt->mid)>>8);
  coap_pkt->buffer[3] = (uint8_t) (coap_pkt->mid);

  /* set Token */
  PRINTF("Token (len %u)", coap_pkt->token_len);
  option = coap_pkt->buffer + COAP_HEADER_LEN;
  for (current_number=0; current_number<coap_pkt->token_len; ++current_number)
  {
    PRINTF(" %02X", coap_pkt->token[current_number]);
    *option = coap_pkt->token[current_number];
    ++option;
  }
  PRINTF("-\n");

  /* Serialize options */
  current_number = 0;

  PRINTF("-Serializing options at %p-\n", option);

  /* The options must be serialized in the order of their number */
  COAP_SERIALIZE_BYTE_OPTION(   COAP_OPTION_IF_MATCH,       if_match, "If-Match")
  COAP_SERIALIZE_STRING_OPTION( COAP_OPTION_URI_HOST,       uri_host, '\0', "Uri-Host")
  COAP_SERIALIZE_BYTE_OPTION(   COAP_OPTION_ETAG,           etag, "ETag")
  COAP_SERIALIZE_INT_OPTION(    COAP_OPTION_IF_NONE_MATCH,  content_type-coap_pkt->content_type, "If-None-Match") /* hack to get a zero field */
  COAP_SERIALIZE_INT_OPTION(    COAP_OPTION_OBSERVE,        observe, "Observe")
  COAP_SERIALIZE_INT_OPTION(    COAP_OPTION_URI_PORT,       uri_port, "Uri-Port")
  COAP_SERIALIZE_MULTI_OPTION(  COAP_OPTION_LOCATION_PATH,  location_path, "Location-Path")
  COAP_SERIALIZE_MULTI_OPTION(  COAP_OPTION_URI_PATH,       uri_path, "Uri-Path")
  COAP_SERIALIZE_INT_OPTION(    COAP_OPTION_CONTENT_TYPE,   content_type, "Content-Format")
  COAP_SERIALIZE_INT_OPTION(    COAP_OPTION_MAX_AGE,        max_age, "Max-Age")
  COAP_SERIALIZE_MULTI_OPTION(  COAP_OPTION_URI_QUERY,      uri_query, "Uri-Query")
  COAP_SERIALIZE_ACCEPT_OPTION( COAP_OPTION_ACCEPT,         accept, "Accept")
  COAP_SERIALIZE_STRING_OPTION( COAP_OPTION_LOCATION_QUERY, location_query, '&', "Location-Query")
  COAP_SERIALIZE_BLOCK_OPTION(  COAP_OPTION_BLOCK2,         block2, "Block2")
  COAP_SERIALIZE_BLOCK_OPTION(  COAP_OPTION_BLOCK1,         block1, "Block1")
  COAP_SERIALIZE_INT_OPTION(    COAP_OPTION_SIZE,           size, "Size")
  COAP_SERIALIZE_STRING_OPTION( COAP_OPTION_PROXY_URI,      proxy_uri, '\0', "Proxy-Uri")

  PRINTF("-Done serializing at %p----\n", option);

  /* Free allocated header fields */
  coap_free_header(packet);

  /* Pack payload */
  /* Payload marker */
  if (coap_pkt->payload_len)
  {
    *option = 0xFF;
    ++option;
  }

  if (coap_pkt->payload != NULL) {
    memmove(option, coap_pkt->payload, coap_pkt->payload_len);
  }

  PRINTF("-Done %u B (header len %u, payload len %u)-\n", coap_pkt->payload_len + option - buffer, option - buffer, coap_pkt->payload_len);

  PRINTF("Dump [0x%02X %02X %02X %02X  %02X %02X %02X %02X]\n",
      coap_pkt->buffer[0],
      coap_pkt->buffer[1],
      coap_pkt->buffer[2],
      coap_pkt->buffer[3],
      coap_pkt->buffer[4],
      coap_pkt->buffer[5],
      coap_pkt->buffer[6],
      coap_pkt->buffer[7]
    );

  return (option - buffer) + coap_pkt->payload_len; /* packet length */
}
/*-----------------------------------------------------------------------------------*/
coap_status_t
coap_parse_message(void *packet, uint8_t *data, uint16_t data_len)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;
  const uint8_t * const data_end = data + data_len;
  uint8_t *current_option;
  uint32_t option_number = 0;
  uint32_t option_delta = 0;
  uint32_t option_length = 0;
  uint32_t *x;
  int i;

  /* Initialize packet */
  memset(coap_pkt, 0, sizeof(coap_packet_t));

  /* Care: Error case requries an initialized coap_pkt! */
  if (data_len < COAP_HEADER_LEN) {
    goto exit_parse_error;
  }

  /* pointer to packet bytes */
  coap_pkt->buffer = data;

  /* parse header fields */
  coap_pkt->version = (COAP_HEADER_VERSION_MASK & coap_pkt->buffer[0])>>COAP_HEADER_VERSION_POSITION;
  coap_pkt->type = (coap_message_type_t) ((COAP_HEADER_TYPE_MASK & coap_pkt->buffer[0])>>COAP_HEADER_TYPE_POSITION);
  coap_pkt->token_len = (COAP_HEADER_TOKEN_LEN_MASK & coap_pkt->buffer[0])>>COAP_HEADER_TOKEN_LEN_POSITION;
  coap_pkt->code = coap_pkt->buffer[1];
  coap_pkt->mid = coap_pkt->buffer[2]<<8 | coap_pkt->buffer[3];

  if (coap_pkt->version != 1)
  {
    coap_error_message = "CoAP version must be 1";
    return BAD_REQUEST_4_00;
  }

  /*
   * Empty Message:
   *   A message with a Code of 0.00; neither a request nor a response.
   *   An Empty message only contains the 4-byte header.
   *
   * TODO: Error on requests and responses
   */
  if ((coap_pkt->code == COAP_EMPTY_MESSAGE_CODE) && (data_len > COAP_HEADER_LEN)) {
    goto exit_parse_error;
  }

  /*
   * Lengths 9-15 are reserved, MUST NOT be sent, and MUST be processed as a
   * message format error.
   */
  if (coap_pkt->token_len > COAP_TOKEN_LEN) {
    goto exit_parse_error;
  }

  current_option = data + COAP_HEADER_LEN;

  if (coap_pkt->token_len != 0)
  {
      if (current_option + coap_pkt->token_len > data_end) {
        goto exit_parse_error;
      }
      memcpy(coap_pkt->token, current_option, coap_pkt->token_len);
      SET_OPTION(coap_pkt, COAP_OPTION_TOKEN);

      PRINTF("Token (len %u) [0x%02X", coap_pkt->token_len, coap_pkt->token[0]);
      for (i = 1; i < coap_pkt->token_len; i++) {
        PRINTF("%02X", coap_pkt->token[i]);
      }
      PRINTF("]\n");
  }

  /* parse options */
  current_option += coap_pkt->token_len;

  while (current_option < data_end)
  {
    if (current_option[0] == 0xFF)
    {
      /*
       * The presence of a marker followed by a zero-length payload MUST be
       * processed as a message format error.
       */
      if (current_option + 1 == data_end) {
        goto exit_parse_error;
      }
      coap_pkt->payload = ++current_option;
      coap_pkt->payload_len = data_end - coap_pkt->payload;

      break;
    }

    option_delta = current_option[0]>>4;
    option_length = current_option[0] & 0x0F;
    /* Error on reserved option values */
    if ((option_delta == COAP_HEADER_OPTION_DELTA_RESERVED) ||
        (option_length == COAP_HEADER_OPTION_LENGTH_RESERVED)) {
      goto exit_parse_error;
    }

    /* avoids code duplication without function overhead */
    x = &option_delta;
    do
    {
      if (*x==13)
      {
        if (++current_option == data_end) {
          goto exit_parse_error;
        }
        *x += current_option[0];
      }
      else if (*x==14)
      {
        *x += 255;
        if (++current_option == data_end) {
          goto exit_parse_error;
        }
        *x += current_option[0]<<8;
        if (++current_option == data_end) {
          goto exit_parse_error;
        }
        *x += current_option[0];
      }
    }
    while (x != &option_length && (x = &option_length));

    option_number += option_delta;

    if (++current_option + option_length > data_end)
    {
        PRINTF("OPTION %u (delta %u, len %u) has invalid length.\n", option_number, option_delta, option_length);
        goto exit_parse_error;
    }
    else
    {
        PRINTF("OPTION %u (delta %u, len %u): ", option_number, option_delta, option_length);
    }

    SET_OPTION(coap_pkt, option_number);

    switch (option_number)
    {
      case COAP_OPTION_CONTENT_TYPE:
        if (option_length > 2) {
            goto exit_parse_error;
        }
        coap_pkt->content_type = (coap_content_type_t) coap_parse_int_option(current_option, option_length);
        PRINTF("Content-Format [%u]\n", coap_pkt->content_type);
        break;
      case COAP_OPTION_MAX_AGE:
        coap_pkt->max_age = coap_parse_int_option(current_option, option_length);
        PRINTF("Max-Age [%lu]\n", coap_pkt->max_age);
        break;
      case COAP_OPTION_ETAG:
        coap_pkt->etag_len = (uint8_t)(MIN(COAP_ETAG_LEN, option_length));
        memcpy(coap_pkt->etag, current_option, coap_pkt->etag_len);
        PRINTF("ETag %u [0x%02X", coap_pkt->etag_len);
        for (i = 1; i < coap_pkt->etag_len; i++) {
          PRINTF("%02X", coap_pkt->etag[i]);
        }
        PRINTF("]\n");
        break;
      case COAP_OPTION_ACCEPT:
        if (coap_pkt->accept_num < COAP_MAX_ACCEPT_NUM)
        {
          coap_pkt->accept[coap_pkt->accept_num] = coap_parse_int_option(current_option, option_length);
          coap_pkt->accept_num += 1;
          PRINTF("Accept [%u]\n", coap_pkt->content_type);
        }
        break;
      case COAP_OPTION_IF_MATCH:
        /*FIXME support multiple ETags */
        coap_pkt->if_match_len = (uint8_t)(MIN(COAP_ETAG_LEN, option_length));
        memcpy(coap_pkt->if_match, current_option, coap_pkt->if_match_len);
        PRINTF("If-Match %u [0x%02X", coap_pkt->if_match_len);
        for (i = 1; i < coap_pkt->if_match_len; i++) {
          PRINTF("%02X", coap_pkt->if_match[i]);
        }
        PRINTF("]\n");
        break;
      case COAP_OPTION_IF_NONE_MATCH:
        coap_pkt->if_none_match = 1;
        PRINTF("If-None-Match\n");
        break;

      case COAP_OPTION_URI_HOST:
        coap_pkt->uri_host = current_option;
        coap_pkt->uri_host_len = option_length;
        PRINTF("Uri-Host [%.*s]\n", coap_pkt->uri_host_len, coap_pkt->uri_host);
        break;
      case COAP_OPTION_URI_PORT:
        coap_pkt->uri_port = coap_parse_int_option(current_option, option_length);
        PRINTF("Uri-Port [%u]\n", coap_pkt->uri_port);
        break;
      case COAP_OPTION_URI_PATH:
        /* coap_merge_multi_option() operates in-place on the IPBUF, but final packet field should be const string -> cast to string */
        // coap_merge_multi_option( (char **) &(coap_pkt->uri_path), &(coap_pkt->uri_path_len), current_option, option_length, 0);
        coap_add_multi_option( &(coap_pkt->uri_path), current_option, option_length, 1);
        PRINTF("Uri-Path [%.*s]\n", option_length, current_option);
        break;
      case COAP_OPTION_URI_QUERY:
        /* coap_merge_multi_option() operates in-place on the IPBUF, but final packet field should be const string -> cast to string */
        // coap_merge_multi_option( (char **) &(coap_pkt->uri_query), &(coap_pkt->uri_query_len), current_option, option_length, '&');
        coap_add_multi_option( &(coap_pkt->uri_query), current_option, option_length, 1);
        PRINTF("Uri-Query [%.*s]\n", option_length, current_option);
        break;

      case COAP_OPTION_LOCATION_PATH:
        coap_add_multi_option( &(coap_pkt->location_path), current_option, option_length, 1);
        break;
      case COAP_OPTION_LOCATION_QUERY:
        /* coap_merge_multi_option() operates in-place on the IPBUF, but final packet field should be const string -> cast to string */
        coap_merge_multi_option( &(coap_pkt->location_query), &(coap_pkt->location_query_len), current_option, option_length, '&');
        PRINTF("Location-Query [%.*s]\n", option_length, current_option);
        break;

      case COAP_OPTION_PROXY_URI:
        /*FIXME check for own end-point */
        coap_pkt->proxy_uri = current_option;
        coap_pkt->proxy_uri_len = option_length;
        /*TODO length > 270 not implemented (actually not required) */
        PRINTF("Proxy-Uri NOT IMPLEMENTED [%.*s]\n", coap_pkt->proxy_uri_len, coap_pkt->proxy_uri);
        coap_error_message = "This is a constrained server (Contiki)";
        return PROXYING_NOT_SUPPORTED_5_05;

      case COAP_OPTION_OBSERVE:
        coap_pkt->observe = coap_parse_int_option(current_option, option_length);
        PRINTF("Observe [%lu]\n", coap_pkt->observe);
        break;
      case COAP_OPTION_BLOCK2:
        coap_pkt->block2_num = coap_parse_int_option(current_option, option_length);
        coap_pkt->block2_more = (coap_pkt->block2_num & 0x08)>>3;
        coap_pkt->block2_size = 16 << (coap_pkt->block2_num & 0x07);
        coap_pkt->block2_offset = (coap_pkt->block2_num & ~0x0000000F)<<(coap_pkt->block2_num & 0x07);
        coap_pkt->block2_num >>= 4;
        PRINTF("Block2 [%lu%s (%u B/blk)]\n", coap_pkt->block2_num, coap_pkt->block2_more ? "+" : "", coap_pkt->block2_size);
        break;
      case COAP_OPTION_BLOCK1:
        coap_pkt->block1_num = coap_parse_int_option(current_option, option_length);
        coap_pkt->block1_more = (coap_pkt->block1_num & 0x08)>>3;
        coap_pkt->block1_size = 16 << (coap_pkt->block1_num & 0x07);
        coap_pkt->block1_offset = (coap_pkt->block1_num & ~0x0000000F)<<(coap_pkt->block1_num & 0x07);
        coap_pkt->block1_num >>= 4;
        PRINTF("Block1 [%lu%s (%u B/blk)]\n", coap_pkt->block1_num, coap_pkt->block1_more ? "+" : "", coap_pkt->block1_size);
        break;
      case COAP_OPTION_SIZE:
        coap_pkt->size = coap_parse_int_option(current_option, option_length);
        PRINTF("Size [%lu]\n", coap_pkt->size);
        break;
      default:
        PRINTF("unknown (%u)\n", option_number);
        /* Check if critical (odd) */
        if (option_number & 1)
        {
          coap_error_message = "Unsupported critical option";
          coap_free_header(coap_pkt);
          return BAD_OPTION_4_02;
        }
    }

    current_option += option_length;
  } /* for */
  PRINTF("-Done parsing-------\n");



  return NO_ERROR;

exit_parse_error:
  coap_free_header(coap_pkt);
  coap_error_message = "Invalid COAP message";
  return BAD_REQUEST_4_00;
}
/*-----------------------------------------------------------------------------------*/
/*- REST FRAMEWORK FUNCTIONS --------------------------------------------------------*/
/*-----------------------------------------------------------------------------------*/
int
coap_get_query_variable(void *packet, const char *name, const char **output)
{
    /* unused parameters */
    (void)packet;
    (void)name;
    (void)output;
/*
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (IS_OPTION(coap_pkt, COAP_OPTION_URI_QUERY)) {
    return coap_get_variable(coap_pkt->uri_query, coap_pkt->uri_query_len, name, output);
  }
*/
  return 0;
}

int
coap_get_post_variable(void *packet, const char *name, const char **output)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (coap_pkt->payload_len) {
    return coap_get_variable(coap_pkt->payload, coap_pkt->payload_len, name, output);
  }
  return 0;
}
/*-----------------------------------------------------------------------------------*/
int
coap_set_status_code(void *packet, unsigned int code)
{
  if (code <= 0xFF)
  {
    ((coap_packet_t *)packet)->code = (uint8_t) code;
    return 1;
  }
  else
  {
    return 0;
  }
}
/*-----------------------------------------------------------------------------------*/
/*- HEADER OPTION GETTERS AND SETTERS -----------------------------------------------*/
/*-----------------------------------------------------------------------------------*/
unsigned int
coap_get_header_content_type(void *packet)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (!IS_OPTION(coap_pkt, COAP_OPTION_CONTENT_TYPE)) return -1;

  return coap_pkt->content_type;
}

int
coap_set_header_content_type(void *packet, unsigned int content_type)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  coap_pkt->content_type = (coap_content_type_t) content_type;
  SET_OPTION(coap_pkt, COAP_OPTION_CONTENT_TYPE);
  return 1;
}
/*-----------------------------------------------------------------------------------*/
int
coap_get_header_accept(void *packet, const uint16_t **accept)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (!IS_OPTION(coap_pkt, COAP_OPTION_ACCEPT)) return 0;

  *accept = coap_pkt->accept;
  return coap_pkt->accept_num;
}

int
coap_set_header_accept(void *packet, uint16_t accept)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (coap_pkt->accept_num < COAP_MAX_ACCEPT_NUM)
  {
    coap_pkt->accept[coap_pkt->accept_num] = accept;
    coap_pkt->accept_num += 1;

    SET_OPTION(coap_pkt, COAP_OPTION_ACCEPT);
  }
  return coap_pkt->accept_num;
}
/*-----------------------------------------------------------------------------------*/
int
coap_get_header_max_age(void *packet, uint32_t *age)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (!IS_OPTION(coap_pkt, COAP_OPTION_MAX_AGE)) {
    *age = COAP_DEFAULT_MAX_AGE;
  } else {
    *age = coap_pkt->max_age;
  }
  return 1;
}

int
coap_set_header_max_age(void *packet, uint32_t age)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  coap_pkt->max_age = age;
  SET_OPTION(coap_pkt, COAP_OPTION_MAX_AGE);
  return 1;
}
/*-----------------------------------------------------------------------------------*/
int
coap_get_header_etag(void *packet, const uint8_t **etag)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (!IS_OPTION(coap_pkt, COAP_OPTION_ETAG)) return 0;

  *etag = coap_pkt->etag;
  return coap_pkt->etag_len;
}

int
coap_set_header_etag(void *packet, const uint8_t *etag, size_t etag_len)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  coap_pkt->etag_len = (uint8_t)(MIN(COAP_ETAG_LEN, etag_len));
  memcpy(coap_pkt->etag, etag, coap_pkt->etag_len);

  SET_OPTION(coap_pkt, COAP_OPTION_ETAG);
  return coap_pkt->etag_len;
}
/*-----------------------------------------------------------------------------------*/
/*FIXME support multiple ETags */
int
coap_get_header_if_match(void *packet, const uint8_t **etag)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (!IS_OPTION(coap_pkt, COAP_OPTION_IF_MATCH)) return 0;

  *etag = coap_pkt->if_match;
  return coap_pkt->if_match_len;
}

int
coap_set_header_if_match(void *packet, const uint8_t *etag, size_t etag_len)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  coap_pkt->if_match_len = (uint8_t)(MIN(COAP_ETAG_LEN, etag_len));
  memcpy(coap_pkt->if_match, etag, coap_pkt->if_match_len);

  SET_OPTION(coap_pkt, COAP_OPTION_IF_MATCH);
  return coap_pkt->if_match_len;
}
/*-----------------------------------------------------------------------------------*/
int
coap_get_header_if_none_match(void *packet)
{
  return IS_OPTION((coap_packet_t *)packet, COAP_OPTION_IF_NONE_MATCH) ? 1 : 0;
}

int
coap_set_header_if_none_match(void *packet)
{
  SET_OPTION((coap_packet_t *)packet, COAP_OPTION_IF_NONE_MATCH);
  return 1;
}
/*-----------------------------------------------------------------------------------*/
int
coap_get_header_token(void *packet, uint8_t **token)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (!IS_OPTION(coap_pkt, COAP_OPTION_TOKEN)) return 0;

  *token = coap_pkt->token;
  return coap_pkt->token_len;
}

int
coap_set_header_token(void *packet, const uint8_t *token, size_t token_len)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  coap_pkt->token_len = (uint8_t)(MIN(COAP_TOKEN_LEN, token_len));
  memcpy(coap_pkt->token, token, coap_pkt->token_len);

  SET_OPTION(coap_pkt, COAP_OPTION_TOKEN);
  return coap_pkt->token_len;
}
/*-----------------------------------------------------------------------------------*/
int
coap_get_header_proxy_uri(void *packet, const char **uri)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (!IS_OPTION(coap_pkt, COAP_OPTION_PROXY_URI)) return 0;

  *uri = (const char *)coap_pkt->proxy_uri;
  return coap_pkt->proxy_uri_len;
}

int
coap_set_header_proxy_uri(void *packet, const char *uri)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  coap_pkt->proxy_uri = (uint8_t *)uri;
  coap_pkt->proxy_uri_len = strlen(uri);

  SET_OPTION(coap_pkt, COAP_OPTION_PROXY_URI);
  return coap_pkt->proxy_uri_len;
}
/*-----------------------------------------------------------------------------------*/
int
coap_get_header_uri_host(void *packet, const char **host)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (!IS_OPTION(coap_pkt, COAP_OPTION_URI_HOST)) return 0;

  *host = (char *)coap_pkt->uri_host;
  return coap_pkt->uri_host_len;
}

int
coap_set_header_uri_host(void *packet, const char *host)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  coap_pkt->uri_host = (uint8_t *)host;
  coap_pkt->uri_host_len = strlen(host);

  SET_OPTION(coap_pkt, COAP_OPTION_URI_HOST);
  return coap_pkt->uri_host_len;
}
/*-----------------------------------------------------------------------------------*/
int
coap_get_header_uri_path(void *packet, const char **path)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (!IS_OPTION(coap_pkt, COAP_OPTION_URI_PATH)) return 0;

  *path = NULL; //coap_pkt->uri_path;
  return 0; //coap_pkt->uri_path_len;
}

int
coap_set_header_uri_path(void *packet, const char *path)
{
  coap_packet_t *coap_pkt = (coap_packet_t *) packet;
  int length = 0;

  free_multi_option(coap_pkt->uri_path);
  coap_pkt->uri_path = NULL;

  if (path[0]=='/') ++path;

  do
  {
      int i = 0;

      while (path[i] != 0 && path[i] != '/') i++;
      coap_add_multi_option(&(coap_pkt->uri_path), (uint8_t *)path, i, 0);

      if (path[i] == '/') i++;
      path += i;
      length += i;
  } while (path[0] != 0);

  SET_OPTION(coap_pkt, COAP_OPTION_URI_PATH);
  return length;
}

int
coap_set_header_uri_path_segment(void *packet, const char *segment)
{
  coap_packet_t *coap_pkt = (coap_packet_t *) packet;
  int length;

  if (segment == NULL || segment[0] == 0)
  {
      coap_add_multi_option(&(coap_pkt->uri_path), NULL, 0, 1);
      length = 0;
  }
  else
  {
      length = strlen(segment);
      coap_add_multi_option(&(coap_pkt->uri_path), (uint8_t *)segment, length, 0);
  }

  SET_OPTION(coap_pkt, COAP_OPTION_URI_PATH);
  return length;
}
/*-----------------------------------------------------------------------------------*/
uint16_t
coap_get_header_uri_port(void *packet)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (!IS_OPTION(coap_pkt, COAP_OPTION_URI_PORT)) return 0;

  return coap_pkt->uri_port;
}

void
coap_set_header_uri_port(void *packet, uint16_t port)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  coap_pkt->uri_port = port;
  SET_OPTION(coap_pkt, COAP_OPTION_URI_PORT);
}
/*-----------------------------------------------------------------------------------*/
int
coap_get_header_uri_query(void *packet, const char **query)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (!IS_OPTION(coap_pkt, COAP_OPTION_URI_QUERY)) return 0;

  *query = NULL; //coap_pkt->uri_query;
  return 0; //coap_pkt->uri_query_len;
}

int
coap_set_header_uri_query(void *packet, const char *query)
{
    int length = 0;
    coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

    free_multi_option(coap_pkt->uri_query);
    coap_pkt->uri_query = NULL;

    if (query[0]=='?') ++query;

    do
    {
        int i = 0;

        while (query[i] != 0 && query[i] != '&') i++;
        coap_add_multi_option(&(coap_pkt->uri_query), (uint8_t *)query, i, 0);

        if (query[i] == '&') i++;
        query += i;
        length += i;
    } while (query[0] != 0);

    SET_OPTION(coap_pkt, COAP_OPTION_URI_QUERY);
    return length;
 }

int
coap_set_header_uri_query_segment(void *packet, const char *segment)
{
  coap_packet_t *coap_pkt = (coap_packet_t *) packet;
  int length;

  if (segment == NULL || segment[0] == 0)
  {
      coap_add_multi_option(&(coap_pkt->uri_query), NULL, 0, 1);
      length = 0;
  }
  else
  {
      length = strlen(segment);
      coap_add_multi_option(&(coap_pkt->uri_query), (uint8_t *)segment, length, 0);
  }

  SET_OPTION(coap_pkt, COAP_OPTION_URI_QUERY);
  return length;
}

/*-----------------------------------------------------------------------------------*/
int
coap_get_header_location_path(void *packet, const char **path)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (!IS_OPTION(coap_pkt, COAP_OPTION_LOCATION_PATH)) return 0;

  *path = NULL; //coap_pkt->location_path;
  return 0; //coap_pkt->location_path_len;
}

int
coap_set_header_location_path(void *packet, const char *path)
{
    coap_packet_t *coap_pkt = (coap_packet_t *) packet;
    int length = 0;

    free_multi_option(coap_pkt->location_path);
    coap_pkt->location_path = NULL;

    if (path[0]=='/') ++path;

    do
    {
        int i = 0;

        while (path[i] != 0 && path[i] != '/') i++;
        coap_add_multi_option(&(coap_pkt->location_path), (uint8_t *)path, i, 0);

        if (path[i] == '/') i++;
        path += i;
        length += i;
    } while (path[0] != 0);

    SET_OPTION(coap_pkt, COAP_OPTION_LOCATION_PATH);
    return length;
}
/*-----------------------------------------------------------------------------------*/
int
coap_get_header_location_query(void *packet, const char **query)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (!IS_OPTION(coap_pkt, COAP_OPTION_LOCATION_QUERY)) return 0;

  *query = (const char*)coap_pkt->location_query;
  return coap_pkt->location_query_len;
}

int
coap_set_header_location_query(void *packet, char *query)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  while (query[0]=='?') ++query;

  coap_pkt->location_query = (uint8_t *)query;
  coap_pkt->location_query_len = strlen(query);

  SET_OPTION(coap_pkt, COAP_OPTION_LOCATION_QUERY);
  return coap_pkt->location_query_len;
}
/*-----------------------------------------------------------------------------------*/
int
coap_get_header_observe(void *packet, uint32_t *observe)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (!IS_OPTION(coap_pkt, COAP_OPTION_OBSERVE)) return 0;

  *observe = coap_pkt->observe;
  return 1;
}

int
coap_set_header_observe(void *packet, uint32_t observe)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  coap_pkt->observe = 0x00FFFFFF & observe;
  SET_OPTION(coap_pkt, COAP_OPTION_OBSERVE);
  return 1;
}
/*-----------------------------------------------------------------------------------*/
int
coap_get_header_block2(void *packet, uint32_t *num, uint8_t *more, uint16_t *size, uint32_t *offset)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (!IS_OPTION(coap_pkt, COAP_OPTION_BLOCK2)) return 0;

  /* pointers may be NULL to get only specific block parameters */
  if (num!=NULL) *num = coap_pkt->block2_num;
  if (more!=NULL) *more = coap_pkt->block2_more;
  if (size!=NULL) *size = coap_pkt->block2_size;
  if (offset!=NULL) *offset = coap_pkt->block2_offset;

  return 1;
}

int
coap_set_header_block2(void *packet, uint32_t num, uint8_t more, uint16_t size)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (size<16) return 0;
  if (size>2048) return 0;
  if (num>0x0FFFFF) return 0;

  coap_pkt->block2_num = num;
  coap_pkt->block2_more = more ? 1 : 0;
  coap_pkt->block2_size = size;

  SET_OPTION(coap_pkt, COAP_OPTION_BLOCK2);
  return 1;
}
/*-----------------------------------------------------------------------------------*/
int
coap_get_header_block1(void *packet, uint32_t *num, uint8_t *more, uint16_t *size, uint32_t *offset)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (!IS_OPTION(coap_pkt, COAP_OPTION_BLOCK1)) return 0;

  /* pointers may be NULL to get only specific block parameters */
  if (num!=NULL) *num = coap_pkt->block1_num;
  if (more!=NULL) *more = coap_pkt->block1_more;
  if (size!=NULL) *size = coap_pkt->block1_size;
  if (offset!=NULL) *offset = coap_pkt->block1_offset;

  return 1;
}

int
coap_set_header_block1(void *packet, uint32_t num, uint8_t more, uint16_t size)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (size<16) return 0;
  if (size>2048) return 0;
  if (num>0x0FFFFF) return 0;

  coap_pkt->block1_num = num;
  coap_pkt->block1_more = more;
  coap_pkt->block1_size = size;

  SET_OPTION(coap_pkt, COAP_OPTION_BLOCK1);
  return 1;
}

int
coap_get_header_block(void *packet, uint32_t *num, uint8_t *more, uint16_t *size, uint32_t *offset)
{
    if (1 == coap_get_header_block1(packet, num, more, size, offset))
    {
        return 1;
    }

    return coap_get_header_block1(packet, num, more, size, offset);
}


/*-----------------------------------------------------------------------------------*/
int
coap_get_header_size(void *packet, uint32_t *size)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (!IS_OPTION(coap_pkt, COAP_OPTION_SIZE)) return 0;
  
  *size = coap_pkt->size;
  return 1;
}

int
coap_set_header_size(void *packet, uint32_t size)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  coap_pkt->size = size;
  SET_OPTION(coap_pkt, COAP_OPTION_SIZE);
  return 1;
}
/*-----------------------------------------------------------------------------------*/
/*- PAYLOAD -------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------*/
size_t coap_get_payload(void *packet, const uint8_t **payload)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  if (coap_pkt->payload) {
    *payload = coap_pkt->payload;
    return coap_pkt->payload_len;
  } else {
    *payload = NULL;
    return 0;
  }
}

size_t coap_set_payload(void *packet, const void *payload, size_t length)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *) packet;

  coap_pkt->payload = (uint8_t *) payload;
  coap_pkt->payload_len = (uint16_t)(length);

  return coap_pkt->payload_len;
}
/*-----------------------------------------------------------------------------------*/
