#pragma once

#include <vector>

struct GlobalConfig
{
    struct Blind
    {
        int direction_pin;
        int enable_pin;
        int step_pin;
        bool reversed;
        unsigned long int max_distance = 4910;
    };

    std::vector<Blind> blinds;
};