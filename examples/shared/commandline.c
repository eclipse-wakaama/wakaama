/*******************************************************************************
 *
 * Copyright (c) 2013, 2014, 2015 Intel Corporation and others.
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
 *    David Navarro, Intel Corporation - initial API and implementation
 *    Fabien Fleutot - Please refer to git log
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
 *
 *******************************************************************************/

#include "liblwm2m.h"
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "commandline.h"

#define HELP_COMMAND "help"
#define HELP_DESC    "Type '"HELP_COMMAND" [COMMAND]' for more details on a command."
#define UNKNOWN_CMD_MSG "Unknown command. Type '"HELP_COMMAND"' for help."


static command_desc_t * prv_find_command(command_desc_t * commandArray,
                                         char * buffer,
                                         size_t length)
{
    int i;

    if (length == 0) return NULL;

    i = 0;
    while (commandArray[i].name != NULL
        && (strlen(commandArray[i].name) != length || strncmp(buffer, commandArray[i].name, length)))
    {
        i++;
    }

    if (commandArray[i].name == NULL)
    {
        return NULL;
    }
    else
    {
        return &commandArray[i];
    }
}

static void prv_displayHelp(command_desc_t * commandArray,
                            char * buffer)
{
    command_desc_t * cmdP;
    int length;

    // find end of first argument
    length = 0;
    while (buffer[length] != 0 && !isspace(buffer[length]&0xff))
        length++;

    cmdP = prv_find_command(commandArray, buffer, length);

    if (cmdP == NULL)
    {
        int i;

        fprintf(stdout, HELP_COMMAND"\t"HELP_DESC"\r\n");

        for (i = 0 ; commandArray[i].name != NULL ; i++)
        {
            fprintf(stdout, "%s\t%s\r\n", commandArray[i].name, commandArray[i].shortDesc);
        }
    }
    else
    {
        fprintf(stdout, "%s\r\n", cmdP->longDesc?cmdP->longDesc:cmdP->shortDesc);
    }
}


void handle_command(lwm2m_context_t *lwm2mH,
                    command_desc_t * commandArray,
                    char * buffer)
{
    command_desc_t * cmdP;
    int length;

    // find end of command name
    length = 0;
    while (buffer[length] != 0 && !isspace(buffer[length]&0xFF))
        length++;

    cmdP = prv_find_command(commandArray, buffer, length);
    if (cmdP != NULL)
    {
        while (buffer[length] != 0 && isspace(buffer[length]&0xFF))
            length++;
        cmdP->callback(lwm2mH, buffer + length, cmdP->userData);
    }
    else
    {
        if (!strncmp(buffer, HELP_COMMAND, length))
        {
            while (buffer[length] != 0 && isspace(buffer[length]&0xFF))
                length++;
            prv_displayHelp(commandArray, buffer + length);
        }
        else
        {
            fprintf(stdout, UNKNOWN_CMD_MSG"\r\n");
        }
    }
}

static char* prv_end_of_space(char* buffer)
{
    while (isspace(buffer[0]&0xff))
    {
        buffer++;
    }
    return buffer;
}

char* get_end_of_arg(char* buffer)
{
    while (buffer[0] != 0 && !isspace(buffer[0]&0xFF))
    {
        buffer++;
    }
    return buffer;
}

char * get_next_arg(char * buffer, char** end)
{
    // skip arg
    buffer = get_end_of_arg(buffer);
    // skip space
    buffer = prv_end_of_space(buffer);
    if (NULL != end)
    {
        *end = get_end_of_arg(buffer);
    }

    return buffer;
}

int check_end_of_args(char* buffer)
{
    buffer = prv_end_of_space(buffer);

    return (0 == buffer[0]);
}

/**********************************************************
 * Display Functions
 */

static void print_indent(FILE * stream,
                         int num)
{
    int i;

    for ( i = 0 ; i < num ; i++)
        fprintf(stream, "    ");
}

