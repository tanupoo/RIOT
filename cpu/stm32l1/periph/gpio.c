/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     cpu_stm32l1
 * @{
 *
 * @file
 * @brief       Low-level GPIO driver implementation
 *
 * @author      Hauke Petersen <mail@haukepetersen.de>
 * @author      Thomas Eichinger <thomas.eichinger@fu-berlin.de>
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 *
 * @}
 */

#include "cpu.h"
#include "stm32l1xx.h"
#include "periph/gpio.h"
#include "periph_conf.h"
#include "board.h"

#include "thread.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

typedef struct {
    void (*cb)(void);
} gpio_state_t;

static gpio_state_t config[GPIO_NUMOF];


int gpio_init_out(gpio_t dev, gpio_pp_t pushpull)
{
    GPIO_TypeDef *port;
    uint32_t pin;

    switch (dev) {
#if GPIO_0_EN
        case GPIO_0:
            GPIO_0_CLKEN();
            port = GPIO_0_PORT;
            pin = GPIO_0_PIN;
            break;
#endif
#if GPIO_1_EN
        case GPIO_1:
            GPIO_1_CLKEN();
            port = GPIO_1_PORT;
            pin = GPIO_1_PIN;
            break;
#endif
#if GPIO_2_EN
        case GPIO_2:
            GPIO_2_CLKEN();
            port = GPIO_2_PORT;
            pin = GPIO_2_PIN;
            break;
#endif
#if GPIO_3_EN
        case GPIO_3:
            GPIO_3_CLKEN();
            port = GPIO_3_PORT;
            pin = GPIO_3_PIN;
            break;
#endif
#if GPIO_4_EN
        case GPIO_4:
            GPIO_4_CLKEN();
            port = GPIO_4_PORT;
            pin = GPIO_4_PIN;
            break;
#endif
#if GPIO_5_EN
        case GPIO_5:
            GPIO_5_CLKEN();
            port = GPIO_5_PORT;
            pin = GPIO_5_PIN;
            break;
#endif
#if GPIO_6_EN
        case GPIO_6:
            GPIO_6_CLKEN();
            port = GPIO_6_PORT;
            pin = GPIO_6_PIN;
            break;
#endif
#if GPIO_7_EN
        case GPIO_7:
            GPIO_7_CLKEN();
            port = GPIO_7_PORT;
            pin = GPIO_7_PIN;
            break;
#endif
#if GPIO_8_EN
        case GPIO_8:
            GPIO_8_CLKEN();
            port = GPIO_8_PORT;
            pin = GPIO_8_PIN;
            break;
#endif
#if GPIO_9_EN
        case GPIO_9:
            GPIO_9_CLKEN();
            port = GPIO_9_PORT;
            pin = GPIO_9_PIN;
            break;
#endif
#if GPIO_10_EN
        case GPIO_10:
            GPIO_10_CLKEN();
            port = GPIO_10_PORT;
            pin = GPIO_10_PIN;
            break;
#endif
#if GPIO_11_EN
        case GPIO_11:
            GPIO_11_CLKEN();
            port = GPIO_11_PORT;
            pin = GPIO_11_PIN;
            break;
#endif
#if GPIO_12_EN
        case GPIO_12:
            GPIO_12_CLKEN();
            port = GPIO_12_PORT;
            pin = GPIO_12_PIN;
            break;
#endif
#if GPIO_13_EN
        case GPIO_13:
            GPIO_13_CLKEN();
            port = GPIO_13_PORT;
            pin = GPIO_13_PIN;
            break;
#endif
#if GPIO_14_EN
        case GPIO_14:
            GPIO_14_CLKEN();
            port = GPIO_14_PORT;
            pin = GPIO_14_PIN;
            break;
#endif
#if GPIO_15_EN
        case GPIO_15:
            GPIO_15_CLKEN();
            port = GPIO_15_PORT;
            pin = GPIO_15_PIN;
            break;
#endif

        case GPIO_UNDEFINED:
        default:
            return -1;
    }

    port->MODER &= ~(2 << (2 * pin));           /* set pin to output mode */
    port->MODER |= (1 << (2 * pin));
    port->OTYPER &= ~(1 << pin);                /* set to push-pull configuration */
    port->OSPEEDR |= (3 << (2 * pin));          /* set to high speed */
    port->PUPDR &= ~(3 << (2 * pin));           /* configure push-pull resistors */
    port->PUPDR |= (pushpull << (2 * pin));
    port->ODR &= ~(1 << pin);                   /* set pin to low signal */

    return 0; /* all OK */
}

