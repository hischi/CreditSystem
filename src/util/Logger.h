#pragma once

#include "Std_Types.h"

class LoggerInterface; // Prototype declaration

class Logger {

public:
    enum LogLevel {
        LL_FATAL = 0,
        LL_ERROR = 1,
        LL_WARNING = 2,
        LL_INFO = 3,
        LL_DEBUG = 4
    };

    static const uint32 LogMaxSize = 65536;
    static const LogLevel DefaultLogLevel = LL_INFO;

public:
    ~Logger() { }

    static Logger& Get() {
        static Logger logger;
        return logger;
    }

    void Log(const LoggerInterface* intf, const char* msg, LogLevel level = LL_INFO);
    void Append(const char* msg, LogLevel level = LL_INFO);
    void NewlineAppend(const char* msg, LogLevel level = LL_INFO);

    void SetLogLevel(LogLevel logLevel) {
        this->logLevel = logLevel;
    }

    LogLevel GetLogLevel() const {
        return logLevel;
    }

    bool TestLogLevel(LogLevel localLogLevel) const {
        return (localLogLevel <= logLevel);
    }

private:
    Logger() : logLevel(DefaultLogLevel), logSize(0) { }
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    const char* LogLevelToString(LogLevel logLevel) const {
        switch(logLevel) {
        case LL_FATAL:
            return "[FATAL] ";
        case LL_ERROR:
            return "[ERROR] ";
        case LL_WARNING:
            return "[WARN]  ";
        case LL_INFO:
            return "[INFO]  ";
        case LL_DEBUG:
            return "[DEBUG] ";
        default:
            return "[UNKWN] ";
        }
    }

    void LogHeader(const LoggerInterface* intf, LogLevel level);
    void Log(const char* msg);

private:
    LogLevel logLevel;
    uint32 logSize;
    char log[LogMaxSize];
};


class LoggerInterface {

public:
    LoggerInterface(const char* name);

    virtual void Log(const char* msg, Logger::LogLevel logLevel = Logger::LL_INFO) {
        Logger::Get().Log(this, msg, logLevel);
    }

    virtual void Append(const char* msg, Logger::LogLevel logLevel = Logger::LL_INFO) {
        Logger::Get().Append(msg, logLevel);
    }

    void Log(const char* msg, const char* str, Logger::LogLevel logLevel = Logger::LL_INFO) {
        Logger::Get().Log(this, msg, logLevel);
        Logger::Get().Append(str, logLevel);
    }

    void Log(const char* msg, uint32 i, Logger::LogLevel logLevel = Logger::LL_INFO) {
        Logger::Get().Log(this, msg, logLevel);
        char str[16];
        snprintf(str, 16, "%lu", i);
        Logger::Get().Append(str, logLevel);
    }

    void Log(const char* msg, int32 i, Logger::LogLevel logLevel = Logger::LL_INFO) {
        Logger::Get().Log(this, msg, logLevel);
        char str[16];
        snprintf(str, 16, "%ld", i);
        Logger::Get().Append(str, logLevel);
    }

    void Log(const char* msg, int32 i, const char* str, Logger::LogLevel logLevel = Logger::LL_INFO) {
        Logger::Get().Log(this, msg, logLevel);
        char str2[16];
        snprintf(str2, 16, "%ld", i);
        Logger::Get().Append(str2, logLevel);
        Logger::Get().Append(str, logLevel);
    }

    void Log(const char* msg, uint32 i, const char* str, Logger::LogLevel logLevel = Logger::LL_INFO) {
        Logger::Get().Log(this, msg, logLevel);
        char str2[16];
        snprintf(str2, 16, "%ld", i);
        Logger::Get().Append(str2, logLevel);
        Logger::Get().Append(str, logLevel);
    }

    void LogHexDump(const char* msg, const uint8* dump, uint32 len, Logger::LogLevel logLevel = Logger::LL_INFO) {
        Logger::Get().Log(this, msg, logLevel);
        char str[16];
        for(uint32 i = 0; i < len; i++) {
            snprintf(str, 16, "%0hhX ", dump[i]);
            Logger::Get().Append(str, logLevel);
        }
    }

public:
    char name[16];
};