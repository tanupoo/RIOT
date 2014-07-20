#ifndef DUMMY_TDMA_H
#define DUMMY_TDMA_H

#include "radio_driver.h"
#ifdef MODULE_CC2420
#include "cc2420.h"
#define DUMMY_TDMA_MAX_DATA_LENGTH CC2420_MAX_DATA_LENGTH
#elif defined MODULE_AT86RF231
#include "at86rf231.h"
#define DUMMY_TDMA_MAX_DATA_LENGTH AT86RF231_MAX_DATA_LENGTH
#else
#warning No valid radio driver used
#define DUMMY_TDMA_MAX_DATA_LENGTH    (0)
#endif

void dummy_tdma_init(const ieee802154_radio_driver_t *radio);

#ifdef MODULE_CC2420
int8_t dummy_tdma_send(cc2420_packet_t *p);
#elif defined MODULE_AT86RF231
int8_t dummy_tdma_send(at86rf231_packet_t *p);
#endif

#endif /* DUMMY_TDMA_H */
