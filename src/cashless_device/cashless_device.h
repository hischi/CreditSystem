#pragma once

#include "../util/error.h"

void cldev_init();

void cldev_run(uint8_t cmd, const uint8_t data[]);

uint8_t cldev_cmd_len(uint8_t cmd);
uint8_t cldev_scmd_len(uint8_t cmd, uint8_t scmd);

uint8_t answer_DisplayRequest(uint8_t answer[], uint8_t time_tenths, const char msg[]);