int gpio_init_in(gpio_t dev, gpio_pp_t pushpull)
{
    GPIO_TypeDef *port;
    uint32_t pin;

    switch (dev) {
#if GPIO_0_EN
        case GPIO_0:
            GPIO_0_CLKEN();
            port = GPIO_0_PORT;
            pin = GPIO_0_PIN;
            break;
#endif
#if GPIO_1_EN
        case GPIO_1:
            GPIO_1_CLKEN();
            port = GPIO_1_PORT;
            pin = GPIO_1_PIN;
            break;
#endif
#if GPIO_2_EN
        case GPIO_2:
            GPIO_2_CLKEN();
            port = GPIO_2_PORT;
            pin = GPIO_2_PIN;
            break;
#endif
#if GPIO_3_EN
        case GPIO_3:
            GPIO_3_CLKEN();
            port = GPIO_3_PORT;
            pin = GPIO_3_PIN;
            break;
#endif
#if GPIO_4_EN
        case GPIO_4:
            GPIO_4_CLKEN();
            port = GPIO_4_PORT;
            pin = GPIO_4_PIN;
            break;
#endif
#if GPIO_5_EN
        case GPIO_5:
            GPIO_5_CLKEN();
            port = GPIO_5_PORT;
            pin = GPIO_5_PIN;
            break;
#endif
#if GPIO_6_EN
        case GPIO_6:
            GPIO_6_CLKEN();
            port = GPIO_6_PORT;
            pin = GPIO_6_PIN;
            break;
#endif
#if GPIO_7_EN
        case GPIO_7:
            GPIO_7_CLKEN();
            port = GPIO_7_PORT;
            pin = GPIO_7_PIN;
            break;
#endif
#if GPIO_8_EN
        case GPIO_8:
            GPIO_8_CLKEN();
            port = GPIO_8_PORT;
            pin = GPIO_8_PIN;
            break;
#endif
#if GPIO_9_EN
        case GPIO_9:
            GPIO_9_CLKEN();
            port = GPIO_9_PORT;
            pin = GPIO_9_PIN;
            break;
#endif
#if GPIO_10_EN
        case GPIO_10:
            GPIO_10_CLKEN();
            port = GPIO_10_PORT;
            pin = GPIO_10_PIN;
            break;
#endif
#if GPIO_11_EN
        case GPIO_11:
            GPIO_11_CLKEN();
            port = GPIO_11_PORT;
            pin = GPIO_11_PIN;
            break;
#endif
#if GPIO_12_EN
        case GPIO_12:
            GPIO_12_CLKEN();
            port = GPIO_12_PORT;
            pin = GPIO_12_PIN;
            break;
#endif
#if GPIO_13_EN
        case GPIO_13:
            GPIO_13_CLKEN();
            port = GPIO_13_PORT;
            pin = GPIO_13_PIN;
            break;
#endif
#if GPIO_14_EN
        case GPIO_14:
            GPIO_14_CLKEN();
            port = GPIO_14_PORT;
            pin = GPIO_14_PIN;
            break;
#endif
#if GPIO_15_EN
        case GPIO_15:
            GPIO_15_CLKEN();
            port = GPIO_15_PORT;
            pin = GPIO_15_PIN;
            break;
#endif
        case GPIO_UNDEFINED:
        default:
            return -1;
    }

    port->MODER &= ~(3 << (2 * pin));           /* configure pin as input */
    port->PUPDR &= ~(3 << (2 * pin));           /* configure push-pull resistors */
    port->PUPDR |= (pushpull << (2 * pin));

    return 0; /* everything alright here */
}

