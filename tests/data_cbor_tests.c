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
 *    David Gräff - Convert to test case
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
 *
 *******************************************************************************/

#include "internals.h"
#include "liblwm2m.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "commandline.h"

#include "CUnit/Basic.h"
#include "tests.h"

#ifdef LWM2M_SUPPORT_SENML_CBOR
#ifndef LWM2M_VERSION_1_1

/* clang-format off */
#ifndef STR_MEDIA_TYPE
#define STR_MEDIA_TYPE(M)                          \
((M) == LWM2M_CONTENT_TEXT ? "Text" :              \
((M) == LWM2M_CONTENT_LINK ? "Link" :              \
((M) == LWM2M_CONTENT_OPAQUE ? "Opaque" :          \
((M) == LWM2M_CONTENT_TLV ? "TLV" :                \
((M) == LWM2M_CONTENT_JSON ? "JSON" :              \
((M) == LWM2M_CONTENT_SENML_JSON ? "SenML JSON" :  \
((M) == LWM2M_CONTENT_CBOR ? "CBOR" :              \
((M) == LWM2M_CONTENT_SENML_CBOR ? "SenML CBOR" :  \
"Unknown"))))))))
#endif
/* clang-format on */

static void cbor_test_raw_error(const char *uriStr, const uint8_t *testBuf, size_t testLen, lwm2m_media_type_t format,
                                const char *id) {
    lwm2m_data_t *tlvP;
    lwm2m_uri_t uri;
    int size;

    if (uriStr != NULL) {
        lwm2m_stringToUri(uriStr, strlen(uriStr), &uri);
    }

    size = lwm2m_data_parse((uriStr != NULL) ? &uri : NULL, testBuf, testLen, format, &tlvP);
    if (size > 0) {
        printf("(Parsing %s from %s succeeded but should have failed.)\t", id, STR_MEDIA_TYPE(format));
    }
    CU_ASSERT_TRUE_FATAL(size <= 0);
}

/**
 * @brief Serializes the data, compares to expected, parses and compares
 *        to original.
 */
static void cbor_test_data_expected(const char *uriStr, lwm2m_data_t *dataP, const uint8_t *expectBuf, size_t expectLen,
                                    lwm2m_media_type_t format, const char *id) {
    lwm2m_data_t *tlvP;
    lwm2m_uri_t uri;
    int size;
    uint8_t *buffer;
    int length;

    if (uriStr != NULL) {
        lwm2m_stringToUri(uriStr, strlen(uriStr), &uri);
    }

    length = lwm2m_data_serialize((uriStr != NULL) ? &uri : NULL, 1, dataP, &format, &buffer);
    if (length <= 0) {
        printf("(Serialize lwm2m_data_t %s to %s failed.)\t", id, STR_MEDIA_TYPE(format));
        CU_TEST_FATAL(CU_FALSE);
        return;
    }

    CU_ASSERT_EQUAL(length, (int)expectLen);
    if (length == (int)expectLen) {
        if (memcmp(buffer, expectBuf, expectLen) != 0) {
            printf("(Serialize lwm2m_data_t %s to %s differs from expected.)\t", id, STR_MEDIA_TYPE(format));
            CU_TEST(CU_FALSE);
        }
    }

    size = lwm2m_data_parse((uriStr != NULL) ? &uri : NULL, buffer, length, format, &tlvP);
    if (size < 0) {
        printf("(Parsing %s from %s failed.)\t", id, STR_MEDIA_TYPE(format));
    }
    CU_ASSERT_TRUE(size == 1);

    if (size == 1) {
        if (tlvP->id != dataP->id) {
            printf("(Parsing %s from %s differs from original.)\t", id, STR_MEDIA_TYPE(format));
            CU_TEST(CU_FALSE);
        } else {
            switch (dataP->type) {
            case LWM2M_TYPE_UNDEFINED:
            case LWM2M_TYPE_OBJECT:
            case LWM2M_TYPE_OBJECT_INSTANCE:
            case LWM2M_TYPE_MULTIPLE_RESOURCE:
            default:
                printf("(Parsing %s from %s invalid type.)\t", id, STR_MEDIA_TYPE(format));
                CU_TEST(CU_FALSE);
                break;
            case LWM2M_TYPE_STRING:
            case LWM2M_TYPE_OPAQUE:
                if (tlvP->type != dataP->type || tlvP->value.asBuffer.length != dataP->value.asBuffer.length ||
                    (dataP->value.asBuffer.buffer != NULL &&
                     memcmp(tlvP->value.asBuffer.buffer, dataP->value.asBuffer.buffer, tlvP->value.asBuffer.length) !=
                         0)) {
                    printf("(Parsing %s from %s differs from original.)\t", id, STR_MEDIA_TYPE(format));
                    CU_TEST(CU_FALSE);
                }
                break;
            case LWM2M_TYPE_INTEGER: {
                {
                    int64_t v1, v2;
                    if (lwm2m_data_decode_int(dataP, &v1) == 0 || lwm2m_data_decode_int(tlvP, &v2) == 0 || v1 != v2) {
                        printf("(Parsing %s from %s differs from original.)\t", id, STR_MEDIA_TYPE(format));
                        CU_TEST(CU_FALSE);
                    }
                }
            } break;
            case LWM2M_TYPE_UNSIGNED_INTEGER: {
                uint64_t v1, v2;
                if (lwm2m_data_decode_uint(dataP, &v1) == 0 || lwm2m_data_decode_uint(tlvP, &v2) == 0 || v1 != v2) {
                    printf("(Parsing %s from %s differs from original.)\t", id, STR_MEDIA_TYPE(format));
                    CU_TEST(CU_FALSE);
                }
            } break;
            case LWM2M_TYPE_FLOAT:
                _Pragma("GCC diagnostic push");
                _Pragma("GCC diagnostic ignored \"-Wfloat-equal\"");
                if (tlvP->type != dataP->type) {
                    printf("(Parsing %s from %s differs from original.)\t", id, STR_MEDIA_TYPE(format));
                    CU_TEST(CU_FALSE);
                } else {
                    if (dataP->value.asFloat != dataP->value.asFloat) {
                        if (tlvP->value.asFloat == tlvP->value.asFloat) {
                            printf("(Parsing %s from %s differs from original.)\t", id, STR_MEDIA_TYPE(format));
                            CU_TEST(CU_FALSE);
                        }
                    } else if (tlvP->value.asFloat != dataP->value.asFloat) {
                        printf("(Parsing %s from %s differs from original.)\t", id, STR_MEDIA_TYPE(format));
                        CU_TEST(CU_FALSE);
                    }
                }
                break;
                _Pragma("GCC diagnostic pop");
            case LWM2M_TYPE_BOOLEAN:
                if (tlvP->type != dataP->type || tlvP->value.asBoolean != dataP->value.asBoolean) {
                    printf("(Parsing %s from %s differs from original.)\t", id, STR_MEDIA_TYPE(format));
                    CU_TEST(CU_FALSE);
                }
                break;
            case LWM2M_TYPE_OBJECT_LINK:
                if (tlvP->type != dataP->type || tlvP->value.asObjLink.objectId != dataP->value.asObjLink.objectId ||
                    tlvP->value.asObjLink.objectInstanceId != dataP->value.asObjLink.objectInstanceId) {
                    printf("(Parsing %s from %s differs from original.)\t", id, STR_MEDIA_TYPE(format));
                    CU_TEST(CU_FALSE);
                }
                break;
            case LWM2M_TYPE_CORE_LINK:
                // Core link parses as string
                if (tlvP->type != LWM2M_TYPE_STRING || tlvP->value.asBuffer.length != dataP->value.asBuffer.length ||
                    memcmp(tlvP->value.asBuffer.buffer, dataP->value.asBuffer.buffer, tlvP->value.asBuffer.length) !=
                        0) {
                    printf("(Parsing %s from %s differs from original.)\t", id, STR_MEDIA_TYPE(format));
                    CU_TEST(CU_FALSE);
                }
                break;
            }
        }
    }

    lwm2m_free(buffer);
    lwm2m_data_free(size, tlvP);
}

