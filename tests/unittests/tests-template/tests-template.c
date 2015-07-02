/*
 * Copyright (C) 2015 AUTHOR
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
#include "embUnit.h"

#include "unittests-constants.h"

static void set_up (void)
{
    /* set up code goes here */
}

static void tear_down(void)
{
    /* tear down code goes here */
}

static void template_test1(void)
{
}


Test *template_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(template_test1),
    };

    EMB_UNIT_TESTCALLER(template_tests_caller, set_up, tear_down, fixtures);

    return (Test *)&template_tests_caller;
}

void tests_template(void)
{
    TESTS_RUN(template_tests());
}
/** @} */
