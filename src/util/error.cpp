#include "error.h"
#include "../clock/clock.h"
#include "../data_handler/data_handler.h"
#include "../periphery/periphery.h"

// Logger:
#define LOG_BUFFER_SIZE     65536
#define LOG_STORE_THRESHOLD 52480
char logBuffer[LOG_BUFFER_SIZE];
uint32_t logLength;
bool logStoreTrigger;
#define LOG_STORE_TRIGGER_DIP 0x08
bool logPrepared;

void LogWrite(const char *str) {
    Serial.write(str);
    int32_t n = snprintf(logBuffer+logLength, LOG_BUFFER_SIZE-logLength, str);
    if(n > 0)
        logLength += n;
}

void LogWriteHex(uint8_t i) {
    char str[8];
    sprintf(str, "%02hhX", i);
    LogWrite(str);
}

void LogWriteUInt(uint32_t i) {
    char str[8];
    sprintf(str, "%lu", i);
    LogWrite(str);
}

void err_log_reset() {
    logBuffer[0] = 0;
    logLength = 0;
    logStoreTrigger = peri_check_dip(LOG_STORE_TRIGGER_DIP);
}

void err_log_can_store() {
    if((logLength >= LOG_STORE_THRESHOLD) || (logStoreTrigger != peri_check_dip(LOG_STORE_TRIGGER_DIP))) {

        if(!logPrepared) {
            log(LL_INFO, LM_MAIN, "Prepare the log-dir for first log save");
            logPrepared = dh_prepare_log();
        }

        if(logPrepared) {
            dh_store_log(logBuffer, logLength);
            err_log_reset();
        }
    }
}

// Error:
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
        case LM_TSERV:
            return "TimeServ";
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
    err_log_reset();

    setLogLevel(LL_DEBUG);
    assertCount = 0;    

    logPrepared = false;
}

void log_header(eLogLevel level, const char module[]) {

    char str[32];
    if(clock_was_init()) {
        // Get current time
        DateTime time = clock_now();

        // Format time string and write it
        sprintf(str, "%02hhu.%02hhu.%02hu %02hhu:%02hhu:%02hhu,%03hd ", time.day(), time.month(), time.year(), time.hour(), time.minute(), time.second(), (int) time.millis());
    } else {
        sprintf(str, "%21u ", (unsigned int) millis());
    }
    LogWrite(str);

    // Write Log-Level
    switch(level) {
        case LL_FATAL:
            LogWrite("[FATAL]   "); 
            break;
        case LL_ERROR:
            LogWrite("[ERROR]   ");
            break;
        case LL_WARNING:
            LogWrite("[WARN]    ");
            break;
        case LL_INFO:
            LogWrite("[INFO]    ");
            break;
        case LL_DEBUG:
            LogWrite("[DEBUG]   ");
            break;
        case LL_VERBOSE:
            LogWrite("[VERBOSE] ");
            break;
        default:
            LogWrite("[UNKNOWN] ");
    }

    // Format and write Module
    snprintf(str, 11, "%-10s ", module);
    LogWrite(str);
}

void log(eLogLevel level, const char module[], const char msg[]) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", module);

    if(level <= logLevel[0]) {// only write if log level is ok
        log_header(level, module);
        // Write Msg
        LogWrite(msg);
        LogWrite("\n");
    }
}

void log(eLogLevel level, const char module[], const char msg[], uint32_t value) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", module);

    if(level <= logLevel[0]) {// only write if log level is ok
        log_header(level, module);
        // Write Msg
        LogWrite(msg);
        LogWrite(" ");
        // Write value
        LogWriteUInt(value);
        LogWrite("\n");
    }
}

void log(eLogLevel level, const char module[], const char msg[], float value) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", module);

    if(level <= logLevel[0]) {// only write if log level is ok
        log_header(level, module);
        // Write Msg
        LogWrite(msg);
        LogWrite(" ");
        // Write value
        LogWriteUInt(value);
        LogWrite("\n");
    }
}

void log(eLogLevel level, const char module[], const char msg[], const char str[]) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", module);

    if(level <= logLevel[0]) {// only write if log level is ok
        log_header(level, module);
        // Write Msg
        LogWrite(msg);
        LogWrite(" ");
        // Write value
        LogWrite(str);
        LogWrite("\n");
    }
}

