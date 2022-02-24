#include "esphome.h"

#include "window_controller.hpp"
#include "window.hpp"
#include "remote.hpp"

#include <esp_task_wdt.h>
#include <sstream>

#define NO_INTERRUPTED_LOOP_DELAY_MS 800
#define AWAIT_LOOP_DELAY_MS 1000
#define TEMP_SEND_PERIOD_MS 1000

#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();


static SemaphoreHandle_t blind_mutex;
static TaskHandle_t run_task_handler;
static bool done;
void run_task(void *params) 
{
    uint32_t timer_ms = millis();
    uint32_t v_task_delay = millis();
    for (;;)
    { 
      const auto boot_state = digitalRead(0);   
      xSemaphoreTake(blind_mutex, portMAX_DELAY);
      if (boot_state == LOW)
      { 
        Window.switch_position();
      }
      done = WindowController.loop();
      xSemaphoreGive(blind_mutex);

      uint32_t now_ms = millis();
      // Serial.println(String(now_ms-timer_ms) + " ms. " + String(portTICK_PERIOD_MS));
      // timer_ms = now_ms;

      if (done)
      {
         vTaskDelay( AWAIT_LOOP_DELAY_MS / portTICK_PERIOD_MS );
      }
      else if (now_ms - v_task_delay > NO_INTERRUPTED_LOOP_DELAY_MS)
      {
        v_task_delay = now_ms;
        vTaskDelay( 1 );
      }
    }
}

class EsphomeBlindComponent : public Component, public CustomMQTTDevice {
 std::vector<String> logs;
 SemaphoreHandle_t log_mutex;
 uint32_t temp_timer_ms = 0;
 public:
  void setup() override 
  {
    // esp_task_wdt_init(60, true);
    blind_mutex = xSemaphoreCreateMutex();
    log_mutex = xSemaphoreCreateMutex();
     
    WindowController.setup();
    Remote.register_print_output([this](const String &data){
      // ESP_LOGD("blind", data.c_str());
      Serial.println(data.c_str());
      xSemaphoreTake(log_mutex, portMAX_DELAY);
      logs.push_back(data);
      xSemaphoreGive(log_mutex);
    });
    subscribe("room/blinds", &EsphomeBlindComponent::on_message);

    xTaskCreatePinnedToCore(
        run_task,                 /* Task function. */
        "run_task",               /* name of task. */
        10000,                    /* Stack size of task */
        NULL,                     /* parameter of the task */
        configMAX_PRIORITIES - 1, /* priority of the task */
        &run_task_handler,        /* Task handle to keep track of created task */
        xPortGetCoreID()          /* pin task to core*/
     ); 
  }

  void on_message(const std::string &topic, const std::string &payload)
  {
    Remote.print("receive cmd: " + String(payload.c_str()));
    xSemaphoreTake(blind_mutex, portMAX_DELAY);
    Remote.on_message(payload.c_str());
    xSemaphoreGive(blind_mutex);
  }

  void send_temp()
  {
      const static uint32_t values_size = 10;
      static std::vector<uint8_t> values(values_size);
      static uint8_t i = 0;
      uint32_t now = millis();
      if (now - temp_timer_ms > TEMP_SEND_PERIOD_MS)
      {
        temp_timer_ms = now;
        int32_t temp = (temprature_sens_read() - 32) / 1.8;
        values[(i++) % values_size] = temp;
   
        if (i % values_size == 0)
        {
          uint32_t sum = 0;
          for (auto t : values) 
          {
              sum += t;
          }

          std::ostringstream msg;
          msg << sum / values_size;

          publish("room/temp/blinds", msg.str().c_str());
          // Remote.print(msg.str().c_str());
        }
      }
  }

  void send_open_state()
  {
      static std::vector<uint8_t> opent_percents = {101, 101, 101};
      const std::vector<uint8_t>& percents = Window.get_open_percents();
      if (percents != opent_percents)
      {
        opent_percents = percents;
        std::ostringstream msg;
        for (size_t i=0; i < percents.size(); ++i)
        {
          msg << static_cast<uint32_t>(percents[i]);
          if (i < percents.size() - 1)
          {
            msg << " | ";
          }
        }
            
        publish("room/state/blinds", msg.str().c_str());
        Remote.print(msg.str().c_str());
      }
  }

  void loop() override 
  {
      if (done)
      {
        xSemaphoreTake(log_mutex, portMAX_DELAY);
        for (const String& log : logs)
        {
          ESP_LOGD("blind", log.c_str());
        }
        logs.clear();
        xSemaphoreGive(log_mutex);

        send_open_state();
        // send_temp();
      }
      vTaskDelay(AWAIT_LOOP_DELAY_MS / portTICK_PERIOD_MS);
  } 
};
