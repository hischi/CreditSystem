#include <Arduino.h>
#include "mdb/mdb.h"
#include "cashless_device/cashless_device.h"
#include "clock/clock.h"
#include "file_handler/file_handler.h"
#include "data_handler/data_handler.h"
#include "rfid/rfid.h"
#include "periphery/periphery.h"
#include "TimerOne.h"

#define AUTO_LOG true

uint8_t cmd;
uint8_t data[64];
uint8_t len;

void test() {
  do {
    len = mdb_read(&cmd, data);
    if(len > 0)
      cldev_run(cmd, data);
  } while(len > 0);
}


void setup() {
  // put your setup code here, to run once:
  err_init();

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
  peri_init();
  fh_init();
  dh_init();
  mdb_init();
  cldev_init();
  rfid_init(AUTO_LOG);
  log(LL_INFO, LM_MAIN, "Startup finished. Start Loop...");

  Timer1.initialize(3000);
  Timer1.attachInterrupt(test);
}

uint32_t transid = 0;

void loop() {
  log(LL_DEBUG, LM_MAIN, "Loop Cycle");

  rfid_run();
  //rfid_program_card(20000000, 20000000);

  delay(500);
}