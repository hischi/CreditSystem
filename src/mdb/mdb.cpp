#include "mdb.h"
#include "../cashless_device/cashless_device.h"
#include "../error_handler/error_handler.h"
#include "../util/soft_timer.h"

#include <HardwareSerial.h>
#define SERIAL_9N1 0x84
#define SERIAL_9E1 0x8E
#define SERIAL_9O1 0x8F
#define SERIAL_9N1_RXINV 0x94
#define SERIAL_9E1_RXINV 0x9E
#define SERIAL_9O1_RXINV 0x9F
#define SERIAL_9N1_TXINV 0xA4
#define SERIAL_9E1_TXINV 0xAE
#define SERIAL_9O1_TXINV 0xAF
#define SERIAL_9N1_RXINV_TXINV 0xB4
#define SERIAL_9E1_RXINV_TXINV 0xBE
#define SERIAL_9O1_RXINV_TXINV 0xBF

#define PERIPHERAL_ADDR 0x10

#define MDB_RESET_PIN 3

cSoftTimer reset_timer;
cSoftTimer nack_timer;

uint32_t check_mdb_state() {
    int available = Serial1.available();
    if(available > 0) {
        log(LL_DEBUG, LM_MDB, "MDB-State: Bytes available");
        reset_timer.Stop();
        return available;
    } else {
        //log(LL_DEBUG, LM_MDB, "MDB-State: No Bytes available");
        if(digitalRead(MDB_RESET_PIN) == HIGH) {
            log(LL_DEBUG, LM_MDB, "MDB-State: RESET high");
            if(!reset_timer.IsStarted()) {
                reset_timer.Start(100);
                //log(LL_DEBUG, LM_MDB, "MDB-State: Reset Timer Started");
            }
            else if(reset_timer.IsOver()) {
                //log(LL_DEBUG, LM_MDB, "MDB-State: Reset Timer Over. Reinit MDB");
                mdb_init();
            } 
        } else {
            //log(LL_DEBUG, LM_MDB, "MDB-State: RESET low");
            reset_timer.Stop();
        }
        return 0;
    }
}

void write(uint8_t data, bool mode) {
    log(LL_DEBUG, LM_MDB, "write");
    uint16_t data_mode = data;
    if(mode)
        data_mode |= 0x100;
    //Serial.print(data_mode>>8, HEX);
    //Serial.print(" ");
    //Serial.println(data&0xFF, HEX);
    Serial1.write9bit(data_mode);
}

bool read(uint8_t *data, uint8_t cmd, uint8_t subcmd) {
    log(LL_DEBUG, LM_MDB, "read");
    char warning[64];
    uint8_t i = 0;

    while(Serial1.available() < 1) {
        
        delayMicroseconds(100);
        i++;
        if(i > 20) {
            sprintf(warning, "Read waited 2 ms for bytes for cmd 0x%02X / sub-cmd 0x%02X", cmd, subcmd);
            log(LL_WARNING, LM_MDB, warning);
            i = 0;
        }
    }

    uint16_t data_mode = Serial1.read();
    *data = data_mode & 0xFF;
    //Serial.print("Daten: ");
    //Serial.print(*data, HEX);
    //if((data_mode & 0x100) > 0) {
    //    Serial.println("   Mode Bit set");
    //} else {
    //    Serial.println("   Mode Bit not set");
    //}
    return ((data_mode & 0x100) > 0);
}



//----------------------------------------------//
// Global interfaces                            //
//----------------------------------------------//

void mdb_init() {
    log(LL_DEBUG, LM_MDB, "mdb_init");

    Serial1.begin(9600, SERIAL_9N1_TXINV); 

    reset_timer.Stop();
    nack_timer.Stop();

    //state = MS_Idle;
    //prev_state = MS_RESET;
}

bool mdb_send_data(uint8_t len, const uint8_t data[]) {
    log_hexdump(LL_DEBUG, LM_MDB, "mdb_send_data ():", len, data);

    uint8_t chk = 0; 

    // Transfer data
    for(uint8_t i = 0; i < len; i++) {
        write(data[i], false);
        chk += data[i];
    } 

    // Transfer CHK
    write(chk, true);

    // Wait for ACK, NACK, RET
    nack_timer.Start(5);

    while(!nack_timer.IsOver()) {
        if(check_mdb_state() > 0) {
            uint8_t response;
            bool mode = read(&response, 255, 0);
            assertDo(mode, LL_ERROR, LM_MDB, "Expected ACK, NACK, RET but got mode bit set", return false;);        

            if(response == 0x00)        // ACK --> we are done here
                return true;
            else if(response == 0xAA || response == 0xFF)   // RET or NACK --> send again
                return mdb_send_data(len, data);
            else
                assertDo(true, LL_ERROR, LM_MDB, "Expected ACK, NACK, RET but got unknown data", return false;);        
        }
    }
    // NACK timer over so try again
    return mdb_send_data(len, data);
}

void mdb_send_ack() {
    log(LL_DEBUG, LM_MDB, "mdb_send_ack");
    write(0x00, true);
}

void mdb_send_nack() {
    log(LL_DEBUG, LM_MDB, "mdb_send_nack");
    write(0xFF, true);
}

