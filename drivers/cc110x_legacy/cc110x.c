#include <cc110x_ng.h>
#include <cc110x-arch.h>
#include <cc110x-config.h>
#include <cc110x-defaultSettings.h>
#include <cc110x-internal.h>
#include <cc110x_spi.h>
#include <cc110x-reg.h>
#include <msg.h>
#include <thread.h>
#include <kernel.h>

#include <hwtimer.h>
#include <config.h>

#define ENABLE_DEBUG    (1)
#include <debug.h>

/* some externals */
extern uint8_t pa_table[];				///< PATABLE with available output powers
extern uint8_t pa_table_index;          ///< Current PATABLE Index

/* global variables */

cc110x_statistic_t cc110x_statistic;

volatile cc110x_flags rflags;		                ///< Radio control flags
volatile uint8_t radio_state = RADIO_UNKNOWN;		///< Radio state

static radio_address_t radio_address;                     ///< Radio address
static uint8_t radio_channel;                             ///< Radio channel

int transceiver_pid;                         ///< the transceiver thread pid
int cc110x_pid;

/* internal function prototypes */
static int rd_set_mode(int mode);
static void reset(void);
static void power_up_reset(void);
static void write_register(uint8_t r, uint8_t value);

msg_t msg_buffer[CC110X_MSG_BUFFER_SIZE];
static char cc110x_handler_stack[KERNEL_CONF_STACKSIZE_MAIN];

// ONLY FOR TESTING
static uint8_t packet[8] = {0, 0, 0, 0, 1, 1, 1, 1};

int irq_gdo0_count = 0;
int irq_gdo2_count = 0;

uint8_t tx_buffer[258];
uint8_t rx_buffer[258];

struct rx_data_t rx_data = {
    0,
    0,
    SYNC,
    rx_buffer,
    0,
    0,
    0
};

struct tx_data_t tx_data = {
    0,
    0,
    0,
    0,
    tx_buffer,
    0
};

uint8_t radio_mode;

/*---------------------------------------------------------------------------*/
// 								Radio Driver API
/*---------------------------------------------------------------------------*/
void cc110x_init(int tpid) {
//    transceiver_pid = tpid;
//   DEBUG("Transceiver PID: %i\n", transceiver_pid);

//	rx_buffer_next = 0;

#ifdef MODULE_CC110X_SPI
    /* Initialize SPI */
	cc110x_spi_init();
#endif

	/* Load driver & reset */
	power_up_reset();

    /* Write configuration to configuration registers */
    cc110x_writeburst_reg(0x00, cc110x_conf, CC1100_CONF_SIZE);

	/* Write PATABLE (power settings) */
	cc110x_write_reg(CC1100_PATABLE, pa_table[pa_table_index]);

	/* Initialize Radio Flags */
/*	rflags._RSSI         = 0x00;
	rflags.LL_ACK       = 0;
	rflags.CAA          = 0;
	rflags.CRC          = 0;
	rflags.SEQ          = 0;
	rflags.MAN_WOR      = 0;
	rflags.KT_RES_ERR   = 0;
	rflags.TX           = 0;
	rflags.WOR_RST      = 0;
*/
	/* Set default channel number */
#ifdef MODULE_CONFIG
	cc110x_set_config_channel(sysconfig.radio_channel);
#else
    cc110x_set_channel(CC1100_DEFAULT_CHANNR);
#endif
    DEBUG("CC1100 initialized and set to channel %i\n", radio_channel);

	// Switch to desired mode (WOR or RX)
	rd_set_mode(RADIO_MODE_ON);

#ifdef DBG_IGNORE
    cc110x_init_ignore();
#endif
}

void cc110x_disable_interrupts(void) {
    puts("cc110x_gdo0_disable_interrupts");
	cc110x_gdo2_disable();
	cc110x_gdo0_disable();
}

void cc110x_gdo0_irq(void) {
	// Air was not free -> Clear CCA flag
	rflags.CAA = false;
    puts("cc110x_gdo0_irq");
	// Disable carrier sense detection (GDO0 interrupt)
	cc110x_gdo0_disable();
}

