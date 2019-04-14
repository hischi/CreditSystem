#ifdef WIN32
#include <stdint.h>
#else
#include <Arduino.h>
#include "error.h"
#endif

uint32_t calculate_checksum(uint16_t *buf, uint32_t len);