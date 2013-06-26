#include <stdio.h>
#include <err.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <arpa/inet.h>

#include "debug.h"

#include "tap.h"
#include "cpu.h"

#include "cc110x-arch.h"
#include "cc110x_ng.h"
#include "cc110x_spi.h"
#include "cc110x-internal.h" /* CC1100 constants */

#define STATE_NULL      0x00
#define STATE_SEL       0x01
#define STATE_WRITE_B   0x02
#define STATE_READ_B    0x03
#define STATE_READ_S    0x04
#define STATE_WRITE_S   0x05

/* TODO: get from elsewhere */
#define BUFFER_LENGTH 2048

static int _native_cc110x_enabled;
static uint8_t _native_cc110x_state;
static uint8_t addr;
static char rx_buffer[BUFFER_LENGTH], tx_buffer[BUFFER_LENGTH];
static int rx_bi, rx_bc, tx_bi, tx_bc;

static int native_cc110x_gd0;
static int native_cc110x_gd1;
static int native_cc110x_gd2;

static int native_cc110x_gd0_enabled;
static int native_cc110x_gd2_enabled;


void _native_handle_cc110xng_input(void)
{
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
        if (ntohs(f->field.header.h_proto) == NATIVE_ETH_PROTO) {
            nread = nread - ETH_HLEN;
            memcpy(rx_buffer, buf+ETH_HLEN, nread);
            rx_bc = nread;
            rx_bi = 0;
            printf("got %d bytes\n", rx_bc);
            cc110x_gdo2_irq();
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

/* TODO: move to tap */
int _native_set_cc110xng_fds(void)
{
    DEBUG("_native_set_cc110xng_fds");
    FD_SET(_native_tap_fd, &_native_rfds);
    return _native_tap_fd;
}

/* TODO: move to tap */
int send_buf(void)
{
    char buf[BUFFER_LENGTH];
    int nsent;

    _native_marshall_ethernet(buf, tx_buffer, tx_bc);
    if ((nsent = write(_native_tap_fd, buf, tx_bc + ETH_HLEN)) == -1) {;
        warn("write");
        return -1;
    }
    return 0;
}

uint8_t do_strobe(void)
{
    switch (addr) {
        case CC1100_SRES:
            warnx("do_strobe: not implemented: CC1100_SRES");
            break;
        case CC1100_SFSTXON:
            warnx("do_strobe: not implemented: CC1100_SFSTXON");
            break;
        case CC1100_SXOFF:
            warnx("do_strobe: not implemented: CC1100_SXOFF");
            break;
        case CC1100_SCAL:
            warnx("do_strobe: not implemented: CC1100_SCAL");
            break;
        case CC1100_SRX:
            warnx("do_strobe: not implemented: CC1100_SRX");
            break;
        case CC1100_STX:
            warnx("do_strobe: not prooperlu implemented: CC1100_STX");
            if ((tx_bc > 0) && (send_buf() == -1)) {
                native_cc110x_gd2 = 1;
            }
            else {
                native_cc110x_gd2 = 1;
            }
            break;
        case CC1100_SIDLE:
            warnx("do_strobe: not implemented: CC1100_SIDLE ");
            break;
        case CC1100_SAFC:
            warnx("do_strobe: not implemented: CC1100_SAFC");
            break;
        case CC1100_SWOR:
            warnx("do_strobe: not implemented: CC1100_SWOR");
            break;
        case CC1100_SPWD:
            warnx("do_strobe: not implemented: CC1100_SPWD");
            break;
        case CC1100_SFRX:
            rx_bc = 0;
            rx_bi = 0;
            break;
        case CC1100_SFTX:
            tx_bc = 0;
            tx_bi = 0;
            break;
        case CC1100_SWORRST:
            warnx("do_strobe: not implemented: CC1100_SWORRST");
            break;
        case CC1100_SNOP:
            warnx("do_strobe: not implemented: CC1100_SNOP");
            break;
        default:
            errx(EXIT_FAILURE, "do_strobe: internal error");
    }
    return addr;
}

uint8_t parse_header(uint8_t c)
{
    printf("parse_header(0x%02X)\n", c);

    addr = c & 0x3F;
    uint8_t mode = c & 0xC0;

    /* set access mode */
    switch (mode) {
        case CC1100_READ_BURST:
            printf("parse_header: CC1100_READ_BURST");
            _native_cc110x_state = STATE_READ_B;
            break;
        case CC1100_WRITE_BURST:
            printf("cc110x_txrx: CC1100_WRITE_BURST");
            _native_cc110x_state = STATE_WRITE_B;
            break;
        case CC1100_READ_SINGLE:
            printf("cc110x_txrx: CC1100_READ_SINGLE");
            _native_cc110x_state = STATE_READ_S;
            break;
        default:
            printf("cc110x_txrx: CC1100_WRITE_SINGLE");
            _native_cc110x_state = STATE_WRITE_S;
    }

    /* parse address type */
    if (addr <= 0x2E) {
        printf(" configuration register");
    }
    else if ((0x30 <= addr) && (addr <= 0x3D)) {
        if ((_native_cc110x_state == STATE_WRITE_B) || (_native_cc110x_state == STATE_WRITE_S)) {
            printf(" strobe command");
            return do_strobe();
        }
        else {
            printf(" status register");
        }
    }
    else if (addr == 0x3E) {
        printf(" patable");
    }
    else if (addr == 0x3F) {
        if ((_native_cc110x_state == STATE_WRITE_B) || (_native_cc110x_state == STATE_WRITE_S)) {
            printf(" TX");
        }
        else {
            printf(" RX");
        }
    }
    else {
        errx(EXIT_FAILURE, "parse_header: unhandled addr: 0x%02X", addr);
    }

    return c;
}

uint8_t read_single(uint8_t c)
{
    printf("read_single\n");
    if (c != CC1100_NOBYTE) {
        warnx("cc110x_txrx: trying to CC1100_READ_SINGLE without C1100_NOBYTE");
        return 0;
    }
    if (addr == CC1100_PKTCTRL0) {
        printf("CC1100_PKTCTRL0\n");
        return VARIABLE_PKTLEN;
    }
    else {
        warnx("read_single: unhandled addr\n");
        return 0;
    }
    c = rx_buffer[rx_bi++];
    if (rx_bi >= rx_bc) {
        warnx("read too much");
    }
    return c;
}

uint8_t read_burst(uint8_t c)
{
    printf("read_burst: ");
    if (c != CC1100_NOBYTE) {
        warnx("cc110x_txrx: trying to CC1100_READ_BURST without C1100_NOBYTE");
        return 0;
    }
    if (addr == CC1100_RXFIFO) {
        printf("CC1100_RXFIFO\n");
        if (rx_bi == rx_bc) {
            warnx("read_burst: buffer empty");
        }
        return rx_buffer[rx_bi++];
    }
    else if (addr == CC1100_RXBYTES) {
        printf("CC1100_RXBYTES\n");
        return BYTES_IN_RXFIFO;
    }
    else {
        printf("unhandled addr: 0x%02X\n", addr);
    }
    return 0;
}

uint8_t write_single(uint8_t c)
{
    printf("write_single\n");
    return 0;
}

uint8_t write_burst(uint8_t c)
{
    printf("write_burst\n");
    if (tx_bc == BUFFER_LENGTH) {
        errx(EXIT_FAILURE, "write_burst: buffer too small");
    }
    tx_buffer[tx_bi++] = c;
    tx_bc++;
    return 0;
}

/* arch */

/**
 * writes to SSP0 data register and reads from it once it is ready
 */
uint8_t cc110x_txrx(uint8_t c)
{
    DEBUG("cc110x_txrx\n");
    switch (_native_cc110x_state) {
        case STATE_SEL:
            c = parse_header(c);
            break;
        case STATE_WRITE_B:
            c = write_burst(c);
            break;
        case STATE_READ_B:
            c = read_burst(c);
            break;
        case STATE_READ_S:
            c = read_single(c);
            break;
        case STATE_WRITE_S:
            c = write_single(c);
            break;
        case STATE_NULL:
            warnx("received command(?) in NULL state");
        default:
            errx(EXIT_FAILURE, "funny cc110x_ng state");
    }
    return c;
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
    DEBUG("cc110x_spi_select\n");
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
