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
 *    
 *******************************************************************************/

#include "liblwm2m.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "commandline.h"


int main(int argc, char *argv[])
{
    lwm2m_tlv_t * tlvP;
    int size;
    int length;
    uint8_t * buffer;

    char buffer1[] = {0x03, 0x0A, 0xC1, 0x01, 0x14, 0x03, 0x0B, 0xC1, 0x01, 0x15, 0x03, 0x0C, 0xC1, 0x01, 0x16};
    char buffer2[] = {0xC8, 0x00, 0x14, 0x4F, 0x70, 0x65, 0x6E, 0x20, 0x4D, 0x6F, 0x62, 0x69, 0x6C, 0x65, 0x20,
                      0x41, 0x6C, 0x6C, 0x69, 0x61, 0x6E, 0x63, 0x65, 0xC8, 0x01, 0x16, 0x4C, 0x69, 0x67, 0x68,
                      0x74, 0x77 , 0x65, 0x69, 0x67, 0x68, 0x74, 0x20, 0x4D, 0x32, 0x4D, 0x20, 0x43, 0x6C, 0x69,
                      0x65, 0x6E, 0x74 , 0xC8, 0x02, 0x09, 0x33, 0x34, 0x35, 0x30, 0x30, 0x30, 0x31, 0x32, 0x33,
                      0xC3, 0x03, 0x31, 0x2E , 0x30, 0x86, 0x06, 0x41, 0x00, 0x01, 0x41, 0x01, 0x05, 0x88, 0x07,
                      0x08, 0x42, 0x00, 0x0E, 0xD8 , 0x42, 0x01, 0x13, 0x88, 0x87, 0x08, 0x41, 0x00, 0x7D, 0x42,
                      0x01, 0x03, 0x84, 0xC1, 0x09, 0x64 , 0xC1, 0x0A, 0x0F, 0x83, 0x0B, 0x41, 0x00, 0x00, 0xC4,
                      0x0D, 0x51, 0x82, 0x42, 0x8F, 0xC6, 0x0E, 0x2B, 0x30, 0x32, 0x3A, 0x30, 0x30, 0xC1, 0x0F, 0x55};

    printf("Buffer 1:\n");
    output_tlv(stdout, buffer1, sizeof(buffer1), 0);
    printf("\n\nBuffer 1 using lwm2m_tlv_t:\n");
    size = lwm2m_tlv_parse(buffer1, sizeof(buffer1), &tlvP);
    dump_tlv(stdout, size, tlvP, 0);
    length = lwm2m_tlv_serialize(size, tlvP, &buffer);
    if (length != sizeof(buffer1))
    {
        printf("\n\nSerialize Buffer 1 failed: %d bytes instead of %d\n", length, sizeof(buffer1));
    }
    else if (memcmp(buffer, buffer1, length) != 0)
    {
        printf("\n\nSerialize Buffer 1 failed:\n");
        output_buffer(stdout, buffer, length, 0);
        printf("\ninstead of:\n");
        output_buffer(stdout, buffer1, length, 0);
    }
    else
    {
        printf("\n\nSerialize Buffer 1 OK\n");
    }
    lwm2m_tlv_free(size, tlvP);

    printf("\n\n============\n\nBuffer 2: \r\r\n");
    output_tlv(stdout, buffer2, sizeof(buffer2), 0);
    printf("\n\nBuffer 2 using lwm2m_tlv_t: \r\r\n");
    size = lwm2m_tlv_parse(buffer2, sizeof(buffer2), &tlvP);
    dump_tlv(stdout, size, tlvP, 0);
    length = lwm2m_tlv_serialize(size, tlvP, &buffer);
    if (length != sizeof(buffer2))
    {
        printf("\n\nSerialize Buffer 2 failed: %d bytes instead of %d\n", length, sizeof(buffer2));
    }
    else if (memcmp(buffer, buffer2, length) != 0)
    {
        printf("\n\nSerialize Buffer 2 failed:\n");
        output_buffer(stdout, buffer, length, 0);
        printf("\ninstead of:\n");
        output_buffer(stdout, buffer2, length, 0);
    }
    else
    {
        printf("\n\nSerialize Buffer 2 OK\n\n");
    }
    lwm2m_tlv_free(size, tlvP);
}

