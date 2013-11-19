/**
 * tap.h implementation
 *
 * Copyright (C) 2013 Ludwig Ortmann
 *
 * This file subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 *
 * @ingroup native_cpu
 * @{
 * @file
 * @author  Ludwig Ortmann <ludwig.ortmann@fu-berlin.de>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <err.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <errno.h>

#ifdef __MACH__
#define _POSIX_C_SOURCE
#include <net/if.h>
#undef _POSIX_C_SOURCE
#include <ifaddrs.h>
#include <net/if_dl.h>
#else
#include <net/if.h>
#include <linux/if_tun.h>
#include <linux/if_ether.h>
#define IP_MAXPACKET (65535)
#endif

#define ENABLE_DEBUG    (0)
#include "debug.h"

#include "packet-zep.h"

#include "cpu.h"
#include "cpu-conf.h"
#include "tap.h"
#include "nativenet.h"
#include "nativenet_internal.h"
#include "native_internal.h"

#define TAP_BUFFER_LENGTH (ETHER_MAX_LEN)
int _native_marshall_ethernet(uint8_t *framebuf, radio_packet_t *packet);

void _native_create_ipv4(unsigned char *framebuf, uint8_t *data, int data_len);
void _native_udp(struct ip *iphdr, unsigned char *databuf, uint8_t *data, int data_len);
void _native_zep(unsigned char *databuf, uint8_t *data, int data_len);

uint16_t checksum(uint16_t *addr, int len);
uint16_t udp4_checksum(struct ip *iphdr, struct udphdr *udphdr, uint8_t *payload, int payloadlen);

int _native_tap_fd;
unsigned char _native_tap_mac[ETHER_ADDR_LEN];

char _native_tap_ip[INET_ADDRSTRLEN];
char _native_tap_brcast[INET_ADDRSTRLEN];

void _native_handle_tap_input(void)
{
    int nread;
    union eth_frame frame;
    radio_packet_t p;

    DEBUG("_native_handle_tap_input\n");

    /* TODO: check whether this is an input or an output event
       TODO: refactor this into general io-signal multiplexer */

    nread = real_read(_native_tap_fd, &frame, sizeof(union eth_frame));
    DEBUG("_native_handle_tap_input - read %d bytes\n", nread);
    if (nread > 0) {
        if (ntohs(frame.field.header.ether_type) == NATIVE_ETH_PROTO) {
            nread = nread - ETHER_HDR_LEN;
            if ((nread - 1) <= 0) {
                DEBUG("_native_handle_tap_input: no payload");
            }
            else {
                /* XXX: check overflow */
                p.length = ntohs(frame.field.payload.nn_header.length);
                p.dst = ntohs(frame.field.payload.nn_header.dst);
                p.src = ntohs(frame.field.payload.nn_header.src);
                p.rssi = 0;
                p.lqi = 0;
                p.processing = 0;
                p.data = frame.field.payload.data;
                DEBUG("_native_handle_tap_input: received packet of length %"PRIu16" for %"PRIu16" from %"PRIu16"\n", p.length, p.dst, p.src);
                _nativenet_handle_packet(&p);
            }
        }
        else {
            DEBUG("ignoring non-native frame\n");
        }

        /* work around lost signals */
        fd_set rfds;
        struct timeval t;
        memset(&t, 0, sizeof(t));
        FD_ZERO(&rfds);
        FD_SET(_native_tap_fd, &rfds);

        _native_in_syscall++; // no switching here
        if (select(_native_tap_fd +1, &rfds, NULL, NULL, &t) == 1) {
            int sig = SIGIO;
            extern int _sig_pipefd[2];
            extern ssize_t (*real_write)(int fd, const void *buf, size_t count);
            real_write(_sig_pipefd[1], &sig, sizeof(int));
            _native_sigpend++;
            DEBUG("_native_handle_tap_input: sigpend++\n");
        }
        else {
            DEBUG("_native_handle_tap_input: no more pending tap data\n");
        }
        _native_in_syscall--;
    }
    else if (nread == -1) {
        if ((errno == EAGAIN ) || (errno == EWOULDBLOCK)) {
            //warn("read");
        }
        else {
            err(EXIT_FAILURE, "_native_handle_tap_input: read");
        }
    }
    else {
        errx(EXIT_FAILURE, "internal error _native_handle_tap_input");
    }
}

