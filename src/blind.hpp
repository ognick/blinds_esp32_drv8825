#pragma once

#include <Arduino.h>

class Blind
{
public:
    enum class Position
    {
        OPEN,
        AJAR,
        CLOSED
    };

    Blind(uint8_t step_pin, uint8_t dir_pin, uint8_t power_pin, bool reversed);

    Position get_position() const;
    uint8_t get_open_percent() const;
    void init(uint8_t _index);
    void pull_up();
    void pull_down();
    void set_configure(bool _configure);
    bool get_configure();
    bool run();
    void power_off(bool save = true);
    bool is_step_phase();
    
private:
    enum class State 
    {
      STOPPED,
      SETUP,
      RUNNING,
      HOLD,

      COUNT
    };

    String stateNames[static_cast<uint8_t>(State::COUNT)] = 
    {
      "STOPPED",
      "SETUP",
      "RUNNING",
      "HOLD"
    };
    
    void set_state(State next_state);
    uint32_t get_step_duration_us();
    uint32_t get_pulse_delay_us();
    void set_power_pin(bool enable);
    void revolution();
    void set_direction(bool up);
    void set_pulse(bool pulse);
    void power_on(bool pull_up);
    bool can_move();

    const uint8_t dir_pin;
    const uint8_t step_pin;
    const uint8_t power_pin;
    const bool reversed;
    bool is_pull_up = true;
    int32_t additional_slowdown_steps = 0;
    int32_t distance = 0;
    State state = State::STOPPED;
    uint32_t state_timer_ms = 0;
    uint32_t revolution_timer_us = 0;
    bool pulse_flag = true;
    bool configure = false;
    uint8_t index = 0;
};
