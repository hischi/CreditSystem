#pragma once

#include <chrono>
#include "../util/Std_Types.h"
#include "../util/Logger.h"
#include "RTClib.h"

class Clock;
using TimePoint = std::chrono::time_point<Clock, std::chrono::microseconds>;

class Clock : public LoggerInterface {

public:
    static const StdErrorCode HwNotReady = 0x02;
    static const StdErrorCode ClockNotReady = 0x03;

public:
    static Clock& Get() {
        static Clock clock;
        return clock;
    }

    ~Clock() { }

    StdNoReturn Init();
    bool IsReady() const { return ready; }

    StdReturn<TimePoint> Now();
    StdNoReturn Adjust(const TimePoint &timePoint);
    
private:
    Clock() : LoggerInterface("Clock"), ready(false), inSync(false) { };
    Clock(const Clock&) = delete;
    Clock& operator=(const Clock&) = delete;

    StdNoReturn Sync();

    bool ready;
    bool inSync;
    TimePoint lastSyncPoint;
    uint32  lastSyncCounter;

    RTC_PCF8523 rtcClock;
};

