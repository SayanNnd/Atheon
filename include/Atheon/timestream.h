#pragma once
#include <string>
#include <ctime>

namespace atheon{
    struct log {
        std::string Hash;
        std::string Message;
        std::time_t Time;
    };
    void timestream();
}
