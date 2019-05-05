#include "clock.h"
#include "RTClib.h"
#include "../util/error.h"

bool initialised = false;
bool referenced;
DateTime last_reference;
uint32_t  last_local;

RTC_PCF8523 rtc_clock;

void clock_reference() {
    DateTime new_reference = rtc_clock.now();
    
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

void clock_init() {
    initialised = false;

    log(LL_DEBUG, LM_CLOCK, "clock_init");

    referenced = false;
    last_local = 0;
    last_reference = DateTime();

    rtc_clock.begin();

    assertCnt(!rtc_clock.initialized(), LL_ERROR, LM_CLOCK, "RTC Clock lost its power, unknown absolute time");

    clock_reference();
    initialised = true;
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