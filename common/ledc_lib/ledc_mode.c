#include "ledc_mode.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "stdint.h"

static ledc_timer_config_t ledc_timer;
static ledc_channel_config_t ledc_channel;

void ledc_config(int GPIOnum, int channel)
{
    
    ledc_timer.duty_resolution = LEDC_TIMER_13_BIT; // resolution of PWM duty
    ledc_timer.freq_hz = 5000;                      // frequency of PWM signal
    ledc_timer.speed_mode = LEDC_HIGH_SPEED_MODE;           // timer mode
    ledc_timer.timer_num = LEDC_TIMER_0;            // timer index
    ledc_timer.clk_cfg = LEDC_AUTO_CLK;              // Auto select the source clock


    ledc_timer_config(&ledc_timer);
    

    ledc_channel.channel    = channel;
    ledc_channel.duty       = 0;
    ledc_channel.gpio_num   = GPIOnum;
    ledc_channel.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_channel.hpoint     = 0;
    ledc_channel.timer_sel  = LEDC_TIMER_0;


    // Set LED Controller with previously prepared configuration
    ledc_channel_config(&ledc_channel);

}


void ledc_create_duty(int channel, int dutyValue)
{
    ledc_set_duty(ledc_channel.speed_mode, channel, dutyValue*8192/100);
    ledc_update_duty(ledc_channel.speed_mode, channel);
}
