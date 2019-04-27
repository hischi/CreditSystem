#include "service_mode.h"
#include "cashless_device.h"
#include "../util/error.h"
#include "../rfid/rfid.h"
#include "../mdb/mdb.h"
#include "../data_handler/data_handler.h"

#define BUTTON_FASTUP 1
#define BUTTON_UP 2
#define BUTTON_DOWN 3
#define BUTTON_FASTDOWN 4
#define BUTTON_PROG 5
#define BUTTON_DELETE 6

#define FAST_STEP 5

uint32_t member_idx;
bool prog_in_progress;
bool delete_in_progress;

void serv_init() {
    log(LL_DEBUG, LM_SERV, "serv_init");

    member_idx = 0;
    prog_in_progress = false;
    delete_in_progress = false;
}

void up(uint8_t step) {
    log(LL_DEBUG, LM_SERV, "up");

    if(member_idx < (uint32_t) step) {
        member_idx = 0;
    } else {
        member_idx -= step;
    }
}

void down(uint8_t step) {
    log(LL_DEBUG, LM_SERV, "down");

    member_idx += step;
}

void prog_done() {
    prog_in_progress = false;
}

void prog_card() {
    log(LL_DEBUG, LM_SERV, "prog_card");
    sMember *member = dh_get_member_from_idx(member_idx);
    if(member != 0) {
        prog_in_progress = true;
        rfid_program_card_async(member->id,member->card_id,&prog_done);
    }    
}

void delete_done() {
    delete_in_progress = false;
}

void delete_card() {
    log(LL_DEBUG, LM_SERV, "delete_card");
    delete_in_progress = true;
    rfid_restore_card_async(&delete_done);
}

void serv_button_pressed(uint8_t button) {
    log(LL_DEBUG, LM_SERV, "serv_button_pressed");

    switch (button)
    {
        case BUTTON_FASTUP:
            up(FAST_STEP);
            break;

        case BUTTON_UP:
            up(1);
            break;    

        case BUTTON_DOWN:
            down(1);
            break;  

        case BUTTON_FASTDOWN:
            down(FAST_STEP);
            break;      
    
        case BUTTON_PROG:
            prog_card();
            break;

        case BUTTON_DELETE:
            delete_card();
            break;

        default:
            break;
    }
}

void serv_run() {
    log(LL_DEBUG, LM_SERV, "serv_run");

    uint8_t answer[64];
    uint8_t len = 0;

    if(prog_in_progress) {
        len = answer_DisplayRequest(answer, 10, "PROGRAM:  BITTE KARTE AUFLEGEN ");
        mdb_send_data(len, answer);
    } else if(delete_in_progress) {
        len = answer_DisplayRequest(answer, 10, "LOESCHEN: BITTE KARTE AUFLEGEN ");
        mdb_send_data(len, answer);
    } else {
        sMember *member = dh_get_member_from_idx(member_idx);
        if(member == 0) {
            len = answer_DisplayRequest(answer, 10, "SERVICE: MITGLIED WAEHLEN      ");
            mdb_send_data(len, answer);
        } else {
            char text[64];
            sprintf(text, "%08lu: %8s, %8s   ", member->id, member->name, member->given_name);
            len = answer_DisplayRequest(answer, 10, text);
            mdb_send_data(len, answer);
        }
    }
}