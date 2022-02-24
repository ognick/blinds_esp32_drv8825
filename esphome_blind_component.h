#include "esphome.h"

#include "window_controller.hpp"
#include "window.hpp"
#include "remote.hpp"
#include "global_config.hpp"

#include <sstream>

#define NO_INTERRUPTED_LOOP_DELAY_MS 800
#define AWAIT_LOOP_DELAY_MS 1000


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

      const uint32_t now_ms = millis();

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
 std::vector<uint8_t> opent_percents;
const GlobalConfig& config;

 public:
  EsphomeBlindComponent(const GlobalConfig& config)
    :config(config)
  {
  }
  
  void setup() override 
  {
    blind_mutex = xSemaphoreCreateMutex();
    log_mutex = xSemaphoreCreateMutex();
     
    WindowController.setup(config);
    Remote.register_print_output([this](const String &data){
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

  void send_open_state()
  {
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
      }
      vTaskDelay(AWAIT_LOOP_DELAY_MS / portTICK_PERIOD_MS);
  } 
};
