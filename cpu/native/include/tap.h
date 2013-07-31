#ifndef _TAP_H
#define _TAP_H

#include <net/ethernet.h>
#include <netinet/udp.h>
#include <netinet/ip.h>       // struct ip and IP_MAXPACKET (which is 65535)

/* Define some constants. */
/* IPv6 header length */
#define IP4_HDRLEN  (20)
/* UDP header length, excludes data */
#define UDP_HDRLEN  (8)

/**
 * create and/or open tap device "name"
 *
 * if "name" is an empty string, the kernel chooses a name
 * if "name" is an existing device, that device is used
 * otherwise a device named "name" is created
 */
int tap_init(char *name);
int send_buf(void);
void _native_marshall_ethernet(uint8_t *framebuf, uint8_t *data, int data_len);
void _native_create_ipv4(char *framebuf, uint8_t *data, int data_len);
void _native_udp(struct ip *iphdr, char *databuf, uint8_t *data, int data_len);
void _native_zep(char *databuf, uint8_t *data, int data_len);
void _native_cc1100_handle_input(void);
uint16_t checksum(uint16_t *addr, int len);
uint16_t udp4_checksum(struct ip *iphdr, struct udphdr *udphdr, uint8_t *payload, int payloadlen);

extern int _native_tap_fd;
extern char _native_tap_mac[ETHER_ADDR_LEN];

union eth_frame {
    struct {
        struct ether_header header;
        char data[ETHERMTU];
    } field;
    unsigned char buffer[ETHER_MAX_LEN];
};

#endif /* _TAP_H */