int gpio_init_int(gpio_t dev, gpio_pp_t pushpull, gpio_flank_t flank, void (*cb)(void))
{
    uint8_t exti_line;
    uint8_t gpio_irq;

    /* configure pin as input */
    int res = gpio_init_in(dev, pushpull);
    if (res < 0) {
        return res;
    }

    /* set interrupt priority (its the same for all EXTI interrupts) */
    NVIC_SetPriority(EXTI0_IRQn, GPIO_IRQ_PRIO);
    NVIC_SetPriority(EXTI1_IRQn, GPIO_IRQ_PRIO);
    NVIC_SetPriority(EXTI2_IRQn, GPIO_IRQ_PRIO);
    NVIC_SetPriority(EXTI4_IRQn, GPIO_IRQ_PRIO);

    /* enable the SYSCFG clock */
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    /* read pin number, set EXTI channel and enable global interrupt for EXTI channel */
    switch (dev) {
#if GPIO_0_EN
        case GPIO_0:
            exti_line = GPIO_0_EXTI_LINE;
            gpio_irq = GPIO_IRQ_0;
            GPIO_0_EXTI_CFG1();
            GPIO_0_EXTI_CFG2();
            NVIC_SetPriority(GPIO_0_IRQ, GPIO_IRQ_PRIO);
            NVIC_EnableIRQ(GPIO_0_IRQ);
            break;
#endif
#if GPIO_1_EN
        case GPIO_1:
            exti_line = GPIO_1_EXTI_LINE;
            gpio_irq = GPIO_IRQ_1;
            GPIO_1_EXTI_CFG1();
            GPIO_1_EXTI_CFG2();
            NVIC_SetPriority(GPIO_1_IRQ, GPIO_IRQ_PRIO);
            NVIC_EnableIRQ(GPIO_1_IRQ);
            break;
#endif
#if GPIO_2_EN
        case GPIO_2:
            exti_line = GPIO_2_EXTI_LINE;
            gpio_irq = GPIO_IRQ_2;
            GPIO_2_EXTI_CFG1();
            GPIO_2_EXTI_CFG2();
            NVIC_SetPriority(GPIO_2_IRQ, GPIO_IRQ_PRIO);
            NVIC_EnableIRQ(GPIO_2_IRQ);
            break;
#endif
#if GPIO_3_EN
        case GPIO_3:
            exti_line = GPIO_3_EXTI_LINE;
            gpio_irq = GPIO_IRQ_3;
            GPIO_3_EXTI_CFG1();
            GPIO_3_EXTI_CFG2();
            NVIC_SetPriority(GPIO_3_IRQ, GPIO_IRQ_PRIO);
            NVIC_EnableIRQ(GPIO_3_IRQ);
            break;
#endif
#if GPIO_4_EN
        case GPIO_4:
            exti_line = GPIO_4_EXTI_LINE;
            gpio_irq = GPIO_IRQ_4;
            GPIO_4_EXTI_CFG1();
            GPIO_4_EXTI_CFG2();
            NVIC_SetPriority(GPIO_4_IRQ, GPIO_IRQ_PRIO);
            NVIC_EnableIRQ(GPIO_4_IRQ);
            break;
#endif
#if GPIO_5_EN
        case GPIO_5:
            exti_line = GPIO_5_EXTI_LINE;
            gpio_irq = GPIO_IRQ_5;
            GPIO_5_EXTI_CFG1();
            GPIO_5_EXTI_CFG2();
            NVIC_SetPriority(GPIO_5_IRQ, GPIO_IRQ_PRIO);
            NVIC_EnableIRQ(GPIO_5_IRQ);
            break;
#endif
#if GPIO_6_EN
        case GPIO_6:
            exti_line = GPIO_6_EXTI_LINE;
            gpio_irq = GPIO_IRQ_6;
            GPIO_6_EXTI_CFG1();
            GPIO_6_EXTI_CFG2();
            NVIC_SetPriority(GPIO_6_IRQ, GPIO_IRQ_PRIO);
            NVIC_EnableIRQ(GPIO_6_IRQ);
            break;
#endif
#if GPIO_7_EN
        case GPIO_7:
            exti_line = GPIO_7_EXTI_LINE;
            gpio_irq = GPIO_IRQ_7;
            GPIO_7_EXTI_CFG1();
            GPIO_7_EXTI_CFG2();
            NVIC_SetPriority(GPIO_7_IRQ, GPIO_IRQ_PRIO);
            NVIC_EnableIRQ(GPIO_7_IRQ);
            break;
#endif
#if GPIO_8_EN
        case GPIO_8:
            exti_line = GPIO_8_EXTI_LINE;
            gpio_irq = GPIO_IRQ_8;
            GPIO_8_EXTI_CFG1();
            GPIO_8_EXTI_CFG2();
            NVIC_SetPriority(GPIO_8_IRQ, GPIO_IRQ_PRIO);
            NVIC_EnableIRQ(GPIO_8_IRQ);
            break;
#endif
#if GPIO_9_EN
        case GPIO_9:
            exti_line = GPIO_9_EXTI_LINE;
            gpio_irq = GPIO_IRQ_9;
            GPIO_9_EXTI_CFG1();
            GPIO_9_EXTI_CFG2();
            NVIC_SetPriority(GPIO_9_IRQ, GPIO_IRQ_PRIO);
            NVIC_EnableIRQ(GPIO_9_IRQ);
            break;
#endif
#if GPIO_10_EN
        case GPIO_10:
            exti_line = GPIO_10_EXTI_LINE;
            gpio_irq = GPIO_IRQ_10;
            GPIO_10_EXTI_CFG1();
            GPIO_10_EXTI_CFG2();
            NVIC_SetPriority(GPIO_10_IRQ, GPIO_IRQ_PRIO);
            NVIC_EnableIRQ(GPIO_10_IRQ);
            break;
#endif
#if GPIO_11_EN
        case GPIO_11:
            exti_line = GPIO_11_EXTI_LINE;
            gpio_irq = GPIO_IRQ_11;
            GPIO_11_EXTI_CFG1();
            GPIO_11_EXTI_CFG2();
            NVIC_SetPriority(GPIO_11_IRQ, GPIO_IRQ_PRIO);
            NVIC_EnableIRQ(GPIO_11_IRQ);
            break;
#endif
#if GPIO_12_EN
        case GPIO_12:
            exti_line = GPIO_12_EXTI_LINE;
            gpio_irq = GPIO_IRQ_12;
            GPIO_12_EXTI_CFG1();
            GPIO_12_EXTI_CFG2();
            NVIC_SetPriority(GPIO_12_IRQ, GPIO_IRQ_PRIO);
            NVIC_EnableIRQ(GPIO_12_IRQ);
            break;
#endif
#if GPIO_13_EN
        case GPIO_13:
            exti_line = GPIO_13_EXTI_LINE;
            gpio_irq = GPIO_IRQ_13;
            GPIO_13_EXTI_CFG1();
            GPIO_13_EXTI_CFG2();
            NVIC_SetPriority(GPIO_13_IRQ, GPIO_IRQ_PRIO);
            NVIC_EnableIRQ(GPIO_13_IRQ);
            break;
#endif
#if GPIO_14_EN
        case GPIO_14:
            exti_line = GPIO_14_EXTI_LINE;
            gpio_irq = GPIO_IRQ_14;
            GPIO_14_EXTI_CFG1();
            GPIO_14_EXTI_CFG2();
            NVIC_SetPriority(GPIO_14_IRQ, GPIO_IRQ_PRIO);
            NVIC_EnableIRQ(GPIO_14_IRQ);
            break;
#endif
#if GPIO_15_EN
        case GPIO_15:
            exti_line = GPIO_15_EXTI_LINE;
            gpio_irq = GPIO_IRQ_15;
            GPIO_15_EXTI_CFG1();
            GPIO_15_EXTI_CFG2();
            NVIC_SetPriority(GPIO_15_IRQ, GPIO_IRQ_PRIO);
            NVIC_EnableIRQ(GPIO_15_IRQ);
            break;
#endif
        case GPIO_UNDEFINED:
        default:
            return -1;
    }

    /* set callback */
    config[gpio_irq].cb = cb;

    /* configure the event that triggers an interrupt */
    switch (flank) {
        case GPIO_RISING:
            EXTI->RTSR |= (1 << exti_line);
            EXTI->FTSR &= ~(1 << exti_line);
            break;
        case GPIO_FALLING:
            EXTI->RTSR &= ~(1 << exti_line);
            EXTI->FTSR |= (1 << exti_line);
            break;
        case GPIO_BOTH:
            EXTI->RTSR |= (1 << exti_line);
            EXTI->FTSR |= (1 << exti_line);
            break;
    }

    /* clear event mask */
    EXTI->EMR &= ~(1 << exti_line);
    /* unmask the pins interrupt channel */
    EXTI->IMR |= (1 << exti_line);

    return 0;
}

