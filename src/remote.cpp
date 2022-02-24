#include "remote.hpp"


void RemoteClass::init(CallbackFn callback_)
{
    callback = callback_;
}

void RemoteClass::print(const String& data)
{
    if (print_output)
    {
      print_output(data);
    }
    else 
    {
      Serial.println(data);
    }
}

void RemoteClass::on_message(const String& msg)
{
  print(msg);
  if (callback) 
  {
    callback(msg);
  }
}

void RemoteClass::register_print_output(CallbackFn output)
{
  print_output = output;
}


RemoteClass Remote;