static void cbor_test_1(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x00};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_INTEGER;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "1");
}

static void cbor_test_2(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x01};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_INTEGER;
    data.value.asInteger = 1;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "2");
}

static void cbor_test_3(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x0a};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_INTEGER;
    data.value.asInteger = 10;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "3");
}

static void cbor_test_4(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x17};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_INTEGER;
    data.value.asInteger = 23;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "4");
}

static void cbor_test_5(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x18, 0x18};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_INTEGER;
    data.value.asInteger = 24;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "5");
}

static void cbor_test_6(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x18, 0x19};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_INTEGER;
    data.value.asInteger = 25;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "6");
}

static void cbor_test_7(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x18, 0x64};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_INTEGER;
    data.value.asInteger = 100;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "7");
}

static void cbor_test_8(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x19, 0x03, 0xe8};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_INTEGER;
    data.value.asInteger = 1000;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "8");
}

static void cbor_test_9(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x1a, 0x00, 0x0f, 0x42, 0x40};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_INTEGER;
    data.value.asInteger = 1000000;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "9");
}

static void cbor_test_10(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x1b, 0x00, 0x00, 0x00, 0xe8, 0xd4, 0xa5, 0x10, 0x00};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_INTEGER;
    data.value.asInteger = 1000000000000ll;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "10");
}

static void cbor_test_11(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x1b, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_UNSIGNED_INTEGER;
    data.value.asUnsigned = UINT64_MAX;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "11");
}

static void cbor_test_12(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x3b, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_INTEGER;
    data.value.asInteger = INT64_MIN;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "12");
}

static void cbor_test_13(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x1b, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_INTEGER;
    data.value.asInteger = INT64_MAX;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "13");
}

static void cbor_test_14(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x20};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_INTEGER;
    data.value.asInteger = -1;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "14");
}

static void cbor_test_15(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x29};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_INTEGER;
    data.value.asInteger = -10;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "15");
}

static void cbor_test_16(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x38, 0x63};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_INTEGER;
    data.value.asInteger = -100;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "16");
}

static void cbor_test_17(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x39, 0x03, 0xe7};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_INTEGER;
    data.value.asInteger = -1000;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "17");
}

static void cbor_test_18(void) {
    lwm2m_data_t data;
#ifndef CBOR_NO_FLOAT16_ENCODING
    const uint8_t expected[] = {0xf9, 0x00, 0x00};
#else
    const uint8_t expected[] = {0xfa, 0x00, 0x00, 0x00, 0x00};
#endif
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_FLOAT;
    data.value.asFloat = 0.0;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "18");
}