int gpio_irq_enable(gpio_t dev)
{
    uint8_t exti_line;

    switch(dev) {
#if GPIO_0_EN
        case GPIO_0:
            exti_line = GPIO_0_EXTI_LINE;
            break;
#endif
#if GPIO_1_EN
        case GPIO_1:
            exti_line = GPIO_1_EXTI_LINE;
            break;
#endif
#if GPIO_2_EN
        case GPIO_2:
            exti_line = GPIO_2_EXTI_LINE;
            break;
#endif
#if GPIO_3_EN
        case GPIO_3:
            exti_line = GPIO_3_EXTI_LINE;
            break;
#endif
#if GPIO_4_EN
        case GPIO_4:
            exti_line = GPIO_4_EXTI_LINE;
            break;
#endif
#if GPIO_5_EN
        case GPIO_5:
            exti_line = GPIO_5_EXTI_LINE;
            break;
#endif
#if GPIO_6_EN
        case GPIO_6:
            exti_line = GPIO_6_EXTI_LINE;
            break;
#endif
#if GPIO_7_EN
        case GPIO_7:
            exti_line = GPIO_7_EXTI_LINE;
            break;
#endif
#if GPIO_8_EN
        case GPIO_8:
            exti_line = GPIO_8_EXTI_LINE;
            break;
#endif
#if GPIO_9_EN
        case GPIO_9:
            exti_line = GPIO_9_EXTI_LINE;
            break;
#endif
#if GPIO_10_EN
        case GPIO_10:
            exti_line = GPIO_10_EXTI_LINE;
            break;
#endif
#if GPIO_11_EN
        case GPIO_11:
            exti_line = GPIO_11_EXTI_LINE;
            break;
#endif
#if GPIO_12_EN
        case GPIO_12:
            exti_line = GPIO_12_EXTI_LINE;
            break;
#endif
#if GPIO_13_EN
        case GPIO_13:
            exti_line = GPIO_13_EXTI_LINE;
            break;
#endif
#if GPIO_14_EN
        case GPIO_14:
            exti_line = GPIO_14_EXTI_LINE;
            break;
#endif
#if GPIO_15_EN
        case GPIO_15:
            exti_line = GPIO_15_EXTI_LINE;
            break;
#endif

        case GPIO_UNDEFINED:
        default:
            return -1;
    }
    /* save state */
    int state = (EXTI->IMR & (1 << exti_line) >> exti_line);

    /* unmask the pins interrupt channel */
    EXTI->IMR |= (1 << exti_line);

    return state;
}

