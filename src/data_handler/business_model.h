#pragma once

#include <Arduino.h>

class cAccount {

public:
bool LoadFromSD(uint8_t sdCard);

uint32_t m_ID;
};

class cMember {

public:
bool LoadFromSD(uint8_t sdCard);

uint32_t m_ID;
char m_Surname[128];
char m_Name[128];
cAccount m_Account;
uint8_t m_Limits[16];
uint8_t m_LimitsCount;
uint8_t m_Discount;
uint32_t m_CardIDs[4];
uint32_t m_CardCount;
};