static void cbor_test_19(void) {
    lwm2m_data_t data;
#ifndef CBOR_NO_FLOAT16_ENCODING
    const uint8_t expected[] = {0xf9, 0x80, 0x00};
#else
    const uint8_t expected[] = {0xfa, 0x80, 0x00, 0x00, 0x00};
#endif
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_FLOAT;
    data.value.asFloat = -0.0;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "19");
}

static void cbor_test_20(void) {
    lwm2m_data_t data;
#ifndef CBOR_NO_FLOAT16_ENCODING
    const uint8_t expected[] = {0xf9, 0x3c, 0x00};
#else
    const uint8_t expected[] = {0xfa, 0x3f, 0x80, 0x00, 0x00};
#endif
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_FLOAT;
    data.value.asFloat = 1.0;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "20");
}

static void cbor_test_21(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0xfb, 0x3f, 0xf1, 0x99, 0x99, 0x99, 0x99, 0x99, 0x9a};

    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_FLOAT;
    data.value.asFloat = 1.1;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "21");
}

static void cbor_test_22(void) {
    lwm2m_data_t data;
#ifndef CBOR_NO_FLOAT16_ENCODING
    const uint8_t expected[] = {0xf9, 0x3e, 0x00};
#else
    const uint8_t expected[] = {0xfa, 0x3f, 0xc0, 0x00, 0x00};
#endif
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_FLOAT;
    data.value.asFloat = 1.5;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "22");
}

static void cbor_test_23(void) {
    lwm2m_data_t data;
#ifndef CBOR_NO_FLOAT16_ENCODING
    const uint8_t expected[] = {0xf9, 0x7b, 0xff};
#else
    const uint8_t expected[] = {0xfa, 0x47, 0x7f, 0xe0, 0x00};
#endif
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_FLOAT;
    data.value.asFloat = 65504.0;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "23");
}

static void cbor_test_24(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0xfa, 0x47, 0xc3, 0x50, 0x00};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_FLOAT;
    data.value.asFloat = 100000.0;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "24");
}

static void cbor_test_25(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0xfa, 0x7f, 0x7f, 0xff, 0xff};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_FLOAT;
    data.value.asFloat = 3.4028234663852886e+38;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "25");
}

static void cbor_test_26(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0xfb, 0x7e, 0x37, 0xe4, 0x3c, 0x88, 0x00, 0x75, 0x9c};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_FLOAT;
    data.value.asFloat = 1.0e+300;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "26");
}

static void cbor_test_27(void) {
    lwm2m_data_t data;
#ifndef CBOR_NO_FLOAT16_ENCODING
    const uint8_t expected[] = {0xf9, 0x00, 0x01};
#else
    const uint8_t expected[] = {0xfa, 0x33, 0x80, 0x00, 0x00};
#endif
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_FLOAT;
    data.value.asFloat = 5.960464477539063e-8;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "27");
}

static void cbor_test_28(void) {
    lwm2m_data_t data;
#ifndef CBOR_NO_FLOAT16_ENCODING
    const uint8_t expected[] = {0xf9, 0x04, 0x00};
#else
    const uint8_t expected[] = {0xfa, 0x38, 0x80, 0x00, 0x00};
#endif
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_FLOAT;
    data.value.asFloat = 0.00006103515625;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "28");
}

static void cbor_test_29(void) {
    lwm2m_data_t data;
#ifndef CBOR_NO_FLOAT16_ENCODING
    const uint8_t expected[] = {0xf9, 0xc4, 0x00};
#else
    const uint8_t expected[] = {0xfa, 0xc0, 0x80, 0x00, 0x00};
#endif
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_FLOAT;
    data.value.asFloat = -4.0;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "29");
}

static void cbor_test_30(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0xfb, 0xc0, 0x10, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_FLOAT;
    data.value.asFloat = -4.1;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "30");
}

static void cbor_test_31(void) {
    /* Check encoding for infinity */
    lwm2m_data_t data;
#ifndef CBOR_NO_FLOAT16_ENCODING
    const uint8_t expected[] = {0xf9, 0x7c, 0x00};
#else
    const uint8_t expected[] = {0xfa, 0x7f, 0x80, 0x00, 0x00};
#endif
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_FLOAT;
    data.value.asFloat = INFINITY;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "31");
}

static void cbor_test_32(void) {
    /* Check encoding for NaN */
    lwm2m_data_t data;
#ifndef CBOR_NO_FLOAT16_ENCODING
    const uint8_t expected[] = {0xf9, 0x7e, 0x00};
#else
    const uint8_t expected[] = {0xfa, 0x7f, 0xc0, 0x00, 0x00};
#endif
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_FLOAT;
    data.value.asFloat = NAN;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "32");
}

static void cbor_test_33(void) {
    /* Check encoding for negative infinity */
    lwm2m_data_t data;
#ifndef CBOR_NO_FLOAT16_ENCODING
    const uint8_t expected[] = {0xf9, 0xfc, 0x00};
#else
    const uint8_t expected[] = {0xfa, 0xff, 0x80, 0x00, 0x00};
#endif
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_FLOAT;
    data.value.asFloat = -INFINITY;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "33");
}

