/**
 * Native CPU configuration
 *
 * Copyright (C) 2013 Ludwig Ortmann
 *
 * This file subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 *
 * @ingroup arch
 * @{
 * @file
 * @author  Ludwig Ortmann <ludwig.ortmann@fu-berlin.de>
 * @}
 */
#ifndef CPUCONF_H_
#define CPUCONF_H_
#include <signal.h>

/* TODO: choose more sensibly? */
#ifdef __MACH__
#define KERNEL_CONF_STACKSIZE_DEFAULT   (163840)
#define KERNEL_CONF_STACKSIZE_IDLE      (163840)
#define NATIVE_ISR_STACKSIZE            (163840)
#define TRANSCEIVER_STACK_SIZE          (163840)
#define MINIMUM_STACK_SIZE              (163840)
#else
#define KERNEL_CONF_STACKSIZE_DEFAULT   (163840)
#define KERNEL_CONF_STACKSIZE_IDLE      (163840)
#define NATIVE_ISR_STACKSIZE            (163840)
/* undefine the TRANSCEIVER_STACK_SIZE (2048 or 512) defined in transceiver.h */
#ifdef TRANSCEIVER_STACK_SIZE
#undef TRANSCEIVER_STACK_SIZE
#endif
#define TRANSCEIVER_STACK_SIZE          (163840)

#define MINIMUM_STACK_SIZE              (163840)
#endif

/* for cc110x_ng */
#define RX_BUF_SIZE (10)
#define TRANSCEIVER_BUFFER_SIZE (3)
#define NATIVE_ETH_PROTO 0x1234

#endif /* CPUCONF_H_ */
