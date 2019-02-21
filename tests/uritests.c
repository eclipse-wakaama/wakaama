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

#include "tests.h"
#include "CUnit/Basic.h"
#include "internals.h"
#include "memtest.h"


static void test_uri_decode(void)
{
    lwm2m_uri_t uri;
    lwm2m_request_type_t requestType;
    multi_option_t extraID = { .next = NULL, .is_static = 1, .len = 3, .data = (uint8_t *) "555" };
#ifndef LWM2M_VERSION_1_0
    multi_option_t riID = { .next = NULL, .is_static = 1, .len = 2, .data = (uint8_t *) "12" };
#endif
    multi_option_t rID = { .next = NULL, .is_static = 1, .len = 1, .data = (uint8_t *) "0" };
    multi_option_t iID = { .next = &rID, .is_static = 1, .len = 2, .data = (uint8_t *) "11" };
    multi_option_t oID = { .next = &iID, .is_static = 1, .len = 4, .data = (uint8_t *) "9050" };
    multi_option_t location = { .next = NULL, .is_static = 1, .len = 4, .data = (uint8_t *) "5a3f" };
    multi_option_t locationDecimal = { .next = NULL, .is_static = 1, .len = 4, .data = (uint8_t *) "5312" };
    multi_option_t reg = { .next = NULL, .is_static = 1, .len = 2, .data = (uint8_t *) "rd" };
    multi_option_t boot = { .next = NULL, .is_static = 1, .len = 2, .data = (uint8_t *) "bs" };

    MEMORY_TRACE_BEFORE;

#ifndef LWM2M_VERSION_1_0
    rID.next = &riID;
#endif

    /* "/rd" */
    requestType = uri_decode(NULL, &reg, &uri);
    CU_ASSERT_EQUAL(requestType, LWM2M_REQUEST_TYPE_REGISTRATION);
    CU_ASSERT(!LWM2M_URI_IS_SET_OBJECT(&uri));
    CU_ASSERT(!LWM2M_URI_IS_SET_INSTANCE(&uri));
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE(&uri));
#ifndef LWM2M_VERSION_1_0
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri));
#endif

    /* "/rd/5a3f" */
    reg.next = &location;
    requestType = uri_decode(NULL, &reg, &uri);
    /* should not fail, error in uri_parse */
    /* CU_ASSERT_EQUAL(requestType, LWM2M_REQUEST_TYPE_REGISTRATION); */

    /* "/rd/5312" */
    reg.next = &locationDecimal;
    requestType = uri_decode(NULL, &reg, &uri);
    CU_ASSERT_EQUAL(requestType, LWM2M_REQUEST_TYPE_REGISTRATION);
    CU_ASSERT(LWM2M_URI_IS_SET_OBJECT(&uri));
    CU_ASSERT(!LWM2M_URI_IS_SET_INSTANCE(&uri));
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE(&uri));
#ifndef LWM2M_VERSION_1_0
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri));
#endif
    CU_ASSERT_EQUAL(uri.objectId, 5312);

    /* "/bs" */
    requestType = uri_decode(NULL, &boot, &uri);
    CU_ASSERT_EQUAL(requestType, LWM2M_REQUEST_TYPE_BOOTSTRAP);
    CU_ASSERT(!LWM2M_URI_IS_SET_OBJECT(&uri));
    CU_ASSERT(!LWM2M_URI_IS_SET_INSTANCE(&uri));
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE(&uri));
#ifndef LWM2M_VERSION_1_0
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri));
#endif

    /* "/bs/5a3f" */
    boot.next = &location;
    requestType = uri_decode(NULL, &boot, &uri);
    CU_ASSERT_EQUAL(requestType, LWM2M_REQUEST_TYPE_UNKNOWN);
    CU_ASSERT(!LWM2M_URI_IS_SET_OBJECT(&uri));
    CU_ASSERT(!LWM2M_URI_IS_SET_INSTANCE(&uri));
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE(&uri));
#ifndef LWM2M_VERSION_1_0
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri));
#endif

    /* "/9050/11/0" or "/9050/11/0/12" */
    requestType = uri_decode(NULL, &oID, &uri);
    CU_ASSERT_EQUAL(requestType, LWM2M_REQUEST_TYPE_DM);
    CU_ASSERT(LWM2M_URI_IS_SET_OBJECT(&uri));
    CU_ASSERT(LWM2M_URI_IS_SET_INSTANCE(&uri));
    CU_ASSERT(LWM2M_URI_IS_SET_RESOURCE(&uri));
    CU_ASSERT_EQUAL(uri.objectId, 9050);
    CU_ASSERT_EQUAL(uri.instanceId, 11);
    CU_ASSERT_EQUAL(uri.resourceId, 0);