uint8_t mdb_read(uint8_t *cmd, uint8_t data[]) {
    log(LL_DEBUG, LM_MDB, "mdb_read");

    uint8_t len = 0;
    uint8_t chk = 0;

    if(check_mdb_state() > 0) {
        
        // Read first byte and check what to expect next
        bool mode = read(cmd, 0, 0);
        chk = *cmd;

        if(mode) {
            if((*cmd & 0xF8) == PERIPHERAL_ADDR)  // it is an address + cmd
            {
                log_hexdump(LL_DEBUG, LM_MDB, "Received cmd", 1, cmd);

                uint8_t rem_len = cldev_cmd_len(*cmd);      // How many to be read based on CMD
                for(uint8_t i = 0; i < rem_len; i++) {      // Read rem_len bytes   
                    mode = read(&data[len], *cmd, 255);
                    chk += data[len];
                    len++;
                    
                    assertDo(mode, LL_ERROR, LM_MDB, "Invalid Block from VCM. Mode-Bit was set unexpectedly.", return 0;);        
                }

                rem_len = cldev_scmd_len(*cmd, data[0]);    // How many to be read based on SCMD
                for(uint8_t i = 0; i < rem_len; i++) {      // Read rem_len bytes
                    mode = read(&data[len], *cmd, data[0]);
                    chk += data[len];
                    len++;
                    
                    assertDo(mode, LL_ERROR, LM_MDB, "Invalid Block from VCM. Mode-Bit was set unexpectedly.", return 0;);        
                }

                uint8_t chk_read = 0;
                mode = read(&chk_read, *cmd, 254);
                assertDo(mode, LL_ERROR, LM_MDB, "Invalid Block from VCM. Mode-Bit was set unexpectedly in CHK", return 0;);
                assertDo(chk != chk_read, LL_ERROR, LM_MDB, "Calculated CHK does not match CHK-Byte", return 0;);

                len++;
                if(len > 1)
                    log_hexdump(LL_DEBUG, LM_MDB, "Received cmd block data", len, data);
                else
                    log_hexdump(LL_DEBUG, LM_MDB, "Received cmd without data", 1, cmd);
            }
        }
    }
    return len;
}






















/*
enum eMdbState {
    MS_Idle,
    MS_ReadCMD,
    MS_ReadSCMD, 
    MS_ReadData,
    MS_ReadCHK,
    MS_SendData,
    MS_Reset
};

eMdbState state;
eMdbState prev_state;

// Locals for state MS_Idle
bool     reset_timer_started;
uint32_t reset_timer;
uint32_t reset_timer_offset;




// Locals for states ReadCmd, ReadScmd and ReadData
uint8_t cmd;
uint8_t data[64];
uint8_t len;

// Locals for state ReadData
uint8_t rem_len;




eMdbState run_mdb_idle() {
    if(Serial1.available() > 0) {
        return MS_ReadCMD;
    } 
    return state;
}

eMdbState run_mdb_readCmd() {
    if(Serial1.available()) {
        bool mode = read(&cmd);
        if(mode && ((cmd & 0xF8) == PERIPHERAL_ADDR)) {
            if(cldev_cmd_len(cmd) > 0)
                return MS_ReadSCMD;
            else
                return MS_ReadCHK;
        }
    } else {
        return MS_Idle;
    }
    return state;
}

eMdbState run_mdb_readScmd() {
    if(Serial1.available()) {
        bool mode = read(&data[0]);
        len = 1;

        assertDo(mode, LL_ERROR, LM_MDB, "Unexpected mode set for first data byte",mdb_send_nack();return MS_Idle;);

        rem_len = cldev_scmd_len(cmd, data[0]);

        if(rem_len > 0)
            return MS_ReadData;
        else
            return MS_ReadCHK;
    }
    return state;
}

eMdbState run_mdb_readData() {
    if(rem_len <= 0)
        return MS_ReadCHK;
    else if(Serial1.available()) {
        bool mode = read(&data[len]);
        len++;
        rem_len--;

        assertDo(mode, LL_ERROR, LM_MDB, "Unexpected mode set for data bytes",mdb_send_nack();return MS_Idle;);

        if(rem_len <= 0)
            return MS_ReadCHK;
    } 
    return state;
}

eMdbState run_mdb_readChk() {
    if(Serial1.available()) {
        uint8_t read_chk = 0;
        bool mode = read(&read_chk);

        assertDo(mode, LL_ERROR, LM_MDB, "Unexpected mode set for chk byte",mdb_send_nack();return MS_Idle;);

        uint8_t calculated_chk = cmd;
        for(uint8_t i = 0; i < len; i++)
            calculated_chk += data[i];

        if(calculated_chk == read_chk) {
            cldev_run(cmd, data);
        } else {
            assertCnt(true, LL_ERROR, LM_MDB, "CHK was not correct");
        }   
        return MS_Idle;
    } 
    return state;
}

void run_mdb() {
    eMdbState next_state;

    // We shall not stay in the same state for more than 100 ms
    // We start thus the reset_timer after each state change
    if(state != prev_state) {
        start_reset_timer();
    }

    // Check if the reset_timer timed-out
    if(reset_timer_over()){
        next_state = MS_Reset;
    } else {
        switch(state) {
        case MS_Idle:
            next_state = run_mdb_idle();
        break;

        case MS_ReadCMD:
            next_state = run_mdb_readCmd();
        break;

        case MS_ReadSCMD:
        break;

        case MS_ReadData:
        break;

        case MS_ReadCHK:
        break;

        case MS_SendData:
        break;

        default:
        assertDo(true, LL_FATAL, LM_MDB, "Unknown state", resetOnError(); return;);
    }
    }

    

    prev_state = state;
    state = next_state;
}
*/