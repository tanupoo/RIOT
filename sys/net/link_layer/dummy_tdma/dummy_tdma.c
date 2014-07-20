/*
 * Copyright (C) 2014 Oliver Hahm <oliver.hahm@inria.fr> 
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

#include <stdint.h>
#include <string.h>

#include "dummy_tdma.h"
#include "ieee802154_frame.h"
#include "radio_driver.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

ieee802154_radio_driver_t _radio;

#define DUMMY_TDMA_SEND_BUFFER_SIZE (10)

static ieee802154_frame_t _send_buffer[DUMMY_TDMA_SEND_BUFFER_SIZE];
static uint8_t _send_buffer_data[DUMMY_TDMA_SEND_BUFFER_SIZE * DUMMY_TDMA_MAX_DATA_LENGTH];
static uint8_t _send_buf_write_idx, _send_buf_read_idx, _send_buf_counter;

void dummy_tdma_receive_802154_packet_callback_t(void *buf, unsigned int len,
                                                  int8_t rssi, uint8_t lqi,
                                                  bool crc_ok)
{
    ieee802154_frame_t frame;
    ieee802154_frame_read(buf, &frame, (len + 2));

    DEBUGF("packet received\n");
    /* TODO: check for particular RTS packet */
    if (_send_buf_counter) {
        DEBUGF("something to send");
        if (_radio.send(PACKET_KIND_DATA,
                    /* dest is ignored anyway */
                        (ieee802154_node_addr_t) ((uint64_t) 0xFFFF),
                        false, false, 
                        _send_buffer[_send_buf_read_idx].payload,
                        _send_buffer[_send_buf_read_idx].payload_len) ==
             RADIO_TX_OK) {
            _send_buf_read_idx++;
                /* wrap around position index */
                if (_send_buf_read_idx >= DUMMY_TDMA_SEND_BUFFER_SIZE) {
                    DEBUGF("readerwrap around");
                    _send_buf_read_idx = 0;
                }
            _send_buf_counter--;
        }
    }
}

void dummy_tdma_init(const ieee802154_radio_driver_t *radio)
{
    memcpy(&_radio, radio, sizeof(ieee802154_radio_driver_t));
    _radio.init();
    _radio.set_receive_callback(dummy_tdma_receive_802154_packet_callback_t);
}

#ifdef MODULE_CC2420
int8_t dummy_tdma_send(cc2420_packet_t *p)
#elif defined MODULE_AT86RF231
int8_t dummy_tdma_send(at86rf231_packet_t *p)
#endif
{
    if (_send_buf_counter > DUMMY_TDMA_SEND_BUFFER_SIZE) {
        puts("Dummy TDMA send buffer full!");
        return 0;
    }
    else {
        DEBUGF("copy data into buffer at position %u, counter is at %u\n", _send_buf_write_idx, _send_buf_counter);
        /* copy header */
        memcpy(&(_send_buffer[_send_buf_write_idx]), p, sizeof(ieee802154_frame_t));
        /* and data */
        memcpy(&(_send_buffer_data[_send_buf_write_idx * (DUMMY_TDMA_MAX_DATA_LENGTH)]), p, sizeof(ieee802154_packet_t));
        _send_buf_write_idx++;

        /* increase buffer filling counter */
        _send_buf_counter++;

        /* wrap around position index */
        if (_send_buf_write_idx >= DUMMY_TDMA_SEND_BUFFER_SIZE) {
            DEBUGF("writer wrap around");
            _send_buf_write_idx = 0;
        }
    }
    return 1;
}