void output_buffer(FILE *stream, const uint8_t *buffer, size_t length, int indent) {
    size_t i;

    if (length == 0) fprintf(stream, "\n");

    if (buffer == NULL) return;

    i = 0;
    while (i < length)
    {
        uint8_t array[16];
        int j;

        print_indent(stream, indent);
        memcpy(array, buffer+i, 16);
        for (j = 0 ; j < 16 && i+j < length; j++)
        {
            fprintf(stream, "%02X ", array[j]);
            if (j%4 == 3) fprintf(stream, " ");
        }
        if (length > 16)
        {
            while (j < 16)
            {
                fprintf(stream, "   ");
                if (j%4 == 3) fprintf(stream, " ");
                j++;
            }
        }
        fprintf(stream, " ");
        for (j = 0 ; j < 16 && i+j < length; j++)
        {
            if (isprint(array[j]))
                fprintf(stream, "%c", array[j]);
            else
                fprintf(stream, ".");
        }
        fprintf(stream, "\n");
        i += 16;
    }
}

void output_tlv(FILE * stream,
                uint8_t * buffer,
                size_t buffer_len,
                int indent)
{
#ifdef LWM2M_SUPPORT_TLV
    lwm2m_data_type_t type;
    uint16_t id;
    size_t dataIndex;
    size_t dataLen;
    int length = 0;
    int result;

    while (0 != (result = lwm2m_decode_TLV((uint8_t*)buffer + length, buffer_len - length, &type, &id, &dataIndex, &dataLen)))
    {
        print_indent(stream, indent);
        fprintf(stream, "{\r\n");
        print_indent(stream, indent+1);
        fprintf(stream, "ID: %d", id);

        fprintf(stream, " type: ");
        switch (type)
        {
        case LWM2M_TYPE_OBJECT_INSTANCE:
            fprintf(stream, "Object Instance");
            break;
        case LWM2M_TYPE_MULTIPLE_RESOURCE:
            fprintf(stream, "Multiple Instances");
            break;
        case LWM2M_TYPE_OPAQUE:
            fprintf(stream, "Resource Value");
            break;
        default:
            printf("unknown (%d)", (int)type);
            break;
        }
        fprintf(stream, "\n");

        print_indent(stream, indent+1);
        fprintf(stream, "{\n");
        if (type == LWM2M_TYPE_OBJECT_INSTANCE || type == LWM2M_TYPE_MULTIPLE_RESOURCE)
        {
            output_tlv(stream, buffer + length + dataIndex, dataLen, indent+1);
        }
        else
        {
            int64_t intValue;
            double floatValue;
            uint8_t tmp;

            print_indent(stream, indent+2);
            fprintf(stream, "data (%zu bytes):\r\n", dataLen);
            output_buffer(stream, (uint8_t*)buffer + length + dataIndex, dataLen, indent+2);

            tmp = buffer[length + dataIndex + dataLen];
            buffer[length + dataIndex + dataLen] = 0;
            if (0 < sscanf((const char *)buffer + length + dataIndex, "%"PRId64, &intValue))
            {
                print_indent(stream, indent+2);
                fprintf(stream, "data as Integer: %" PRId64 "\r\n", intValue);
            }
            if (0 < sscanf((const char*)buffer + length + dataIndex, "%lg", &floatValue))
            {
                print_indent(stream, indent+2);
                fprintf(stream, "data as Float: %.16g\r\n", floatValue);
            }
            buffer[length + dataIndex + dataLen] = tmp;
        }
        print_indent(stream, indent+1);
        fprintf(stream, "}\r\n");
        length += result;
        print_indent(stream, indent);
        fprintf(stream, "}\r\n");
    }
#else
    /* Unused parameters */
    (void)buffer;
    (void)buffer_len;

    print_indent(stream, indent);
    fprintf(stream, "Unsupported.\r\n");
#endif
}

#ifdef LWM2M_SUPPORT_SENML_CBOR
#define CBOR_UNSIGNED_INTEGER 0
#define CBOR_NEGATIVE_INTEGER 1
#define CBOR_BYTE_STRING 2
#define CBOR_TEXT_STRING 3
#define CBOR_ARRAY 4
#define CBOR_MAP 5
#define CBOR_SEMANTIC_TAG 6
#define CBOR_FLOATING_OR_SIMPLE 7

#define CBOR_AI_ONE_BYTE_VALUE 24
#define CBOR_AI_TWO_BYTE_VALUE 25
#define CBOR_AI_FOUR_BYTE_VALUE 26
#define CBOR_AI_EIGHT_BYTE_VALUE 27
#define CBOR_AI_INDEFINITE_OR_BREAK 31

#define CBOR_SIMPLE_FALSE 20
#define CBOR_SIMPLE_TRUE 21
#define CBOR_SIMPLE_NULL 22
#define CBOR_SIMPLE_UNDEFINED 23

