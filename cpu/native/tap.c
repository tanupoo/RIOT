#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <err.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_tun.h>
#include <linux/if_ether.h>

#include "cpu-conf.h"
#include "tap.h"

int _native_tap_fd;
char _native_tap_mac[ETH_ALEN];

int tap_init(char *name)
{
    struct ifreq ifr;
    char *clonedev = "/dev/net/tun";

    if ((_native_tap_fd = open(clonedev , O_RDWR)) == -1) {
        err(EXIT_FAILURE, "open");
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    strncpy(ifr.ifr_name, name, IFNAMSIZ);

    if (ioctl(_native_tap_fd, TUNSETIFF, (void *)&ifr) == -1) {
        warn("ioctl");
        if (close(_native_tap_fd) == -1) {
            warn("close");
        }
        exit(EXIT_FAILURE);
    }

    /* TODO: use strncpy */
    strcpy(name, ifr.ifr_name);


    /* get MAC address */
    memset (&ifr, 0, sizeof (ifr));
    snprintf (ifr.ifr_name, sizeof (ifr.ifr_name), "%s", name);
    if (ioctl(_native_tap_fd, SIOCGIFHWADDR, &ifr) == -1) {
        warn("ioctl");
        if (close(_native_tap_fd) == -1) {
            warn("close");
        }
        exit(EXIT_FAILURE);
    }
    memcpy(_native_tap_mac, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

    puts("RIOT native tap initialized.");
    return _native_tap_fd;
}

void _native_marshall_ethernet(char *framebuf, char *data, int data_len)
{
    union eth_frame *f;
    char addr[ETH_ALEN];

    f = (union eth_frame*)framebuf;
    addr[0] = addr[1] = addr[2] = addr[3] = addr[4] = addr[5] = (char)0xFF;

    //memcpy(f->field.header.h_dest, dst, ETH_ALEN);
    memcpy(f->field.header.h_dest, addr, ETH_ALEN);
    //memcpy(f->field.header.h_source, src, ETH_ALEN);
    memcpy(f->field.header.h_source, _native_tap_mac, ETH_ALEN);
    f->field.header.h_proto = htons(NATIVE_ETH_PROTO);
    memcpy(f->field.data, data, data_len);
}

#ifdef TAPTESTBINARY
/**
 * test tap device
 */
int main(int argc, char *argv[])
{
    int fd;
    char buffer[2048];

    if (argc < 2) {
        errx(EXIT_FAILURE, "you need to specify a tap name");
    }
    fd = tap_init(argv[1]);

    printf("trying to write to fd: %i\n", _native_tap_fd);
    char *payld = "abcdefg";
    int data_len = strlen(payld);
    _native_marshall_ethernet(buffer, payld, data_len);
    if (write(_native_tap_fd, buffer, ETH_HLEN + data_len) == -1) {
        err(EXIT_FAILURE, "write");
    }

    printf("reading\n");
    int nread;
    while (1) {
        /* Note that "buffer" should be at least the MTU size of the
         * interface, eg 1500 bytes */
        nread = read(fd,buffer,sizeof(buffer));
        if(nread < 0) {
            warn("Reading from interface");
            if (close(fd) == -1) {
                warn("close");
            }
            exit(EXIT_FAILURE);
        }

        /* Do whatever with the data */
        printf("Read %d bytes\n", nread);
    }

    return EXIT_SUCCESS;
}
#endif
