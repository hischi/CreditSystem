#pragma once

#include "../util/error.h"

void mdb_init();

bool mdb_send_data(uint8_t len, const uint8_t data[]);

void mdb_send_ack();

void mdb_send_nack();

uint8_t mdb_read(uint8_t *cmd, uint8_t data[]);