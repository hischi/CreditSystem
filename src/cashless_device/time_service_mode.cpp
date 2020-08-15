#include "time_service_mode.h"
#include "cashless_device.h"
#include "../util/error.h"
#include "../clock/clock.h"
#include "../mdb/mdb.h"
#include "../data_handler/data_handler.h"

#define BUTTON_INC 1
#define BUTTON_DEC 2
#define BUTTON_LEFT 3
#define BUTTON_RIGHT 4
#define BUTTON_GET 5
#define BUTTON_SET 6

DateTime new_datetime;
enum eTimeParts{
    TP_DAY = 0,
    TP_MONTH = 1,
    TP_YEAR = 2,
    TP_HOUR = 3,
    TP_MINUTE = 4,
    TP_SECOND = 5
};
eTimeParts time_state; 

void time_serv_init() {
    log(LL_DEBUG, LM_SERV, "time_serv_init");

    new_datetime = clock_now();
    time_state = TP_HOUR;
}

void datetime_math(int16_t d_year, int8_t d_month) {

    uint16_t year = new_datetime.year() + d_year;
    uint8_t month = new_datetime.month() + d_month;
    uint8_t day = new_datetime.day();

    if(year == 0)
        year = 9999;

    month = month % 12;
    if(month == 0)
        month = 12;     

    if(month == 2)
        day = 1;

    new_datetime = DateTime(year, month, day, new_datetime.hour(), new_datetime.minute(), new_datetime.second());
}

void inc() {
    switch(time_state) {
        case TP_DAY:
        new_datetime = new_datetime + TimeSpan(1, 0, 0, 0);
        break;

        case TP_MONTH:
        datetime_math(0, 1);
        break;

        case TP_YEAR:
        datetime_math(1, 0);
        break;

        case TP_HOUR:
        new_datetime = new_datetime + TimeSpan(0, 1, 0, 0);
        break;

        case TP_MINUTE:
        new_datetime = new_datetime + TimeSpan(0, 0, 1, 0);
        break;

        case TP_SECOND:
        new_datetime = new_datetime + TimeSpan(0, 0, 0, 1);
        break;

        default:
        assertRtn(true, LL_ERROR, LM_TSERV, "Invalid time part");
    }
}

void dec() {
    switch(time_state) {
        case TP_DAY:
        new_datetime = new_datetime + TimeSpan(-1, 0, 0, 0);
        break;

        case TP_MONTH:
        datetime_math(0, -1);
        break;

        case TP_YEAR:
        datetime_math(-1, 0);
        break;

        case TP_HOUR:
        new_datetime = new_datetime + TimeSpan(0, -1, 0, 0);
        break;

        case TP_MINUTE:
        new_datetime = new_datetime + TimeSpan(0, 0, -1, 0);
        break;

        case TP_SECOND:
        new_datetime = new_datetime + TimeSpan(0, 0, 0, -1);
        break;

        default:
        assertRtn(true, LL_ERROR, LM_TSERV, "Invalid time part");
    }
}

void time_serv_button_pressed(uint8_t button) {
    log(LL_DEBUG, LM_SERV, "time_serv_button_pressed");

    switch (button)
    {
        case BUTTON_INC:
            inc();
            break;

        case BUTTON_DEC:
            dec();
            break;    

        case BUTTON_LEFT:
            if(time_state != TP_DAY)
                time_state = (eTimeParts) (time_state - 1);
            else
                time_state = TP_SECOND;            
            break;  

        case BUTTON_RIGHT:
            if(time_state != TP_SECOND)
                time_state = (eTimeParts) (time_state + 1);
            else
                time_state = TP_DAY;            
            break; 
    
        case BUTTON_GET:
            new_datetime = clock_now();
            break;

        case BUTTON_SET:
            clock_adjust(new_datetime);
            new_datetime = 1565968661;  //16.08.2019 17:17:41
            break;

        default:
            break;
    }
}

void time_serv_run() {
    log(LL_DEBUG, LM_SERV, "time_serv_run");

    uint8_t answer[64];
    uint8_t len = 0;
    char text[64];
    char state_text[8];

    switch(time_state) {
        case TP_DAY:
        sprintf(state_text, "%s", "TAG  ");
        break;

        case TP_MONTH:
        sprintf(state_text, "%s", "MONAT");
        break;

        case TP_YEAR:
        sprintf(state_text, "%s", "JAHR ");
        break;

        case TP_HOUR:
        sprintf(state_text, "%s", "STUND");
        break;

        case TP_MINUTE:
        sprintf(state_text, "%s", "MIN  ");
        break;

        case TP_SECOND:
        sprintf(state_text, "%s", "SEK  ");
        break;

        default:
        assertRtn(true, LL_ERROR, LM_TSERV, "Invalid time part");
    }

    // Format time string and write it
    sprintf(text, "%02hhu.%02hhu.%04u %02hhu:%02hhu:%02hhu,%03hd %s   ", new_datetime.day(), new_datetime.month(), new_datetime.year(), new_datetime.hour(), new_datetime.minute(), new_datetime.second(), (int) new_datetime.millis(), state_text);
    len = answer_DisplayRequest(answer, 10, text);
    mdb_send_data(len, answer);
}