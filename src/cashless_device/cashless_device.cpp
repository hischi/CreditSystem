#include "cashless_device.h"
#include "../error_handler/error_handler.h"
#include "../mdb/mdb.h"
#include "../util/price.h"
#include <string.h>

#define SERIAL_NUMBER 9051993
#define MODEL_NUMBER 11235812

#define SW_VERSION_MAJ 0x01
#define SW_VERSION_MIN 0x00

#define CMD_RESET   0x10
#define CMD_SETUP   0x11
#define CMD_POLL    0x12
#define CMD_VEND    0x13
#define CMD_READER  0x14

#define SCMD_SETUP_CONFIG   0x00
#define SCMD_SETUP_PRICES   0x01

#define SCMD_VEND_REQUEST   0x00
#define SCMD_VEND_CANCEL    0x01
#define SCMD_VEND_SUCCESS   0x02
#define SCMD_VEND_FAILURE   0x03
#define SCMD_VEND_COMPLETE  0x04
#define SCMD_VEND_CASHSALE  0x05

#define SCMD_READER_DISABLE 0x00
#define SCMD_READER_ENABLE  0x01
#define SCMD_READER_CANCEL  0x02

#define ERR_MEDIA_ERROR             0x00
#define ERR_MEDIA_INVALID           0x10
#define ERR_TAMPER_ERROR            0x20
#define ERR_SINGLE_ERROR            0x30
#define ERR_COMMUNICATION_ERROR     0x40
#define ERR_REQUIRES_SERVICE        0x50
#define ERR_PERSISTENT_ERROR        0x70
#define ERR_READER_FAIL             0x80
#define ERR_COMMUNICATION_FATAL     0x90
#define ERR_MEDIA_JAMMED            0xA0
#define ERR_PERSISTENT_FATAL        0xB0

enum eCashlessState {
  CS_Inactive,
  CS_Disabled,
  CS_Enabled,
  CS_Session_Idle,
  CS_Vend
};

eCashlessState state;

struct sVcmSetup {
uint8_t level;
uint8_t columnsOnDisplay;
uint8_t rowsOnDisplay;
uint8_t displayInfo;
cPrice maxPrice;
cPrice minPrice;
} vcmSetup;

uint8_t answer_JustReset(uint8_t answer[]) {
    answer[0] = 0x00;
    return 1;
}

uint8_t answer_ReaderConfigInfo(uint8_t answer[]) {
    answer[0] = 0x01;               // Reader Config Data Response
    answer[1] = 0x01;               // Only level 1 cmds are supported
    answer[2] = 0x19;               // Currency Code (EUR) High
    answer[3] = 0x78;               // Currency Code (EUR) Low
    answer[4] = price_scale();      // Scale Factor 1
    answer[5] = price_dec_places(); // 2 Decimal Places
    answer[6] = 0x05;               // Max. 5 s response time
    answer[7] = 0x00;               // No restoring founds, No multivend, No display, No Cash-Sale
    return 8;
}

uint8_t answer_DisplayRequest(uint8_t answer[], uint8_t time_tenths, const char msg[]) {
    uint16_t maxDispSize = ((uint16_t) vcmSetup.rowsOnDisplay)* ((uint16_t) vcmSetup.columnsOnDisplay);
    maxDispSize = (maxDispSize < 32) ? maxDispSize : 32;
    
    uint16_t msg_len = strlen(msg);

    answer[0] = 0x02;         // Display Request Response
    answer[1] = time_tenths;  // Display time

    assertCnt(msg_len > maxDispSize, LL_WARNING, LM_CLDEV, "Message longer than VCM Display");
    msg_len = (msg_len <= maxDispSize) ? msg_len : maxDispSize;

    memcpy(&answer[2], msg, msg_len);
    return msg_len + 2;
}

uint8_t answer_BeginSession(uint8_t answer[]) {
    answer[0] = 0x03;   // Begin Session Response
    answer[1] = 0xFF;   // no found applicable
    answer[2] = 0xFF;   // no found applicable
    return 3;
}

uint8_t answer_SessionCancelRequest(uint8_t answer[]) {
    answer[0] = 0x04;   // Session Cancel Request
    return 1;
}

uint8_t answer_VendApproved(uint8_t answer[], const cPrice &price) {
    answer[0] = 0x05;   // Vend Approved
    write_price_uint16(price, &answer[1]);
    return 3;    
}

uint8_t answer_VendDenied(uint8_t answer[]) {
    answer[0] = 0x06;   // Vend Denied
    return 1;
}

