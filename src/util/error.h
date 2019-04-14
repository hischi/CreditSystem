#pragma once

#include <Arduino.h>
#include "price.h"
#include "time_format.h"

class cPrice;

#define assertRtn(check, level, module, msg) if(check) {assertInc(); log(level, module, msg); return;}
#define assertCnt(check, level, module, msg) if(check) {assertInc(); log(level, module, msg);}
#define assertDo(check, level, module, msg, func) if(check) {assertInc(); log(level, module, msg); func;}

enum eLogLevel {
    LL_FATAL = 0,
    LL_ERROR = 1,
    LL_WARNING = 2,
    LL_INFO = 3,
    LL_DEBUG = 4,
    LL_VERBOSE = 5
};

enum eLogModule {
    LM_MAIN = 0,
    LM_CLDEV = 1,
    LM_CLOCK = 2,
    LM_EH = 3,
    LM_FH = 4,
    LM_MDB = 5,
    LM_RFIDBUF = 6,
    LM_RFIDCLASSIC = 7,
    LM_DESFIRE = 8,
    LM_DESKEY = 9,
    LM_PN532 = 10,
    LM_RFID = 11,
    LM_DH = 12,
    LM_BM = 13,
    LM_CS = 14
};

void setLogLevel(eLogLevel level);
void setLogLevel(eLogModule module, eLogLevel level);
eLogLevel getLogLevel(eLogModule module);
eLogLevel getLogLevel();
bool checkLogLevel(eLogModule module, eLogLevel level);

void err_init();

uint32_t getAssertCount();
void assertInc();

void log(eLogLevel level, const char module[], const char msg[]);
void log(eLogLevel level, const char module[], const char msg[], uint32_t value);
void log(eLogLevel level, const char module[], const char msg[], float value);
void log(eLogLevel level, const char module[], const char msg[], const char str[]);
void log(eLogLevel level, const char module[], const char msg[], const cPrice &price);
void log_hexdump(eLogLevel level, const char module[], const char msg[], uint16_t len, const uint8_t data[]);

void log(eLogLevel level, eLogModule module, const char msg[]);
void log(eLogLevel level, eLogModule module, const char msg[], uint32_t value);
void log(eLogLevel level, eLogModule module, const char msg[], float value);
void log(eLogLevel level, eLogModule module, const char msg[], const char str[]);
void log(eLogLevel level, eLogModule module, const char msg[], const cPrice &price);
void log(eLogLevel level, eLogModule module, const char msg[], const DateTime &datetime);
void log_hexdump(eLogLevel level, eLogModule module, const char msg[], uint16_t len, const uint8_t data[]);
