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
#include "tests-sixlowpan_frag.h"
#include "embUnit.h"

#include "kernel_types.h"

#include "unittests-constants.h"

#include "net/ng_sixlowpan/frag.h"

#define NO_FRAG0_0 { .u16 = 0x00 }

static void set_up (void)
{
}

static void test_sixlowpan_frag_is_no_frag_0(void)
{
    ng_sixlowpan_frag_t frag = {.disp_size = NO_FRAG0_0, .tag = { .u16 = TEST_UINT16}};
    TEST_ASSERT(!ng_sixlowpan_frag_is(&frag));
}

Test *test_sixlowpan_frag_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_sixlowpan_frag_is_no_frag_0),
    };

    EMB_UNIT_TESTCALLER(test_sixlowpan_frag_tests_caller, set_up, NULL, fixtures);

    return (Test *)&test_sixlowpan_frag_tests_caller;
}

void tests_sixlowpan_frag(void)
{
    TESTS_RUN(test_sixlowpan_frag_tests());
}
/** @} */