#ifndef LWM2M_VERSION_1_0
    CU_ASSERT(LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri));
    CU_ASSERT_EQUAL(uri.resourceInstanceId, 12);
#endif

    /* "/11/0" or "/11/0/12"*/
    requestType = uri_decode(NULL, &iID, &uri);
    CU_ASSERT_EQUAL(requestType, LWM2M_REQUEST_TYPE_DM);
    CU_ASSERT(LWM2M_URI_IS_SET_OBJECT(&uri));
    CU_ASSERT(LWM2M_URI_IS_SET_INSTANCE(&uri));
    CU_ASSERT_EQUAL(uri.objectId, 11);
    CU_ASSERT_EQUAL(uri.instanceId, 0);
#ifdef LWM2M_VERSION_1_0
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE(&uri));
#else
    CU_ASSERT(LWM2M_URI_IS_SET_RESOURCE(&uri));
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri));
    CU_ASSERT_EQUAL(uri.resourceId, 12);
#endif

    /* "/0" or "/0/12" */
    requestType = uri_decode(NULL, &rID, &uri);
    CU_ASSERT_EQUAL(requestType, LWM2M_REQUEST_TYPE_DM);
    CU_ASSERT(LWM2M_URI_IS_SET_OBJECT(&uri));
    CU_ASSERT_EQUAL(uri.objectId, 0);
#ifdef LWM2M_VERSION_1_0
    CU_ASSERT(!LWM2M_URI_IS_SET_INSTANCE(&uri));
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE(&uri));
#else
    CU_ASSERT(LWM2M_URI_IS_SET_INSTANCE(&uri));
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE(&uri));
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri));
    CU_ASSERT_EQUAL(uri.instanceId, 12);
#endif

#ifndef LWM2M_VERSION_1_0
    /* "/12" */
    requestType = uri_decode(NULL, &riID, &uri);
    CU_ASSERT_EQUAL(requestType, LWM2M_REQUEST_TYPE_DM);
    CU_ASSERT(LWM2M_URI_IS_SET_OBJECT(&uri));
    CU_ASSERT_EQUAL(uri.objectId, 12);
    CU_ASSERT(!LWM2M_URI_IS_SET_INSTANCE(&uri));
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE(&uri));
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri));
#endif

    /* "/9050/11/0/555" */
#ifdef LWM2M_VERSION_1_0
    rID.next = &extraID;
#else
    riID.next = &extraID;
#endif
    requestType = uri_decode(NULL, &oID, &uri);
    CU_ASSERT_EQUAL(requestType, LWM2M_REQUEST_TYPE_UNKNOWN);
    CU_ASSERT(!LWM2M_URI_IS_SET_OBJECT(&uri));
    CU_ASSERT(!LWM2M_URI_IS_SET_INSTANCE(&uri));
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE(&uri));
#ifndef LWM2M_VERSION_1_0
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri));
#endif

    /* "/0/5a3f" */
    rID.next = &location;
    requestType = uri_decode(NULL, &rID, &uri);
    CU_ASSERT_EQUAL(requestType, LWM2M_REQUEST_TYPE_UNKNOWN);
    CU_ASSERT(!LWM2M_URI_IS_SET_OBJECT(&uri));
    CU_ASSERT(!LWM2M_URI_IS_SET_INSTANCE(&uri));
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE(&uri));
#ifndef LWM2M_VERSION_1_0
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri));
#endif

    MEMORY_TRACE_AFTER_EQ;
}


