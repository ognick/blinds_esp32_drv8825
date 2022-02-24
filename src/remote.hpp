#pragma once


#include <Arduino.h>
#include <functional>
#include <memory>

class PubSubClient;
class WiFiClient;
class RemoteClass
{
public:
    using CallbackFn = std::function<void(const String&)>;
    void init(CallbackFn callback);
    void print(const String& data);
    void on_message(const String& msg);
    void register_print_output(CallbackFn output);
private:
    CallbackFn callback;
    CallbackFn print_output;
};

extern RemoteClass Remote;
