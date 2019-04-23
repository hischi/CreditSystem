#ifndef _PERIPHERY_H_
#define _PERIPHERY_H_

#include <Arduino.h>

void peri_init();

void peri_set_led(uint8_t led_id, bool led_on);

bool peri_check_dip(uint8_t mask);

#endif //_PERIPHERY_H_