static void test_string_to_uri(void)
{
    int result;
    lwm2m_uri_t uri;
    MEMORY_TRACE_BEFORE;
    result = lwm2m_stringToUri("", 0, &uri);
    CU_ASSERT_EQUAL(result, 0);
    result = lwm2m_stringToUri("no_uri", 6, &uri);
    CU_ASSERT_EQUAL(result, 0);
    result = lwm2m_stringToUri("/1", 2, &uri);
    CU_ASSERT_EQUAL(result, 2);
    CU_ASSERT(LWM2M_URI_IS_SET_OBJECT(&uri));
    CU_ASSERT_EQUAL(uri.objectId, 1);
    CU_ASSERT(!LWM2M_URI_IS_SET_INSTANCE(&uri));
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE(&uri));
#ifndef LWM2M_VERSION_1_0
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri));
#endif

    result = lwm2m_stringToUri("/1/2", 4, &uri);
    CU_ASSERT_EQUAL(result, 4);
    CU_ASSERT(LWM2M_URI_IS_SET_OBJECT(&uri));
    CU_ASSERT_EQUAL(uri.objectId, 1);
    CU_ASSERT(LWM2M_URI_IS_SET_INSTANCE(&uri));
    CU_ASSERT_EQUAL(uri.instanceId, 2);
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE(&uri));
#ifndef LWM2M_VERSION_1_0
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri));
#endif

    result = lwm2m_stringToUri("/1/2/3", 6, &uri);
    CU_ASSERT_EQUAL(result, 6);
    CU_ASSERT(LWM2M_URI_IS_SET_OBJECT(&uri));
    CU_ASSERT_EQUAL(uri.objectId, 1);
    CU_ASSERT(LWM2M_URI_IS_SET_INSTANCE(&uri));
    CU_ASSERT_EQUAL(uri.instanceId, 2);
    CU_ASSERT(LWM2M_URI_IS_SET_RESOURCE(&uri));
    CU_ASSERT_EQUAL(uri.resourceId, 3);
#ifndef LWM2M_VERSION_1_0
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri));
#endif

    result = lwm2m_stringToUri("/1/2/3/4", 8, &uri);
#ifndef LWM2M_VERSION_1_0
    CU_ASSERT_EQUAL(result, 8);
    CU_ASSERT(LWM2M_URI_IS_SET_OBJECT(&uri));
    CU_ASSERT_EQUAL(uri.objectId, 1);
    CU_ASSERT(LWM2M_URI_IS_SET_INSTANCE(&uri));
    CU_ASSERT_EQUAL(uri.instanceId, 2);
    CU_ASSERT(LWM2M_URI_IS_SET_RESOURCE(&uri));
    CU_ASSERT_EQUAL(uri.resourceId, 3);
    CU_ASSERT(LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri));
    CU_ASSERT_EQUAL(uri.resourceInstanceId, 4);

    result = lwm2m_stringToUri("/1/2/3/4/5", 10, &uri);
#endif
    CU_ASSERT_EQUAL(result, 0);

    MEMORY_TRACE_AFTER_EQ;
}