static void cbor_test_34(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0xf4};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_BOOLEAN;
    data.value.asBoolean = false;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "34");
}

static void cbor_test_35(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0xf5};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_BOOLEAN;
    data.value.asBoolean = true;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "35");
}

static void cbor_test_36(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x74, 0x32, 0x30, 0x31, 0x33, 0x2d, 0x30, 0x33, 0x2d, 0x32, 0x31,
                                0x54, 0x32, 0x30, 0x3a, 0x30, 0x34, 0x3a, 0x30, 0x30, 0x5a};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_STRING;
    data.value.asBuffer.buffer = (uint8_t *)"2013-03-21T20:04:00Z";
    data.value.asBuffer.length = strlen((char *)data.value.asBuffer.buffer);

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "36");
}

static void cbor_test_37(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x1a, 0x51, 0x4b, 0x67, 0xb0};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_INTEGER;
    data.value.asInteger = 1363896240;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "37");
}

static void cbor_test_38(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0xfb, 0x41, 0xd4, 0x52, 0xd9, 0xec, 0x20, 0x00, 0x00};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_FLOAT;
    data.value.asFloat = 1363896240.5;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "38");
}

static void cbor_test_39(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x44, 0x01, 0x02, 0x03, 0x04};
    const uint8_t bin[] = {0x01, 0x02, 0x03, 0x04};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_OPAQUE;
    data.value.asBuffer.buffer = (uint8_t *)bin;
    data.value.asBuffer.length = sizeof(bin);

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "39");
}

static void cbor_test_40(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x45, 0x64, 0x49, 0x45, 0x54, 0x46};
    const uint8_t bin[] = {0x64, 0x49, 0x45, 0x54, 0x46};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_OPAQUE;
    data.value.asBuffer.buffer = (uint8_t *)bin;
    data.value.asBuffer.length = sizeof(bin);

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "40");
}

static void cbor_test_41(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x76, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77, 0x77, 0x77, 0x2e,
                                0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_STRING;
    data.value.asBuffer.buffer = (uint8_t *)"http://www.example.com";
    data.value.asBuffer.length = strlen((char *)data.value.asBuffer.buffer);

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "41");
}

static void cbor_test_42(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x40};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_OPAQUE;
    data.value.asBuffer.buffer = NULL;
    data.value.asBuffer.length = 0;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "42");
}

static void cbor_test_43(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x60};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_STRING;
    data.value.asBuffer.buffer = NULL;
    data.value.asBuffer.length = 0;

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "43");
}

static void cbor_test_44(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x61, 0x61};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_STRING;
    data.value.asBuffer.buffer = (uint8_t *)"a";
    data.value.asBuffer.length = strlen((char *)data.value.asBuffer.buffer);

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "44");
}

static void cbor_test_45(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x64, 0x49, 0x45, 0x54, 0x46};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_STRING;
    data.value.asBuffer.buffer = (uint8_t *)"IETF";
    data.value.asBuffer.length = strlen((char *)data.value.asBuffer.buffer);

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "45");
}

static void cbor_test_46(void) {
    lwm2m_data_t data;
    const uint8_t expected[] = {0x62, 0x22, 0x5c};
    memset(&data, 0, sizeof(data));
    data.type = LWM2M_TYPE_STRING;
    data.value.asBuffer.buffer = (uint8_t *)"\"\\";
    data.value.asBuffer.length = strlen((char *)data.value.asBuffer.buffer);

    cbor_test_data_expected("/1/1/0", &data, expected, sizeof(expected), LWM2M_CONTENT_CBOR, "46");
}

static void cbor_test_47(void) {
    // Integer can't be represented
    const uint8_t buffer[] = {0x3b, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "47");
}

static void cbor_test_48(void) {
    // null
    const uint8_t buffer[] = {0xf6};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "48");
}

static void cbor_test_49(void) {
    // undefined
    const uint8_t buffer[] = {0xf7};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "49");
}

static void cbor_test_50(void) {
    // simple(16)
    const uint8_t buffer[] = {0xf0};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "50");
}

static void cbor_test_51(void) {
    // simple(24)
    const uint8_t buffer[] = {0xf8, 0x18};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "51");
}

static void cbor_test_52(void) {
    // simple(255)
    const uint8_t buffer[] = {0xf8, 0xff};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "52");
}

static void cbor_test_53(void) {
    // []
    const uint8_t buffer[] = {0x80};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "53");
}

static void cbor_test_54(void) {
    // [1, 2, 3]
    const uint8_t buffer[] = {0x83, 0x01, 0x02, 0x03};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "54");
}

static void cbor_test_55(void) {
    // {}
    const uint8_t buffer[] = {0xa0};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "55");
}

static void cbor_test_56(void) {
    // {1: 2, 3: 4}
    const uint8_t buffer[] = {0xa2, 0x01, 0x02, 0x03, 0x04};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "56");
}

static void cbor_test_57(void) {
    // {"a": 1, "b": [2, 3]}
    const uint8_t buffer[] = {0xa2, 0x61, 0x61, 0x01, 0x61, 0x62, 0x82, 0x02, 0x03};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "57");
}

static void cbor_test_58(void) {
    // ["a", {"b": "c"}]
    const uint8_t buffer[] = {0x82, 0x61, 0x61, 0xa1, 0x61, 0x62, 0x61, 0x63};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "58");
}

