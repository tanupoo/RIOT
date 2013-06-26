#ifndef _TAP_H
#define _TAP_H

#include <net/ethernet.h>

/**
 * create and/or open tap device "name"
 *
 * if "name" is an empty string, the kernel chooses a name
 * if "name" is an existing device, that device is used
 * otherwise a device named "name" is created
 */
int tap_init(char *name);
void _native_marshall_ethernet(char *framebuf, char *data, int data_len);
void _native_handle_cc110xng_input(void);

extern int _native_tap_fd;
extern char _native_tap_mac[ETHER_ADDR_LEN];

union eth_frame {
    struct {
        struct ethhdr header;
        char data[ETH_DATA_LEN];
    } field;
    unsigned char buffer[ETH_FRAME_LEN];
};

#endif /* _TAP_H */