static void test_uri_to_string(void)
{
    lwm2m_uri_t uri;
    lwm2m_uri_t uri2;
    uri_depth_t depth;
    int result;
    char buffer[URI_MAX_STRING_LEN];

    MEMORY_TRACE_BEFORE;

    result = uri_toString(NULL, (uint8_t*)buffer, sizeof(buffer), &depth);
    CU_ASSERT_EQUAL(result, 0);
    CU_ASSERT_EQUAL(depth, URI_DEPTH_NONE);
    CU_ASSERT_EQUAL(lwm2m_stringToUri(buffer, result, &uri2), result);

    LWM2M_URI_RESET(&uri);
    result = uri_toString(&uri, (uint8_t*)buffer, sizeof(buffer), &depth);
    CU_ASSERT_EQUAL(result, 1);
    CU_ASSERT_EQUAL(depth, URI_DEPTH_NONE);
    CU_ASSERT_NSTRING_EQUAL(buffer, "/", result);
    CU_ASSERT_EQUAL(lwm2m_stringToUri(buffer, result, &uri2), result);
    CU_ASSERT(!LWM2M_URI_IS_SET_OBJECT(&uri2));
    CU_ASSERT(!LWM2M_URI_IS_SET_INSTANCE(&uri2));
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE(&uri2));
#ifndef LWM2M_VERSION_1_0
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri2));
#endif

    uri.objectId = 1;
    result = uri_toString(&uri, (uint8_t*)buffer, sizeof(buffer), &depth);
    CU_ASSERT_EQUAL(result, 2);
    CU_ASSERT_EQUAL(depth, URI_DEPTH_OBJECT);
    CU_ASSERT_NSTRING_EQUAL(buffer, "/1", result);
    CU_ASSERT_EQUAL(lwm2m_stringToUri(buffer, result, &uri2), result);
    CU_ASSERT(LWM2M_URI_IS_SET_OBJECT(&uri2));
    CU_ASSERT(!LWM2M_URI_IS_SET_INSTANCE(&uri2));
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE(&uri2));
#ifndef LWM2M_VERSION_1_0
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri2));
#endif
    CU_ASSERT_EQUAL(uri2.objectId, uri.objectId);

    uri.instanceId = 2;
    result = uri_toString(&uri, (uint8_t*)buffer, sizeof(buffer), &depth);
    CU_ASSERT_EQUAL(result, 4);
    CU_ASSERT_EQUAL(depth, URI_DEPTH_OBJECT_INSTANCE);
    CU_ASSERT_NSTRING_EQUAL(buffer, "/1/2", result);
    CU_ASSERT_EQUAL(lwm2m_stringToUri(buffer, result, &uri2), result);
    CU_ASSERT(LWM2M_URI_IS_SET_OBJECT(&uri2));
    CU_ASSERT(LWM2M_URI_IS_SET_INSTANCE(&uri2));
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE(&uri2));
#ifndef LWM2M_VERSION_1_0
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri2));
#endif
    CU_ASSERT_EQUAL(uri2.objectId, uri.objectId);
    CU_ASSERT_EQUAL(uri2.instanceId, uri.instanceId);

    uri.resourceId = 3;
    result = uri_toString(&uri, (uint8_t*)buffer, sizeof(buffer), &depth);
    CU_ASSERT_EQUAL(result, 6);
    CU_ASSERT_EQUAL(depth, URI_DEPTH_RESOURCE);
    CU_ASSERT_NSTRING_EQUAL(buffer, "/1/2/3", result);
    CU_ASSERT_EQUAL(lwm2m_stringToUri(buffer, result, &uri2), result);
    CU_ASSERT(LWM2M_URI_IS_SET_OBJECT(&uri2));
    CU_ASSERT(LWM2M_URI_IS_SET_INSTANCE(&uri2));
    CU_ASSERT(LWM2M_URI_IS_SET_RESOURCE(&uri2));
#ifndef LWM2M_VERSION_1_0
    CU_ASSERT(!LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri2));
#endif
    CU_ASSERT_EQUAL(uri2.objectId, uri.objectId);
    CU_ASSERT_EQUAL(uri2.instanceId, uri.instanceId);
    CU_ASSERT_EQUAL(uri2.resourceId, uri.resourceId);

