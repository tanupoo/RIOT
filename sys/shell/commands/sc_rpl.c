/*
 * Copyright (C) 2014 Oliver Hahm <oliver.hahm@inria.fr>
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup shell_commands
 * @{
 * @file    sc_rpl.c
 * @brief   provides shell commands to manage and query RPL
 * @author  Oliver Hahm <oliver.hahm@inria.fr>
 * @}
 */

#include <stdio.h>
#include <stdint.h>

#include "rpl.h"

char addr_str[IPV6_MAX_ADDR_STR_LEN];

void _rpl_route_handler(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    rpl_routing_entry_t *rtable;
    rtable = rpl_get_routing_table();
    uint8_t c = 0;
    puts("--------------------------------------------------------------------");
    puts("Routing table");
    puts(" #    target\t\t\tnext hop\t\tlifetime");
    puts("--------------------------------------------------------------------");

    for (int i = 0; i < RPL_MAX_ROUTING_ENTRIES; i++) {
        if (rtable[i].used) {
            c++;
            printf(" %03d: %-18s\t", i, ipv6_addr_to_str(addr_str, IPV6_MAX_ADDR_STR_LEN,
                                            (&rtable[i].address)));
            printf("%-18s\t", ipv6_addr_to_str(addr_str, IPV6_MAX_ADDR_STR_LEN,
                                            (&rtable[i].next_hop)));
            printf("%d\n", rtable[i].lifetime);

            puts("--------------------------------------------------------------------");
        }
    }
    printf(" %d routing table entries\n", c);

    printf("$\n");
}

