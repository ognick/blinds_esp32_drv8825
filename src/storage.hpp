#pragma once

#include <Arduino.h>

struct StorageClass
{
    void init(size_t size = 512);
    void write_int32(uint8_t index, int32_t number);
    int32_t read_int32(uint8_t index);
};

extern StorageClass Storage;