void cc110x_gdo2_irq(void) {
    puts("cc110x_gdo2_irq");
	//cc110x_rx_handler();
}

uint8_t cc110x_get_buffer_pos(void) {
    return (rx_buffer_next-1);
}

radio_address_t cc110x_get_address() {
    return radio_address;
}

radio_address_t cc110x_set_address(radio_address_t address) {
	if ((address < MIN_UID) || (address > MAX_UID)) {
		return 0;
	}

	uint8_t id = (uint8_t) address;
	if (radio_state != RADIO_UNKNOWN) {
		write_register(CC1100_ADDR, id);
	}

	radio_address = id;
	return radio_address;
}

#ifdef MODULE_CONFIG
radio_address_t cc110x_set_config_address(radio_address_t address) {
    radio_address_t a = cc110x_set_address(address);
    if (a) {
        sysconfig.radio_address = a;
    }
    config_save();
    return a;
}
#endif

void cc110x_set_monitor(uint8_t mode) {
    if (mode) {
        write_register(CC1100_PKTCTRL1, (0x04));
    }
    else {
        write_register(CC1100_PKTCTRL1, (0x06));
    }
}

void cc110x_setup_rx_mode(void) {
	// Stay in RX mode until end of packet
	cc110x_write_reg(CC1100_MCSM2, 0x07);
	cc110x_switch_to_rx();
}

void cc110x_switch_to_rx(void) {
	radio_state = RADIO_RX;
	cc110x_strobe(CC1100_SRX);
}

void cc110x_wakeup_from_rx(void) {
	if (radio_state != RADIO_RX) {
        return;
    }
    DEBUG("CC1100 going to idle\n");
	cc110x_strobe(CC1100_SIDLE);
	radio_state = RADIO_IDLE;
}

char* cc110x_get_marc_state(void) {
	uint8_t state;

	// Save old radio state
	uint8_t old_state = radio_state;

	// Read content of status register
	state = cc110x_read_status(CC1100_MARCSTATE) & MARC_STATE;

	// Make sure in IDLE state.
	// Only goes to IDLE if state was RX/WOR
	cc110x_wakeup_from_rx();

	// Have to put radio back to WOR/RX if old radio state
	// was WOR/RX, otherwise no action is necessary
	if (old_state == RADIO_WOR || old_state == RADIO_RX) {
		cc110x_switch_to_rx();
	}

	switch (state)
	{
		// Note: it is not possible to read back the SLEEP or XOFF state numbers
		// because setting CSn low will make the chip enter the IDLE mode from the
		// SLEEP (0) or XOFF (2) states.
		case 1: return "IDLE";
		case 3: case 4: case 5: return "MANCAL";
		case 6: case 7: return "FS_WAKEUP";
		case 8: case 12: return "CALIBRATE";
		case 9: case 10: case 11: return "SETTLING";
		case 13: case 14: case 15: return "RX";
		case 16: return "TXRX_SETTLING";
		case 17: return "RXFIFO_OVERFLOW";
		case 18: return "FSTXON";
		case 19: case 20: return "TX";
		case 21: return "RXTX_SETTLING";
		case 22: return "TXFIFO_UNDERFLOW";
		default: return "UNKNOWN";
	}
}

char* cc110x_state_to_text(uint8_t state) {
	switch (state)
	{
		case RADIO_UNKNOWN:
			return "Unknown";
		case RADIO_AIR_FREE_WAITING:
			return "CS";
		case RADIO_WOR:
			return "WOR";
		case RADIO_IDLE:
			return "IDLE";
		case RADIO_SEND_BURST:
			return "TX BURST";
		case RADIO_RX:
			return "RX";
		case RADIO_SEND_ACK:
			return "TX ACK";
		case RADIO_PWD:
			return "PWD";
		default:
			return "unknown";
	}
}

void cc110x_print_config(void) {
	printf("Current radio state:          %s\r\n", cc110x_state_to_text(radio_state));
	printf("Current MARC state:           %s\r\n", cc110x_get_marc_state());
	printf("Current channel number:       %u\r\n", radio_channel);
}