uint8_t answer_EndSession(uint8_t answer[]) {
    answer[0] = 0x07;   // End Session
    return 1;
}

uint8_t answer_Cancelled(uint8_t answer[]) {
    answer[0] = 0x08;   // Cancelled
    return 1;
}

uint8_t answer_PeripheralID(uint8_t answer[]) {
    answer[0] = 0x09;   // Peripheral ID
    // 3 chars Manufacturer Code, 12 chars serial number, 12 chars model number
    sprintf((char*) &answer[1], "FGH%012d%012d", SERIAL_NUMBER, MODEL_NUMBER);
    answer[28] = SW_VERSION_MAJ;    // SW Version Major
    answer[29] = SW_VERSION_MIN;    // SW Version Minor
    return 30;
}

uint8_t answer_MalfunctionError(uint8_t answer[], uint8_t errorCode, uint8_t subCode) {
    answer[0] = 0x0A;   // Malfunction / Error
    answer[1] = errorCode | subCode;
    return 2;
}

uint8_t answer_OutOfSequence(uint8_t answer[]) {
    answer[0] = 0x0B;   // out of sequence
    return 1;
}

void do_cmd_setup_config(const uint8_t data[]) {
    log(LL_DEBUG, LM_CLDEV, "do_cmd_setup_config");

    // Copy the vcm data into vcmSetup
    vcmSetup.level = data[0];
    vcmSetup.columnsOnDisplay = data[1];
    vcmSetup.rowsOnDisplay = data[2];
    vcmSetup.displayInfo = data[3];

    // Answer
    uint8_t answer[32];
    uint8_t len = 0;
    len = answer_ReaderConfigInfo(answer);
    mdb_send_data(len, answer);
}

void do_cmd_setup_prices(const uint8_t data[]) {
    log(LL_DEBUG, LM_CLDEV, "do_cmd_setup_prices");

    vcmSetup.maxPrice = read_price_uint16(data);        // max. price (not used)
    vcmSetup.minPrice = read_price_uint16(&data[2]);    // min. price (not used)

    // No Answer
    mdb_send_ack();
}

void do_cmd_poll() {
    log(LL_DEBUG, LM_CLDEV, "do_cmd_poll");

    // TODO

    mdb_send_ack();
}

void do_cmd_vend_request(const uint8_t data[]) {
    log(LL_DEBUG, LM_CLDEV, "do_cmd_vend_request");

    cPrice price = read_price_uint16(data);
    uint16_t item = (((uint16_t) data[2]) << 8) + data[3];

    // TODO: Do Vend handling
    uint8_t answer[32];
    uint8_t len = 0;
    len = answer_VendApproved(answer, price);
    mdb_send_data(len, answer);
}

void do_cmd_vend_cancel() {
    log(LL_DEBUG, LM_CLDEV, "do_cmd_vend_cancel");

    uint8_t answer[32];
    uint8_t len = 0;
    len = answer_VendDenied(answer);
    mdb_send_data(len, answer);
}

void do_cmd_vend_success(const uint8_t data[]) {
    log(LL_DEBUG, LM_CLDEV, "do_cmd_vend_success");

    uint16_t item = (((uint16_t) data[0]) << 8) + data[1];
    // TODO: Do Vend handling

    mdb_send_ack();
}

void do_cmd_vend_failure() {
    log(LL_DEBUG, LM_CLDEV, "do_cmd_vend_failure");

    // TODO: Do Vend handling

    mdb_send_ack();
}

void do_cmd_vend_complete() {
    log(LL_DEBUG, LM_CLDEV, "do_cmd_vend_complete");

    uint8_t answer[32];
    uint8_t len = 0;
    len = answer_EndSession(answer);
    mdb_send_data(len, answer);
}

void do_cmd_vend_cashsale() {
    assertCnt(true, LL_ERROR, LM_CLDEV, "Command Vend Cash Sale not supported");
    mdb_send_nack(); // Not supported
}

void do_cmd_reader_disable() {
    log(LL_DEBUG, LM_CLDEV, "do_cmd_reader_disable");

    mdb_send_ack(); // Do nothing but respond
}

void do_cmd_reader_enable() {
    log(LL_DEBUG, LM_CLDEV, "do_cmd_reader_enable");

    mdb_send_ack(); // Do nothing but respond
}

