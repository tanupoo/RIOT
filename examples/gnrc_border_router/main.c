/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Example application for demonstrating the RIOT network stack
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>

#include "shell.h"
#include "net/gnrc/pktbuf.h"

int _gnrc_pktbuf_stats(int argc, char **args)
{
    (void) argc;
    (void) args;
    gnrc_pktbuf_stats();
    return 0;
}

static const shell_command_t shell_commands[] = {
    { "stats", "print packet buffer", _gnrc_pktbuf_stats},
    { NULL, NULL, NULL }
};

int main(void)
{
    puts("RIOT border router example application");

    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}
