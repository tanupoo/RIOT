/*
 * Copyright (C) 2015 Oliver Hahm <oliver.hahm@inria.fr>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 */
#include "tests-sixlowpan_netif.h"
#include "embUnit.h"

#include "kernel_types.h"

#include "unittests-constants.h"

#include "net/ng_sixlowpan/netif.h"
#include "net/ng_netif.h"

static void set_up (void)
{
    ng_netif_init();
    ng_sixlowpan_netif_init();
}

/* try to get an interface why none exist */
static void test_sixlowpan_netif_get_none_exist(void)
{
    TEST_ASSERT_NULL(ng_sixlowpan_netif_get(TEST_UINT8));
}

/* add a non-existing PID to 6LoWPAN and try to get it */
static void test_sixlowpan_netif_add_one_non_existent_0(void)
{
    ng_sixlowpan_netif_add(TEST_UINT8, 4);

    TEST_ASSERT_NULL(ng_sixlowpan_netif_get(TEST_UINT8));
}

static void test_sixlowpan_netif_add_one_non_existent_1(void)
{
    TEST_ASSERT_EQUAL_INT(0, ng_netif_add(TEST_UINT8));

    ng_sixlowpan_netif_add(TEST_UINT8  - 1, 4);

    TEST_ASSERT_NULL(ng_sixlowpan_netif_get(TEST_UINT8));
    TEST_ASSERT_NULL(ng_sixlowpan_netif_get(TEST_UINT8 - 1));
}

static void test_sixlowpan_netif_add_one_0(void)
{
    TEST_ASSERT_EQUAL_INT(0, ng_netif_add(TEST_UINT8));

    ng_sixlowpan_netif_add(TEST_UINT8, 0);

    TEST_ASSERT_NOT_NULL(ng_sixlowpan_netif_get(TEST_UINT8));
}

static void test_sixlowpan_netif_add_one_1(void)
{
    TEST_ASSERT_EQUAL_INT(0, ng_netif_add(TEST_UINT8));

    ng_sixlowpan_netif_add(TEST_UINT8, 1);

    TEST_ASSERT_NOT_NULL(ng_sixlowpan_netif_get(TEST_UINT8));
}

static void test_sixlowpan_netif_add_one_2(void)
{
    TEST_ASSERT_EQUAL_INT(0, ng_netif_add(TEST_UINT8));

    ng_sixlowpan_netif_add(TEST_UINT8, 4);

    TEST_ASSERT_NOT_NULL(ng_sixlowpan_netif_get(TEST_UINT8));
}

static void test_sixlowpan_netif_add_one_3(void)
{
    TEST_ASSERT_EQUAL_INT(0, ng_netif_add(TEST_UINT8));

    ng_sixlowpan_netif_add(TEST_UINT8, TEST_UINT8);

    TEST_ASSERT_NOT_NULL(ng_sixlowpan_netif_get(TEST_UINT8));
}

static void test_sixlowpan_netif_add_one_4(void)
{
    TEST_ASSERT_EQUAL_INT(0, ng_netif_add(TEST_UINT8));

    ng_sixlowpan_netif_add(TEST_UINT8, UINT8_MAX);

    TEST_ASSERT_NOT_NULL(ng_sixlowpan_netif_get(TEST_UINT8));
}

/* XXX: case is not specified */
static void test_sixlowpan_netif_add_one_twice(void)
{
    TEST_ASSERT_EQUAL_INT(0, ng_netif_add(TEST_UINT8));

    ng_sixlowpan_netif_add(TEST_UINT8, 4);
    ng_sixlowpan_netif_add(TEST_UINT8, 4);

    TEST_ASSERT_NOT_NULL(ng_sixlowpan_netif_get(TEST_UINT8));
}

static void test_sixlowpan_netif_add_multiple_0(void)
{
    kernel_pid_t p1 = TEST_UINT8;
    kernel_pid_t p2 = TEST_UINT8 + 1;

    TEST_ASSERT_EQUAL_INT(0, ng_netif_add(p1));
    TEST_ASSERT_EQUAL_INT(0, ng_netif_add(p2));

    ng_sixlowpan_netif_add(p1, 4);
    ng_sixlowpan_netif_add(p2, 4);

    TEST_ASSERT_NOT_NULL(ng_sixlowpan_netif_get(TEST_UINT8));
    TEST_ASSERT_NOT_NULL(ng_sixlowpan_netif_get(TEST_UINT8 + 1));
}

static void test_sixlowpan_netif_add_multiple_1(void)
{
    kernel_pid_t p[4];

    for (unsigned i = 0; i < 4; i++) {
        p[i] = TEST_UINT8 + i;
        TEST_ASSERT_EQUAL_INT(0, ng_netif_add(p[i]));
        ng_sixlowpan_netif_add(p[i], 4);
        TEST_ASSERT_NOT_NULL(ng_sixlowpan_netif_get(p[i]));
    }
}

