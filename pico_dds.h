#ifndef PICO_DDS_H
#define PICO_DDS_H

#include <Arduino.h>

void dds_begin();
void dds_down();
void dds_setfreq(int freq);
void dds_pwm_interrupt_handler();

#endif
