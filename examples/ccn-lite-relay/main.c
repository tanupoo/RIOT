#include <stdio.h>

#include "msg.h"
#include "timex.h"
#include "shell_commands.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "ccn-lite-riot.h"
#include "ccnl-headers.h"
#include "ccnl-pkt-ndntlv.h"
#include "ccnl-defs.h"

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];
//static char addr_str[IPV6_ADDR_MAX_STR_LEN];
struct ccnl_forward_s fwd;

extern struct ccnl_relay_s theRelay;

extern int ccnl_open_udpdev(int port);

static int _ccnl_fwd(int argc, char **argv);

static const shell_command_t shell_commands[] = {
    { "ccnl", "start a NDN forwarder", _ccnl_fwd},
    { NULL, NULL, NULL }
};

static void _usage(void)
{
    puts("ccnl <prefix> <defaultgw> <UDP port>");
}

static int _ccnl_fwd(int argc, char **argv)
{
    sockunion sun;
    int suite = CCNL_SUITE_NDNTLV;
    char *prefix;

    if (argc < 4) {
        _usage();
        return -1;
    }

    if (inet_pton(AF_INET6, argv[2], &sun.ip6.sin6_addr) != 1) {
        _usage();
        return -1;
    }
    sun.ip6.sin6_port = atoi(argv[3]);

    prefix = argv[1];

    fwd.prefix = ccnl_URItoPrefix(prefix, suite, NULL, NULL);
    fwd.suite = suite;
    fwd.face = ccnl_get_face_or_create(&theRelay, 0, &sun.sa, sizeof(sun.ip6));
    fwd.face->flags |= CCNL_FACE_FLAGS_STATIC;
    theRelay.fib = &fwd;

    ccnl_set_timer(SEC_IN_USEC, ccnl_minimalrelay_ageing, &theRelay, 0);
    ccnl_event_loop(&theRelay);

    return 0;
}


int main(void)
{
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    struct ccnl_if_s *i;
    int udpport = NDN_UDP_PORT;

    puts("Basic CCN-Lite example");

    ccnl_core_init();

    i = &theRelay.ifs[0];
    i->mtu = NDN_DEFAULT_MTU;
    i->fwdalli = 1;
    i->sock = ccnl_open_udpdev(udpport);
    if (i->sock < 0) {
        puts("Something went wrong opening the UDP port");
        return(-1);
    }
    theRelay.ifcount++;
    printf("NDN minimalrelay started, listening on UDP port %d\n", udpport);

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
    return 0;
}

#if 0
int main(int argc, char **argv)
{
    int opt;
    int udpport = 0;
    char *prefix, *defaultgw;
    struct ccnl_if_s *i;
    struct ccnl_forward_s *fwd;
    sockunion sun;

    srandom(time(NULL));

    int suite = CCNL_SUITE_NDNTLV;

    while ((opt = getopt(argc, argv, "hs:u:v:")) != -1) {
        switch (opt) {
        case 's':
            opt = ccnl_str2suite(optarg);
            if (opt >= 0 && opt < CCNL_SUITE_LAST)
                suite = opt;
            else
                fprintf(stderr, "Suite parameter <%s> ignored.\n", optarg);
            break;
        case 'u':
            udpport = atoi(optarg);
            break;
        case 'v':
            debug_level = atoi(optarg);
            break;
        case 'h':
        default:
usage:
            fprintf(stderr,
                    "usage:    %s [options] PREFIX DGWIP/DGWUDPPORT\n"
                    "options:  [-h] [-s SUITE] [-u udpport] [-v debuglevel]\n"
                    "example:  %s /ndn 128.252.153.194/6363\n",
                    argv[0], argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if ((optind+1) >= argc)
        goto usage;
    prefix = argv[optind];
    defaultgw = argv[optind+1];

    ccnl_core_init();

//    if (theRelay.suite == CCNL_SUITE_NDNTLV && !udpport)
        udpport = NDN_UDP_PORT;

    i = &theRelay.ifs[0];
    i->mtu = NDN_DEFAULT_MTU;
    i->fwdalli = 1;
    i->sock = ccnl_open_udpdev(udpport);
    if (i->sock < 0)
        exit(-1);
    theRelay.ifcount++;
    fprintf(stderr, "NDN minimalrelay started, listening on UDP port %d\n",
            udpport);

    inet_aton(strtok(defaultgw,"/"), &sun.ip4.sin_addr);
    sun.ip4.sin_port = atoi(strtok(NULL, ""));
    fwd = (struct ccnl_forward_s *) ccnl_calloc(1, sizeof(*fwd));
    fwd->prefix = ccnl_URItoPrefix(prefix, suite, NULL, NULL);
    fwd->suite = suite;
    fwd->face = ccnl_get_face_or_create(&theRelay, 0, &sun.sa, sizeof(sun.ip4));
    fwd->face->flags |= CCNL_FACE_FLAGS_STATIC;
    theRelay.fib = fwd;

    ccnl_set_timer(1000000, ccnl_minimalrelay_ageing, &theRelay, 0);
    ccnl_io_loop(&theRelay);

    return 0;
}

#endif
