/*
 * Copyright (C) 2015 Attilio Dona'
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     cpu_cc3200
 * @{
 *
 * @file
 * @brief       Implementation of the kernels irq interface
 *
 * @author      Attilio Dona'
 *
 * @}
 */

#include <stdint.h>
#include "arch/irq_arch.h"
#include "cpu.h"

/**
 * @brief Disable all maskable interrupts
 */
unsigned int irq_arch_disable(void) {
    uint32_t mask = IntPriorityMaskGet();
    //__disable_irq();
    IntMasterDisable();
    return mask;
}

/**
 * @brief Enable all maskable interrupts
 */
unsigned int irq_arch_enable(void) {
    //__enable_irq();
    IntMasterEnable();
    return IntPriorityMaskGet();
}

/**
 * @brief Restore the state of the IRQ flags
 */
void irq_arch_restore(unsigned int priMask) {
    __ASM volatile ("MSR primask, %0" : : "r" (priMask) : "memory");
}

/**
 * @brief See if the current context is inside an ISR
 */
int irq_arch_in(void) {
    //return (__get_IPSR() & 0xFF);

    uint32_t result;

    __ASM volatile ("MRS %0, ipsr" : "=r" (result) );
    /* cppcheck-suppress uninitvar */
    return (result);
}