void log(eLogLevel level, const char module[], const char msg[], const cPrice &price) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", module);

    if(level <= logLevel[0]) {// only write if log level is ok
        log_header(level, module);
        // Write Msg
        LogWrite(msg);
        LogWrite(" ");
        // Write value
        LogWriteUInt(price.GetEuros());
        LogWrite(",");
        LogWriteUInt(price.GetCents());
        LogWrite(" EUR\n");
    }
}

void log_hexdump(eLogLevel level, const char module[], const char msg[], uint16_t len, const uint8_t data[]) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", module);

    if(level <= logLevel[0]) {// only write if log level is ok
        log_header(level, module);
        // Write Msg
        LogWrite(msg);
        LogWrite(" ");
        // Write hex values
        for(uint16_t i = 0; i < len; i++) {
            LogWriteHex(data[i]);
            LogWrite(" ");
        }
        LogWrite("\n");
    }
}

void log(eLogLevel level, eLogModule module, const char msg[]) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", getModuleName(module));

    if(level <= logLevel[module]) {// only write if log level is ok
        log_header(level, getModuleName(module));
        // Write Msg
        LogWrite(msg);
        LogWrite("\n");
    }
}

void log(eLogLevel level, eLogModule module, const char msg[], uint32_t value) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", getModuleName(module));

    if(level <= logLevel[module]) {// only write if log level is ok
        log_header(level, getModuleName(module));
        // Write Msg
        LogWrite(msg);
        LogWrite(" ");
        // Write value
        LogWriteUInt(value);
        LogWrite("\n");
    }
}

void log(eLogLevel level, eLogModule module, const char msg[], float value) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", getModuleName(module));

    if(level <= logLevel[module]) {// only write if log level is ok
        log_header(level, getModuleName(module));
        // Write Msg
        LogWrite(msg);
        LogWrite(" ");
        // Write value
        LogWriteUInt(value);
        LogWrite("\n");
    }
}

void log(eLogLevel level, eLogModule module, const char msg[], const char str[]) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", getModuleName(module));

    if(level <= logLevel[module]) {// only write if log level is ok
        log_header(level, getModuleName(module));
        // Write Msg
        LogWrite(msg);
        LogWrite(" ");
        // Write value
        LogWrite(str);
        LogWrite("\n");
    }
}

void log(eLogLevel level, eLogModule module, const char msg[], const cPrice &price) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", getModuleName(module));

    if(level <= logLevel[module]) {// only write if log level is ok
        log_header(level, getModuleName(module));
        // Write Msg
        LogWrite(msg);
        LogWrite(" ");
        // Write value
        LogWriteUInt(price.GetEuros());
        LogWrite(",");
        LogWriteUInt(price.GetCents());
        LogWrite(" EUR\n");
    }
}

void log(eLogLevel level, eLogModule module, const char msg[], const DateTime &datetime) {
        // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", getModuleName(module));

    if(level <= logLevel[module]) {// only write if log level is ok
        log_header(level, getModuleName(module));
        // Write Msg
        LogWrite(msg);
        LogWrite(" ");

        // Write value
        char datetime_str[20];
        sprintf(datetime_str, "%02hu.%02hu.%04u %02hu:%02hu:%02hu", datetime.day(), datetime.month(), datetime.year(), datetime.hour(), datetime.minute(), datetime.second());
        LogWrite(datetime_str);
        LogWrite("\n");
    }
}

void log_hexdump(eLogLevel level, eLogModule module, const char msg[], uint16_t len, const uint8_t data[]) {
    // Check args
    assertRtn(level > LL_VERBOSE, LL_WARNING, "InvalidLL", getModuleName(module));

    if(level <= logLevel[module]) {// only write if log level is ok
        log_header(level, getModuleName(module));
        // Write Msg
        LogWrite(msg);
        LogWrite(" ");
        // Write hex values
        for(uint16_t i = 0; i < len; i++) {
            LogWriteHex(data[i]);
            LogWrite(" ");
        }
        LogWrite("\n");
    }
}