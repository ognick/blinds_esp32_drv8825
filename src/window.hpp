#pragma once

#include <Arduino.h>
#include <vector>
#include <memory>

struct Blind;
struct Led;

struct WindowClass
{
    WindowClass();
    ~WindowClass();

    void add_blind(uint8_t step_pin, uint8_t dir_pin, uint8_t power_pin, bool reversed);
    void switch_position();
    const std::vector<uint8_t>& get_open_percents() const;
    void set_active_blind(uint8_t active_blind_);
    void sunrise(uint32_t sunrise_delay_ms);
    void power_off();
    void pull_down();
    void pull_up();
    void configure();
    bool run();

private:
    using BlindMethod = void (Blind::*)(void);
    void invoke(BlindMethod blind_method);
    
    uint8_t active_blind = 0;
    std::vector <std::unique_ptr<Blind>> blinds;
    uint32_t activated_blind_timer = 0;
    bool any_configure = false;

    uint32_t sunrise_timer_ms = 0;
    uint32_t sunrise_delay_ms = 0;
    uint8_t sunrise_next_blind_index = 0;
    std::vector<uint8_t> open_percents;
};

extern WindowClass Window;