static int prv_output_cbor_indefinite(FILE *stream, uint8_t *buffer, size_t buffer_len, bool breakable, uint8_t *mt);

static double prv_convert_half(uint16_t half) {
    double result;
    int exp = (half >> 10) & 0x1f;
    int mant = half & 0x3ff;
    if (exp == 0) {
        result = ldexp(mant, -24);
    } else if (exp != 31) {
        result = ldexp(mant + 1024, exp - 25);
    } else if (mant == 0) {
        result = INFINITY;
    } else {
        result = NAN;
    }
    if ((half & 0x8000) != 0) {
        result = -result;
    }
    return result;
}

static void prv_output_cbor_float(FILE *stream, double val) {
    _Pragma("GCC diagnostic push");
    _Pragma("GCC diagnostic ignored \"-Wfloat-equal\"");
    if (val != val) {
        fprintf(stream, "NaN");
    } else if (val == INFINITY) {
        fprintf(stream, "Infinity");
    } else if (val == -INFINITY) {
        fprintf(stream, "-Infinity");
    } else {
        fprintf(stream, "%g", val);
    }
    _Pragma("GCC diagnostic pop");
}

static int prv_output_cbor_definite(FILE *stream, uint8_t *buffer, size_t buffer_len, bool breakable, uint8_t *mt) {
    int head;
    int res;
    uint8_t ai;
    union {
        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;
        float f;
        double d;
    } val;
    uint8_t valbits;
    uint64_t i;
    uint64_t u64;
    uint64_t i64;

    *mt = buffer[0] >> 5;
    ai = buffer[0] & 0x1f;
    head = 1;

    switch (ai) {
    default:
        valbits = 8;
        val.u8 = ai;
        break;
    case CBOR_AI_ONE_BYTE_VALUE:
        valbits = 8;
        val.u8 = buffer[head++];
        break;
    case CBOR_AI_TWO_BYTE_VALUE:
        valbits = 16;
        val.u16 = ((uint16_t)buffer[head++]) << 8;
        val.u16 += buffer[head++];
        break;
    case CBOR_AI_FOUR_BYTE_VALUE:
        valbits = 32;
        val.u32 = ((uint32_t)buffer[head++]) << 24;
        val.u32 = ((uint32_t)buffer[head++]) << 16;
        val.u32 = ((uint32_t)buffer[head++]) << 8;
        val.u32 += buffer[head++];
        break;
    case CBOR_AI_EIGHT_BYTE_VALUE:
        valbits = 64;
        val.u64 = ((uint64_t)buffer[head++]) << 56;
        val.u64 = ((uint64_t)buffer[head++]) << 48;
        val.u64 = ((uint64_t)buffer[head++]) << 40;
        val.u64 = ((uint64_t)buffer[head++]) << 32;
        val.u64 = ((uint64_t)buffer[head++]) << 24;
        val.u64 = ((uint64_t)buffer[head++]) << 16;
        val.u64 = ((uint64_t)buffer[head++]) << 8;
        val.u64 += buffer[head++];
        break;
    case 28:
    case 29:
    case 30:
        // Invalid
        return -1;
    case CBOR_AI_INDEFINITE_OR_BREAK:
        res = prv_output_cbor_indefinite(stream, buffer + head, buffer_len - head, breakable, mt);
        if (res < 0)
            return res;
        return head + res;
    }

    switch (valbits) {
    case 8:
        u64 = val.u8;
        i64 = ~(int64_t)val.u8;
        break;
    case 16:
        u64 = val.u16;
        i64 = ~(int64_t)val.u16;
        break;
    case 32:
        u64 = val.u32;
        i64 = ~(int64_t)val.u32;
        break;
    case 64:
        u64 = val.u64;
        i64 = ~(int64_t)val.u64;
        break;
    default:
        // Shouldn't be possible
        return -1;
    }

    switch (*mt) {
    case CBOR_UNSIGNED_INTEGER:
        fprintf(stream, "%" PRIu64, u64);
        break;
    case CBOR_NEGATIVE_INTEGER:
        if ((u64 >> 63) != 0) {
            // Unrepresentable
            return -1;
        } else {
            fprintf(stream, "%" PRId64, i64);
        }
        break;
    case CBOR_BYTE_STRING:
        fprintf(stream, "h'");
        if (buffer_len - head < u64)
            return -1;
        for (i = 0; i < u64; i++) {
            fprintf(stream, "%02x", buffer[head++]);
        }
        fprintf(stream, "'");
        break;
    case CBOR_TEXT_STRING:
        fprintf(stream, "\"");
        if (buffer_len - head < u64)
            return -1;
        for (i = 0; i < u64; i++) {
            fprintf(stream, "%c", buffer[head++]);
        }
        fprintf(stream, "\"");
        break;
    case CBOR_ARRAY:
        fprintf(stream, "[");
        for (i = 0; i < u64; i++) {
            if (i != 0)
                fprintf(stream, ", ");
            res = prv_output_cbor_definite(stream, buffer + head, buffer_len - head, false, mt);
            if (res <= 0)
                return -1;
            head += res;
        }
        fprintf(stream, "]");
        break;
    case CBOR_MAP:
        fprintf(stream, "{");
        for (i = 0; i < u64; i++) {
            if (i != 0)
                fprintf(stream, ", ");
            res = prv_output_cbor_definite(stream, buffer + head, buffer_len - head, false, mt);
            if (res <= 0)
                return -1;
            head += res;
            fprintf(stream, ": ");
            res = prv_output_cbor_definite(stream, buffer + head, buffer_len - head, false, mt);
            if (res <= 0)
                return -1;
            head += res;
        }
        fprintf(stream, "}");
        break;
    case CBOR_SEMANTIC_TAG:
        fprintf(stream, "%" PRIu64 "(", u64);
        res = prv_output_cbor_definite(stream, buffer + head, buffer_len - head, false, mt);
        if (res <= 0)
            return -1;
        head += res;
        fprintf(stream, ")");
        break;
    case CBOR_FLOATING_OR_SIMPLE:
        switch (ai) {
        default:
            fprintf(stream, "simple(%u)", val.u8);
            break;
        case CBOR_SIMPLE_FALSE:
            fprintf(stream, "false");
            break;
        case CBOR_SIMPLE_TRUE:
            fprintf(stream, "true");
            break;
        case CBOR_SIMPLE_NULL:
            fprintf(stream, "null");
            break;
        case CBOR_SIMPLE_UNDEFINED:
            fprintf(stream, "undefined");
            break;
        case CBOR_AI_TWO_BYTE_VALUE:
            prv_output_cbor_float(stream, prv_convert_half(val.u16));
            break;
        case CBOR_AI_FOUR_BYTE_VALUE:
            prv_output_cbor_float(stream, val.f);
            break;
        case CBOR_AI_EIGHT_BYTE_VALUE:
            prv_output_cbor_float(stream, val.d);
            break;
        case 28:
        case 29:
        case 30:
        case CBOR_AI_INDEFINITE_OR_BREAK:
            // Shouldn't be possible
            return -1;
        }
        break;
    default:
        // Should not be possible
        return -1;
    }
    return head;
}