int gpio_irq_disable(gpio_t dev)
{
    uint8_t exti_line;

    switch(dev) {
#if GPIO_0_EN
        case GPIO_0:
            exti_line = GPIO_0_EXTI_LINE;
            break;
#endif
#if GPIO_1_EN
        case GPIO_1:
            exti_line = GPIO_1_EXTI_LINE;
            break;
#endif
#if GPIO_2_EN
        case GPIO_2:
            exti_line = GPIO_2_EXTI_LINE;
            break;
#endif
#if GPIO_3_EN
        case GPIO_3:
            exti_line = GPIO_3_EXTI_LINE;
            break;
#endif
#if GPIO_4_EN
        case GPIO_4:
            exti_line = GPIO_4_EXTI_LINE;
            break;
#endif
#if GPIO_5_EN
        case GPIO_5:
            exti_line = GPIO_5_EXTI_LINE;
            break;
#endif
#if GPIO_6_EN
        case GPIO_6:
            exti_line = GPIO_6_EXTI_LINE;
            break;
#endif
#if GPIO_7_EN
        case GPIO_7:
            exti_line = GPIO_7_EXTI_LINE;
            break;
#endif
#if GPIO_8_EN
        case GPIO_8:
            exti_line = GPIO_8_EXTI_LINE;
            break;
#endif
#if GPIO_9_EN
        case GPIO_9:
            exti_line = GPIO_9_EXTI_LINE;
            break;
#endif
#if GPIO_10_EN
        case GPIO_10:
            exti_line = GPIO_10_EXTI_LINE;
            break;
#endif
#if GPIO_11_EN
        case GPIO_11:
            exti_line = GPIO_11_EXTI_LINE;
            break;
#endif
#if GPIO_12_EN
        case GPIO_12:
            exti_line = GPIO_12_EXTI_LINE;
            break;
#endif
#if GPIO_13_EN
        case GPIO_13:
            exti_line = GPIO_13_EXTI_LINE;
            break;
#endif
#if GPIO_14_EN
        case GPIO_14:
            exti_line = GPIO_14_EXTI_LINE;
            break;
#endif
#if GPIO_15_EN
        case GPIO_15:
            exti_line = GPIO_15_EXTI_LINE;
            break;
#endif
        case GPIO_UNDEFINED:
        default:
            return -1;
    }
    /* save state */
    int state = ((EXTI->IMR & (1 << exti_line)) >> exti_line);

    /* unmask the pins interrupt channel */
    EXTI->IMR &= ~(1 << exti_line);

    return state;
}

