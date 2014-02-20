
#include <stdio.h>
#include "bitarithm.h"
#include "lpc2387.h"
#include "periph/pwm.h"
#include "periph_conf.h"


#define PCPWM1      BIT6


/**
 * @note The PWM is always initialized with left-aligned mode.
 */
int pwm_init(pwm_t dev, pwm_mode_t mode, unsigned int frequency, unsigned int resolution)
{
    switch (dev) {
        case PWM_0:
            // select function PWM[3] for pin P2.2, P2.3, P2.4
            PINSEL4 &= ~(BIT5 + BIT7 + BIT9);
            PINSEL4 |= (BIT4 + BIT6 + BIT8);

            // power on PWM1
            PCONP |= BIT6;

            // select PWM1 clock
            PCLKSEL0 &= ~(BIT13);
            PCLKSEL0 |= (BIT12);

            // reset PWM1s counter
            PWM1TCR = BIT1;

            // set Prescaler
            // PWM1PR = 71;
            PWM1PR = (F_CPU / (frequency * resolution)) - 1;

            // set match register
            PWM1MR0 = resolution;
            PWM1MR3 = 0;
            PWM1MR4 = 0;
            PWM1MR5 = 0;

            // reset timer counter on MR0 match
            PWM1MCR = BIT1;

            // enable PWM1 channel 3, 4 and 5
            PWM1PCR = (BIT11 + BIT12 + BIT13);

            // enable PWM1 timer in PWM mode
            PWM1TCR = BIT0 + BIT3;

            // update match registers
            PWM1LER = (BIT0 + BIT3 + BIT4 + BIT5);
            break;
    }
    return 0;
}

int pwm_set(pwm_t dev, int channel, unsigned int value)
{
    switch (channel) {
        case 0:
            PWM1MR3 = value;
            PWM1LER = BIT3;
            break;
        case 1:
            PWM1MR4 = value;
            PWM1LER = BIT4;
            break;
        case 2:
            PWM1MR5 = value;
            PWM1LER = BIT5;
            break;
    }
    return 0;
}

void pwm_start(pwm_t dev)
{
    PCONP |= PCPWM1;        // enable PWM1 device
}

void pwm_stop(pwm_t dev)
{
    PCONP &= ~PCPWM1;        // disable PWM1 device
}
