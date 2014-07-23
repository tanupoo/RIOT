/*
 * @f ccnl-ext-appserver.c
 *
 * Copyright (C) 2013, Christian Mehlis, Freie Universität Berlin
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

#define RIOT_CCN_APPSERVER (1)

#if RIOT_CCN_APPSERVER

#include "msg.h"
#include "thread.h"
#include "ccnl-riot-compat.h"
#include "ccn_lite/util/ccnl-riot-client.h"

#include "ccnl-includes.h"
#include "ccnl-core.h"
#include "ccnl-ext.h"
#include "ccnl-pdu.h"
#include "ccnx.h"

/** The size of the message queue between router daemon and transceiver AND clients */
#define APPSERVER_MSG_BUFFER_SIZE (64)

/** message buffer */
msg_t msg_buffer_appserver[APPSERVER_MSG_BUFFER_SIZE];

int relay_pid;
char prefix[] = "/riot/appserver/";

#ifdef MODULE_SIXLOWPAN
#include "destiny.h"
#define SERVER_PORT     (0xFF01)
static bool received_payload = false;
char udp_buf[CCNL_RIOT_CHUNK_SIZE - 1];
char tmp_buf[4];
#endif

static int appserver_sent_content(uint8_t *buf, int len, uint16_t from)
{
    static riot_ccnl_msg_t rmsg;
    rmsg.payload = buf;
    rmsg.size = len;
    DEBUGMSG(1, "datalen=%d\n", rmsg.size);

    msg_t m;
    m.type = CCNL_RIOT_MSG;
    m.content.ptr = (char *) &rmsg;
    uint16_t dest_pid = from;
    DEBUGMSG(1, "sending msg to pid=%u\n", dest_pid);
    int ret = msg_send(&m, dest_pid, 1);
    DEBUGMSG(1, "msg_reply returned: %d\n", ret);
    return ret;
}

static int appserver_create_content(char **prefix, uint8_t *out)
{
    char buf[CCNL_RIOT_CHUNK_SIZE - 1];

    for (int i = 0; i < CCNL_RIOT_CHUNK_SIZE - 1; i++) {
        buf[i] = 'a' + i%26;
    }

    int len = mkContent(prefix, buf, CCNL_RIOT_CHUNK_SIZE - 1, out);
    return len;
}

static int appserver_create_prefix(char *name, char **prefix)
{
    int i = 0;
    char *cp = strtok(name, "/");

    while (i < (CCNL_MAX_NAME_COMP - 1) && cp) {
        prefix[i++] = cp;
        cp = strtok(NULL, "/");
    }

    prefix[i] = NULL;

    return i;
}

static int appserver_handle_interest(char *data, uint16_t datalen, uint16_t from)
{
    (void) data;
    (void) datalen;

    char *prefix[CCNL_MAX_NAME_COMP];
    //struct ccnl_interest_s *i = appserver_parse_interest(data, datalen);

    char name[] = "/riot/appserver/test/0";
    appserver_create_prefix(name, prefix);

    unsigned char *content_pkg = malloc(PAYLOAD_SIZE);
    if (!content_pkg) {
        puts("appserver_handle_interest: malloc failed");
        return 0;
    }
#ifdef MODULE_SIXLOWPAN
    int len;
    if (received_payload) {
        DEBUGF("I have some content received over UDP - will deliver\n");
        memcpy(udp_buf, data, datalen);
        len = mkContent(prefix, udp_buf, CCNL_RIOT_CHUNK_SIZE - 1, content_pkg);
        received_payload = false;
    }
    else {
        DEBUGF("I have no content yet, gonna send an UDP request\n");
        int sock;
        sockaddr6_t sa;
        ipv6_addr_t ipaddr;

        sock = destiny_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
        memset(&sa, 0, sizeof(sa));

        ipv6_addr_set_all_nodes_addr(&ipaddr);

        sa.sin6_family = AF_INET;
        memcpy(&sa.sin6_addr, &ipaddr, 16);
        sa.sin6_port = HTONS(SERVER_PORT);

        destiny_socket_sendto(sock, tmp_buf, sizeof(tmp_buf), 0, &sa, sizeof(sa));

        destiny_socket_close(sock);
        return 0;
    }
#else
    DEBUG("create content on the fly\n");
    int len = appserver_create_content(prefix, content_pkg);
#endif
    /*
     struct ccnl_prefix *myprefix = ccnl_path_to_prefix(name);

     if (ccnl_prefix_cmp(myprefix, 0, i->prefix, CMP_EXACT) != CMP_EXACT) {
     DEBUGMSG(1, "APPSERVER: it's a match");
     }
     */
    int ret = appserver_sent_content(content_pkg, len, from);
    free(content_pkg);

    return ret;
}

static void riot_ccnl_appserver_ioloop(void)
{
    DEBUGMSG(1, "starting appserver main event and IO loop\n");

    if (msg_init_queue(msg_buffer_appserver, APPSERVER_MSG_BUFFER_SIZE) != 0) {
        DEBUGMSG(1, "msg init queue failed...abording\n");
    }

    msg_t in;
    riot_ccnl_msg_t *m;

    while (1) {
        DEBUGMSG(1, "appserver: waiting for incomming msg\n");
        msg_receive(&in);

        switch (in.type) {
            case (CCNL_RIOT_MSG):
                m = (riot_ccnl_msg_t *) in.content.ptr;
                DEBUGMSG(1, "new msg: size=%" PRIu16 " sender_pid=%" PRIu16 "\n",
                         m->size, in.sender_pid);
                appserver_handle_interest(m->payload, m->size, in.sender_pid);

                ccnl_free(m);
                break;

#ifdef MODULE_SIXLOWPAN
            case (CCNL_RIOT_UDP):
                m = (riot_ccnl_msg_t *) in.content.ptr;
                DEBUGMSG(1, "Got content from UDP, size: %" PRIu16 ", sender_pid=%" PRIu16 "\n",
                         m->size, in.sender_pid);
                received_payload = true;

                appserver_handle_interest(m->payload, m->size, relay_pid);

                break;
#endif
            default:
                DEBUGMSG(1,
                         "received unknown msg type: '%" PRIu16 "' dropping it\n",
                         in.type);
                break;
        }
    }
}

static void riot_ccnl_appserver_register(void)
{
    char faceid[10];
    snprintf(faceid, sizeof(faceid), "%d", thread_getpid());
    char *type = "newMSGface";

    unsigned char *mgnt_pkg = malloc(256);
    if (!mgnt_pkg) {
        puts("riot_ccnl_appserver_register: malloc failed");
        return;
    }
    int content_len = ccnl_riot_client_publish(relay_pid, prefix, faceid, type, mgnt_pkg);
    (void) content_len;
    DEBUG("received %d bytes.\n", content_len);
    DEBUG("appserver received: '%s'\n", mgnt_pkg);

    free(mgnt_pkg);
}

void *ccnl_riot_appserver_start(void *arg)
{
	int _relay_pid = (int) arg;
    relay_pid = _relay_pid;
    riot_ccnl_appserver_register();
    riot_ccnl_appserver_ioloop();
    DEBUGMSG(1, "appserver terminated\n");
    return NULL;
}

#endif
