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

#include "CUnit/CUnit.h"
#include "liblwm2m.h"
#include "tests.h"

typedef struct list_node_ {
    struct list_node_ *next; // matches lwm2m_list_t::next
    uint16_t mID;            // matches lwm2m_list_t::id
    char data;
} list_node_t;

#define NUM_TEST_NODES 3

static list_node_t *reset_nodes(void) {
    static list_node_t test_nodes[NUM_TEST_NODES];
    memset(test_nodes, 0, sizeof(test_nodes));
    for (uint_fast8_t i = 0; i < NUM_TEST_NODES; ++i) {
        test_nodes[i].mID = 50 + i;
        test_nodes[i].data = 'a' + i;
    }

    return test_nodes;
}

static list_node_t *reset_list(void) {
    static list_node_t list;

    memset(&list, 0, sizeof(list));

    list_node_t *test_nodes = reset_nodes();
    LWM2M_LIST_ADD(&list, &test_nodes[0]);
    LWM2M_LIST_ADD(&list, &test_nodes[1]);
    LWM2M_LIST_ADD(&list, &test_nodes[2]);

    return &list;
}

static void test_list_find(void) {
    list_node_t *list = reset_list();
    lwm2m_list_t *ret = LWM2M_LIST_FIND(list, 51);
    CU_ASSERT_EQUAL(ret->id, 51);
    CU_ASSERT_EQUAL(((list_node_t *)ret)->data, 'b');
}

static void test_list_not_find(void) {
    list_node_t *list = reset_list();
    lwm2m_list_t *ret = LWM2M_LIST_FIND(list, 100);
    CU_ASSERT_PTR_NULL(ret);
}

static void test_list_rm(void) {
    list_node_t *list = reset_list();
    lwm2m_list_t *ret = LWM2M_LIST_FIND(list, 51);
    CU_ASSERT_PTR_NOT_NULL(ret);

    list_node_t *targetP;
    ret = LWM2M_LIST_RM(list, 51, &targetP);
    CU_ASSERT_PTR_NOT_NULL(ret);
    CU_ASSERT_PTR_NOT_NULL(targetP);
    CU_ASSERT_EQUAL(targetP->mID, 51);
    CU_ASSERT_EQUAL(targetP->data, 'b');
}

static void test_list_newId(void) {
    list_node_t *list = reset_list();
    const uint16_t id = lwm2m_list_newId((lwm2m_list_t *)list);
    CU_ASSERT_EQUAL(id, 1);
}

static void test_list_malloc_free(void) {
    list_node_t *list = lwm2m_malloc(sizeof(list_node_t));
    memset(list, 0, sizeof(*list));

    for (uint_fast8_t i = 0; i < 5; ++i) {
        list_node_t *node = lwm2m_malloc(sizeof(list_node_t));
        node->mID = i;
        LWM2M_LIST_ADD(list, node);
    }

    lwm2m_list_t *ret = LWM2M_LIST_FIND(list, 1);
    CU_ASSERT_PTR_NOT_NULL(ret);

    /* Functionality implicitly tested by leak sanitizer */
    LWM2M_LIST_FREE(list);
}

static struct TestTable table[] = {
    {"test_list_find", test_list_find},
    {"test_list_not_find", test_list_not_find},
    {"test_list_rm", test_list_rm},
    {"test_list_newId", test_list_newId},
    {"test_list_malloc_free", test_list_malloc_free},
    {NULL, NULL},
};

CU_ErrorCode create_list_test_suit(void) {
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite("Suite_list", NULL, NULL);
    if (NULL == pSuite) {
        return CU_get_error();
    }

    return add_tests(pSuite, table);
}
