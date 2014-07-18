/**
 * Shell commands for system calls
 *
 * Copyright (C) 2014  Ludwig Ortmann <ludwig.ortmann@fu-berlin.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 *
 * @ingroup shell_commands
 * @{
 * @file
 * @brief   shell commands for system calls
 * @author  Ludwig Ortmann <ludwig.ortmann@fu-berlin.de>
 * @}
 */

#include "kernel.h"
#include "crash.h"

void _halt_handler(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    core_panic(0, "System halted");
}

void _reboot_handler(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    (void) reboot(RB_AUTOBOOT);
}
