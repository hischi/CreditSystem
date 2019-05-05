#pragma once

#include <Arduino.h>
#include "../util/price.h"

void fh_init();

bool fh_fopen(uint8_t card, const char path[]);
void fh_fclose();

bool fh_fs_ready(uint8_t card);

int32_t fh_fread(uint32_t pos, uint16_t len, uint8_t *buf);
int32_t fh_fwrite(uint32_t pos, uint16_t len, uint8_t *buf);
int32_t fh_fappend(uint16_t len, uint8_t *buf);

int32_t fh_flen();
void fh_clear();

void fh_flog(uint32_t pos);
