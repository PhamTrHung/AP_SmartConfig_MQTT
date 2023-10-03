#ifndef LEDC_MODE_H
#define LEDC_MODE_H


void ledc_config(int GPIOnum, int channel);
void ledc_create_duty(int channel, int dutyValue);

#endif