static void cbor_test_59(void) {
    // {"a": "A", "b": "B", "c": "C", "d": "D", "e": "E"}
    const uint8_t buffer[] = {0xa5, 0x61, 0x61, 0x61, 0x41, 0x61, 0x62, 0x61, 0x42, 0x61, 0x63,
                              0x61, 0x43, 0x61, 0x64, 0x61, 0x44, 0x61, 0x65, 0x61, 0x45};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "59");
}

static void cbor_test_60(void) {
    // (_ h’0102’, h’030405’)
    const uint8_t buffer[] = {0x5f, 0x42, 0x01, 0x02, 0x43, 0x03, 0x04, 0x05, 0xff};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "60");
}

static void cbor_test_61(void) {
    // (_ "strea", "ming")
    const uint8_t buffer[] = {0x7f, 0x65, 0x73, 0x74, 0x72, 0x65, 0x61, 0x64, 0x6d, 0x69, 0x6e, 0x67, 0xff};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "61");
}

static void cbor_test_62(void) {
    // [_ ]
    const uint8_t buffer[] = {0x9f, 0xff};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "62");
}

static void cbor_test_63(void) {
    // [_ 1, [2, 3], [_ 4, 5]]
    const uint8_t buffer[] = {0x9f, 0x01, 0x82, 0x02, 0x03, 0x9f, 0x04, 0x05, 0xff, 0xff};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "63");
}

static void cbor_test_64(void) {
    // [_ 1, [2, 3], [4, 5]]
    const uint8_t buffer[] = {0x9f, 0x01, 0x82, 0x02, 0x03, 0x82, 0x04, 0x05, 0xff};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "64");
}

static void cbor_test_65(void) {
    // [1, [2, 3], [_ 4, 5]]
    const uint8_t buffer[] = {0x83, 0x01, 0x82, 0x02, 0x03, 0x9f, 0x04, 0x05, 0xff};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "64");
}

static void cbor_test_66(void) {
    // [1, [_ 2, 3], [4, 5]]
    const uint8_t buffer[] = {0x83, 0x01, 0x9f, 0x02, 0x03, 0xff, 0x82, 0x04, 0x05};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "64");
}

static void cbor_test_67(void) {
    // [_ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    // 20, 21, 22, 23, 24, 25]
    const uint8_t buffer[] = {0x9f, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
                              0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x18, 0x18, 0x19, 0xff};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "67");
}

static void cbor_test_68(void) {
    // {_ "a": 1, "b": [_ 2, 3]}
    const uint8_t buffer[] = {0xbf, 0x61, 0x61, 0x01, 0x61, 0x62, 0x9f, 0x02, 0x03, 0xff, 0xff};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "68");
}

static void cbor_test_69(void) {
    // ["a", {_ "b": "c"}]
    const uint8_t buffer[] = {0x82, 0x61, 0x61, 0xbf, 0x61, 0x62, 0x61, 0x63, 0xff};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "69");
}

static void cbor_test_70(void) {
    // {_ "Fun": true, "Amt": -2}
    const uint8_t buffer[] = {0xbf, 0x63, 0x46, 0x75, 0x6e, 0xf5, 0x63, 0x41, 0x6d, 0x74, 0x21, 0xff};

    cbor_test_raw_error("/1/1/0", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "70");
}

static void cbor_test_71(void) {
    // Infinity
    const uint8_t buffer[] = {0xfa, 0x7f, 0x80, 0x00, 0x00};
    lwm2m_uri_t uri;
    int size;
    lwm2m_data_t *tlvP;
    LWM2M_URI_RESET(&uri);
    uri.objectId = 1;
    uri.instanceId = 1;
    uri.resourceId = 1;

    size = lwm2m_data_parse(&uri, buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, &tlvP);
    CU_ASSERT_EQUAL_FATAL(size, 1);
    CU_ASSERT_EQUAL(tlvP->id, 1);
    CU_ASSERT_EQUAL_FATAL(tlvP->type, LWM2M_TYPE_FLOAT);
    CU_ASSERT_EQUAL_FATAL(tlvP->value.asFloat, INFINITY);
    lwm2m_data_free(size, tlvP);
}

static void cbor_test_72(void) {
    // NaN
    const uint8_t buffer[] = {0xfa, 0x7f, 0xc0, 0x00, 0x00};
    lwm2m_uri_t uri;
    int size;
    lwm2m_data_t *tlvP;
    LWM2M_URI_RESET(&uri);
    uri.objectId = 1;
    uri.instanceId = 1;
    uri.resourceId = 1;

    size = lwm2m_data_parse(&uri, buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, &tlvP);
    CU_ASSERT_EQUAL_FATAL(size, 1);
    CU_ASSERT_EQUAL(tlvP->id, 1);
    CU_ASSERT_EQUAL_FATAL(tlvP->type, LWM2M_TYPE_FLOAT);
    _Pragma("GCC diagnostic push");
    _Pragma("GCC diagnostic ignored \"-Wfloat-equal\"");
    CU_ASSERT_NOT_EQUAL_FATAL(tlvP->value.asFloat, tlvP->value.asFloat);
    _Pragma("GCC diagnostic pop");
    lwm2m_data_free(size, tlvP);
}