static int prv_output_cbor_indefinite(FILE *stream, uint8_t *buffer, size_t buffer_len, bool breakable, uint8_t *mt) {
    uint8_t it;
    int head = 0;
    int res;

    switch (*mt) {
    case CBOR_BYTE_STRING:
    case CBOR_TEXT_STRING:
        fprintf(stream, "(_ ");
        while (1) {
            res = prv_output_cbor_definite(stream, buffer + head, buffer_len - head, true, &it);
            if (res <= 0)
                return -1;
            head += res;
            if (it == UINT8_MAX)
                break;
            if (it != *mt)
                return -1;
            fprintf(stream, ", ");
        }
        fprintf(stream, ")");
        break;
    case CBOR_ARRAY:
        fprintf(stream, "[_ ");
        while (1) {
            res = prv_output_cbor_definite(stream, buffer + head, buffer_len - head, true, &it);
            if (res <= 0)
                return -1;
            head += res;
            if (it == UINT8_MAX)
                break;
            fprintf(stream, ", ");
        }
        fprintf(stream, "]");
        break;
    case CBOR_MAP:
        fprintf(stream, "{_ ");
        while (1) {
            res = prv_output_cbor_definite(stream, buffer + head, buffer_len - head, true, &it);
            if (res <= 0)
                return -1;
            head += res;
            if (it == UINT8_MAX)
                break;
            fprintf(stream, ": ");
            res = prv_output_cbor_definite(stream, buffer + head, buffer_len - head, false, &it);
            if (res <= 0)
                return -1;
            head += res;
            fprintf(stream, ", ");
        }
        fprintf(stream, "}");
        break;
    case CBOR_AI_INDEFINITE_OR_BREAK:
        if (!breakable)
            return -1;
        *mt = UINT8_MAX;
        break;
    default:
        return -1;
    }
    return head;
}
#endif

