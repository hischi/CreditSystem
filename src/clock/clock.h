#pragma once

#include "../util/time_format.h"

void clock_init();
bool clock_was_init();

DateTime clock_now();
void clock_adjust(DateTime &new_datetime);