int gpio_read(gpio_t dev)
{
    GPIO_TypeDef *port;
    uint32_t pin;

    switch (dev) {
#if GPIO_0_EN
        case GPIO_0:
            port = GPIO_0_PORT;
            pin = GPIO_0_PIN;
            break;
#endif
#if GPIO_1_EN
        case GPIO_1:
            port = GPIO_1_PORT;
            pin = GPIO_1_PIN;
            break;
#endif
#if GPIO_2_EN
        case GPIO_2:
            port = GPIO_2_PORT;
            pin = GPIO_2_PIN;
            break;
#endif
#if GPIO_3_EN
        case GPIO_3:
            port = GPIO_3_PORT;
            pin = GPIO_3_PIN;
            break;
#endif
#if GPIO_4_EN
        case GPIO_4:
            port = GPIO_4_PORT;
            pin = GPIO_4_PIN;
            break;
#endif
#if GPIO_5_EN
        case GPIO_5:
            port = GPIO_5_PORT;
            pin = GPIO_5_PIN;
            break;
#endif
#if GPIO_6_EN
        case GPIO_6:
            port = GPIO_6_PORT;
            pin = GPIO_6_PIN;
            break;
#endif
#if GPIO_7_EN
        case GPIO_7:
            port = GPIO_7_PORT;
            pin = GPIO_7_PIN;
            break;
#endif
#if GPIO_8_EN
        case GPIO_8:
            port = GPIO_8_PORT;
            pin = GPIO_8_PIN;
            break;
#endif
#if GPIO_9_EN
        case GPIO_9:
            port = GPIO_9_PORT;
            pin = GPIO_9_PIN;
            break;
#endif
#if GPIO_10_EN
        case GPIO_10:
            port = GPIO_10_PORT;
            pin = GPIO_10_PIN;
            break;
#endif
#if GPIO_11_EN
        case GPIO_11:
            port = GPIO_11_PORT;
            pin = GPIO_11_PIN;
            break;
#endif
#if GPIO_12_EN
        case GPIO_12:
            port = GPIO_12_PORT;
            pin = GPIO_12_PIN;
            break;
#endif
#if GPIO_13_EN
        case GPIO_13:
            port = GPIO_13_PORT;
            pin = GPIO_13_PIN;
            break;
#endif
#if GPIO_14_EN
        case GPIO_14:
            port = GPIO_14_PORT;
            pin = GPIO_14_PIN;
            break;
#endif
#if GPIO_15_EN
        case GPIO_15:
            port = GPIO_15_PORT;
            pin = GPIO_15_PIN;
            break;
#endif
        case GPIO_UNDEFINED:
        default:
            return -1;
    }

    if (port->MODER & (3 << (pin * 2))) {       /* if configured as output */
        return port->ODR & (1 << pin);          /* read output data register */
    }
    else {
        return port->IDR & (1 << pin);          /* else read input data register */
    }
}

int gpio_set(gpio_t dev)
{
    switch (dev) {
#if GPIO_0_EN
        case GPIO_0:
            GPIO_0_PORT->ODR |= (1 << GPIO_0_PIN);
            break;
#endif
#if GPIO_1_EN
        case GPIO_1:
            GPIO_1_PORT->ODR |= (1 << GPIO_1_PIN);
            break;
#endif
#if GPIO_2_EN
        case GPIO_2:
            GPIO_2_PORT->ODR |= (1 << GPIO_2_PIN);
            break;
#endif
#if GPIO_3_EN
        case GPIO_3:
            GPIO_3_PORT->ODR |= (1 << GPIO_3_PIN);
            break;
#endif
#if GPIO_4_EN
        case GPIO_4:
            GPIO_4_PORT->ODR |= (1 << GPIO_4_PIN);
            break;
#endif
#if GPIO_5_EN
        case GPIO_5:
            GPIO_5_PORT->ODR |= (1 << GPIO_5_PIN);
            break;
#endif
#if GPIO_6_EN
        case GPIO_6:
            GPIO_6_PORT->ODR |= (1 << GPIO_6_PIN);
            break;
#endif
#if GPIO_7_EN
        case GPIO_7:
            GPIO_7_PORT->ODR |= (1 << GPIO_7_PIN);
            break;
#endif
#if GPIO_8_EN
        case GPIO_8:
            GPIO_8_PORT->ODR |= (1 << GPIO_8_PIN);
            break;
#endif
#if GPIO_9_EN
        case GPIO_9:
            GPIO_9_PORT->ODR |= (1 << GPIO_9_PIN);
            break;
#endif
#if GPIO_10_EN
        case GPIO_10:
            GPIO_10_PORT->ODR |= (1 << GPIO_10_PIN);
            break;
#endif
#if GPIO_11_EN
        case GPIO_11:
            GPIO_11_PORT->ODR |= (1 << GPIO_11_PIN);
            break;
#endif
#if GPIO_12_EN
        case GPIO_12:
            GPIO_12_PORT->ODR |= (1 << GPIO_12_PIN);
            break;
#endif
#if GPIO_13_EN
        case GPIO_13:
            GPIO_13_PORT->ODR |= (1 << GPIO_13_PIN);
            break;
#endif
#if GPIO_14_EN
        case GPIO_14:
            GPIO_14_PORT->ODR |= (1 << GPIO_14_PIN);
            break;
#endif
#if GPIO_15_EN
        case GPIO_15:
            GPIO_15_PORT->ODR |= (1 << GPIO_15_PIN);
            break;
#endif
        case GPIO_UNDEFINED:
        default:
            return -1;
    }

    return 0;
}

