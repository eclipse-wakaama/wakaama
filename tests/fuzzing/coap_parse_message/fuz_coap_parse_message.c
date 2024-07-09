/*******************************************************************************
 *
 * Copyright (c) 2023 GARDENA GmbH
 *
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
 *   Lukas Woodtli, GARDENA GmbH - Please refer to git log
 *
 *******************************************************************************/

#include "er-coap-13/er-coap-13.h"

#include <stdint.h>
#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (0 >= size || size > UINT16_MAX) {
        return 0;
    }

    uint8_t non_const_data[size];
    memcpy(non_const_data, data, size);

    coap_packet_t coap_pkt;
    memset(&coap_pkt, 0, sizeof(coap_packet_t));
    coap_parse_message(&coap_pkt, non_const_data, (uint16_t)size);
    coap_free_header(&coap_pkt);

    return 0;
}