void output_cbor(FILE *stream, uint8_t *buffer, size_t buffer_len, int indent) {
#ifdef LWM2M_SUPPORT_SENML_CBOR
    size_t head = 0;
    int res;
    uint8_t mt;
    print_indent(stream, indent);
    while (head < buffer_len) {
        if (head != 0)
            fprintf(stream, ", ");
        res = prv_output_cbor_definite(stream, buffer + head, buffer_len - head, false, &mt);
        if (res <= 0) {
            fprintf(stream, "Error.\r\n");
            break;
        }
        head += res;
    }
#else
    /* Unused parameters */
    (void)buffer;
    (void)buffer_len;

    print_indent(stream, indent);
    fprintf(stream, "Unsupported.\r\n");
#endif
}

void output_data(FILE *stream, block_info_t *block_info, lwm2m_media_type_t format, uint8_t *data, size_t dataLength,
                 int indent) {
    print_indent(stream, indent);
    if (block_info != NULL) {
        fprintf(stream, "block transfer: size: %d, num: %d, more: %d\n\r", block_info->block_size, block_info->block_num, block_info->block_more);
    } else {
        fprintf(stream, "non block transfer\n\r");
    }
    print_indent(stream, indent);
    fprintf(stream, "%zu bytes received of type ", dataLength);

    switch (format)
    {
    case LWM2M_CONTENT_TEXT:
        fprintf(stream, "text/plain:\r\n");
        output_buffer(stream, data, dataLength, indent);
        break;

    case LWM2M_CONTENT_OPAQUE:
        fprintf(stream, "application/octet-stream:\r\n");
        output_buffer(stream, data, dataLength, indent);
        break;

    case LWM2M_CONTENT_TLV:
        fprintf(stream, "application/vnd.oma.lwm2m+tlv:\r\n");
        output_tlv(stream, data, dataLength, indent);
        break;

    case LWM2M_CONTENT_JSON:
        fprintf(stream, "application/vnd.oma.lwm2m+json:\r\n");
        print_indent(stream, indent);
        for (size_t i = 0; i < dataLength; i++) {
            fprintf(stream, "%c", data[i]);
        }
        fprintf(stream, "\n");
        break;

    case LWM2M_CONTENT_SENML_JSON:
        fprintf(stream, "application/senml+json:\r\n");
        print_indent(stream, indent);
        for (size_t i = 0; i < dataLength; i++) {
            fprintf(stream, "%c", data[i]);
        }
        fprintf(stream, "\n");
        break;

    case LWM2M_CONTENT_CBOR:
        fprintf(stream, "application/cbor:\r\n");
        output_cbor(stream, data, dataLength, indent);
        break;

    case LWM2M_CONTENT_SENML_CBOR:
        fprintf(stream, "application/senml+cbor:\r\n");
        output_cbor(stream, data, dataLength, indent);
        break;

    case LWM2M_CONTENT_LINK:
        fprintf(stream, "application/link-format:\r\n");
        print_indent(stream, indent);
        for (size_t i = 0; i < dataLength; i++) {
            fprintf(stream, "%c", data[i]);
        }
        fprintf(stream, "\n");
        break;

    default:
        fprintf(stream, "Unknown (%d):\r\n", format);
        output_buffer(stream, data, dataLength, indent);
        break;
    }
}