void cc110x_switch_to_pwd(void) {
    DEBUG("[cc110x_ng] switching to powerdown\n");
    cc110x_wakeup_from_rx();
	cc110x_strobe(CC1100_SPWD);
	radio_state = RADIO_PWD;
}
    
/*---------------------------------------------------------------------------*/
int16_t cc110x_set_channel(uint8_t channr) {
	uint8_t state = cc110x_read_status(CC1100_MARCSTATE) & MARC_STATE;
	if ((state != 1) && (channr > MAX_CHANNR)) {
        return -1;
    }
	write_register(CC1100_CHANNR, channr*10);
	radio_channel = channr;
	return radio_channel;
}

#ifdef MODULE_CONFIG
int16_t cc110x_set_config_channel(uint8_t channr) {
    int16_t c = cc110x_set_channel(channr);
    if (c) {
        sysconfig.radio_channel = c;
    }
    config_save();
    return c;
}
#endif

int16_t cc110x_get_channel(void) {
    return radio_channel;
}


/*---------------------------------------------------------------------------*/
// 							CC1100 reset functionality
/*---------------------------------------------------------------------------*/

static void reset(void) {
	cc110x_wakeup_from_rx();
#ifdef MODULE_CC110x_SPI
	cc110x_spi_select();
#endif
	cc110x_strobe(CC1100_SRES);
	hwtimer_wait(RTIMER_TICKS(100));
}

static void power_up_reset(void) {
#ifdef MODULE_CC110x_SPI
	cc110x_spi_unselect();
	cc110x_spi_cs();
	cc110x_spi_unselect();
#endif
	hwtimer_wait(RESET_WAIT_TIME);
	reset();
	radio_state = RADIO_IDLE;
}
  
static void write_register(uint8_t r, uint8_t value) {
	// Save old radio state
	uint8_t old_state = radio_state;

	/* Wake up from WOR/RX (if in WOR/RX, else no effect) */
	cc110x_wakeup_from_rx();
	cc110x_write_reg(r, value);

	// Have to put radio back to WOR/RX if old radio state
	// was WOR/RX, otherwise no action is necessary
	if ((old_state == RADIO_WOR) || (old_state == RADIO_RX)) {
		cc110x_switch_to_rx();
	}
}

void cc110x_gdo0_interrupt(void) 
{
    irq_gdo0_count++;

    if (radio_mode == TX_MODE) {
        tx_data.write_reamining_data_flag = 0;
        tx_data.packet_sent_flag = 1;
    } else {
        if (rx_data.sync_or_end_of_packet_flag == SYNC) {

            hwtimer_wait(64);

            if ((cc110x_read_status(CC1100_RXBYTES) & BYTES_IN_RXFIFO)) {
                //msg_t m;
                //m.type = (uint16_t) RX_MODE;
                //msg_send_int(&m, cc110x_pid);

                rx_data.length_byte = cc110x_read_reg(CC1100_RXFIFO);
                
                if (rx_data.length_byte != MTU_CC110X) {
                    cc110x_gdo2_disable();
                    msg_t m;
                    m.type = (uint16_t) RX_INVALID_MODE;
                    msg_send_int(&m, cc110x_pid);
                    
                    cc110x_enable_interrupt(GDO0, LOW);
                } else {
                    msg_t m;
                    m.type = (uint16_t) RX_MODE;
                    msg_send_int(&m, cc110x_pid);               
                    rx_data.bytes_left = rx_data.length_byte + 2;
                
                    rx_data.rx_buffer_position++;
                    rx_data.sync_or_end_of_packet_flag = END_OF_PACKET;
        
                    if (rx_data.bytes_left < FIFO_SIZE) {
                        cc110x_gdo2_disable();
                    }
               
                    cc110x_enable_interrupt(GDO0, LOW);
                }
            } else {
                cc110x_gdo2_disable();
                msg_t m;
                m.type = (uint16_t) RX_INVALID_MODE;
                msg_send_int(&m, cc110x_pid);
            }
        } else {
            cc110x_readburst_reg(CC1100_RXFIFO, (char*)rx_data.rx_buffer_position,
                                                        rx_data.bytes_left);
            rx_buffer[0] = rx_data.length_byte;
            rx_data.sync_or_end_of_packet_flag = SYNC;
            rx_data.packet_received_flag = 1;
            rx_data.crc_ok = ((rx_buffer[rx_data.length_byte + 2]) & CRC_OK);
        } 
    }
}

