#include "Logger.h"
#include "../clock/clock.h"

void Logger::Log(const LoggerInterface* intf, const char* msg, LogLevel level) {
    if(TestLogLevel(level)){
        LogHeader(intf, level);
        Log(msg);
    }
}

void Logger::Append(const char* msg, LogLevel level) {
    if(TestLogLevel(level)){
        Log(msg);
    }
}

void Logger::NewlineAppend(const char* msg, LogLevel level) {
    if(TestLogLevel(level)){
        Log("\n                                              "); // Skip what would be taken by the header instead
        Log(msg);
    }
}

void Logger::LogHeader(const LoggerInterface* intf, LogLevel level) {
    // Print time
    char str[32];
    if(clock_was_init()) {
        // Get current time
        DateTime time = clock_now();

        // Format time string and write it
        sprintf(str, "\n%04hu/%02hu/%02hu %02hhu:%02hhu:%02hhu,%03hd ", time.year(), time.month(), time.day(), time.hour(), time.minute(), time.second(), (int) time.millis());
    } else {
        sprintf(str, "\n%23u ", (unsigned int) millis());
    }
    Log(str);

    // Print log level
    Log(LogLevelToString(level));

    // Print intf
    Log(intf->name);
}

void Logger::Log(const char* msg) {
    Serial.write(msg);
    int32_t n = snprintf(&log[logSize], LogMaxSize-logSize, msg);
    if(n > 0)
        logSize += n;
}

LoggerInterface::LoggerInterface(const char* name) {
    uint16 len = strlen(name);
    if(len > 12)
        len = 12;
    snprintf(this->name, 16, " [%s]%*c ", name, 11-len, ' ');
}