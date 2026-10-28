#pragma once

#include <stdint.h>

struct measurements
{
    float temperature;
    float pressure;
    float humidity;
    //1 = seconds since boot, 0 = unix time; fits the padding so sizeof stays 24
    uint8_t relative;
    //unix time, or seconds since boot when relative is set
    uint64_t timestamp;
};

static_assert(sizeof(measurements) == 24,
              "the relative flag must stay inside the existing padding");
