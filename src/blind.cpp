#include "blind.hpp"

#define CFG_PULSE_DELAY_US 3000
#define CFG_STEP_DURATION_US 3000

#define UP_PULSE_DELAY_US 1800
#define UP_STEP_DURATION_US 1000

#define DOWN_PULSE_DELAY_US 900
#define DOWN_STEP_DURATION_US 800

#define ADDITIONAL_PULSE_DELAY_US 1000
#define ADDITIONAL_SLOWDOWN_STEPS 100

#define HOLD_LOCK_MS 3000
#define SETUP_LOCK_MS 100

#define MAX_DISTANCE 4910

#include "remote.hpp"
#include "storage.hpp"

Blind::Blind(uint8_t step_pin, uint8_t dir_pin, uint8_t power_pin, bool reversed)
        : dir_pin(dir_pin), step_pin(step_pin), power_pin(power_pin), reversed(reversed)
{
}

Blind::Position Blind::get_position() const
{
    if (distance == 0) return Position::CLOSED;
    if (distance == MAX_DISTANCE) return Position::OPEN;
    return Position::AJAR;
}

uint8_t Blind::get_open_percent() const
{ 
  const uint32_t d = ((distance < 0) ? 0 : distance) * 100;
  return static_cast<uint8_t>(d / uint32_t(MAX_DISTANCE));
}

void Blind::init(uint8_t _index)
{
    index = _index;
    pinMode(dir_pin, OUTPUT);
    pinMode(step_pin, OUTPUT);
    pinMode(power_pin, OUTPUT);
    distance = Storage.read_int32(index);
    Remote.print("Init: " + String(distance));
    power_off(false);
}

void Blind::pull_up()
{
    Remote.print(String(index) + " pull up");
    power_on(true);
}

void Blind::pull_down()
{
    Remote.print(String(index) + " pull down");
    power_on(false);
}

void Blind::set_configure(bool _configure)
{
    configure = _configure;
}

bool Blind::get_configure()
{
  return configure;
}

bool Blind::run()
{
    auto now_ms = millis();
    if (state == State::HOLD && now_ms - state_timer_ms > HOLD_LOCK_MS)
    {
        set_power_pin(false);
        set_state(State::STOPPED);
    }

    if (state == State::SETUP && now_ms - state_timer_ms > SETUP_LOCK_MS)
    {
      set_state(State::RUNNING);
    }

    if (state == State::RUNNING)
    {
        if (can_move())
        {
            revolution();
        } 
        else
        {
            power_off();
        }
    }

    return (state == State::STOPPED && !configure);
}

void Blind::power_off(bool save)
{
    set_state(State::HOLD);
    pulse_flag = false;
    set_pulse(pulse_flag);
    digitalWrite(dir_pin, LOW);
    Remote.print(String(index) + " power off. Distance: " + String((uint32_t) distance));
    if (configure) distance = 0;
    if (save)
    {
        Storage.write_int32(index, distance);
    }
}

bool Blind::is_step_phase()
{
  return pulse_flag;
}

uint32_t Blind::get_step_duration_us()
{
    if (configure) return CFG_STEP_DURATION_US;
    if (is_pull_up) return UP_STEP_DURATION_US;
    return DOWN_STEP_DURATION_US;
}

uint32_t Blind::get_pulse_delay_us()
{
    if (configure) return CFG_PULSE_DELAY_US;
    uint32_t additional_delay_us = 0;
    if (distance + ADDITIONAL_SLOWDOWN_STEPS > MAX_DISTANCE || distance < ADDITIONAL_SLOWDOWN_STEPS)
    {
        additional_delay_us = ADDITIONAL_PULSE_DELAY_US;
    }
    if (is_pull_up) 
    {
      if (additional_slowdown_steps > 0)
      {
          additional_delay_us = ADDITIONAL_PULSE_DELAY_US;
      }
      return UP_PULSE_DELAY_US + additional_delay_us;
    }
    return DOWN_PULSE_DELAY_US + additional_delay_us;
}

void Blind::set_power_pin(bool enable)
{
    digitalWrite(power_pin, !enable);
    Remote.print(String(index) + ((enable) ? " enabled" : " disabled"));
}

void Blind::revolution()
{
    uint32_t now_us = micros();
    uint32_t interval_us = ((pulse_flag) ? get_step_duration_us() : get_pulse_delay_us());
    if (now_us - revolution_timer_us > interval_us)
    {
//      Remote.print(String(index) + " step  " + String(pulse_flag) + " " +String(now_us - revolution_r_us) + " us");
        revolution_timer_us = now_us;
        set_direction(is_pull_up ^ reversed);
        set_pulse(pulse_flag);
        distance += ((is_pull_up) ? 1 : -1) * pulse_flag;
        pulse_flag = !pulse_flag;
        additional_slowdown_steps -= 1;
    }
}

void Blind::set_direction(bool up)
{
    digitalWrite(dir_pin, up);
}

void Blind::set_pulse(bool pulse)
{
    digitalWrite(step_pin, pulse);
}

void Blind::power_on(bool pull_up)
{
    const bool change_dir = is_pull_up != pull_up;
    is_pull_up = pull_up;
    if (can_move())
    {
        if (state == State::STOPPED || state == State::HOLD || (state == State::RUNNING && change_dir))
        {
            additional_slowdown_steps = ADDITIONAL_SLOWDOWN_STEPS;
            set_state(State::SETUP);
            set_power_pin(true);
        }
    }
}

bool Blind::can_move()
{
    return configure || ((is_pull_up && distance < MAX_DISTANCE) || (!is_pull_up && distance > 0));
}

void Blind::set_state(State next_state)
{
  Remote.print("Set state " + String(index) + " " + stateNames[int(state)] + " => " + stateNames[int(next_state)]);
  state = next_state;
  state_timer_ms = millis();
}
