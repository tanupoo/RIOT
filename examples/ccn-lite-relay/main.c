#include <stdio.h>

int main(void)
{
    puts("Basic CCN-Lite example");

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
