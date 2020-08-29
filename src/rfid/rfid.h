#pragma once

#include <Arduino.h>

void rfid_init(bool autoLog);

uint32_t rfid_member_present();
void rfid_member_reset();

void rfid_low_power_mode();

bool rfid_read_tennis_app(uint8_t tennisCardID[], uint8_t tennisCustomerID[]);
bool rfid_store_tennis_app(uint8_t tennisCardID[], uint8_t tennisCustomerID[]);
bool rfid_restore_card();
void rfid_restore_card_async(void (*restore_done)());
bool rfid_set_PICC();

void rfid_run();

void rfid_program_card(uint32_t membId, uint32_t cardId);
void rfid_program_card_async(uint32_t membId, uint32_t cardId, void (*prog_done)());