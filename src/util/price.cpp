#include "price.h"

cPrice read_price_uint16(const uint8_t data[]) {
    cPrice result;
    result.SetAsCents(false, (((uint16_t) data[0]) << 8) + data[1]);
    return result;
}

void write_price_uint16(const cPrice &price, uint8_t data[]) {
    uint32_t cents = price.GetAsCents(); 
    data[0] = cents & 0x0000FF00;
    data[1] = cents & 0x000000FF;
}

uint8_t price_scale() {
    return 1;
}

uint8_t price_dec_places() {
    return 2;
}