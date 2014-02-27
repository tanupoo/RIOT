
#include <stdio.h>

#include "vtimer.h"

#include "rgbled.h"

#define FREQU           (500U)
#define RESOLUTION      (255U)


void rgbled_init(rgbled_t *led, pwm_t pwm, int channel_r, int channel_g, int channel_b)
{
    led->device = pwm;
    led->channel_r = channel_r;
    led->channel_g = channel_g;
    led->channel_b = channel_b;
    pwm_init(pwm, PWM_LEFT, FREQU, RESOLUTION);

    // blink each color once
    timex_t delay = timex_set(1, 0);
    rgb_t col = {240, 0, 0};
    rgbled_set(led, &col);
    vtimer_sleep(delay);
    col.r = 0;
    col.g = 240;
    rgbled_set(led, &col);
    vtimer_sleep(delay);
    col.g = 0;
    col.b = 240;
    rgbled_set(led, &col);
    vtimer_sleep(delay);
    col.b = 0;
    rgbled_set(led, &col);
}

void rgbled_set(rgbled_t *led, rgb_t *color)
{
    pwm_set(led->device, led->channel_r, (unsigned int)color->r);
    pwm_set(led->device, led->channel_g, (unsigned int)color->g);
    pwm_set(led->device, led->channel_b, (unsigned int)color->b);
}