void do_cmd_reader_cancel() {
    log(LL_DEBUG, LM_CLDEV, "do_cmd_reader_cancel");

    uint8_t answer[32];
    uint8_t len = 0;
    len = answer_Cancelled(answer);
    mdb_send_data(len, answer);
}

void do_cmd(uint8_t cmd, const uint8_t data[]) {
    log(LL_DEBUG, LM_CLDEV, "do_cmd");

    switch(cmd) {
        case CMD_RESET:
            // TODO: Reset
            break;

        case CMD_SETUP:
            switch(data[0]) {
                case SCMD_SETUP_CONFIG:
                    do_cmd_setup_config(&data[1]);
                    break;
                case SCMD_SETUP_PRICES:
                    do_cmd_setup_prices(&data[1]);
                    break;  
                default:
                    assertRtn(true, LL_ERROR, LM_CLDEV, "Unknown SETUP-Subcmd");
            } 
            break;

        case CMD_POLL:
            do_cmd_poll();
            break;

        case CMD_VEND:
            switch(data[0]) {
                case SCMD_VEND_REQUEST:
                    do_cmd_vend_request(&data[1]);
                    break;
                case SCMD_VEND_CANCEL:
                    do_cmd_vend_cancel();
                    break;   
                case SCMD_VEND_SUCCESS:
                    do_cmd_vend_success(&data[1]);
                    break;
                case SCMD_VEND_FAILURE:
                    do_cmd_vend_failure();
                    break;
                case SCMD_VEND_COMPLETE:
                    do_cmd_vend_complete();
                    break;
                case SCMD_VEND_CASHSALE:
                    do_cmd_vend_cashsale();
                    break;
                default:
                    assertRtn(true, LL_ERROR, LM_CLDEV, "Unknown VEND-Subcmd");
            }
            break;   

        case CMD_READER:
            switch(data[0]) {
                case SCMD_READER_DISABLE:
                    do_cmd_reader_disable();
                    break;
                case SCMD_READER_ENABLE:
                    do_cmd_reader_enable();
                    break;   
                case SCMD_READER_CANCEL:
                    do_cmd_reader_cancel();
                    break;
                default:
                    assertRtn(true, LL_ERROR, LM_CLDEV, "Unknown READER-Subcmd");
            }
            break;       
        default:
            assertRtn(true, LL_ERROR, LM_CLDEV, "Unknown Cmd");
    }
}

//----------------------------------------------//
// Internal CHECKSs for Transitions             //
//----------------------------------------------//
bool check_SetupReceived(uint8_t cmd, const uint8_t data[]) {
    return (cmd == CMD_SETUP) && (data[0] == SCMD_SETUP_CONFIG);
}

bool check_Reset(uint8_t cmd, const uint8_t data[]) {
    return (cmd == CMD_RESET);
}

bool check_ReaderEnable(uint8_t cmd, const uint8_t data[]) {
    return (cmd == CMD_READER) && (data[0] == SCMD_READER_ENABLE);
}

bool check_ReaderDisable(uint8_t cmd, const uint8_t data[]) {
    return (cmd == CMD_READER) && (data[0] == SCMD_READER_DISABLE);
}

bool check_MediaReady(uint8_t cmd, const uint8_t data[]) {
    // TODO
    return true;
}

bool check_MediaNotReady(uint8_t cmd, const uint8_t data[]) {
    // TODO
    return false;
}

bool check_SessionComplete(uint8_t cmd, const uint8_t data[]) {
    return (cmd == CMD_VEND) && (data[0] == SCMD_VEND_COMPLETE);
}

bool check_VendRequest(uint8_t cmd, const uint8_t data[]) {
    return (cmd == CMD_VEND) && (data[0] == SCMD_VEND_REQUEST);
}

bool check_VendEnd(uint8_t cmd, const uint8_t data[]) {
    if(cmd == CMD_VEND) {
        return (data[0] == SCMD_VEND_CANCEL) || (data[0] == SCMD_VEND_SUCCESS) || (data[0] == SCMD_VEND_FAILURE);
    } else
        return false;
}

//----------------------------------------------//
// Internal RUNs for each state                 //
//----------------------------------------------//
void transition_inactive(uint8_t cmd, const uint8_t data[]) {
    log(LL_DEBUG, LM_CLDEV, "transition_inactive");

    if(check_SetupReceived(cmd, data)) {
        state = CS_Disabled;
    }
}

