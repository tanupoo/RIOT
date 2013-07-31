#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <err.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <arpa/inet.h>

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
#include <netinet/ip.h>       // struct ip and IP_MAXPACKET (which is 65535)
#endif

#define ENABLE_DEBUG    (0)
#include "debug.h"

#include "packet-zep.h"
#include "cpu.h"
#include "cpu-conf.h"
#include "tap.h"
#include "cc1100sim.h"
#include "cc110x-internal.h" /* CC1100 constants */

#define TAP_BUFFER_LENGTH (CC1100_FIFO_LENGTH + ETHER_HDR_LEN + IP4_HDRLEN + UDP_HDRLEN + ZEP_V1_HEADER_LEN)

int _native_tap_fd;
char _native_tap_mac[ETHER_ADDR_LEN];
char _native_tap_ip[INET_ADDRSTRLEN];
char _native_tap_brcast[INET_ADDRSTRLEN];

void _native_handle_cc110xng_input(void)
{
    int nread, offset;
    char buf[TAP_BUFFER_LENGTH];
    union eth_frame *f;

    DEBUG("_native_handle_cc110xng_input\n");

    /* TODO: check whether this is an input or an output event
       TODO: refactor this into general io-signal multiplexer */

    _native_in_syscall = 1;
    nread = read(_native_tap_fd, buf, TAP_BUFFER_LENGTH);
    _native_in_syscall = 0;
    DEBUG("_native_handle_cc110xng_input - read %d bytes\n", nread);
    if (nread > 0) {
        f = (union eth_frame*)&buf;
        if (ntohs(f->field.header.ether_type) == NATIVE_ETH_PROTO) {
            nread = nread - ETHER_HDR_LEN;
            if ((nread - 1) <= 0) {
                DEBUG("_native_handle_cc110xng_input: no payload");
            }
            else {
                nread = buf[ETHER_HDR_LEN];
                offset = ETHER_HDR_LEN + IP4_HDRLEN + UDP_HDRLEN;
                _native_cc1100_handle_packet(buf+offset+ZEP_V1_HEADER_LEN, nread);
            }
        }
        else {
            DEBUG("ignoring non-native frame\n");
        }
    }
    else if (nread == -1) {
        err(EXIT_FAILURE, "read");
    }
    else {
        errx(EXIT_FAILURE, "internal error in _native_handle_cc110xng_input");
    }
}

int send_buf(void)
{
    uint8_t buf[TAP_BUFFER_LENGTH];
    int nsent;
    uint8_t to_send;

    to_send = status_registers[CC1100_TXBYTES - 0x30];
    _native_marshall_ethernet(buf, tx_fifo, to_send-2);
    to_send += IP4_HDRLEN + UDP_HDRLEN + ZEP_V1_HEADER_LEN-2;

    if ((ETHER_HDR_LEN + to_send) < ETHERMIN) {
        DEBUG("padding data! (%d ->", to_send);
        to_send = ETHERMIN - ETHER_HDR_LEN;
        DEBUG("%d)\n", to_send);
    }

    if ((nsent = write(_native_tap_fd, buf, to_send + ETHER_HDR_LEN)) == -1) {;
        warn("write");
        return -1;
    }
    return 0;
}

int tap_init(char *name)
{
    int _native_tap_ip_fd;

#ifdef __MACH__ /* OSX */
    char clonedev[255] = "/dev/"; /* XXX bad size */
    strncpy(clonedev+5, name, 250);
#else /* Linux */
    struct ifreq ifr;
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
    memset (&ifr, 0, sizeof(ifr));
    snprintf (ifr.ifr_name, sizeof(ifr.ifr_name), "%s", name);
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

/*TODO:  check OSX vvv */
    /* configure signal handler for fds */
    register_interrupt(SIGIO, _native_handle_cc110xng_input);

    /* configure fds to send signals on io */
    if (fcntl(_native_tap_fd, F_SETOWN, getpid()) == -1) {
        err(1, "_native_init_uart0(): fcntl()");
    }

    /* set file access mode to nonblocking */
    if (fcntl(_native_tap_fd, F_SETFL, O_NONBLOCK|O_ASYNC) == -1) {
        err(1, "_native_init_uart0(): fcntl()");
    }
/*TODO:  check OSX ^^^ */

    puts("RIOT native tap initialized.");
    return _native_tap_fd;
}

void _native_marshall_ethernet(uint8_t *framebuf, uint8_t *data, int data_len)
{
    union eth_frame *f;
    char addr[ETHER_ADDR_LEN];

    f = (union eth_frame*)framebuf;
    addr[0] = addr[1] = addr[2] = addr[3] = addr[4] = addr[5] = (char)0xFF;

    //memcpy(f->field.header.ether_dhost, dst, ETHER_ADDR_LEN);
    memcpy(f->field.header.ether_dhost, addr, ETHER_ADDR_LEN);
    //memcpy(f->field.header.ether_shost, src, ETHER_ADDR_LEN);
    memcpy(f->field.header.ether_shost, _native_tap_mac, ETHER_ADDR_LEN);
    /* use our own ethertype, to filter frames at receiving side */
    f->field.header.ether_type = htons(NATIVE_ETH_PROTO);
    /* f->field.header.ether_type = htons(ETHERTYPE_IP); */
    _native_create_ipv4(f->field.data, data, data_len);
}

void _native_create_ipv4(char *framebuf, uint8_t *data, int data_len)
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
    fprintf (stderr, "#1 inet_pton() failed.\nError message: %s", strerror(status));
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

void _native_udp(struct ip *iphdr, char *databuf, uint8_t *data, int data_len)
{
    struct udphdr header;

    header.source = htons(0);
    header.dest = htons(ZEP_DEFAULT_PORT);
    header.len = htons(UDP_HDRLEN + ZEP_V1_HEADER_LEN + data_len);
    header.check = udp4_checksum(iphdr, &header, data, CC1100_FIFO_LENGTH);
    memcpy(databuf, &header, UDP_HDRLEN);
    _native_zep(databuf+UDP_HDRLEN, data, data_len);
}

void _native_zep(char *databuf, uint8_t *data, int data_len)
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
    memcpy(databuf+ZEP_V1_HEADER_LEN, data+4, data_len-2);

    /* RSSI = 0 */
    databuf[ZEP_V1_HEADER_LEN+data_len-2] = 0x0;
    /* FCS Valid = 1 / LQI Correlation Value = 0 */
    databuf[ZEP_V1_HEADER_LEN+data_len-1] = 0x80;
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
    if (write(_native_tap_fd, buffer, ETHER_HDR_LEN + data_len) == -1) {
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
