#include <stdio.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include "output_iot.h"

void output_io_config(gpio_num_t GPIO_num)
{
    gpio_pad_select_gpio(GPIO_num);
    gpio_set_direction(GPIO_num, GPIO_MODE_INPUT_OUTPUT);
}

void output_io_set_level(gpio_num_t GPIO_num, uint32_t level)
{
    gpio_set_level(GPIO_num, level);
}

void output_toggle_mode(gpio_num_t GPIO_num)
{
    int old_level = gpio_get_level(GPIO_num);
    gpio_set_level(GPIO_num, 1 - old_level);
}