static void cbor_test_73(void) {
    // -Infinity
    const uint8_t buffer[] = {0xfa, 0xff, 0x80, 0x00, 0x00};
    lwm2m_uri_t uri;
    int size;
    lwm2m_data_t *tlvP;
    LWM2M_URI_RESET(&uri);
    uri.objectId = 1;
    uri.instanceId = 1;
    uri.resourceId = 1;

    size = lwm2m_data_parse(&uri, buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, &tlvP);
    CU_ASSERT_EQUAL_FATAL(size, 1);
    CU_ASSERT_EQUAL(tlvP->id, 1);
    CU_ASSERT_EQUAL_FATAL(tlvP->type, LWM2M_TYPE_FLOAT);
    _Pragma("GCC diagnostic push");
    _Pragma("GCC diagnostic ignored \"-Wfloat-equal\"");
    CU_ASSERT_EQUAL_FATAL(tlvP->value.asFloat, -INFINITY);
    _Pragma("GCC diagnostic pop");
    lwm2m_data_free(size, tlvP);
}

static void cbor_test_74(void) {
    // Infinity
    const uint8_t buffer[] = {0xfb, 0x7f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    lwm2m_uri_t uri;
    int size;
    lwm2m_data_t *tlvP;
    LWM2M_URI_RESET(&uri);
    uri.objectId = 1;
    uri.instanceId = 1;
    uri.resourceId = 1;

    size = lwm2m_data_parse(&uri, buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, &tlvP);
    CU_ASSERT_EQUAL_FATAL(size, 1);
    CU_ASSERT_EQUAL(tlvP->id, 1);
    CU_ASSERT_EQUAL_FATAL(tlvP->type, LWM2M_TYPE_FLOAT);
    _Pragma("GCC diagnostic push");
    _Pragma("GCC diagnostic ignored \"-Wfloat-equal\"");
    CU_ASSERT_EQUAL_FATAL(tlvP->value.asFloat, INFINITY);
    _Pragma("GCC diagnostic pop");
    lwm2m_data_free(size, tlvP);
}

static void cbor_test_75(void) {
    // NaN
    const uint8_t buffer[] = {0xfb, 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    lwm2m_uri_t uri;
    int size;
    lwm2m_data_t *tlvP;
    LWM2M_URI_RESET(&uri);
    uri.objectId = 1;
    uri.instanceId = 1;
    uri.resourceId = 1;

    size = lwm2m_data_parse(&uri, buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, &tlvP);
    CU_ASSERT_EQUAL_FATAL(size, 1);
    CU_ASSERT_EQUAL(tlvP->id, 1);
    CU_ASSERT_EQUAL_FATAL(tlvP->type, LWM2M_TYPE_FLOAT);
    _Pragma("GCC diagnostic push");
    _Pragma("GCC diagnostic ignored \"-Wfloat-equal\"");
    CU_ASSERT_NOT_EQUAL_FATAL(tlvP->value.asFloat, tlvP->value.asFloat);
    _Pragma("GCC diagnostic pop");
    lwm2m_data_free(size, tlvP);
}

static void cbor_test_76(void) {
    // -Infinity
    const uint8_t buffer[] = {0xfb, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    lwm2m_uri_t uri;
    int size;
    lwm2m_data_t *tlvP;
    LWM2M_URI_RESET(&uri);
    uri.objectId = 1;
    uri.instanceId = 1;
    uri.resourceId = 1;

    size = lwm2m_data_parse(&uri, buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, &tlvP);
    CU_ASSERT_EQUAL_FATAL(size, 1);
    CU_ASSERT_EQUAL(tlvP->id, 1);
    CU_ASSERT_EQUAL_FATAL(tlvP->type, LWM2M_TYPE_FLOAT);
    _Pragma("GCC diagnostic push");
    _Pragma("GCC diagnostic ignored \"-Wfloat-equal\"");
    CU_ASSERT_EQUAL_FATAL(tlvP->value.asFloat, -INFINITY);
    _Pragma("GCC diagnostic pop");
    lwm2m_data_free(size, tlvP);
}

static void cbor_test_77(void) {
    // 0("2013-03-21T20:04:00Z")
    const uint8_t buffer[] = {0xc0, 0x74, 0x32, 0x30, 0x31, 0x33, 0x2d, 0x30, 0x33, 0x2d, 0x32,
                              0x31, 0x54, 0x32, 0x30, 0x3a, 0x30, 0x34, 0x3a, 0x30, 0x30, 0x5a};
    lwm2m_uri_t uri;
    int size;
    lwm2m_data_t *tlvP;
    LWM2M_URI_RESET(&uri);
    uri.objectId = 1;
    uri.instanceId = 1;
    uri.resourceId = 1;

    size = lwm2m_data_parse(&uri, buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, &tlvP);
    CU_ASSERT_EQUAL_FATAL(size, 1);
    CU_ASSERT_EQUAL(tlvP->id, 1);
    CU_ASSERT_EQUAL_FATAL(tlvP->type, LWM2M_TYPE_STRING);
    CU_ASSERT_EQUAL_FATAL(tlvP->value.asBuffer.length, 20);
    CU_ASSERT_EQUAL_FATAL(memcmp(tlvP->value.asBuffer.buffer, "2013-03-21T20:04:00Z", 20), 0);
    lwm2m_data_free(size, tlvP);
}

static void cbor_test_78(void) {
    // 1(1363896240)
    const uint8_t buffer[] = {0xc1, 0x1a, 0x51, 0x4b, 0x67, 0xb0};
    lwm2m_uri_t uri;
    int size;
    lwm2m_data_t *tlvP;
    LWM2M_URI_RESET(&uri);
    uri.objectId = 1;
    uri.instanceId = 1;
    uri.resourceId = 1;

    size = lwm2m_data_parse(&uri, buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, &tlvP);
    CU_ASSERT_EQUAL_FATAL(size, 1);
    CU_ASSERT_EQUAL(tlvP->id, 1);
    CU_ASSERT_EQUAL_FATAL(tlvP->type, LWM2M_TYPE_UNSIGNED_INTEGER);
    CU_ASSERT_EQUAL_FATAL(tlvP->value.asUnsigned, 1363896240);
    lwm2m_data_free(size, tlvP);
}

static void cbor_test_79(void) {
    // 1(1363896240.5)
    const uint8_t buffer[] = {0xc1, 0xfb, 0x41, 0xd4, 0x52, 0xd9, 0xec, 0x20, 0x00, 0x00};
    lwm2m_uri_t uri;
    int size;
    lwm2m_data_t *tlvP;
    LWM2M_URI_RESET(&uri);
    uri.objectId = 1;
    uri.instanceId = 1;
    uri.resourceId = 1;

    size = lwm2m_data_parse(&uri, buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, &tlvP);
    CU_ASSERT_EQUAL_FATAL(size, 1);
    CU_ASSERT_EQUAL(tlvP->id, 1);
    CU_ASSERT_EQUAL_FATAL(tlvP->type, LWM2M_TYPE_FLOAT);
    CU_ASSERT_EQUAL_FATAL(tlvP->value.asFloat, 1363896240.5);
    lwm2m_data_free(size, tlvP);
}

static void cbor_test_80(void) {
    // 23(h’01020304’)
    const uint8_t buffer[] = {0xd7, 0x44, 0x01, 0x02, 0x03, 0x04};
    const uint8_t expected[] = {0x01, 0x02, 0x03, 0x04};
    lwm2m_uri_t uri;
    int size;
    lwm2m_data_t *tlvP;
    LWM2M_URI_RESET(&uri);
    uri.objectId = 1;
    uri.instanceId = 1;
    uri.resourceId = 1;

    size = lwm2m_data_parse(&uri, buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, &tlvP);
    CU_ASSERT_EQUAL_FATAL(size, 1);
    CU_ASSERT_EQUAL(tlvP->id, 1);
    CU_ASSERT_EQUAL_FATAL(tlvP->type, LWM2M_TYPE_OPAQUE);
    CU_ASSERT_EQUAL_FATAL(tlvP->value.asBuffer.length, sizeof(expected));
    CU_ASSERT_EQUAL_FATAL(memcmp(tlvP->value.asBuffer.buffer, expected, sizeof(expected)), 0);
    lwm2m_data_free(size, tlvP);
}

static void cbor_test_81(void) {
    // 24(h’6449455446’)
    const uint8_t buffer[] = {0xd8, 0x18, 0x45, 0x64, 0x49, 0x45, 0x54, 0x46};
    const uint8_t expected[] = {0x64, 0x49, 0x45, 0x54, 0x46};
    lwm2m_uri_t uri;
    int size;
    lwm2m_data_t *tlvP;
    LWM2M_URI_RESET(&uri);
    uri.objectId = 1;
    uri.instanceId = 1;
    uri.resourceId = 1;

    size = lwm2m_data_parse(&uri, buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, &tlvP);
    CU_ASSERT_EQUAL_FATAL(size, 1);
    CU_ASSERT_EQUAL(tlvP->id, 1);
    CU_ASSERT_EQUAL_FATAL(tlvP->type, LWM2M_TYPE_OPAQUE);
    CU_ASSERT_EQUAL_FATAL(tlvP->value.asBuffer.length, sizeof(expected));
    CU_ASSERT_EQUAL_FATAL(memcmp(tlvP->value.asBuffer.buffer, expected, sizeof(expected)), 0);
    lwm2m_data_free(size, tlvP);
}

static void cbor_test_82(void) {
    // 32("http://www.example.com")
    const uint8_t buffer[] = {0xd8, 0x20, 0x76, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77, 0x77, 0x77,
                              0x2e, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d};
    lwm2m_uri_t uri;
    int size;
    lwm2m_data_t *tlvP;
    LWM2M_URI_RESET(&uri);
    uri.objectId = 1;
    uri.instanceId = 1;
    uri.resourceId = 1;

    size = lwm2m_data_parse(&uri, buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, &tlvP);
    CU_ASSERT_EQUAL_FATAL(size, 1);
    CU_ASSERT_EQUAL(tlvP->id, 1);
    CU_ASSERT_EQUAL_FATAL(tlvP->type, LWM2M_TYPE_STRING);
    CU_ASSERT_EQUAL_FATAL(tlvP->value.asBuffer.length, 22);
    CU_ASSERT_EQUAL_FATAL(memcmp(tlvP->value.asBuffer.buffer, "http://www.example.com", 22), 0);
    lwm2m_data_free(size, tlvP);
}

static void cbor_test_83(void) {
    /* Test multiple semantic tags */
    // 32(32("http://www.example.com"))
    const uint8_t buffer[] = {0xd8, 0x20, 0xd8, 0x20, 0x76, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77, 0x77,
                              0x77, 0x2e, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d};
    cbor_test_raw_error("/1/2/3", buffer, sizeof(buffer), LWM2M_CONTENT_CBOR, "83");
}
static struct TestTable table[] = {
    {"test of cbor_test_1()", cbor_test_1},   {"test of cbor_test_2()", cbor_test_2},
    {"test of cbor_test_3()", cbor_test_3},   {"test of cbor_test_4()", cbor_test_4},
    {"test of cbor_test_5()", cbor_test_5},   {"test of cbor_test_6()", cbor_test_6},
    {"test of cbor_test_7()", cbor_test_7},   {"test of cbor_test_8()", cbor_test_8},
    {"test of cbor_test_9()", cbor_test_9},   {"test of cbor_test_10()", cbor_test_10},
    {"test of cbor_test_11()", cbor_test_11}, {"test of cbor_test_12()", cbor_test_12},
    {"test of cbor_test_13()", cbor_test_13}, {"test of cbor_test_14()", cbor_test_14},
    {"test of cbor_test_15()", cbor_test_15}, {"test of cbor_test_16()", cbor_test_16},
    {"test of cbor_test_17()", cbor_test_17}, {"test of cbor_test_18()", cbor_test_18},
    {"test of cbor_test_19()", cbor_test_19}, {"test of cbor_test_20()", cbor_test_20},
    {"test of cbor_test_21()", cbor_test_21}, {"test of cbor_test_22()", cbor_test_22},
    {"test of cbor_test_23()", cbor_test_23}, {"test of cbor_test_24()", cbor_test_24},
    {"test of cbor_test_25()", cbor_test_25}, {"test of cbor_test_26()", cbor_test_26},
    {"test of cbor_test_27()", cbor_test_27}, {"test of cbor_test_28()", cbor_test_28},
    {"test of cbor_test_29()", cbor_test_29}, {"test of cbor_test_30()", cbor_test_30},
    {"test of cbor_test_31()", cbor_test_31}, {"test of cbor_test_32()", cbor_test_32},
    {"test of cbor_test_33()", cbor_test_33}, {"test of cbor_test_34()", cbor_test_34},
    {"test of cbor_test_35()", cbor_test_35}, {"test of cbor_test_36()", cbor_test_36},
    {"test of cbor_test_37()", cbor_test_37}, {"test of cbor_test_38()", cbor_test_38},
    {"test of cbor_test_39()", cbor_test_39}, {"test of cbor_test_40()", cbor_test_40},
    {"test of cbor_test_41()", cbor_test_41}, {"test of cbor_test_42()", cbor_test_42},
    {"test of cbor_test_43()", cbor_test_43}, {"test of cbor_test_44()", cbor_test_44},
    {"test of cbor_test_45()", cbor_test_45}, {"test of cbor_test_46()", cbor_test_46},
    {"test of cbor_test_47()", cbor_test_47}, {"test of cbor_test_48()", cbor_test_48},
    {"test of cbor_test_49()", cbor_test_49}, {"test of cbor_test_50()", cbor_test_50},
    {"test of cbor_test_51()", cbor_test_51}, {"test of cbor_test_52()", cbor_test_52},
    {"test of cbor_test_53()", cbor_test_53}, {"test of cbor_test_54()", cbor_test_54},
    {"test of cbor_test_55()", cbor_test_55}, {"test of cbor_test_56()", cbor_test_56},
    {"test of cbor_test_57()", cbor_test_57}, {"test of cbor_test_58()", cbor_test_58},
    {"test of cbor_test_59()", cbor_test_59}, {"test of cbor_test_60()", cbor_test_60},
    {"test of cbor_test_61()", cbor_test_61}, {"test of cbor_test_62()", cbor_test_62},
    {"test of cbor_test_63()", cbor_test_63}, {"test of cbor_test_64()", cbor_test_64},
    {"test of cbor_test_65()", cbor_test_65}, {"test of cbor_test_66()", cbor_test_66},
    {"test of cbor_test_67()", cbor_test_67}, {"test of cbor_test_68()", cbor_test_68},
    {"test of cbor_test_69()", cbor_test_69}, {"test of cbor_test_70()", cbor_test_70},
    {"test of cbor_test_71()", cbor_test_71}, {"test of cbor_test_72()", cbor_test_72},
    {"test of cbor_test_73()", cbor_test_73}, {"test of cbor_test_74()", cbor_test_74},
    {"test of cbor_test_75()", cbor_test_75}, {"test of cbor_test_76()", cbor_test_76},
    {"test of cbor_test_77()", cbor_test_77}, {"test of cbor_test_78()", cbor_test_78},
    {"test of cbor_test_79()", cbor_test_79}, {"test of cbor_test_80()", cbor_test_80},
    {"test of cbor_test_81()", cbor_test_81}, {"test of cbor_test_82()", cbor_test_82},
    {"test of cbor_test_83()", cbor_test_83}, {NULL, NULL},
};

CU_ErrorCode create_cbor_suit(void) {
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite("Suite_CBOR", NULL, NULL);
    if (NULL == pSuite) {
        return CU_get_error();
    }

    return add_tests(pSuite, table);
}

#endif
#endif