int gpio_clear(gpio_t dev)
{
    switch (dev) {
#if GPIO_0_EN
        case GPIO_0:
            GPIO_0_PORT->ODR &= ~(1 << GPIO_0_PIN);
            break;
#endif
#if GPIO_1_EN
        case GPIO_1:
            GPIO_1_PORT->ODR &= ~(1 << GPIO_1_PIN);
            break;
#endif
#if GPIO_2_EN
        case GPIO_2:
            GPIO_2_PORT->ODR &= ~(1 << GPIO_2_PIN);
            break;
#endif
#if GPIO_3_EN
        case GPIO_3:
            GPIO_3_PORT->ODR &= ~(1 << GPIO_3_PIN);
            break;
#endif
#if GPIO_4_EN
        case GPIO_4:
            GPIO_4_PORT->ODR &= ~(1 << GPIO_4_PIN);
            break;
#endif
#if GPIO_5_EN
        case GPIO_5:
            GPIO_5_PORT->ODR &= ~(1 << GPIO_5_PIN);
            break;
#endif
#if GPIO_6_EN
        case GPIO_6:
            GPIO_6_PORT->ODR &= ~(1 << GPIO_6_PIN);
            break;
#endif
#if GPIO_7_EN
        case GPIO_7:
            GPIO_7_PORT->ODR &= ~(1 << GPIO_7_PIN);
            break;
#endif
#if GPIO_8_EN
        case GPIO_8:
            GPIO_8_PORT->ODR &= ~(1 << GPIO_8_PIN);
            break;
#endif
#if GPIO_9_EN
        case GPIO_9:
            GPIO_9_PORT->ODR &= ~(1 << GPIO_9_PIN);
            break;
#endif
#if GPIO_10_EN
        case GPIO_10:
            GPIO_10_PORT->ODR &= ~(1 << GPIO_10_PIN);
            break;
#endif
#if GPIO_11_EN
        case GPIO_11:
            GPIO_11_PORT->ODR &= ~(1 << GPIO_11_PIN);
            break;
#endif
#if GPIO_12_EN
        case GPIO_12:
            GPIO_12_PORT->ODR &= ~(1 << GPIO_12_PIN);
            break;
#endif
#if GPIO_13_EN
        case GPIO_13:
            GPIO_13_PORT->ODR &= ~(1 << GPIO_13_PIN);
            break;
#endif
#if GPIO_14_EN
        case GPIO_14:
            GPIO_14_PORT->ODR &= ~(1 << GPIO_14_PIN);
            break;
#endif
#if GPIO_15_EN
        case GPIO_15:
            GPIO_15_PORT->ODR &= ~(1 << GPIO_15_PIN);
            break;
#endif
        case GPIO_UNDEFINED:
        default:
            return -1;
    }

    return 0;
}


int gpio_toggle(gpio_t dev)
{
    if (gpio_read(dev)) {
        return gpio_clear(dev);
    }
    else {
        return gpio_set(dev);
    }
}

int gpio_write(gpio_t dev, int value)
{
    if (value) {
        return gpio_set(dev);
    }
    else {
        return gpio_clear(dev);
    }
}

__attribute__((naked)) void isr_exti0(void)
{
    ISR_ENTER();
    if (EXTI->PR & EXTI_PR_PR0) {
        EXTI->PR |= EXTI_PR_PR0;        /* clear status bit by writing a 1 to it */
        config[GPIO_0].cb();
    }

    if (sched_context_switch_request) {
        thread_yield();
    }
    ISR_EXIT();
}

