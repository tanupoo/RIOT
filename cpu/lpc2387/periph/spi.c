/*
 * Copyright (C) 2015 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     cpu_lpc2387
 * @{
 *
 * @file
 * @brief       Low-level SPI driver implementation
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 *
 * @}
 */

#include "cpu.h"
#include "mutex.h"
#include "periph/gpio.h"
#include "periph/spi.h"
#include "periph_conf.h"
#include "board.h"
#define ENABLE_DEBUG (0)
#include "debug.h"

/**
 * @brief Array holding one pre-initialized mutex for each SPI device
 */
static mutex_t locks[] =  {
#if SPI_0_EN
    [SPI_0] = MUTEX_INIT,
#endif
#if SPI_1_EN
    [SPI_1] = MUTEX_INIT,
#endif
#if SPI_2_EN
    [SPI_2] = MUTEX_INIT
#endif
};

int spi_init_master(spi_t dev, spi_conf_t conf, spi_speed_t speed)
{
    uint32_t   f_baud = 0;
    switch(speed)
    {
    case SPI_SPEED_100KHZ:
        f_baud = 100;
        break;
    case SPI_SPEED_400KHZ:
        f_baud = 400;
        break;
    case SPI_SPEED_1MHZ:
        f_baud = 1000;
        break;
    case SPI_SPEED_5MHZ:
        f_baud = 5000;
        break;
    case SPI_SPEED_10MHZ:
        f_baud = 10000;
        break;
    }

#if 0
    /* TODO */
    switch(conf)
    {
    case SPI_CONF_FIRST_RISING:
        /**< first data bit is transacted on the first rising SCK edge */
        cpha = 0;
        cpol = 0;
        break;
    case SPI_CONF_SECOND_RISING:
        /**< first data bit is transacted on the second rising SCK edge */
        cpha = 1;
        cpol = 0;
        break;
    case SPI_CONF_FIRST_FALLING:
        /**< first data bit is transacted on the first falling SCK edge */
        cpha = 0;
        cpol = 1;
        break;
    case SPI_CONF_SECOND_FALLING:
        /**< first data bit is transacted on the second falling SCK edge */
        cpha = 1;
        cpol = 1;
        break;
    }
#endif

    uint32_t pclksel;
    uint32_t cpsr;
    int dummy;

    switch(dev) {
#ifdef SPI_0_EN
        case SPI_0:
            /* Power*/
            PCONP |= SPI_0_POWER;

            /* PIN Setup*/
            SPI_0_CLK;
            SPI_0_MISO;
            SPI_0_MOSI;

            /* Interface Setup*/
            SPI_0_SSP = SPI_0_DSS;

            /* Clock Setup*/
            lpc2387_pclk_scale(F_CPU / 1000, f_baud, &pclksel, &cpsr);

            SPI_0_CCLK;
            SPI_0_CLK_SEL |= pclksel << SPI_0_CLK_SHIFT;
            SPI_0_CPSR = cpsr;

            /* Enable*/
            SPI_0_SSP_EN; /* SSP-Enable*/

            /* Clear RxFIFO:*/
            while (SPI_0_RX_AVAIL) {          /* while RNE (Receive FIFO Not Empty)...*/
                dummy = SPI_0_DR;             /* read data*/
            }

            break;
#endif
#ifdef SPI_1_EN
        case SPI_1:
            /* Power*/
            PCONP |= SPI_1_POWER;

            /* PIN Setup*/
            SPI_1_CLK;
            SPI_1_MISO;
            SPI_1_MOSI;

            /* Interface Setup*/
            SPI_1_SSP = SPI_1_DSS;

            /* Clock Setup*/
            lpc2387_pclk_scale(F_CPU / 1000, f_baud, &pclksel, &cpsr);

            SPI_1_CCLK;
            SPI_1_CLK_SEL |= pclksel << SPI_1_CLK_SHIFT;
            SPI_1_CPSR = cpsr;

            /* Enable*/
            SPI_1_SSP_EN; /* SSP-Enable*/

            /* Clear RxFIFO:*/
            while (SPI_1_RX_AVAIL) {          /* while RNE (Receive FIFO Not Empty)...*/
                dummy = SPI_1_DR;             /* read data*/
            }

            break;
#endif
        default:
            return -1;
    }
    /* to suppress unused-but-set-variable */
    (void) dummy;
    return 0;
}

int spi_init_slave(spi_t dev, spi_conf_t conf, char (*cb)(char))
{
    (void)dev;
    (void)conf;
    (void)cb;
    printf("%s:%s(): stub\n", RIOT_FILE_RELATIVE, __func__);
    /* TODO */
    return 0;
}

void spi_transmission_begin(spi_t dev, char reset_val)
{
    (void)dev;
    (void)reset_val;
    printf("%s:%s(): stub\n", RIOT_FILE_RELATIVE, __func__);
    /* TODO*/
}

int spi_acquire(spi_t dev)
{
    if (dev >= SPI_NUMOF) {
        return -1;
    }
    mutex_lock(&locks[dev]);
    return 0;
}

int spi_release(spi_t dev)
{
    if (dev >= SPI_NUMOF) {
        return -1;
    }
    mutex_unlock(&locks[dev]);
    return 0;
}

int spi_transfer_byte(spi_t dev, char out, char *in)
{
    char tmp = 0;
    switch(dev) {
#ifdef SPI_0_EN
        case SPI_0:
            while (!SPI_0_TX_EMPTY);
            SPI_0_DR = out;
            while (SPI_0_BUSY);
            while (!SPI_0_RX_AVAIL);

            tmp = (char) SPI_0_DR;
            break;
#endif
#ifdef SPI_1_EN
        case SPI_1:
            while (!SPI_1_TX_EMPTY);
            SPI_1_DR = out;
            while (SPI_1_BUSY);
            while (!SPI_1_RX_AVAIL);

            tmp = (char) SPI_1_DR;
            break;
#endif
    }

    if (in != NULL) {
        *in = tmp;
    }

    return 1;
}

void spi_poweron(spi_t dev)
{
}

void spi_poweroff(spi_t dev)
{
}

