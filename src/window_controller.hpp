#pragma once

#include <IRrecv.h>
#include <IRutils.h>

class WindowControllerClass
{
public:
    WindowControllerClass();
    void setup();
    bool loop();

private:
    void dispatch_button(uint32_t button);
    IRrecv ir;
    decode_results results;    
};

extern WindowControllerClass WindowController;
