/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     cpu_stm32f1
 * @{
 *
 * @file        rtt.c
 * @brief       Low-level RTT driver implementation
 *
 * @author      Thomas Eichinger <thomas.eichinger@fu-berlin.de>
 *
 * @}
 */

#include "cpu.h"
#include "sched.h"
#include "thread.h"
#include "periph/rtt.h"
#include "periph_conf.h"

/* guard file in case no RTT device was specified */
#if RTT_NUMOF

#define ENABLE_DEBUG (0)
#include "debug.h"

#define RTT_FLAG_RTOFF       ((uint16_t)0x0020)  /**< RTC Operation OFF flag */
#define RTT_FLAG_RSF         ((uint16_t)0x0008)  /**< Registers Synchronized flag */
#define RTT_FLAG_OW          ((uint16_t)0x0004)  /**< Overflow flag */
#define RTT_FLAG_ALR         ((uint16_t)0x0002)  /**< Alarm flag */
#define RTT_FLAG_SEC         ((uint16_t)0x0001)  /**< Second flag */

inline void _rtt_enter_config_mode(void);
inline void _rtt_leave_config_mode(void);

/*
 * callback and argument for an active alarm
 */
static rtt_alarm_cb_t alarm_cb;
static void *alarm_arg;

void rtt_init(void)
{
    rtt_poweron();

    /* configure interrupt */
    NVIC_SetPriority(RTT_IRQ, RTT_IRQ_PRIO);
    NVIC_EnableIRQ(RTT_IRQ);

    /* clear RSF flag */
    RTT_DEV->CRL &= ~(RTT_FLAG_RSF);

    _rtt_enter_config_mode();

    /* Reset RTC counter MSB word */
    RTT_DEV->CNTH = 0x0000;
    /* Set RTC counter LSB word */
    RTT_DEV->CNTL = 0x0000;
    /* set prescaler */
    RTT_DEV->PRLH = ((RTT_PRESCALER>>16)&0x000f);
    RTT_DEV->PRLL = (RTT_PRESCALER&0xffff);

    _rtt_leave_config_mode();
}

uint32_t rtt_get_counter(void)
{
    /* wait for syncronization */
    while (!(RTT_DEV->CRL & RTT_FLAG_RSF));

    return (((uint32_t)RTT_DEV->CNTH << 16 ) | (uint32_t)(RTT_DEV->CNTL));
}

void rtt_set_counter(uint32_t counter)
{
    _rtt_enter_config_mode();

    /* Set RTC counter MSB word */
    RTT_DEV->CNTH = counter >> 16;
    /* Set RTC counter LSB word */
    RTT_DEV->CNTL = (counter & 0xffff);

    _rtt_leave_config_mode();
}

uint32_t rtt_get_alarm(void)
{
    /* wait for syncronization */
    while (!(RTT_DEV->CRL & RTT_FLAG_RSF));

    return (((uint32_t)RTT_DEV->ALRH << 16 ) | (uint32_t)(RTT_DEV->ALRL));
}

void rtt_set_alarm(uint32_t alarm, rtt_alarm_cb_t cb, void *arg)
{
    _rtt_enter_config_mode();

    /* Set the alarm MSB word */
    RTT_DEV->ALRH = alarm >> 16;
    /* Set the alarm LSB word */
    RTT_DEV->ALRL = (alarm & 0xffff);

    /* Enable alarm interrupt */
    RTT_DEV->CRH |= RTC_CRH_ALRIE;

    _rtt_leave_config_mode();

    alarm_cb = cb;
    alarm_arg = arg;
}

void rtt_clear_alarm(void)
{
    _rtt_enter_config_mode();

    /* Disable alarm interrupt */
    RTT_DEV->CRH &= ~RTC_CRH_ALRIE;
    /* Set the ALARM MSB word to reset value */
    RTT_DEV->ALRH = 0xffff;
    /* Set the ALARM LSB word to reset value */
    RTT_DEV->ALRL = 0xffff;

    _rtt_leave_config_mode();
}

void rtt_poweron(void)
{
    RCC->APB1ENR |= (RCC_APB1ENR_BKPEN|RCC_APB1ENR_PWREN); /* enable BKP and PWR, Clock */
    /* RTC clock source configuration */
    PWR->CR |= PWR_CR_DBP;                  /* Allow access to BKP Domain */
    RCC->BDCR |= RCC_BDCR_LSEON;            /* Enable LSE OSC */
    while(!(RCC->BDCR & RCC_BDCR_LSERDY));   /* Wait till LSE is ready */
    RCC->BDCR |= RCC_BDCR_RTCSEL_LSE;        /* Select the RTC Clock Source */
    RCC->BDCR |= RCC_BDCR_RTCEN;             /* enable RTC */
}

void rtt_poweroff(void)
{
    PWR->CR |= PWR_CR_DBP;                   /* Allow access to BKP Domain */
    RCC->BDCR &= ~RCC_BDCR_RTCEN;            /* disable RTC */
    RCC->APB1ENR &= ~(RCC_APB1ENR_BKPEN|RCC_APB1ENR_PWREN); /* disable BKP and PWR, Clock */
}

inline void _rtt_enter_config_mode(void)
{
    /* Loop until RTOFF flag is set */
    while (!(RTT_DEV->CRL & RTT_FLAG_RTOFF));
    /* enter configuration mode */
    RTT_DEV->CRL |= RTC_CRL_CNF;
}

inline void _rtt_leave_config_mode(void)
{
    /* leave configuration mode */
    RTT_DEV->CRL &= ~RTC_CRL_CNF;
    /* Loop until RTOFF flag is set */
    while (!(RTT_DEV->CRL & RTT_FLAG_RTOFF));
}

void RTT_ISR(void)
{
    if (RTT_DEV->CRL & RTC_CRL_ALRF) {
        RTT_DEV->CRL &= ~(RTC_CRL_ALRF);
        alarm_cb(alarm_arg);
    }
    if (sched_context_switch_request) {
        thread_yield();
    }
}

#endif /* RTT_NUMOF */