int _native_marshall_ethernet(uint8_t *framebuf, radio_packet_t *packet)
{
    int data_len;
    union eth_frame2 *f;
    unsigned char addr[ETHER_ADDR_LEN];

    f = (union eth_frame2*)framebuf;
    addr[0] = addr[1] = addr[2] = addr[3] = addr[4] = addr[5] = (char)0xFF;

    //memcpy(f->field.header.ether_dhost, dst, ETHER_ADDR_LEN);
    memcpy(f->field.header.ether_dhost, addr, ETHER_ADDR_LEN);
    //memcpy(f->field.header.ether_shost, src, ETHER_ADDR_LEN);
    memcpy(f->field.header.ether_shost, _native_tap_mac, ETHER_ADDR_LEN);
    /* use our own ethertype, to filter frames at receiving side */
    f->field.header.ether_type = htons(NATIVE_ETH_PROTO);
    /* f->field.header.ether_type = htons(ETHERTYPE_IP); */
    _native_create_ipv4(f->field.data, packet->data, packet->length);

    data_len = packet->length + IP4_HDRLEN + UDP_HDRLEN + ZEP_V1_HEADER_LEN;
    /*
    int data_len;
    union eth_frame *f;
    unsigned char addr[ETHER_ADDR_LEN];

    f = (union eth_frame*)framebuf;
    addr[0] = addr[1] = addr[2] = addr[3] = addr[4] = addr[5] = 0xFF;

    memcpy(f->field.header.ether_dhost, addr, ETHER_ADDR_LEN);
    memcpy(f->field.header.ether_shost, _native_tap_mac, ETHER_ADDR_LEN);
    f->field.header.ether_type = htons(NATIVE_ETH_PROTO);

    memcpy(f->field.payload.data, packet->data, packet->length);
    f->field.payload.nn_header.length = htons(packet->length);
    f->field.payload.nn_header.dst = htons(packet->dst);
    f->field.payload.nn_header.src = htons(packet->src);

    data_len = packet->length + sizeof(struct nativenet_header);

    */
    /* Pad to minimum payload size.
     * Linux does this on its own, but it doesn't hurt to do it here.
     * As of now only tuntaposx needs this. */
    if (data_len < ETHERMIN) {
        DEBUG("padding data! (%d -> ", data_len);
        data_len = ETHERMIN;
        DEBUG("%d)\n", data_len);
    }

    return data_len + ETHER_HDR_LEN;
}

int send_buf(radio_packet_t *packet)
{
    uint8_t buf[TAP_BUFFER_LENGTH];
    int nsent, to_send;

    memset(buf, 0, sizeof(buf));

    DEBUG("send_buf:  Sending packet of length %"PRIu16" from %"PRIu16" to %"PRIu16"\n", packet->length, packet->src, packet->dst);
    to_send = _native_marshall_ethernet(buf, packet);

    DEBUG("send_buf: trying to send %d bytes\n", to_send);

    if ((nsent = write(_native_tap_fd, buf, to_send)) == -1) {;
        warn("write");
        return -1;
    }
    return 0;
}

int tap_init(char *name)
{

#ifdef __MACH__ /* OSX */
    char clonedev[255] = "/dev/"; /* XXX bad size */
    strncpy(clonedev+5, name, 250);
#else /* Linux */
    struct ifreq ifr;
    int _native_tap_ip_fd;
    char *clonedev = "/dev/net/tun";
#endif

    /* implicitly create the tap interface */
    if ((_native_tap_fd = open(clonedev , O_RDWR)) == -1) {
        err(EXIT_FAILURE, "open(%s)", clonedev);
    }

#ifdef __MACH__ /* OSX */
    struct ifaddrs* iflist;
    if (getifaddrs(&iflist) == 0) {
        for (struct ifaddrs *cur = iflist; cur; cur = cur->ifa_next) {
            if ((cur->ifa_addr->sa_family == AF_LINK) && (strcmp(cur->ifa_name, name) == 0) && cur->ifa_addr) {
                struct sockaddr_dl* sdl = (struct sockaddr_dl*)cur->ifa_addr;
                memcpy(_native_tap_mac, LLADDR(sdl), sdl->sdl_alen);
                break;
            }
        }

        freeifaddrs(iflist);
    }
#else /* Linux */
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
    memcpy(_native_tap_mac, ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN);

    _native_tap_ip_fd = socket(AF_INET, SOCK_DGRAM, 0);
    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;

    /* I want IP address attached to "eth0" */
    strncpy(ifr.ifr_name, name, IFNAMSIZ-1);

    if (ioctl(_native_tap_ip_fd, SIOCGIFADDR, &ifr) == -1) {
        warn("ioctl2");
        if (close(_native_tap_ip_fd) == -1) {
            warn("close");
        }
        exit(EXIT_FAILURE);
    }
    memcpy(_native_tap_ip, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), INET_ADDRSTRLEN);

    if (ioctl(_native_tap_ip_fd, SIOCGIFBRDADDR, &ifr) == -1) {
        warn("ioctl2");
        if (close(_native_tap_ip_fd) == -1) {
            warn("close");
        }
        exit(EXIT_FAILURE);
    }
    memcpy(_native_tap_brcast, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_addr), INET_ADDRSTRLEN);

