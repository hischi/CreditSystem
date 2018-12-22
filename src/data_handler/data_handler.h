#pragma once

#include <Arduino.h>
#include "../util/time_format.h"

struct sMember {
    uint32_t iD;
    char surname[128];
    char name[128];
    uint32_t account;
    uint8_t limits[16];
    uint8_t limitsCount;
    uint8_t discount;
    uint32_t cardIDs[4];
    uint32_t cardCount;
};

struct sVend {
    uint32_t number;

};

void dh_init();

bool dh_get_member(uint32_t memberID, uint32_t cardID, sMember *member);

//bool dh_mark_vend(sMember &member, )

