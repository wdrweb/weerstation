#pragma once

#include <stdint.h>

struct measurements
{
    float temperature;
    float pressure;
    float humidity;
    //unix time
    uint64_t timestamp;
};