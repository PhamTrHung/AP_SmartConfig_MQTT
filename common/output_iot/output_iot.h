#ifndef OUTPUT_IO_H
#define OUTPUT_IO_H
#include "esp_err.h"
#include "hal/gpio_types.h"

void output_io_config(gpio_num_t GPIO_num);
void output_io_set_level(gpio_num_t GPIO_num, uint32_t level);
void output_toggle_mode(gpio_num_t GPIO_num);

#endif
