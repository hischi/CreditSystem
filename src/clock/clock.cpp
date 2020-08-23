#include "clock.h"

StdNoReturn Sync() {
    TimePoint new_reference = rtcClock.now();
    
    if(new_reference.day() > 31 || new_reference.month() > 12 || new_reference.hour() > 24) {
        log(LL_ERROR, LM_CLOCK, "RTClock delivered invalid timestamp");
        return;
    }

    uint32_t new_local = micros();

    if(new_reference.second() != last_reference.second()) {
        last_local = new_local;
        last_reference = new_reference;
    }
    
    referenced = true;
}

StdNoReturn Clock::Init() {
    ready = false;

    inSync = false;
    lastSyncCounter = 0;
    lastSyncPoint = TimePoint();

    rtcClock.begin();
    if(!rtcClock.initialized()) {
        Log("RTC Clock lost its power, unknown absolute time", Logger::LL_WARNING);
    }

    StdNoReturn ret = Sync();
    if(!ret.IsError()) {
        ready = true;
    }
    return ret;
}

bool clock_was_init() {
    return initialised;
}

DateTime clock_now() {
    uint32_t corrected_micros = micros();
    
    if(last_local > corrected_micros)
        corrected_micros = (0xFFFFFFFF - last_local) + corrected_micros;
    else
        corrected_micros = corrected_micros - last_local;

    if(corrected_micros > 1000000) {
        last_reference.SetMicros(999999);
        clock_reference();
        return clock_now();
    } else {
        last_reference.SetMicros(corrected_micros);
        return last_reference;
    }
}

void clock_adjust(DateTime &new_datetime) {
    log(LL_DEBUG, LM_CLOCK, "clock_adjust");

    rtc_clock.adjust(new_datetime);
}