#endif

    /* configure signal handler for fds */
    register_interrupt(SIGIO, _native_handle_tap_input);

#ifndef __MACH__ /* tuntap signalled IO not working in OSX */
    /* configure fds to send signals on io */
    if (fcntl(_native_tap_fd, F_SETOWN, getpid()) == -1) {
        err(1, "tap_init(): fcntl(F_SETOWN)");
    }

    /* set file access mode to nonblocking */
    if (fcntl(_native_tap_fd, F_SETFL, O_NONBLOCK|O_ASYNC) == -1) {
        err(1, "tap_init(): fcntl(F_SETFL)");
    }
#endif /* OSX */

    DEBUG("RIOT native tap initialized.\n");
    return _native_tap_fd;
}

void _native_create_ipv4(unsigned char *framebuf, uint8_t *data, int data_len)
{
  struct ip iphdr;
  int status;
  int ip_flags[4];

  /* IPv4 header length (4 bits): Number of 32-bit words in header = 5 */
  iphdr.ip_hl = IP4_HDRLEN / sizeof(uint32_t);

  /* Internet Protocol version (4 bits): IPv4 */
  iphdr.ip_v = 4;

  /* Type of service (8 bits) */
  iphdr.ip_tos = 0;

  /* Total length of datagram (16 bits): IP header + UDP header + datalen */
  iphdr.ip_len = htons(IP4_HDRLEN + UDP_HDRLEN + ZEP_V1_HEADER_LEN + data_len);

  /* ID sequence number (16 bits): unused, since single datagram */
  iphdr.ip_id = htons (0);

  /* Flags, and Fragmentation offset (3, 13 bits): 0 since single datagram */
  /* Zero (1 bit) */
  ip_flags[0] = 0;

  /* Do not fragment flag (1 bit) */
  ip_flags[1] = 0;

  /* More fragments following flag (1 bit) */
  ip_flags[2] = 0;

  /* Fragmentation offset (13 bits) */
  ip_flags[3] = 0;

  iphdr.ip_off = htons ((ip_flags[0] << 15)
                      + (ip_flags[1] << 14)
                      + (ip_flags[2] << 13)
                      +  ip_flags[3]);

  /* Time-to-Live (8 bits): default to maximum value */
  iphdr.ip_ttl = 255;

  /* Transport layer protocol (8 bits): 17 for UDP */
  iphdr.ip_p = IPPROTO_UDP;

  /* Source IPv4 address (32 bits) */
  if ((status = inet_pton(AF_INET, _native_tap_ip, &(iphdr.ip_src))) != 1) {
    fprintf (stderr, "#1 inet_pton() for %s failed.\nError message: %s", _native_tap_ip, strerror(status));
    exit (EXIT_FAILURE);
  }

  /* Destination IPv4 address (32 bits) */
  if ((status = inet_pton (AF_INET, _native_tap_brcast, &(iphdr.ip_dst))) != 1) {
    fprintf (stderr, "#2 inet_pton() failed.\nError message: %s", strerror (status));
    exit (EXIT_FAILURE);
  }

  /* IPv4 header checksum (16 bits): set to 0 when calculating checksum */
  iphdr.ip_sum = 0;
  iphdr.ip_sum = checksum((uint16_t *)&iphdr, IP4_HDRLEN);
  
  memcpy(framebuf, &iphdr, IP4_HDRLEN);
  _native_udp(&iphdr, framebuf+IP4_HDRLEN, data, data_len);
}

void _native_udp(struct ip *iphdr, unsigned char *databuf, uint8_t *data, int data_len)
{
    struct udphdr header;

    header.source = htons(0);
    header.dest = htons(ZEP_DEFAULT_PORT);
    header.len = htons(UDP_HDRLEN + ZEP_V1_HEADER_LEN + data_len);
    header.check = udp4_checksum(iphdr, &header, data, ETHER_MAX_LEN);
    memcpy(databuf, &header, UDP_HDRLEN);
    _native_zep(databuf+UDP_HDRLEN, data, data_len);
}