void dump_tlv(FILE * stream,
              int size,
              lwm2m_data_t * dataP,
              int indent)
{
    int i;

    for(i= 0 ; i < size ; i++)
    {
        print_indent(stream, indent);
        fprintf(stream, "{\r\n");
        print_indent(stream, indent+1);
        fprintf(stream, "id: %d\r\n", dataP[i].id);

        print_indent(stream, indent+1);
        fprintf(stream, "type: ");
        switch (dataP[i].type)
        {
        case LWM2M_TYPE_OBJECT:
            fprintf(stream, "LWM2M_TYPE_OBJECT\r\n");
            dump_tlv(stream, dataP[i].value.asChildren.count, dataP[i].value.asChildren.array, indent + 1);
            break;
        case LWM2M_TYPE_OBJECT_INSTANCE:
            fprintf(stream, "LWM2M_TYPE_OBJECT_INSTANCE\r\n");
            dump_tlv(stream, dataP[i].value.asChildren.count, dataP[i].value.asChildren.array, indent + 1);
            break;
        case LWM2M_TYPE_MULTIPLE_RESOURCE:
            fprintf(stream, "LWM2M_TYPE_MULTIPLE_RESOURCE\r\n");
            dump_tlv(stream, dataP[i].value.asChildren.count, dataP[i].value.asChildren.array, indent + 1);
            break;
        case LWM2M_TYPE_UNDEFINED:
            fprintf(stream, "LWM2M_TYPE_UNDEFINED\r\n");
            break;
        case LWM2M_TYPE_STRING:
            fprintf(stream, "LWM2M_TYPE_STRING\r\n");
            print_indent(stream, indent + 1);
            fprintf(stream, "\"%.*s\"\r\n", (int)dataP[i].value.asBuffer.length, dataP[i].value.asBuffer.buffer);
            break;
        case LWM2M_TYPE_CORE_LINK:
            fprintf(stream, "LWM2M_TYPE_CORE_LINK\r\n");
            print_indent(stream, indent + 1);
            fprintf(stream, "\"%.*s\"\r\n", (int)dataP[i].value.asBuffer.length, dataP[i].value.asBuffer.buffer);
            break;
        case LWM2M_TYPE_OPAQUE:
            fprintf(stream, "LWM2M_TYPE_OPAQUE\r\n");
            output_buffer(stream, dataP[i].value.asBuffer.buffer, dataP[i].value.asBuffer.length, indent + 1);
            break;
        case LWM2M_TYPE_INTEGER:
            fprintf(stream, "LWM2M_TYPE_INTEGER: ");
            print_indent(stream, indent + 1);
            fprintf(stream, "%" PRId64, dataP[i].value.asInteger);
            fprintf(stream, "\r\n");
            break;
        case LWM2M_TYPE_UNSIGNED_INTEGER:
            fprintf(stream, "LWM2M_TYPE_UNSIGNED_INTEGER: ");
            print_indent(stream, indent + 1);
            fprintf(stream, "%" PRIu64, dataP[i].value.asUnsigned);
            fprintf(stream, "\r\n");
            break;
        case LWM2M_TYPE_FLOAT:
            fprintf(stream, "LWM2M_TYPE_FLOAT: ");
            print_indent(stream, indent + 1);
            fprintf(stream, "%" PRId64, dataP[i].value.asInteger);
            fprintf(stream, "\r\n");
            break;
        case LWM2M_TYPE_BOOLEAN:
            fprintf(stream, "LWM2M_TYPE_BOOLEAN: ");
            fprintf(stream, "%s", dataP[i].value.asBoolean ? "true" : "false");
            fprintf(stream, "\r\n");
            break;
        case LWM2M_TYPE_OBJECT_LINK:
            fprintf(stream, "LWM2M_TYPE_OBJECT_LINK\r\n");
            break;
        default:
            fprintf(stream, "unknown (%d)\r\n", (int)dataP[i].type);
            break;
        }
        print_indent(stream, indent);
        fprintf(stream, "}\r\n");
    }
}

#define CODE_TO_STRING(X)   case X : return #X

static const char* prv_status_to_string(int status)
{
    switch(status)
    {
    CODE_TO_STRING(COAP_NO_ERROR);
    CODE_TO_STRING(COAP_IGNORE);
    CODE_TO_STRING(COAP_201_CREATED);
    CODE_TO_STRING(COAP_202_DELETED);
    CODE_TO_STRING(COAP_204_CHANGED);
    CODE_TO_STRING(COAP_205_CONTENT);
    CODE_TO_STRING(COAP_400_BAD_REQUEST);
    CODE_TO_STRING(COAP_401_UNAUTHORIZED);
    CODE_TO_STRING(COAP_404_NOT_FOUND);
    CODE_TO_STRING(COAP_405_METHOD_NOT_ALLOWED);
    CODE_TO_STRING(COAP_406_NOT_ACCEPTABLE);
    CODE_TO_STRING(COAP_500_INTERNAL_SERVER_ERROR);
    CODE_TO_STRING(COAP_501_NOT_IMPLEMENTED);
    CODE_TO_STRING(COAP_503_SERVICE_UNAVAILABLE);
    default: return "";
    }
}

void print_status(FILE * stream,
                  uint8_t status)
{
    fprintf(stream, "%d.%02d (%s)", (status&0xE0)>>5, status&0x1F, prv_status_to_string(status));
}