void cc110x_gdo2_interrupt(void) 
{
    irq_gdo2_count++;

    if (radio_mode == TX_MODE) {
        if (tx_data.write_reamining_data_flag) {

            cc110x_writeburst_reg(CC1100_TXFIFO,(char*)tx_data.tx_buffer_position,
                                                        tx_data.bytes_left);
            cc110x_gdo2_disable();
        } else {
            cc110x_writeburst_reg(CC1100_TXFIFO,(char*)tx_data.tx_buffer_position,
                                                            BYTES_IN_TX_FIFO);
            tx_data.tx_buffer_position += BYTES_IN_TX_FIFO;
            tx_data.bytes_left -= BYTES_IN_TX_FIFO;

            if (!(--tx_data.iterations)) {
                tx_data.write_reamining_data_flag = 1;
            }
        } 
    } else {
        cc110x_readburst_reg(CC1100_RXFIFO, (char*)rx_data.rx_buffer_position,
                                                    (BYTES_IN_RX_FIFO - 1));
        rx_data.bytes_left -= (BYTES_IN_RX_FIFO - 1);
        rx_data.rx_buffer_position += (BYTES_IN_RX_FIFO - 1); 
    }

    cc110x_clear_interrupt(GDO2);
}


void cc110x_state_machine(void) 
{
    msg_t init_message;

    msg_init_queue(msg_buffer, CC110X_MSG_BUFFER_SIZE);

    uint8_t state = SETUP;
    uint8_t length;
    unsigned int cpsr;

    cc110x_write_reg(CC1100_FIFOTHR, 0x0E);

    while (1) {
        switch (state) {
            case(SETUP):
                //set interrupts
                DEBUG("[SETUP] %d, %d, %d, %d\n", init_message.type, irq_gdo0_count, rx_data.length_byte, radio_mode);
                 
                msg_receive(&init_message);
                
                cpsr = disableIRQ();

                if (init_message.type == RX_MODE) {
                    state = RX_WAIT;
                    radio_mode = RX_MODE;
                } else if(init_message.type == TX_MODE) {
                    state = TX_START;
                    radio_mode = TX_MODE;
                    
                    cc110x_write_reg(CC1100_IOCFG2, 0x02);
                    cc110x_write_reg(CC1100_IOCFG0, 0x06);
                    cc110x_enable_interrupt(GDO0, LOW);
                    cc110x_enable_interrupt(GDO2, LOW);
                } else if(init_message.type == INIT_MODE) {
                    radio_mode = RX_MODE;
                    
                    cc110x_write_reg(CC1100_IOCFG2, 0x00);
                    cc110x_write_reg(CC1100_IOCFG0, 0x06);
                    cc110x_enable_interrupt(GDO0, HIGH);
                    cc110x_enable_interrupt(GDO2, HIGH);
                    cc110x_strobe(CC1100_SRX);
                } if (init_message.type == RX_INVALID_MODE) {
                    state = RX_INVALID;
                    radio_mode = RX_MODE;
                }

                restoreIRQ(cpsr);
                break;
            case(TX_START):
                DEBUG("[TX_START]\n");

                /* TODO: create data packet, set length */
                length = MTU_CC110X;

                tx_buffer[0] = MTU_CC110X;

                for (int i = 1; i < MTU_CC110X; i++) {
                    tx_buffer[i] = i;
                }

                if (length < FIFO_SIZE) {
                    cc110x_strobe(CC1100_SIDLE);
                    cc110x_strobe(CC1100_SFTX);
                    cc110x_writeburst_reg(CC1100_TXFIFO,(char*) tx_buffer, 64); 
                    cc110x_strobe(CC1100_STX);

                    cc110x_gdo2_disable();
                } else {
                    cc110x_writeburst_reg(CC1100_TXFIFO, (char*) tx_buffer,
                                                                    FIFO_SIZE);
                    cc110x_strobe(CC1100_STX);
                    tx_data.bytes_left = length + 1 - FIFO_SIZE;
                    tx_data.tx_buffer_position = tx_buffer + FIFO_SIZE;
                    tx_data.iterations = (tx_data.bytes_left / BYTES_IN_TX_FIFO);

                    if (!(tx_data.iterations)) {
                        tx_data.write_reamining_data_flag = 1;
                    }
                    
                    cpsr = disableIRQ();
                    cc110x_enable_interrupt(GDO2, LOW);
                    restoreIRQ(cpsr);
                }

                state = TX_WAIT;

                break;
            case(TX_WAIT):
                if (tx_data.packet_sent_flag) {
                    DEBUG("[TX_WAIT] packet sent\n");
                    tx_data.packet_sent_flag = 0;
                    tx_data.packets_sent = 0;

                    state = SWITCH_TX_TO_RX;
                } 
                break;
            case(SWITCH_TX_TO_RX):
                radio_mode = RX_MODE;
                
                cpsr = disableIRQ();
                cc110x_write_reg(CC1100_IOCFG2, 0x00);
                cc110x_write_reg(CC1100_IOCFG0, 0x06);
                cc110x_enable_interrupt(GDO0, HIGH);
                cc110x_enable_interrupt(GDO2, HIGH);
                cc110x_strobe(CC1100_SRX);
                restoreIRQ(cpsr);
                
                state = SETUP;

                break;

            case(RX_WAIT):
                if (rx_data.packet_received_flag) {
                    rx_data.packet_received_flag = 0;
                    rx_data.rx_buffer_position = rx_buffer;
                    
                    if (rx_data.crc_ok) {
                        DEBUG("[RX_WAIT] packet received\n");
                        for (int i=0; i<258; ++i) {
                            printf("i= %d, %d\n",i, rx_buffer[i]);
                        }
                    }

                    //state = RX_START;
                    state = SETUP;
                    
                    cpsr = disableIRQ();
                    cc110x_enable_interrupt(GDO2, HIGH);
                    cc110x_enable_interrupt(GDO0, HIGH);
                    cc110x_strobe(CC1100_SRX);
                    restoreIRQ(cpsr);
                }
                break;
            case(RX_INVALID):
                cpsr = disableIRQ();

                rx_data.rx_buffer_position = rx_buffer;
                rx_data.sync_or_end_of_packet_flag = SYNC;
                cc110x_strobe(CC1100_SIDLE);
                cc110x_strobe(CC1100_SFRX);
                
                cc110x_write_reg(CC1100_IOCFG2, 0x00);
                cc110x_write_reg(CC1100_IOCFG0, 0x06);

                cc110x_enable_interrupt(GDO0, HIGH);
                cc110x_enable_interrupt(GDO2, HIGH);
                cc110x_strobe(CC1100_SRX);
                restoreIRQ(cpsr);
                
                state = SETUP;
                break;
            default:
                break;
        }
    }
}

static int rd_set_mode(int mode) {
	int result;

	// Get current radio mode
	if ((radio_state == RADIO_UNKNOWN) || (radio_state == RADIO_PWD)) {
		result = RADIO_MODE_OFF;
	}
	else {
		result = RADIO_MODE_ON;
	}

	switch (mode) {
		case RADIO_MODE_ON:
			//cc110x_init_interrupts();			// Enable interrupts
			//cc110x_setup_rx_mode();				// Set chip to desired mode
            
            cc110x_pid = thread_create(cc110x_handler_stack,
                                       sizeof(cc110x_handler_stack),
                                       PRIORITY_CC110X,
                                       CREATE_STACKTEST,
                                       cc110x_state_machine,
                                       "cc110x_event_handler");

            msg_t m;
            m.type = (uint16_t) INIT_MODE;
            msg_send(&m, cc110x_pid, 1);


            break;
		case RADIO_MODE_OFF:
			cc110x_disable_interrupts();		// Disable interrupts
			cc110x_switch_to_pwd();					// Set chip to power down mode
			break;
		case RADIO_MODE_GET:
			// do nothing, just return current mode
		default:
			// do nothing
			break;
	}

	// Return previous mode
	return result;
}


