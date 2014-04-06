/*
 * Copyright (C) 2013 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     core_util
 * @{
 *
 * @file        bitarithm.c
 * @brief       Bit arithmetic helper functions implementation
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Martin Lenders <mlenders@inf.fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>

#define ENABLE_DEBUG    (0)
#include "debug.h"

unsigned
number_of_highest_bit(unsigned v)
{
    DEBUG_PIN_TOGGLE;
    register unsigned r; // result of log2(v) will go here

#if ARCH_32_BIT
    register unsigned shift;

    r =     (v > 0xFFFF) << 4; v >>= r;
    shift = (v > 0xFF  ) << 3; v >>= shift; r |= shift;
    shift = (v > 0xF   ) << 2; v >>= shift; r |= shift;
    shift = (v > 0x3   ) << 1; v >>= shift; r |= shift;
                                            r |= (v >> 1);
#else
    r = 0;
    while (v >>= 1) { // unroll for more speed...
        r++;
    }

#endif

    DEBUG_PIN_TOGGLE;
    return r;
}
/*---------------------------------------------------------------------------*/
unsigned
number_of_lowest_bit(register unsigned v)
{
    DEBUG_PIN_TOGGLE;
    register unsigned r = 0;

    while ((v & 0x01) == 0) {
        v >>= 1;
        r++;
    };

    DEBUG_PIN_TOGGLE;
    return r;
}
/*---------------------------------------------------------------------------*/
unsigned
number_of_bits_set(unsigned v)
{
    DEBUG_PIN_TOGGLE;
    unsigned c; // c accumulates the total bits set in v

    for (c = 0; v; c++) {
        v &= v - 1; // clear the least significant bit set
    }

    DEBUG_PIN_TOGGLE;
    return c;
}
