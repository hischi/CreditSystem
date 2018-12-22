#pragma once

#include <Arduino.h>
#include "../util/price.h"

void fh_init();

bool fh_fopen(uint8_t card, const char path[]);
void fh_fclose();

int32_t csv_read_string(uint32_t col, uint32_t row, uint32_t maxLength, char str[]);
void csv_write_string(uint32_t col, uint32_t row, uint32_t length, char str[]);

uint32_t csv_read_uint32(uint32_t col, uint32_t row);
void csv_write_uint32(uint32_t col, uint32_t row, uint32_t value);

float csv_read_float32(uint32_t col, uint32_t row);
void csv_write_float32(uint32_t col, uint32_t row, float value);

cPrice csv_read_price(uint32_t col, uint32_t row);
void csv_write_price(uint32_t col, uint32_t row, const cPrice &price);
