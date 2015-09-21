/*
 * Copyright (C) 2015 Martine Lenders <mlenders@inf.fu-berlin.de>
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
 * @brief       Example application for demonstrating the RIOT's POSIX sockets
 *
 * @author      Martine Lenders <mlenders@inf.fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>

#include "msg.h"
#include "shell.h"

#define MAIN_MSG_QUEUE_SIZE (1)

extern int ip_cmd(int argc, char **argv);

static const shell_command_t shell_commands[] = {
    { "ip", "send hex over IP and listen for IP packets of certain type", ip_cmd },
    { NULL, NULL, NULL }
};
static msg_t main_msg_queue[MAIN_MSG_QUEUE_SIZE];


int main(void)
{
    puts("RIOT socket example application");
    msg_init_queue(main_msg_queue, MAIN_MSG_QUEUE_SIZE);
    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}
