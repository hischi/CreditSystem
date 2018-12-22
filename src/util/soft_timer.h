#pragma once

#include <Arduino.h>

class cSoftTimer {

public:
cSoftTimer() : m_Started(false) {}

void Start(uint32_t delay) {
    m_EndTime = millis();
    if(m_EndTime > (0xFFFFFFFF - delay)) {
        m_Offset = m_EndTime;
        m_EndTime = delay;
    }
    else {
        m_Offset = 0;
        m_EndTime += delay;
    }

    m_Started = true;
}

void Stop() {
    m_Started = false;
}

bool IsStarted() {
    return m_Started;
}

bool IsOver() {
    if(m_Started) {
        if(m_EndTime < (millis() - m_Offset)) {
            m_Started = false;
            return true;
        }
    }
    return false;
}

private:
bool     m_Started;
uint32_t m_EndTime;
uint32_t m_Offset;

};