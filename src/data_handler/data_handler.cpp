#include "data_handler.h"
#include "../util/error.h"
#include "../file_handler/file_handler.h"

void dh_init() {
    log(LL_DEBUG, LM_DH, "dh_init");
}

bool dh_get_member(uint32_t memberID, sMember *member) {
    log(LL_DEBUG, LM_DH, "dh_get_member");

    // Open member list on sd card
    assertDo(!fh_fopen(2, "MITGLIED.CSV"), LL_ERROR, LM_DH, "Can't open list of members <MITGLIED.CSV> on sd card 1", return false;);

    // Find in column 0 the member id
    char memberIDstr[16];
    sprintf(memberIDstr, "%d", memberID);
    int32_t row = csv_findInColumn(0, memberIDstr);
    log(LL_DEBUG, LM_DH, "Member search returned with: ", (uint32_t) row);
    assertDo(row < 0, LL_ERROR, LM_DH, "Can't find member within member list", return false;);

    // Fill data from SD card into member struct
    member->id = memberID;
    assertDo(csv_read_string(1, row, sizeof(member->surname), member->surname) < 0, LL_ERROR, LM_DH, "Can't read member's surname", return false;);
    assertDo(csv_read_string(2, row, sizeof(member->name), member->name) < 0, LL_ERROR, LM_DH, "Can't read member's name", return false;);
    member->account = csv_read_uint32(3, row);
    member->properties = csv_read_uint32(4, row);
    member->discount = csv_read_uint32(5, row);

    for(uint8_t i = 0; i < 4; i++) {
        member->cardIDs[member->cardCount] = csv_read_uint32(6 + i, row);
        if(member->cardIDs[member->cardCount] == 0)
            break;
        member->cardCount++;
    }

    // Log member info
    log(LL_INFO, LM_DH, "Member found:");
    log(LL_INFO, LM_DH, "ID:          ", member->id);
    log(LL_INFO, LM_DH, "Name:        ", member->name);
    log(LL_INFO, LM_DH, "Surname:     ", member->surname);
    log(LL_INFO, LM_DH, "Account:     ", member->account);
    log(LL_INFO, LM_DH, "Properties:  ", member->properties);
    log(LL_INFO, LM_DH, "Discount [%]:", member->discount);
    log(LL_INFO, LM_DH, "Card count:  ", member->cardCount);
    for(uint8_t i = 0; i < member->cardCount; i++) 
        log(LL_INFO, LM_DH, "Card:        ", member->cardIDs[i]);

    return true;
}