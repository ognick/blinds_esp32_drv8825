#include "window.hpp"

#define HOLD_ACTIVE_BLIND_MS 60000
#define NO_INTERRUPTED_LOOP_DELAY_US 100000

#include "blind.hpp"
#include "led.hpp"
#include "remote.hpp"

WindowClass::WindowClass() = default;
WindowClass::~WindowClass() = default;

void WindowClass::add_blind(uint8_t step_pin, uint8_t dir_pin, uint8_t power_pin, bool reversed)
{
    std::unique_ptr <Blind> blind{new Blind(step_pin, dir_pin, power_pin, reversed)};
    blind->init(blinds.size() + 1);
    open_percents.push_back(blind->get_open_percent());
    blinds.push_back(std::move(blind));
}

void WindowClass::switch_position()
{
    bool should_be_closed = false;
    for (auto& b: blinds)
    {
        if (b->get_position() != Blind::Position::CLOSED)
        {
            should_be_closed = true;
        }
    }
    if (should_be_closed)
    {
        pull_down();
    } else
    {
        pull_up();
    }
}

const std::vector<uint8_t>& WindowClass::get_open_percents() const
{
  return open_percents;
}

void WindowClass::set_active_blind(uint8_t active_blind_)
{
    if (active_blind == active_blind_)
    {
        return;
    }

    Remote.print("set_active_blind: " + String(active_blind_));
    if (active_blind_ >= 0 && active_blind_ <= blinds.size())
    {
        active_blind = active_blind_;
        if (active_blind != 0)
        {
            activated_blind_timer = millis();
        }
    }

    for (auto& b: blinds)
    {
        b->set_configure(false);
    }
}

void WindowClass::sunrise(uint32_t sunrise_delay_ms_)
{
  if (sunrise_delay_ms_ == 0) 
  { 
    Remote.print("sunrise disabled");
  }
  sunrise_delay_ms = sunrise_delay_ms_;
  sunrise_next_blind_index = 0;
  sunrise_timer_ms = millis() + sunrise_delay_ms;
}

void WindowClass::power_off()
{
    sunrise(0);
    for (size_t i = 0; i < blinds.size(); ++i)
    {
        if (active_blind == 0 || i + 1 == active_blind)
        {
            blinds[i]->power_off();
        }
    }
}

void WindowClass::pull_down()
{
    for (size_t i = 0; i < blinds.size(); ++i)
    {
        if (active_blind == 0 || i + 1 == active_blind)
        {
            blinds[i]->pull_down();
        }
    }
}

void WindowClass::pull_up()
{
    for (size_t i = 0; i < blinds.size(); ++i)
    {
        if (active_blind == 0 || i + 1 == active_blind)
        {
            blinds[i]->pull_up();
        }
    }
}

void WindowClass::configure()
{
    if (active_blind > 0)
    {
        auto& blind = blinds[active_blind - 1];
        blind->set_configure(true);
        Remote.print("Blind: " + String(active_blind) + " is configured");
    }
}

bool WindowClass::run()
{
    uint32_t now_ms = millis();
    if (sunrise_delay_ms > 0 && now_ms - sunrise_timer_ms > sunrise_delay_ms)
    {
        sunrise_timer_ms = now_ms;
        ++sunrise_next_blind_index;
        set_active_blind(sunrise_next_blind_index);
        pull_up();
        if (sunrise_next_blind_index == blinds.size())
        {
          sunrise(0);
          set_active_blind(0);
        }
    }
    
    bool done = true;
    bool has_any_configure = false;
    auto now_us = micros();
    auto rotation_timer_us = now_us;
    noInterrupts();
    bool any_step_phase = false;
    while (micros() - rotation_timer_us < NO_INTERRUPTED_LOOP_DELAY_US)
    {
        for (auto& b: blinds)
        {
            done &= b->run();
            any_step_phase |= b->is_step_phase();
            has_any_configure |= b->get_configure();
        }
    }

    while (any_step_phase)
    {
        any_step_phase = false;
        for (auto& b: blinds)
        {
            if (b->is_step_phase())
            {
              done &= b->run();
              any_step_phase |= b->is_step_phase();
            }
        }
    }

    for (size_t i=0; i<blinds.size(); ++i)
    {
      open_percents[i] = blinds[i]->get_open_percent();
    }
    
    interrupts();
    if (has_any_configure != any_configure)
    {
        any_configure = has_any_configure;
        Remote.print("Configured: " + String(any_configure));
        Led.set_turn(any_configure);
    }

    if (activated_blind_timer > 0 && now_ms - activated_blind_timer > HOLD_ACTIVE_BLIND_MS)
    {
        activated_blind_timer = 0;
        set_active_blind(0);
    }

    return done;
}

WindowClass Window;
