#include "storage.hpp"

#include <EEPROM.h>

void StorageClass::init(size_t size)
{
    EEPROM.begin(size);
}

void StorageClass::write_int32(uint8_t index, int32_t number)
{
    const size_t address = index * sizeof(int32_t);
    EEPROM.write(address, (number >> 24) & 0xFF);
    EEPROM.write(address + 1, (number >> 16) & 0xFF);
    EEPROM.write(address + 2, (number >> 8) & 0xFF);
    EEPROM.write(address + 3, number & 0xFF);
    EEPROM.commit();
}

int32_t StorageClass::read_int32(uint8_t index)
{
    const size_t address = index * sizeof(int32_t);
    return ((int32_t) EEPROM.read(address) << 24) +
        ((int32_t) EEPROM.read(address + 1) << 16) +
        ((int32_t) EEPROM.read(address + 2) << 8) +
        (int32_t) EEPROM.read(address + 3);
}

StorageClass Storage;