void _native_zep(unsigned char *databuf, uint8_t *data, int data_len)
{
    zep_info zep;

    zep.version = 1;
    zep.channel_id = 26;
    zep.device_id = data[2];
    /* disabling checksum */
    zep.lqi_mode = 0;
    zep.lqi = 0;

    databuf[0] = ZEP_PREAMBLE[0];
    databuf[1] = ZEP_PREAMBLE[1];
    databuf[2] = zep.version;
    databuf[3] = zep.channel_id;
    /* ZEP header has two bytes for device_id */
    databuf[4] = 0;
    databuf[5] = zep.device_id;
    databuf[6] = zep.lqi_mode;
    databuf[7] = zep.lqi;
    /* misusing reserved bytes */
    /* cc110x length */
    databuf[8] = data[0];
    /* cc110x dst address */
    databuf[9] = data[1];
    /* cc110x flags */
    databuf[10] = data[3];
    databuf[11] = databuf[12] = databuf[13] = databuf[14] = 0;
    databuf[15] = data_len;

    /* why -2 - maybe 802.15.4 checksum is missing at all? */
    memcpy(databuf+ZEP_V1_HEADER_LEN, data, data_len);
}

uint16_t checksum(uint16_t *addr, int len)
{
  int nleft = len;
  int sum = 0;
  uint16_t *w = addr;
  uint16_t answer = 0;

  while (nleft > 1) {
    sum += *w++;
    nleft -= sizeof(uint16_t);
  }

  if (nleft == 1) {
    *(uint8_t *) (&answer) = *(uint8_t *) w;
    sum += answer;
  }

  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  answer = ~sum;
  return (answer);
}

// Build IPv4 UDP pseudo-header and call checksum function.
uint16_t udp4_checksum(struct ip *iphdr, struct udphdr *udphdr, uint8_t *payload, int payloadlen)
{
  char buf[IP_MAXPACKET];
  char *ptr;
  int chksumlen = 0;
  int i;

  ptr = &buf[0];  // ptr points to beginning of buffer buf

  // Copy source IP address into buf (32 bits)
  memcpy (ptr, &(iphdr->ip_src.s_addr), sizeof(iphdr->ip_src.s_addr));
  ptr += sizeof(iphdr->ip_src.s_addr);
  chksumlen += sizeof(iphdr->ip_src.s_addr);

  // Copy destination IP address into buf (32 bits)
  memcpy (ptr, &(iphdr->ip_dst.s_addr), sizeof(iphdr->ip_dst.s_addr));
  ptr += sizeof(iphdr->ip_dst.s_addr);
  chksumlen += sizeof(iphdr->ip_dst.s_addr);

  // Copy zero field to buf (8 bits)
  *ptr = 0; ptr++;
  chksumlen += 1;

  // Copy transport layer protocol to buf (8 bits)
  memcpy (ptr, &(iphdr->ip_p), sizeof(iphdr->ip_p));
  ptr += sizeof(iphdr->ip_p);
  chksumlen += sizeof(iphdr->ip_p);

  // Copy UDP length to buf (16 bits)
  memcpy (ptr, &(udphdr->len), sizeof(udphdr->len));
  ptr += sizeof(udphdr->len);
  chksumlen += sizeof(udphdr->len);

  // Copy UDP source port to buf (16 bits)
  memcpy (ptr, &(udphdr->source), sizeof(udphdr->source));
  ptr += sizeof(udphdr->source);
  chksumlen += sizeof(udphdr->source);

  // Copy UDP destination port to buf (16 bits)
  memcpy (ptr, &(udphdr->dest), sizeof(udphdr->dest));
  ptr += sizeof(udphdr->dest);
  chksumlen += sizeof(udphdr->dest);

  // Copy UDP length again to buf (16 bits)
  memcpy (ptr, &(udphdr->len), sizeof(udphdr->len));
  ptr += sizeof(udphdr->len);
  chksumlen += sizeof(udphdr->len);

  // Copy UDP checksum to buf (16 bits)
  // Zero, since we don't know it yet
  *ptr = 0; ptr++;
  *ptr = 0; ptr++;
  chksumlen += 2;

  // Copy payload to buf
  memcpy (ptr, payload, payloadlen);
  ptr += payloadlen;
  chksumlen += payloadlen;

  // Pad to the next 16-bit boundary
  for (i=0; i<payloadlen%2; i++, ptr++) {
    *ptr = 0;
    ptr++;
    chksumlen++;
  }

  return checksum ((uint16_t *) buf, chksumlen);
}

/** @} */
