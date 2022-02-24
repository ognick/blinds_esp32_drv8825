#pragma once

#include "global_config.hpp"

#include <IRrecv.h>
#include <IRutils.h>


class WindowControllerClass
{
public:
    WindowControllerClass();
    void setup(const GlobalConfig& config);
    bool loop();

private:
    void dispatch_button(uint32_t button);
    IRrecv ir;
    decode_results results;    
};

extern WindowControllerClass WindowController;
