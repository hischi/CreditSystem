#include <Arduino.h>
#include "mdb/mdb.h"
#include "cashless_device/cashless_device.h"
#include "clock/clock.h"
#include "file_handler/file_handler.h"
#include "rfid/rfid.h"

uint8_t cmd;
uint8_t data[64];
uint8_t len;

void setup() {
  // put your setup code here, to run once:
  err_init();

  setLogLevel(LM_PN532, LL_INFO);
  setLogLevel(LM_DESFIRE, LL_INFO);
  setLogLevel(LM_DESKEY, LL_INFO);
  setLogLevel(LM_MDB, LL_INFO);
  setLogLevel(LL_INFO);
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

uint8_t tennisCardID[8] = {0x01, 0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78};
uint8_t tennisCustomerID[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

uint8_t tennisCardIDRead[8];
uint8_t tennisCustomerIDRead[8];

void loop() {
  //log(LL_DEBUG, LM_MAIN, "Loop Cycle");

  //len = mdb_read(&cmd, data);
  //if(len > 0)
  //  log(LL_DEBUG, LM_MAIN, "Received Data");
  //else
  //  log(LL_DEBUG, LM_MAIN, "Received No Data");

 // if(rfid_card_present()) {
 //   log(LL_DEBUG, LM_MAIN, "Card present");
    //rfid_set_PICC();
    //rfid_restore_card();

    //if(rfid_store_tennis_app(tennisCardID, tennisCustomerID)){
    //  log(LL_DEBUG, LM_MAIN, "Tennis data stored");
 //     if(rfid_read_tennis_app(tennisCardIDRead, tennisCustomerIDRead)){
 //       log(LL_DEBUG, LM_MAIN, "Tennis data found:");
 //       log_hexdump(LL_DEBUG, LM_MAIN, "Tennis-Card-ID:    ", 8, tennisCardIDRead);
 //       log_hexdump(LL_DEBUG, LM_MAIN, "Tennis-Customer-ID:", 8, tennisCustomerIDRead);
 //     } else 
 //       log(LL_DEBUG, LM_MAIN, "No tennis data found");
    //} else
    //  log(LL_DEBUG, LM_MAIN, "No tennis data stored");
 // }
 // else
  //  log(LL_DEBUG, LM_MAIN, "Card not present");
  //rfid_low_power_mode();

  len = mdb_read(&cmd, data);
  if(len > 0)
    cldev_run(cmd, data);

  //mdb_send_ack();
//Serial.print("Data");
  delay(1);
}