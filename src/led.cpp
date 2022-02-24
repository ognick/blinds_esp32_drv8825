#include "led.hpp"

#define BLINK_PERIOD_MS 100

void LedClass::init(uint8_t pin_)
{
    pin = pin_;
    pinMode(pin, OUTPUT);
}

void LedClass::set_turn(bool turn, bool blink, uint32_t duration_ms)
{
  is_turn = turn;
  is_blink = blink;
  
  if (duration_ms > 0)
  {
    is_turn = false;
    is_duration = true;
    turn_duration_ms = duration_ms;
    disable_timer_ms = millis();
  }

  run();
}

bool LedClass::run()
{
    static bool blink_phase = false;
    uint32_t now = millis();
    if (now - update_timer_ms > BLINK_PERIOD_MS)
    {
        update_timer_ms = now;
        blink_phase = !blink_phase;
    }

    if (is_duration && now - disable_timer_ms > turn_duration_ms)
    {
      is_duration = false;
    }

    bool flag = (is_turn || is_duration) && (!is_blink || blink_phase);
    digitalWrite(pin, flag);

    return !is_duration;
}

LedClass Led;
