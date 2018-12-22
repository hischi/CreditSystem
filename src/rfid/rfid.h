#pragma once

#include <Arduino.h>

void rfid_init();

bool rfid_card_present();

void rfid_low_power_mode();

bool rfid_read_tennis_app(uint8_t tennisCardID[], uint8_t tennisCustomerID[]);
bool rfid_store_tennis_app(uint8_t tennisCardID[], uint8_t tennisCustomerID[]);
bool rfid_restore_card();
bool rfid_set_PICC();