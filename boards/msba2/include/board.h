/*
 * Copyright (C) 2013 Freie Universität Berlin
 *
 * This file subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @defgroup    boards_msba2 MSB-A2
 * @ingroup     boards
 * @brief       Support for the MSB-A2 board
 * @{
 *
 * @file        board.h
 * @brief       Basic definitions for the MSB-A2 board
 *
 * @author      unknown
 */

#ifndef __BOARD_H
#define __BOARD_H

#include "msba2_common.h"
#include "bitarithm.h"

#define LED_RED_PIN (BIT25)
#define LED_GREEN_PIN (BIT26)

#define LED_GREEN_OFF (FIO3SET = LED_GREEN_PIN)
#define LED_GREEN_ON (FIO3CLR = LED_GREEN_PIN)
#define LED_GREEN_TOGGLE (FIO3PIN ^= LED_GREEN_PIN)

#define LED_RED_OFF (FIO3SET = LED_RED_PIN)
#define LED_RED_ON (FIO3CLR = LED_RED_PIN)
#define LED_RED_TOGGLE (FIO3PIN ^= LED_RED_PIN)

/* Use P0.0 as debug PIN, since we don't use neither CAN, USART3, nor I2C on
 * this board*/
#define DEBUG_PIN   (BIT0)

#define BOARD_DEBUG_PIN_OFF       (FIO0CLR = DEBUG_PIN)
#define BOARD_DEBUG_PIN_ON        (FIO0SET = DEBUG_PIN)
#define BOARD_DEBUG_PIN_TOGGLE    (FIO0PIN ^= DEBUG_PIN)

void init_clks1(void);

typedef uint8_t radio_packet_length_t;

#endif /* __BOARD_H */