#ifndef LWM2M_VERSION_1_0
    uri.resourceInstanceId = 4;
    result = uri_toString(&uri, (uint8_t*)buffer, sizeof(buffer), &depth);
    CU_ASSERT_EQUAL(depth, URI_DEPTH_RESOURCE_INSTANCE);
    CU_ASSERT_EQUAL(result, 8);
    CU_ASSERT_NSTRING_EQUAL(buffer, "/1/2/3/4", result);
    CU_ASSERT_EQUAL(lwm2m_stringToUri(buffer, result, &uri2), result);
    CU_ASSERT(LWM2M_URI_IS_SET_OBJECT(&uri2));
    CU_ASSERT(LWM2M_URI_IS_SET_INSTANCE(&uri2));
    CU_ASSERT(LWM2M_URI_IS_SET_RESOURCE(&uri2));
    CU_ASSERT(LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri2));
    CU_ASSERT_EQUAL(uri2.objectId, uri.objectId);
    CU_ASSERT_EQUAL(uri2.instanceId, uri.instanceId);
    CU_ASSERT_EQUAL(uri2.resourceId, uri.resourceId);
    CU_ASSERT_EQUAL(uri2.resourceInstanceId, uri.resourceInstanceId);
#endif

    LWM2M_URI_RESET(&uri);
    uri.objectId = LWM2M_MAX_ID - 1;
    uri.instanceId = LWM2M_MAX_ID - 1;
    uri.resourceId = LWM2M_MAX_ID - 1;
#ifndef LWM2M_VERSION_1_0
    uri.resourceInstanceId = LWM2M_MAX_ID - 1;
#endif
    result = uri_toString(&uri, (uint8_t*)buffer, sizeof(buffer), &depth);
    CU_ASSERT_EQUAL(result, URI_MAX_STRING_LEN);
#ifdef LWM2M_VERSION_1_0
    CU_ASSERT_EQUAL(depth, URI_DEPTH_RESOURCE);
    CU_ASSERT_EQUAL(URI_MAX_STRING_LEN, 3*6);
    CU_ASSERT_NSTRING_EQUAL(buffer, "/65534/65534/65534", result);
#else
    CU_ASSERT_EQUAL(depth, URI_DEPTH_RESOURCE_INSTANCE);
    CU_ASSERT_EQUAL(URI_MAX_STRING_LEN, 4*6);
    CU_ASSERT_NSTRING_EQUAL(buffer, "/65534/65534/65534/65534", result);
#endif
    CU_ASSERT_EQUAL(lwm2m_stringToUri(buffer, result, &uri2), result);
    CU_ASSERT(LWM2M_URI_IS_SET_OBJECT(&uri2));
    CU_ASSERT(LWM2M_URI_IS_SET_INSTANCE(&uri2));
    CU_ASSERT(LWM2M_URI_IS_SET_RESOURCE(&uri2));
#ifndef LWM2M_VERSION_1_0
    CU_ASSERT(LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&uri2));
#endif
    CU_ASSERT_EQUAL(uri2.objectId, uri.objectId);
    CU_ASSERT_EQUAL(uri2.instanceId, uri.instanceId);
    CU_ASSERT_EQUAL(uri2.resourceId, uri.resourceId);
#ifndef LWM2M_VERSION_1_0
    CU_ASSERT_EQUAL(uri2.resourceInstanceId, uri.resourceInstanceId);
#endif

    MEMORY_TRACE_AFTER_EQ;
}

static struct TestTable table[] = {
        { "test of uri_decode()", test_uri_decode },
        { "test of lwm2m_stringToUri()", test_string_to_uri },
        { "test of uri_toString()", test_uri_to_string },
        { NULL, NULL },
};

CU_ErrorCode create_uri_suit()
{
   CU_pSuite pSuite = NULL;

   pSuite = CU_add_suite("Suite_URI", NULL, NULL);
   if (NULL == pSuite) {
      return CU_get_error();
   }

   return add_tests(pSuite, table);
}

