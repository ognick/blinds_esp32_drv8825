#pragma once

#include <Arduino.h>

struct LedClass
{
public:
    void init(uint8_t pin);
    bool run();
    void set_turn(bool turn, bool blink = false, uint32_t duration_ms = 0);
private:
    bool is_turn = false;
    bool is_blink = false;
    bool is_duration = false;

    uint32_t turn_duration_ms = 0;
    uint8_t pin;
    uint32_t update_timer_ms = 0;
    uint32_t disable_timer_ms = 0;
};

extern LedClass Led;