__attribute__((naked)) void isr_exti1(void)
{
    ISR_ENTER();
    if (EXTI->PR & EXTI_PR_PR1) {
        EXTI->PR |= EXTI_PR_PR1;        /* clear status bit by writing a 1 to it */
        config[GPIO_1].cb();
    }

    if (sched_context_switch_request) {
        thread_yield();
    }
    ISR_EXIT();
}

__attribute__((naked)) void isr_exti2(void)
{
    ISR_ENTER();
    if (EXTI->PR & EXTI_PR_PR2) {
        EXTI->PR |= EXTI_PR_PR2;        /* clear status bit by writing a 1 to it */
        config[GPIO_2].cb();
    }

    if (sched_context_switch_request) {
        thread_yield();
    }
    ISR_EXIT();
}

__attribute__((naked)) void isr_exti3(void)
{
    ISR_ENTER();
    if (EXTI->PR & EXTI_PR_PR3) {
        EXTI->PR |= EXTI_PR_PR3;        /* clear status bit by writing a 1 to it */
        config[GPIO_3].cb();
    }

    if (sched_context_switch_request) {
        thread_yield();
    }
    ISR_EXIT();
}

__attribute__((naked)) void isr_exti4(void)
{
    ISR_ENTER();
    if (EXTI->PR & EXTI_PR_PR4) {
        EXTI->PR |= EXTI_PR_PR4;        /* clear status bit by writing a 1 to it */
        config[GPIO_4].cb();
    }

    if (sched_context_switch_request) {
        thread_yield();
    }
    ISR_EXIT();
}

__attribute__((naked)) void isr_exti9_5(void)
{
    ISR_ENTER();
    if (EXTI->PR & EXTI_PR_PR5) {
        EXTI->PR |= EXTI_PR_PR5;        /* clear status bit by writing a 1 to it */
        config[GPIO_5].cb();
    }
    else if (EXTI->PR & EXTI_PR_PR6) {
        EXTI->PR |= EXTI_PR_PR6;        /* clear status bit by writing a 1 to it */
        config[GPIO_6].cb();
    }
    else if (EXTI->PR & EXTI_PR_PR7) {
        EXTI->PR |= EXTI_PR_PR7;        /* clear status bit by writing a 1 to it */
        config[GPIO_7].cb();
    }
    else if (EXTI->PR & EXTI_PR_PR8) {
        EXTI->PR |= EXTI_PR_PR8;        /* clear status bit by writing a 1 to it */
        config[GPIO_8].cb();
    }
    else if (EXTI->PR & EXTI_PR_PR9) {
        EXTI->PR |= EXTI_PR_PR9;        /* clear status bit by writing a 1 to it */
        config[GPIO_9].cb();
    }

    if (sched_context_switch_request) {
        thread_yield();
    }
    ISR_EXIT();
}

__attribute__((naked)) void isr_exti15_10(void)
{
    ISR_ENTER();
    if (EXTI->PR & EXTI_PR_PR10) {
        EXTI->PR |= EXTI_PR_PR10;        /* clear status bit by writing a 1 to it */
        config[GPIO_10].cb();
    }
    else if (EXTI->PR & EXTI_PR_PR11) {
        EXTI->PR |= EXTI_PR_PR11;        /* clear status bit by writing a 1 to it */
        config[GPIO_11].cb();
    }
    else if (EXTI->PR & EXTI_PR_PR12) {
        EXTI->PR |= EXTI_PR_PR12;        /* clear status bit by writing a 1 to it */
        config[GPIO_12].cb();
    }
    else if (EXTI->PR & EXTI_PR_PR13) {
        EXTI->PR |= EXTI_PR_PR13;        /* clear status bit by writing a 1 to it */
        config[GPIO_13].cb();
    }
    else if (EXTI->PR & EXTI_PR_PR14) {
        EXTI->PR |= EXTI_PR_PR14;        /* clear status bit by writing a 1 to it */
        config[GPIO_14].cb();
    }
    else if (EXTI->PR & EXTI_PR_PR15) {
        EXTI->PR |= EXTI_PR_PR15;        /* clear status bit by writing a 1 to it */
        config[GPIO_15].cb();
    }

    if (sched_context_switch_request) {
        thread_yield();
    }
    ISR_EXIT();
}
