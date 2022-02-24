#include "window_controller.hpp"

#define LED_PIN 2
#define IR_PIN 13

#define LEFT_DIRECTION_PIN 4
#define LEFT_ENABLE_PIN 15
#define LEFT_STEP_PIN 14

#define CENTER_DIRECTION_PIN 32
#define CENTER_ENABLE_PIN 5
#define CENTER_STEP_PIN 33

#define RIGHT_DIRECTION_PIN 27
#define RIGHT_ENABLE_PIN 25
#define RIGHT_STEP_PIN 26

#define BUTTON_1 0xFF22DD
#define BUTTON_2 0xFFC23D
#define BUTTON_3 0xFFE01F
#define BUTTON_MINUS 0xFF10EF
#define BUTTON_OFF 0xFFE21D
#define BUTTON_ON 0xFFA25D
#define BUTTON_PLUS 0xFF5AA5
#define BUTTON_TIMER 0xFF629D
#define EMPTY_DATA 0xFFFFFF

#define TIME_ZONE_OFFSET 10800
#define DRIFT_BUTTON_TIME_MS 300

#define MS_TO_US 1000
#define TIME_TO_SLEEP_US 5000 * MS_TO_US
#define SUNRISE_DELAY_US 1000 * 60 * 5

#include <Arduino.h>
#include <IRremoteESP8266.h>

#include "window.hpp"
#include "led.hpp"
#include "remote.hpp"
#include "storage.hpp"


void IRAM_ATTR on_boot_press()
{
    static uint32_t boot_button_timer_ms = 0;
    uint32_t now_ms = millis();
    if (now_ms - boot_button_timer_ms > DRIFT_BUTTON_TIME_MS)
    {
        boot_button_timer_ms = now_ms;
        Window.switch_position();
    }
}


WindowControllerClass::WindowControllerClass()
  : ir(IR_PIN)
{
}

void WindowControllerClass::setup(const GlobalConfig& config)
{
    Serial.begin(115200);
    Led.init(LED_PIN);
    Remote.init([this](const String& msg){
        if (msg == "OPEN")
          dispatch_button(BUTTON_MINUS);
        else if (msg == "CLOSE")
          dispatch_button(BUTTON_PLUS);
        else if (msg == "SUNRISE")
          dispatch_button(BUTTON_TIMER);  
        else if (msg == "OFF")
          dispatch_button(BUTTON_OFF);
        else if (msg == "SELECT_1")
          dispatch_button(BUTTON_1); 
        else if (msg == "SELECT_2")
          dispatch_button(BUTTON_2); 
        else if (msg == "SELECT_3")
          dispatch_button(BUTTON_3); 
        else if (msg == "ON")
          dispatch_button(BUTTON_ON);
    });
    Storage.init();
    ir.enableIRIn();

    // TODO: use global config
    // for (const auto& c : config.blinds)
    // {
    //    Window.add_blind(c.step_pin, c.direction_pin, c.enable_pin, c.reversed);
    // }

    Window.add_blind(LEFT_STEP_PIN, LEFT_DIRECTION_PIN, LEFT_ENABLE_PIN, false);
    Window.add_blind(CENTER_STEP_PIN, CENTER_DIRECTION_PIN, CENTER_ENABLE_PIN, false);
    Window.add_blind(RIGHT_STEP_PIN, RIGHT_DIRECTION_PIN, RIGHT_ENABLE_PIN, true);

    #ifdef INTERUPT_BY_BOOT
      attachInterrupt(0, on_boot_press, FALLING);
    #endif
    Remote.print("Blind was started");   
}


bool WindowControllerClass::loop()
{
    if (ir.decode(&results))
    {
        if (results.value != EMPTY_DATA) Serial.println((int)results.value, HEX);
        dispatch_button(results.value);
        ir.resume();
    }

    bool done = Window.run();
    if (done)
    {
        done &= Led.run();
    }

    return done;
}

void WindowControllerClass::dispatch_button(uint32_t button)
{
    switch (button)
    {
        case BUTTON_OFF:
            Window.power_off();
            Window.set_active_blind(0);
            break;
        case BUTTON_PLUS:
            Window.sunrise(0);
            Window.pull_down();
            break;
        case BUTTON_MINUS:
            Window.sunrise(0);
            Window.pull_up();
            break;
        case BUTTON_1:
            Window.set_active_blind(1);
            break;
        case BUTTON_2:
            Window.set_active_blind(2);
            break;
        case BUTTON_3:
            Window.set_active_blind(3);
            break;
        case BUTTON_ON:
            Window.configure();
            break;
        case BUTTON_TIMER:
            Window.sunrise(SUNRISE_DELAY_US);
            break;
    }
}

WindowControllerClass WindowController;