static void test_sixlowpan_netif_add_multiple_2(void)
{
    kernel_pid_t p[UINT8_MAX];

    for (unsigned i = 0; i < UINT8_MAX; i++) {
        p[i] = TEST_UINT8 + i;
        TEST_ASSERT_EQUAL_INT(0, ng_netif_add(p[i]));
        ng_sixlowpan_netif_add(p[i], 4);
        TEST_ASSERT_NOT_NULL(ng_sixlowpan_netif_get(p[i]));
    }
}

static void test_sixlowpan_netif_remove_none_exist_0(void)
{
    ng_sixlowpan_netif_remove(TEST_UINT8);

    TEST_ASSERT_NULL(ng_sixlowpan_netif_get(TEST_UINT8));
}

static void test_sixlowpan_netif_remove_none_exist_1(void)
{
    for (unsigned i = 0; i < UINT_MAX; i++) {
        ng_sixlowpan_netif_remove(TEST_UINT8);
    }

    TEST_ASSERT_NULL(ng_sixlowpan_netif_get(TEST_UINT8));
}

static void test_sixlowpan_netif_remove_none_exist_2(void)
{
    ng_sixlowpan_netif_add(TEST_UINT8, 4);

    ng_sixlowpan_netif_remove(TEST_UINT8 + 1);

    TEST_ASSERT_NULL(ng_sixlowpan_netif_get(TEST_UINT8));
    TEST_ASSERT_NULL(ng_sixlowpan_netif_get(TEST_UINT8 + 1));
}

static void test_sixlowpan_netif_remove_not_added(void)
{
    TEST_ASSERT_EQUAL_INT(0, ng_netif_add(TEST_UINT8));

    ng_sixlowpan_netif_add(TEST_UINT8, 4);

    ng_sixlowpan_netif_remove(TEST_UINT8 + 1);

    TEST_ASSERT(ng_sixlowpan_netif_get(TEST_UINT8));
    TEST_ASSERT_NULL(ng_sixlowpan_netif_get(TEST_UINT8 + 1));
}

static void test_sixlowpan_netif_remove_twice(void)
{
    TEST_ASSERT_EQUAL_INT(0, ng_netif_add(TEST_UINT8));

    ng_sixlowpan_netif_add(TEST_UINT8, 4);

    ng_sixlowpan_netif_remove(TEST_UINT8);

    TEST_ASSERT_NULL(ng_sixlowpan_netif_get(TEST_UINT8));
    
    ng_sixlowpan_netif_remove(TEST_UINT8);

    TEST_ASSERT_NULL(ng_sixlowpan_netif_get(TEST_UINT8 + 1));
}

static void test_sixlowpan_netif_remove(void)
{
    TEST_ASSERT_EQUAL_INT(0, ng_netif_add(TEST_UINT8));

    ng_sixlowpan_netif_add(TEST_UINT8, 4);

    ng_sixlowpan_netif_remove(TEST_UINT8 + 1);

    TEST_ASSERT(ng_sixlowpan_netif_get(TEST_UINT8));
    
    ng_sixlowpan_netif_remove(TEST_UINT8);

    TEST_ASSERT_NULL(ng_sixlowpan_netif_get(TEST_UINT8 + 1));
}

Test *test_sixlowpan_netif_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_sixlowpan_netif_get_none_exist),
        new_TestFixture(test_sixlowpan_netif_add_one_non_existent_0),
        new_TestFixture(test_sixlowpan_netif_add_one_non_existent_1),
        new_TestFixture(test_sixlowpan_netif_add_one_0),
        new_TestFixture(test_sixlowpan_netif_add_one_1),
        new_TestFixture(test_sixlowpan_netif_add_one_2),
        new_TestFixture(test_sixlowpan_netif_add_one_3),
        new_TestFixture(test_sixlowpan_netif_add_one_4),
        new_TestFixture(test_sixlowpan_netif_add_one_twice),
        new_TestFixture(test_sixlowpan_netif_add_multiple_0),
        new_TestFixture(test_sixlowpan_netif_add_multiple_1),
        new_TestFixture(test_sixlowpan_netif_add_multiple_2),
        new_TestFixture(test_sixlowpan_netif_remove_none_exist_0),
        new_TestFixture(test_sixlowpan_netif_remove_none_exist_1),
        new_TestFixture(test_sixlowpan_netif_remove_none_exist_2),
        new_TestFixture(test_sixlowpan_netif_remove_not_added),
        new_TestFixture(test_sixlowpan_netif_remove_twice),
        new_TestFixture(test_sixlowpan_netif_remove),
    };

    EMB_UNIT_TESTCALLER(test_sixlowpan_netif_tests_caller, set_up, NULL, fixtures);

    return (Test *)&test_sixlowpan_netif_tests_caller;
}

void tests_sixlowpan_netif(void)
{
    TESTS_RUN(test_sixlowpan_netif_tests());
}
/** @} */
