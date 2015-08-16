/*
 * @f ccnl-riot-compat.c
 *
 * Copyright (C) 2013, Christian Mehlis, Freie University Berlin
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "msg.h"
#include "thread.h"

#include "ccnl.h"
#include "ccnl-core.h"
#include "ccnl-pdu.h"
#include "ccnl-riot-compat.h"

char relay_helper_stack[THREAD_STACKSIZE_MAIN];

void riot_send_netapi(uint8_t *buf, size_t size, uint8_t *to)
{
    ng_netif_hdr_t *hdr;
    ng_pktsnip_t *netif;
    kernel_pid_t ifs[NG_NETIF_NUMOF];
    size_t ifnum = ng_netif_get(ifs);
    uint16_t l2_addr_len = 0, payload_size = 0;

    for (int i = 0; i < ifnum; i++) {
        if (!ng_netapi_get(ifs[i], NETOPT_ADDR_LEN, 0, &l2_addr_len, sizeof(uint16_t))) {
            continue;
        }
        if (!ng_netapi_get(ifs[i], NETOPT_MAX_PACKET_SIZE, 0, &payload_size, sizeof(uint16_t))) {
            continue;
        }

        netif = ng_netif_hdr_build(NULL, 0, to, l2_addr_len);
        DEBUGMSG(1, "this is a RIOT netapi based connection\n");
        DEBUGMSG(1, "size=%" PRIu16 " to=%" PRIu16 "\n", size, to);

        if (size > payload_size) {
            DEBUGMSG(1, "size > PAYLOAD_SIZE: %d > %d\n", size, PAYLOAD_SIZE);
            return;
        }

        if (netif == NULL) {
            DEBUG("ccnl: error on interface header allocation, dropping packet\n");
            ng_pktbuf_release(pkt);
            return;
        }

        /* add netif to front of the pkt list */
        LL_PREPEND(pkt, netif);
        ((ng_netif_hdr_t *)pkt->data)->if_pid = iface;
        ng_netapi_send(ifs[i], pkt);
    }
}

int riot_send_msg(uint8_t *buf, uint16_t size, uint8_t *to)
{
    DEBUGMSG(1, "this is a RIOT MSG based connection\n");
    DEBUGMSG(1, "size=%" PRIu16 " to=%" PRIkernel_pid"\n", size,
             (kernel_pid_t) *to);

    uint8_t *buf2 = ccnl_malloc(sizeof(riot_ccnl_msg_t) + size);
    if (!buf2) {
        DEBUGMSG(1, "  malloc failed...dorpping msg!\n");
        return 0;
    }

    riot_ccnl_msg_t *rmsg = (riot_ccnl_msg_t *) buf2;
    rmsg->payload = buf2 + sizeof(riot_ccnl_msg_t);
    rmsg->size = size;

    memcpy(rmsg->payload, buf, size);

    msg_t m;
    m.type = CCNL_RIOT_MSG;
    m.content.ptr = (char *) rmsg;
    DEBUGMSG(1, "sending msg to pid=%" PRIkernel_pid "\n", (kernel_pid_t) *to);
    msg_send(&m, *to);

    return size;
}

void riot_send_nack(uint16_t to)
{
    msg_t m;
    m.type = CCNL_RIOT_NACK;
    DEBUGMSG(1, "sending NACK msg to pid=%" PRIkernel_pid"\n", to);
    msg_try_send(&m, to);
}

void *ccnl_riot_relay_helper_start(void *);

kernel_pid_t riot_start_helper_thread(void)
{
    return thread_create(relay_helper_stack, sizeof(relay_helper_stack),
                         THREAD_PRIORITY_MAIN - 2, CREATE_STACKTEST,
                         ccnl_riot_relay_helper_start, NULL, "relay-helper");
}

char *riot_ccnl_event_to_string(int event)
{
    switch (event) {

        case CCNL_RIOT_MSG:
            return "RIOT_MSG";

        case CCNL_RIOT_HALT:
            return "RIOT_HALT";

        case CCNL_RIOT_POPULATE:
            return "RIOT_POPULATE";

        case CCNL_RIOT_PRINT_STAT:
            return "CCNL_RIOT_PRINT_STAT";

        case ENOBUFFER:
            return "ENOBUFFER";

        default:
            return "UNKNOWN";
    }
}
