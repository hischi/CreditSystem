#include "error.h"
#include "../clock/clock.h"


eLogLevel logLevel[32];
uint8_t logLevelCount = 32;
uint32_t assertCount;

const char* getModuleName(eLogModule module) {
    switch(module) {
        case LM_MAIN:
            return "Main"; 
        case LM_CLDEV:
            return "ClDev"; 
        case LM_CLOCK:
            return "Clock"; 
        case LM_EH:
            return "ErrHandle"; 
        case LM_FH:
            return "FileHandl"; 
        case LM_MDB:
            return "MDB"; 
        case LM_RFIDBUF:
            return "RFIDBuf"; 
        case LM_RFIDCLASSIC:
            return "RFIDClass"; 
        case LM_DESFIRE:
            return "Desfire";
        case LM_DESKEY:
            return "DesKey"; 
        case LM_PN532:
            return "PN532"; 
        case LM_RFID:
            return "RFID"; 
        case LM_DH:
            return "DataHandl";
        case LM_BM:
            return "BusiModel";
        case LM_CS:
            return "CheckSum";
        case LM_PERI:
            return "Periphery";
        case LM_SERV:
            return "Service";
        default:
            return "UNKNOWN"; 
    }
}

void setLogLevel(eLogLevel level) {
    for(uint8_t i = 0; i < logLevelCount; i++)
        logLevel[i] = level;
}

void setLogLevel(eLogModule module, eLogLevel level) {
    logLevel[module] = level;
}

eLogLevel getLogLevel(eLogModule module) {
    return logLevel[module];
}

eLogLevel getLogLevel() {
    return logLevel[0];
}

bool checkLogLevel(eLogModule module, eLogLevel level) {
    return (level <= logLevel[module]);
}

uint32_t getAssertCount() {
    return assertCount;
}

void assertInc() {
    assertCount++;
}

void err_init() {
    setLogLevel(LL_DEBUG);
    assertCount = 0;    
}

void log_header(eLogLevel level, const char module[]) {

    char str[32];
    if(clock_was_init()) {
        // Get current time
        DateTime time = clock_now();

        // Format time string and write it
        sprintf(str, "%02hhu:%02hhu:%02hhu,%03hd ", time.hour(), time.minute(), time.second(), (int) time.millis());
    } else {
        sprintf(str, "%12u ", (unsigned int) millis());
    }
    Serial.write(str);

    // Write Log-Level
    switch(level) {
        case LL_FATAL:
            Serial.write("[FATAL]   "); 
            break;
        case LL_ERROR:
            Serial.write("[ERROR]   ");
            break;
        case LL_WARNING:
            Serial.write("[WARN]    ");
            break;
        case LL_INFO:
            Serial.write("[INFO]    ");
            break;
        case LL_DEBUG:
            Serial.write("[DEBUG]   ");
            break;
        case LL_VERBOSE:
            Serial.write("[VERBOSE] ");
            break;
        default:
            Serial.write("[UNKNOWN] ");
    }

    // Format and write Module
    snprintf(str, 11, "%-10s ", module);
    Serial.write(str);
}

void log(eLogLevel level, const char module[], const char msg[]) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", module);

    if(level <= logLevel[0]) {// only write if log level is ok
        log_header(level, module);
        // Write Msg
        Serial.write(msg);
        Serial.write('\n');
        Serial.flush();
    }
}

void log(eLogLevel level, const char module[], const char msg[], uint32_t value) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", module);

    if(level <= logLevel[0]) {// only write if log level is ok
        log_header(level, module);
        // Write Msg
        Serial.write(msg);
        Serial.print(" ");
        // Write value
        Serial.print(value);
        Serial.write('\n');
        Serial.flush();
    }
}

void log(eLogLevel level, const char module[], const char msg[], float value) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", module);

    if(level <= logLevel[0]) {// only write if log level is ok
        log_header(level, module);
        // Write Msg
        Serial.write(msg);
        Serial.print(" ");
        // Write value
        Serial.print(value);
        Serial.write('\n');
        Serial.flush();
    }
}

void log(eLogLevel level, const char module[], const char msg[], const char str[]) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", module);

    if(level <= logLevel[0]) {// only write if log level is ok
        log_header(level, module);
        // Write Msg
        Serial.write(msg);
        Serial.print(" ");
        // Write value
        Serial.write(str);
        Serial.write('\n');
        Serial.flush();
    }
}

