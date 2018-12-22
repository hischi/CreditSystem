#pragma once

#include <Arduino.h>
#include "error.h"

class cPrice {

public:
// Default Constructor
cPrice() : m_Value(0), m_Valid(true) {};

// Copy-Constructor
cPrice(const cPrice &rhs) {
    m_Value = rhs.m_Value;
    m_Valid = rhs.m_Valid;
}

// Desctructor
virtual ~cPrice() {}

// Assignment
void operator=(const cPrice& rhs) {
    m_Value = rhs.m_Value;
    m_Valid = rhs.m_Valid;
}

cPrice operator+(const cPrice& rhs) {
    cPrice result;
    result.m_Value = m_Value + rhs.m_Value;
    return result;
}

cPrice operator-(const cPrice& rhs) {
    cPrice result;
    result.m_Value = m_Value - rhs.m_Value;
    return result;
}

void Set(bool negative, uint16_t euros, uint8_t cents) {
    m_Value = euros * 100 + cents;

    if(negative)
        m_Value = -m_Value;
}

void SetAsCents(bool negative, uint32_t cents) {
    m_Value = cents;

    if(negative)
        m_Value = -m_Value;
}

uint8_t GetCents() const {
    return m_Value % 100;
}

uint32_t GetAsCents() const {
    return m_Value;
}

uint16_t GetEuros() const {
    if(m_Value < 0) {
        return (-m_Value) / 100;
    } else
        return m_Value / 100;
}

bool IsNegative() {
    return (m_Value < 0);
}

bool IsValid() {
    return m_Valid;
}

void SetValid(bool valid) {
    m_Valid = valid;
}

private:
int32_t m_Value;
bool m_Valid;
};

cPrice read_price_uint16(const uint8_t data[]);

void write_price_uint16(const cPrice &price, uint8_t data[]);

uint8_t price_scale();

uint8_t price_dec_places();