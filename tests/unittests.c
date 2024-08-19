/*******************************************************************************
 *
 * Copyright (c) 2015 Bosch Software Innovations GmbH, Germany.
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
 *    Bosch Software Innovations GmbH - Please refer to git log
 *
 *******************************************************************************/

#include <stdio.h>
#include <stdint.h>

#include "CUnit/Basic.h"

#include "tests.h"

// stub function
void * lwm2m_connect_server(uint16_t secObjInstID,
                            void * userData)
{
    (void)userData;
    return (void *)(uintptr_t)secObjInstID;
}

void lwm2m_close_connection(void * sessionH,
                            void * userData)
{
    (void)sessionH;
    (void)userData;
    return;
}

CU_ErrorCode add_tests(CU_pSuite pSuite, struct TestTable* testTable)
{
    int index;
    for (index = 0; NULL != testTable && NULL != testTable[index].name; ++index) {
        if (NULL == CU_add_test(pSuite, testTable[index].name, testTable[index].function)) {
            fprintf(stderr, "Failed to add test %s\n", testTable[index].name);
            return CU_get_error();
         }
    }
    return CUE_SUCCESS;
}

int main(void) {
    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    if (CUE_SUCCESS != create_block1_suit())
        goto exit;

    if (CUE_SUCCESS != create_block2_suit())
        goto exit;

    if (CUE_SUCCESS != create_convert_numbers_suit())
        goto exit;

    if (CUE_SUCCESS != create_tlv_json_suit())
        goto exit;

    if (CUE_SUCCESS != create_tlv_suit())
        goto exit;

    if (CUE_SUCCESS != create_uri_suit())
        goto exit;

#ifdef LWM2M_SUPPORT_SENML_JSON
   if (CUE_SUCCESS != create_senml_json_suit())
       goto exit;
#endif

#ifdef LWM2M_SUPPORT_SENML_CBOR
#ifdef LWM2M_VERSION_1_0
   if (CUE_SUCCESS != create_cbor_suit())
       goto exit;
#endif

   if (CUE_SUCCESS != create_senml_cbor_suit())
       goto exit;
#endif

   if (CUE_SUCCESS != create_er_coap_parse_message_suit())
       goto exit;

   if (CUE_SUCCESS != create_list_test_suit())
       goto exit;
#ifdef LWM2M_SERVER_MODE
   if (CUE_SUCCESS != create_registration_test_suit())
       goto exit;
#endif

#if LWM2M_LOG_LEVEL != LWM2M_LOG_DISABLED
   if (CUE_SUCCESS != create_logging_test_suit())
       goto exit;
#endif

   CU_basic_set_mode(CU_BRM_VERBOSE);
   CU_basic_run_tests();
   CU_basic_show_failures(CU_get_failure_list());
   printf("\n");

   if (CU_get_number_of_tests_failed() > 0) {
     return 1;
   }
   return 0;

exit:
   CU_cleanup_registry();
   return CU_get_error();
}
