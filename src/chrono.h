#pragma once
#include <stdint.h>

typedef int64_t microseconds_t;

microseconds_t get_current_microseconds();
void sleep_microseconds(microseconds_t time);