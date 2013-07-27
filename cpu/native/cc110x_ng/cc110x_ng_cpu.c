#include <stdio.h>
#include <err.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <arpa/inet.h>

#include "debug.h"

#include "tap.h"
#include "cc1100sim.h"
#include "cpu.h"

#include "cc110x-arch.h"
#include "cc110x_ng.h"
#include "cc110x_spi.h"
#include "cc110x-internal.h" /* CC1100 constants */


static int _native_cc110x_enabled;

void _native_handle_cc110xng_input(void)
{

    /* TODO: partially move to tap */
    int nread;
    char buf[BUFFER_LENGTH];
    union eth_frame *f;

    if (!FD_ISSET(_native_tap_fd, &_native_rfds)) {
        DEBUG("_native_handle_cc110xng_input - nothing to do\n");
        return;
    }
    nread = read(_native_tap_fd, buf, BUFFER_LENGTH);
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
                memcpy(rx_fifo, buf+ETHER_HDR_LEN+1, nread);
                status_registers[CC1100_RXBYTES - 0x30] = nread;
                rx_fifo_idx = 0;
                DEBUG("_native_handle_cc110xng_input: got %d bytes payload\n", nread);
                cc110x_gdo2_irq();
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
    cpu_switch_context_exit();
}

/* TODO: move to tap */
int _native_set_cc110xng_fds(void)
{
    DEBUG("_native_set_cc110xng_fds");
    FD_SET(_native_tap_fd, &_native_rfds);
    return _native_tap_fd;
}

/* arch */

/**
 * writes to SSP0 data register and reads from it once it is ready
 * TODO: move content to simulator
 */
uint8_t cc110x_txrx(uint8_t c)
{
    DEBUG("cc110x_txrx\n");
    return do_txrx(c);
}

/**
 * disables GDO0 interrupt
 */
void cc110x_gdo0_enable(void)
{
    /* this would be for rising/high edge if this was proper hardware */
    DEBUG("cc110x_gdo0_enable\n");
    native_cc110x_gd0_enabled = 1;
    return;
}

/**
 * enables GDO0 interrupt
 */
void cc110x_gdo0_disable(void)
{
    DEBUG("cc110x_gdo0_disable\n");
    native_cc110x_gd0_enabled = 0;
    return;
}

/**
 * enables GDO2 interrupt
 */
void cc110x_gdo2_enable(void)
{
    /* this would be for falling/low edge if this was proper hardware */
    DEBUG("cc110x_gdo2_enable\n");
    native_cc110x_gd2_enabled = 1;
    return;
}

/**
 * disables GDO2 interrupt
 */
void cc110x_gdo2_disable(void)
{
    DEBUG("cc110x_gdo2_disable\n");
    native_cc110x_gd2_enabled = 0;
    return;
}

/**
 * enable interrupts for GDO0 and GDO2
 */
void cc110x_init_interrupts(void)
{
    /* this would be for low edge in both cases if this was proper hardware */
    DEBUG("cc110x_init_interrupts\n");
    cc110x_gdo2_enable();
    cc110x_gdo0_enable();
    return;
}

/* Disables RX interrupt etc. */
void cc110x_before_send(void)
{
    DEBUG("cc110x_before_send\n");
    cc110x_gdo2_disable();
    return;
}
void cc110x_after_send(void)
{
    DEBUG("cc110x_after_send\n");
    cc110x_gdo2_enable();
    return;
}

/* spi */

int cc110x_get_gdo0(void)
{
    DEBUG("cc110x_get_gdo0\n");
    return native_cc110x_gd0;
}
int cc110x_get_gdo1(void)
{
    DEBUG("cc110x_get_gdo1\n");
    return native_cc110x_gd1;
}
int cc110x_get_gdo2(void)
{
    DEBUG("cc110x_get_gdo2\n");
    return native_cc110x_gd2--;
}

void cc110x_spi_init(void)
{
    DEBUG("cc110x_spi_init\n");
    _native_cc110x_enabled = 1; /* power on */
    return;
}

void cc110x_spi_cs(void)
{
    DEBUG("cc110x_spi_cs\n");
    return;
}
void cc110x_spi_select(void)
{
    DEBUG("___cc110x_spi_select\n");
    _native_cc110x_state = STATE_SEL;
    return;
}
void cc110x_spi_unselect(void)
{
    DEBUG("cc110x_spi_unselect\n");
    _native_cc110x_state = STATE_NULL;
    return;
}

/* ng */
