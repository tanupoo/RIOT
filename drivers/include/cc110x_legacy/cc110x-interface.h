#ifndef CC1100_H
#define CC1100_H

#include <radio/radio.h>
#include <radio/types.h>
#include <stdint.h>
#include <cc110x-config.h>

#define CC1100_MAX_DATA_LENGTH (58)

#define CC1100_HEADER_LENGTH   (3)             ///< Header covers SRC, DST and FLAGS

#define	CC1100_BROADCAST_ADDRESS (0x00)	///< CC1100 broadcast address

#define MAX_UID					 (0xFF) ///< Maximum UID of a node is 255
#define MIN_UID					 (0x01) ///< Minimum UID of a node is 1

#define MIN_CHANNR					(0)	///< Minimum channel number
#define MAX_CHANNR				   (24)	///< Maximum channel number

#define MIN_OUTPUT_POWER			(0)	///< Minimum output power value
#define MAX_OUTPUT_POWER		   (11)	///< Maximum output power value

#define PACKET_LENGTH				(0x3E)		///< Packet length = 62 Bytes.
#define CC1100_SYNC_WORD_TX_TIME   (90000)		// loop count (max. timeout ~ 15 ms) to wait for
												// sync word to be transmitted (GDO2 from low to high)
/**
 * @name	Defines used as state values for state machine
 * @{
 */
#define RADIO_UNKNOWN			(0)
#define RADIO_AIR_FREE_WAITING	(1)
#define RADIO_WOR				(2)
#define RADIO_IDLE				(3)
#define RADIO_SEND_BURST		(4)
#define RADIO_RX				(5)
#define RADIO_SEND_ACK			(6)
#define RADIO_PWD				(7)

#define TX_START                (0)
#define TX_WAIT                 (1)
#define RX_START                (2)
#define RX_WAIT                 (3)
#define SETUP                   (4)
#define SWITCH_TX_TO_RX         5
#define RX_INVALID              6              

#define TX_MODE                 0
#define RX_MODE                 1
#define INIT_MODE               2
#define RX_INVALID_MODE         3

#define LOW                     0
#define HIGH                    1

#define FIFO_SIZE               64

#define SYNC                    0
#define END_OF_PACKET           1 

#define BYTES_IN_TX_FIFO        60
#define BYTES_IN_RX_FIFO        60

#define CC110X_MSG_BUFFER_SIZE  32
#define PRIORITY_CC110X         (PRIORITY_MAIN - 1)

#define MTU_IEEE802154          127
#define MTU_CC110X              MTU_IEEE802154 

/** @} */

extern volatile cc110x_flags rflags;	///< Radio flags
extern char cc110x_conf[];

/**
 * @brief	CC1100 layer 0 protocol
 *
 * <pre>
---------------------------------------------------
|        |         |         |       |            |
| Length | Address | PhySrc  | Flags |    Data    |
|        |         |         |       |            |
---------------------------------------------------
  1 byte   1 byte    1 byte   1 byte   <= 58 bytes

Flags:
		Bit | Meaning
		--------------------
		7:4	| -
		3:1 | Protocol
		  0 | Identification
</pre>
Notes:
\li length & address are given by CC1100
\li Identification is increased is used to scan duplicates. It must be increased
	for each new packet and kept for packet retransmissions.
 */
typedef struct __attribute__ ((packed)) {
	uint8_t length;					///< Length of the packet (without length byte)
	uint8_t address;				///< Destination address
	uint8_t phy_src;				///< Source address (physical source)
	uint8_t flags;					///< Flags
	uint8_t data[CC1100_MAX_DATA_LENGTH];	///< Data (high layer protocol)
} cc110x_packet_t;

typedef struct {
    uint8_t rssi;
    uint8_t lqi;
    cc110x_packet_t packet;
} rx_buffer_t;

struct tx_data_t {
    uint16_t bytes_left;
    uint8_t iterations;
    uint8_t write_reamining_data_flag;
    uint8_t packet_sent_flag;
    uint8_t *tx_buffer_position;
    uint16_t packets_sent;
} tx_data_t;

struct rx_data_t {
    uint16_t bytes_left;
    uint8_t packet_received_flag;
    uint8_t sync_or_end_of_packet_flag;
    uint8_t *rx_buffer_position;
    uint8_t length_byte;
    uint8_t crc_ok;
    uint16_t packets_received;
} rx_data_t;

enum radio_mode {
	RADIO_MODE_GET	= -1,		///< leave mode unchanged
	RADIO_MODE_OFF	= 0,		///< turn radio off
	RADIO_MODE_ON	= 1			///< turn radio on
};

extern rx_buffer_t cc110x_rx_buffer[]; 

extern volatile uint8_t rx_buffer_next;	    ///< Next packet in RX queue

extern volatile uint8_t radio_state;		///< Radio state
extern cc110x_statistic_t cc110x_statistic;

int transceiver_pid;                         ///< the transceiver thread pid

void cc110x_init(int transceiver_pid);

void cc110x_rx_handler(void);

uint8_t cc110x_send(cc110x_packet_t *pkt);

uint8_t cc110x_get_buffer_pos(void);

void cc110x_setup_rx_mode(void);
void cc110x_switch_to_rx(void);
void cc110x_wakeup_from_rx(void);
void cc110x_switch_to_pwd(void);

void cc110x_disable_interrupts(void);
int16_t cc110x_set_config_channel(uint8_t channr);
int16_t cc110x_set_channel(uint8_t channr);
int16_t cc110x_get_channel(void);

radio_address_t cc110x_set_address(radio_address_t addr);
radio_address_t cc110x_set_config_address(radio_address_t addr);
radio_address_t cc110x_get_address(void);
void cc110x_set_monitor(uint8_t mode);

void cc110x_print_config(void);

/**
 * @brief	GDO0 interrupt handler.
 */
void cc110x_gdo0_irq(void);

/**
 * @brief	GDO2 interrupt handler.
 *
 * @note	Wakes up MCU on packet reception.
 */
void cc110x_gdo2_irq(void);

void cc110x_gdo0_interrupt(void);
void cc110x_gdo2_interrupt(void);

#ifdef DBG_IGNORE
/**
 * @brief   Initialize ignore function
 */
void cc110x_init_ignore(void);

/**
 * @brief       Adds a address to the ignore list
 *
 * @param addr  The physical address to be ignored
 *
 * @return      0 if list is full, 1 otherwise
 *
 */
uint8_t cc110x_add_ignored(radio_address_t addr);
#endif



#endif
