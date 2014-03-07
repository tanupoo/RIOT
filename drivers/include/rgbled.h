


#ifndef __RGBLED_H
#define __RGBLED_H

#include <stdint.h>

#include "periph/pwm.h"


typedef struct {
    int channel_r;
    int channel_g;
    int channel_b;
    pwm_t device;
} rgbled_t;


typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_t;


void rgbled_init(rgbled_t *led, pwm_t pwm, int channel_r, int channel_g, int channel_b);

void rgbled_set(rgbled_t *led, rgb_t *color);


#endif /* __RGBLED_H */