void log(eLogLevel level, const char module[], const char msg[], const cPrice &price) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", module);

    if(level <= logLevel[0]) {// only write if log level is ok
        log_header(level, module);
        // Write Msg
        Serial.write(msg);
        Serial.print(" ");
        // Write value
        Serial.print(price.GetEuros());
        Serial.write(",");
        Serial.print(price.GetCents());
        Serial.write(" EUR\n");
        Serial.flush();
    }
}

void log_hexdump(eLogLevel level, const char module[], const char msg[], uint16_t len, const uint8_t data[]) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", module);

    if(level <= logLevel[0]) {// only write if log level is ok
        log_header(level, module);
        // Write Msg
        Serial.write(msg);
        Serial.print(' ');
        // Write hex values
        for(uint16_t i = 0; i < len; i++) {
            Serial.print(data[i], HEX);
            Serial.print(' ');
        }
        Serial.write('\n');
        Serial.flush();
    }
}

void log(eLogLevel level, eLogModule module, const char msg[]) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", getModuleName(module));

    if(level <= logLevel[module]) {// only write if log level is ok
        log_header(level, getModuleName(module));
        // Write Msg
        Serial.write(msg);
        Serial.write('\n');
        Serial.flush();
    }
}

void log(eLogLevel level, eLogModule module, const char msg[], uint32_t value) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", getModuleName(module));

    if(level <= logLevel[module]) {// only write if log level is ok
        log_header(level, getModuleName(module));
        // Write Msg
        Serial.write(msg);
        Serial.print(" ");
        // Write value
        Serial.print(value);
        Serial.write('\n');
        Serial.flush();
    }
}

void log(eLogLevel level, eLogModule module, const char msg[], float value) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", getModuleName(module));

    if(level <= logLevel[module]) {// only write if log level is ok
        log_header(level, getModuleName(module));
        // Write Msg
        Serial.write(msg);
        Serial.print(" ");
        // Write value
        Serial.print(value);
        Serial.write('\n');
        Serial.flush();
    }
}

void log(eLogLevel level, eLogModule module, const char msg[], const char str[]) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", getModuleName(module));

    if(level <= logLevel[module]) {// only write if log level is ok
        log_header(level, getModuleName(module));
        // Write Msg
        Serial.write(msg);
        Serial.print(" ");
        // Write value
        Serial.write(str);
        Serial.write('\n');
        Serial.flush();
    }
}

void log(eLogLevel level, eLogModule module, const char msg[], const cPrice &price) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", getModuleName(module));

    if(level <= logLevel[module]) {// only write if log level is ok
        log_header(level, getModuleName(module));
        // Write Msg
        Serial.write(msg);
        Serial.print(" ");
        // Write value
        Serial.print(price.GetEuros());
        Serial.write(",");
        Serial.print(price.GetCents());
        Serial.write(" EUR\n");
        Serial.flush();
    }
}

void log(eLogLevel level, eLogModule module, const char msg[], const DateTime &datetime) {
        // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", getModuleName(module));

    if(level <= logLevel[module]) {// only write if log level is ok
        log_header(level, getModuleName(module));
        // Write Msg
        Serial.write(msg);
        Serial.print(" ");

        // Write value
        char datetime_str[20];
        sprintf(datetime_str, "%02hu.%02hu.%04u %02hu:%02hu:%02hu", datetime.day(), datetime.month(), datetime.year(), datetime.hour(), datetime.minute(), datetime.second());
        Serial.write(datetime_str);
        Serial.write("\n");
        Serial.flush();
    }
}

void log_hexdump(eLogLevel level, eLogModule module, const char msg[], uint16_t len, const uint8_t data[]) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", getModuleName(module));

    if(level <= logLevel[module]) {// only write if log level is ok
        log_header(level, getModuleName(module));
        // Write Msg
        Serial.write(msg);
        Serial.print(' ');
        // Write hex values
        for(uint16_t i = 0; i < len; i++) {
            Serial.print(data[i], HEX);
            Serial.print(' ');
        }
        Serial.write('\n');
        Serial.flush();
    }
}