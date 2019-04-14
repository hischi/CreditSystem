#include <Arduino.h>
#include "mdb/mdb.h"
#include "cashless_device/cashless_device.h"
#include "clock/clock.h"
#include "file_handler/file_handler.h"
#include "data_handler/data_handler.h"
#include "rfid/rfid.h"

uint8_t cmd;
uint8_t data[64];
uint8_t len;

void setup() {
  // put your setup code here, to run once:
  err_init();

  setLogLevel(LL_DEBUG);
  setLogLevel(LM_MAIN, LL_DEBUG);
  setLogLevel(LM_PN532, LL_INFO);
  setLogLevel(LM_DESFIRE, LL_INFO);
  setLogLevel(LM_DESKEY, LL_INFO);
  setLogLevel(LM_MDB, LL_INFO);
  setLogLevel(LM_CS, LL_INFO);
  
  delay(5000);

  log(LL_INFO, LM_MAIN, "**************************************");
  log(LL_INFO, LM_MAIN, "* Cashless Payment on Teensy         *");
  log(LL_INFO, LM_MAIN, "*                                    *");
  log(LL_INFO, LM_MAIN, "* Written by Florian Hisch           *");
  log(LL_INFO, LM_MAIN, "**************************************");
  log(LL_INFO, LM_MAIN, "");

  log(LL_INFO, LM_MAIN, "Startup Sequence...");
  clock_init();
  fh_init();
  dh_init();
  mdb_init();
  cldev_init();
  rfid_init();
  log(LL_INFO, LM_MAIN, "Startup finished. Start Loop...");

  //fh_fopen(1, "00012345.CSV");
  //csv_read_price(5, 5);
  //csv_read_price(5, 21);
  //cPrice price;
  //price.SetAsCents(false, 5678);
  //csv_write_price(5, 24, price);
  //fh_fclose();
}

uint8_t tennisCardID[8] = {0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};
uint8_t tennisCustomerID[8] = {0x01, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x07};

uint8_t tennisCardIDRead[8];
uint8_t tennisCustomerIDRead[8];

uint32_t transid = 0;

void loop() {
  //log(LL_DEBUG, LM_MAIN, "Loop Cycle");

  //len = mdb_read(&cmd, data);
  //if(len > 0)
  //  log(LL_DEBUG, LM_MAIN, "Received Data");
  //else
  //  log(LL_DEBUG, LM_MAIN, "Received No Data");

  if(rfid_card_present()) {
    log(LL_DEBUG, LM_MAIN, "Card present");
    //rfid_set_PICC();
    //rfid_restore_card();
    

    //if(rfid_store_tennis_app(tennisCardID, tennisCustomerID)){
    //  log(LL_DEBUG, LM_MAIN, "Tennis data stored");
    //}
     if(rfid_read_tennis_app(tennisCardIDRead, tennisCustomerIDRead)){
        log(LL_DEBUG, LM_MAIN, "Tennis data found:");
        log_hexdump(LL_DEBUG, LM_MAIN, "Tennis-Card-ID:    ", 8, tennisCardIDRead);
        log_hexdump(LL_DEBUG, LM_MAIN, "Tennis-Customer-ID:", 8, tennisCustomerIDRead);

        uint32_t cardID = 0;
        uint32_t membID = 0;
        for(uint8_t i = 0; i < 8; i++) {
          cardID *= 10;
          membID *= 10;

          cardID += tennisCardIDRead[i];
          membID += tennisCustomerIDRead[i];
        }

        log(LL_DEBUG, LM_MAIN, "Tennis Card ID", cardID);
        log(LL_DEBUG, LM_MAIN, "Tennis Memb ID", membID);

        sMember member;
        //dh_get_member(10027021, &member);
        if(dh_is_authorised(membID, cardID)) {
          log(LL_INFO, LM_MAIN, "Member was authorised by card");
          dh_create_transaction(membID, 2, 150, 3);
          dh_approve_transaction();
          dh_complete_transaction();

        }
        else
          assertCnt(true, LL_WARNING, LM_MAIN, "Member can't be authorised with card");
        

      } else 
        log(LL_DEBUG, LM_MAIN, "No tennis data found");
    //} else
    //  log(LL_DEBUG, LM_MAIN, "No tennis data stored"); 
  }
 else
    log(LL_DEBUG, LM_MAIN, "Card not present");
  rfid_low_power_mode();

  //len = mdb_read(&cmd, data);
  //if(len > 0)
 //   cldev_run(cmd, data);

  //mdb_send_ack();
//Serial.print("Data");
  delay(3000);
}