void transition_disabled(uint8_t cmd, const uint8_t data[]) {
    log(LL_DEBUG, LM_CLDEV, "transition_disabled");

    if(check_Reset(cmd, data))
        state = CS_Inactive;
    else if(check_ReaderEnable(cmd, data))
        state = CS_Enabled;    
}

void transition_enabled(uint8_t cmd, const uint8_t data[]) {
    log(LL_DEBUG, LM_CLDEV, "transition_enabled");

    if(check_Reset(cmd, data))
        state = CS_Inactive;
    else if(check_ReaderDisable(cmd, data))
        state = CS_Disabled;
    else if(check_MediaReady(cmd, data))
        state = CS_Session_Idle;    
}

void transition_session_idle(uint8_t cmd, const uint8_t data[]) {
    log(LL_DEBUG, LM_CLDEV, "transition_session_idle");

    if(check_Reset(cmd, data))
        state = CS_Inactive;
    else if(check_MediaNotReady(cmd, data) || check_SessionComplete(cmd, data))   
        state = CS_Enabled;
    else if(check_VendRequest(cmd, data)) 
        state = CS_Vend;     
}

void transition_vend(uint8_t cmd, const uint8_t data[]) {
    log(LL_DEBUG, LM_CLDEV, "transition_vend");

    if(check_Reset(cmd, data))
        state = CS_Inactive; // TODO: Check for MediaNotReady
    else if(check_VendEnd(cmd, data))
        state = CS_Session_Idle;    
}

//----------------------------------------------//
// Global interfaces                            //
//----------------------------------------------//

void cldev_init() {
    state = CS_Inactive;
    memset(&vcmSetup, 0, sizeof(vcmSetup));
}

void cldev_run(uint8_t cmd, const uint8_t data[]) {

    // Execute Cmd
    do_cmd(cmd, data);

    // Move along transistions
    switch(state) {
        case CS_Inactive:
            transition_inactive(cmd, data);
            break;

        case CS_Disabled:
            transition_disabled(cmd, data);
            break;

        case CS_Enabled:
            transition_enabled(cmd, data);
            break;

        case CS_Session_Idle:  
            transition_session_idle(cmd, data);
            break;

        case CS_Vend:
            transition_vend(cmd, data);
            break;

        default:
            assertDo(true, LL_FATAL, LM_CLDEV, "Unknown state", resetOnError());            
    }
}

uint8_t cldev_cmd_len(uint8_t cmd) {
    log(LL_DEBUG, LM_CLDEV, "cldev_cmd_len");

    switch(cmd) {
        case CMD_RESET:
            return 0;

        case CMD_SETUP:
            return 1;

        case CMD_POLL:
            return 0;

        case CMD_VEND:
            return 1;

        case CMD_READER:
            return 1;    
        default:
            assertCnt(true, LL_ERROR, LM_CLDEV, "Unknown Cmd");
            return 0;
    };
}

uint8_t cldev_scmd_len(uint8_t cmd, uint8_t scmd) {
    log(LL_DEBUG, LM_CLDEV, "cldev_cmd_len");

    switch(cmd) {
        case CMD_RESET:
            return 0;

        case CMD_SETUP:
            switch(scmd) {
                case SCMD_SETUP_CONFIG:
                    return 4;
                case SCMD_SETUP_PRICES:
                    return 4;
                default:
                    assertDo(true, LL_ERROR, LM_CLDEV, "Unknown SETUP-Subcmd", return 0;);
            } 
            break;

        case CMD_POLL:
            return 0;

        case CMD_VEND:
            switch(scmd) {
                case SCMD_VEND_REQUEST:
                    return 4;
                case SCMD_VEND_CANCEL:
                    return 0;
                case SCMD_VEND_SUCCESS:
                    return 2;
                case SCMD_VEND_FAILURE:
                    return 0;
                case SCMD_VEND_COMPLETE:
                    return 0;
                case SCMD_VEND_CASHSALE:
                    return 4;
                default:
                    assertDo(true, LL_ERROR, LM_CLDEV, "Unknown VEND-Subcmd", return 0;);
            }
            break;   

        case CMD_READER:
            switch(scmd) {
                case SCMD_READER_DISABLE:
                    return 0;
                case SCMD_READER_ENABLE:
                    return 0;  
                case SCMD_READER_CANCEL:
                    return 0;
                default:
                    assertDo(true, LL_ERROR, LM_CLDEV, "Unknown READER-Subcmd", return 0;);
            }
            break;       
        default:
            assertCnt(true, LL_ERROR, LM_CLDEV, "Unknown Cmd");
            return 0;
    }
}