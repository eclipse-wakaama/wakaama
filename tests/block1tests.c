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
 *   Julien Vermillard - Please refer to git log
 *   Tuve Nordius, Husqvarna Group - Please refer to git log
 *
 *******************************************************************************/

#include "tests.h"
#include "CUnit/Basic.h"
#include "internals.h"
#include "liblwm2m.h"

static const char *URI = "/1/2/3";
static const uint16_t BLOCK_SIZE = 5;

static uint8_t handle_12345(lwm2m_block_data_t **blk1, uint8_t **resultBuffer, size_t *resultLen) {
    const char *buffer = "12345";
    const size_t bufferLength = strlen(buffer);
    const int BLOCK_NUM = 0;
    const bool BLOCK_MORE = true;

    return coap_block1_handler(blk1, URI, (const uint8_t *const)buffer, bufferLength, BLOCK_SIZE, BLOCK_NUM, BLOCK_MORE,
                               resultBuffer, resultLen);
}

static uint8_t handle_67(lwm2m_block_data_t **blk1, uint8_t **resultBuffer, size_t *resultLen) {
    const char *buffer = "67";
    size_t bufferLength = strlen(buffer);

    const int BLOCK_NUM = 1;
    bool BLOCK_MORE = false;
    return coap_block1_handler(blk1, URI, (const uint8_t *const)buffer, bufferLength, BLOCK_SIZE, BLOCK_NUM, BLOCK_MORE,
                               resultBuffer, resultLen);
}

static void test_block1_nominal(void)
{
    lwm2m_block_data_t * blk1 = NULL;

    uint8_t *resultBuffer = NULL;
    size_t resultLen = 0;
    uint8_t status = handle_12345(&blk1, &resultBuffer, &resultLen);
    CU_ASSERT_EQUAL(status, COAP_231_CONTINUE)
    CU_ASSERT_PTR_NOT_NULL(blk1)
    CU_ASSERT_PTR_NULL(resultBuffer)

    resultBuffer = NULL;
    resultLen = 0;
    status = handle_67(&blk1, &resultBuffer, &resultLen);
    CU_ASSERT_EQUAL(status, NO_ERROR)
    CU_ASSERT_PTR_NOT_NULL(blk1)
    CU_ASSERT_PTR_NOT_NULL(resultBuffer)
    CU_ASSERT_EQUAL(resultLen, 7)
    CU_ASSERT_NSTRING_EQUAL(resultBuffer, "1234567", 7)

    free_block_data(blk1);
}

static void test_block1_retransmit(void)
{
    lwm2m_block_data_t * blk1 = NULL;

    uint8_t *resultBuffer = NULL;
    size_t resultLen = 0;

    uint8_t status = handle_12345(&blk1, &resultBuffer, &resultLen);
    CU_ASSERT_EQUAL(status, COAP_231_CONTINUE)
    CU_ASSERT_PTR_NOT_NULL(blk1)
    CU_ASSERT_PTR_NULL(resultBuffer)

    resultBuffer = NULL;
    resultLen = 0;
    status = handle_12345(&blk1, &resultBuffer, &resultLen);
    CU_ASSERT_EQUAL(status, COAP_231_CONTINUE)
    CU_ASSERT_PTR_NULL(resultBuffer)

    resultBuffer = NULL;
    resultLen = 0;
    status = handle_67(&blk1, &resultBuffer, &resultLen);
    CU_ASSERT_EQUAL(status, NO_ERROR)
    CU_ASSERT_PTR_NOT_NULL(blk1)
    CU_ASSERT_PTR_NOT_NULL(resultBuffer)
    CU_ASSERT_EQUAL(resultLen, 7)
    CU_ASSERT_NSTRING_EQUAL(resultBuffer, "1234567", 7)

    /* Resetting `resultBuffer` or `resultLen` here gives an error. */
    status = handle_67(&blk1, &resultBuffer, &resultLen);
    CU_ASSERT_EQUAL(status, COAP_RETRANSMISSION)
    CU_ASSERT_PTR_NOT_NULL(blk1)
    CU_ASSERT_PTR_NOT_NULL(resultBuffer)
    CU_ASSERT_EQUAL(resultLen, 7)
    CU_ASSERT_NSTRING_EQUAL(resultBuffer, "1234567", 7)

    /* Resetting `resultBuffer` or `resultLen` here gives an error. */
    status = handle_67(&blk1, &resultBuffer, &resultLen);
    CU_ASSERT_EQUAL(status, COAP_RETRANSMISSION)
    CU_ASSERT_PTR_NOT_NULL(blk1)
    CU_ASSERT_PTR_NOT_NULL(resultBuffer)
    CU_ASSERT_EQUAL(resultLen, 7)
    CU_ASSERT_NSTRING_EQUAL(resultBuffer, "1234567", 7)

    free_block_data(blk1);
}
static void test_block1_same_message_after_success(void) {
    lwm2m_block_data_t *blk1 = NULL;

    uint8_t *resultBuffer = NULL;
    size_t resultLen;

    uint8_t status = handle_12345(&blk1, &resultBuffer, &resultLen);
    CU_ASSERT_EQUAL(status, COAP_231_CONTINUE)
    CU_ASSERT_PTR_NULL(resultBuffer)

    status = handle_67(&blk1, &resultBuffer, &resultLen);
    CU_ASSERT_EQUAL(status, NO_ERROR)
    CU_ASSERT_PTR_NOT_NULL(resultBuffer)
    CU_ASSERT_EQUAL(resultLen, 7)
    CU_ASSERT_NSTRING_EQUAL(resultBuffer, "1234567", 7)

    /* Same message again */
    status = handle_12345(&blk1, &resultBuffer, &resultLen);
    CU_ASSERT_EQUAL(status, COAP_231_CONTINUE)
    CU_ASSERT_PTR_NOT_NULL(resultBuffer)

    status = handle_67(&blk1, &resultBuffer, &resultLen);
    CU_ASSERT_EQUAL(status, NO_ERROR)
    CU_ASSERT_PTR_NOT_NULL(resultBuffer)
    CU_ASSERT_EQUAL(resultLen, 7)
    CU_ASSERT_NSTRING_EQUAL(resultBuffer, "1234567", 7)

    free_block_data(blk1);
}

static struct TestTable table[] = {
    {"test of test_block1_nominal()", test_block1_nominal},
    {"test of test_block1_retransmit()", test_block1_retransmit},
    {"test of test_block1_same_message_after_success()", test_block1_same_message_after_success},
    {NULL, NULL},
};

CU_ErrorCode create_block1_suit(void) {
    CU_pSuite pSuite = NULL;
    pSuite = CU_add_suite("Suite_block1", NULL, NULL);

    if (NULL == pSuite) {
        return CU_get_error();
    }
    return add_tests(pSuite, table);
}
