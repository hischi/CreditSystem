#pragma once

#include <Arduino.h>
#include "../util/time_format.h"

#define DH_MEMBERPROP_NOT_ITEM0 0x00000001
#define DH_MEMBERPROP_NOT_ITEM1 0x00000002
#define DH_MEMBERPROP_NOT_ITEM2 0x00000004
#define DH_MEMBERPROP_NOT_ITEM3 0x00000008
#define DH_MEMBERPROP_NOT_ITEM4 0x00000010
#define DH_MEMBERPROP_NOT_ITEM5 0x00000020
#define DH_MEMBERPROP_NOT_ITEM6 0x00000040
#define DH_MEMBERPROP_NOT_ITEM7 0x00000080

#define DH_MEMBERPROP_CLUBCARD  0x00000100
#define DH_MEMBERPROP_TEAMCARD  0x00000200


struct sMember {
    uint32_t id;
    char surname[128];
    char name[128];
    uint32_t account;
    uint32_t properties;
    uint32_t discount;
    uint32_t cardIDs[4];
    uint32_t cardCount;
};

void dh_init();

bool dh_get_member(uint32_t memberID, sMember *member);

//bool dh_mark_vend(sMember &member, )

