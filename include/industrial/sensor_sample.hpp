#pragma once

#include <chrono>

namespace industrial {

using TimePoint = std::chrono::steady_clock::time_point;

struct SensorSample {
    TimePoint ts{};            // capture time
    float temperature_c{};     // degrees Celsius
    float pressure_kpa{};      // kiloPascals
};

} // namespace industrial
