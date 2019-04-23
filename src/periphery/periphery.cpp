#include "periphery.h"
#include "../util/error.h"

#define LED1_PIN 35 
#define LED2_PIN 36

#define DIP1_PIN 32
#define DIP2_PIN 31
#define DIP3_PIN 30
#define DIP4_PIN 29

void peri_init() {
    log(LL_DEBUG, LM_PERI, "peri_init");

    pinMode(LED1_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);

    pinMode(DIP1_PIN, INPUT_PULLUP);
    pinMode(DIP2_PIN, INPUT_PULLUP);
    pinMode(DIP3_PIN, INPUT_PULLUP);
    pinMode(DIP4_PIN, INPUT_PULLUP);
}

void peri_set_led(uint8_t led_id, bool led_on) {
    log(LL_DEBUG, LM_PERI, "peri_set_led");

    uint8_t pin = 0;
    switch(led_id) {
        case 1:
            pin = LED1_PIN;
            break;
        case 2:
            pin = LED2_PIN;
            break;
        default:
            assertRtn(true, LL_ERROR, LM_PERI, "The selected LED is not available.");
    }

    if(led_on)
        digitalWrite(pin, HIGH);
    else
        digitalWrite(pin, LOW);
}

bool peri_check_dip(uint8_t mask) {
    log(LL_DEBUG, LM_PERI, "peri_check_dip");

    uint8_t dip_state = 0;
    dip_state = digitalRead(DIP1_PIN);
    dip_state |= (digitalRead(DIP2_PIN) << 1);
    dip_state |= (digitalRead(DIP3_PIN) << 2);
    dip_state |= (digitalRead(DIP4_PIN) << 3);

    return (dip_state & mask) > 0;
}