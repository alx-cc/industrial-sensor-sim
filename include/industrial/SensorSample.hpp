#pragma once

#include <chrono>

namespace industrial {

using TimePoint = std::chrono::steady_clock::time_point; // Host demo type; on embedded, prefer HAL/RTOS tick or device timer

struct SensorSample {
    TimePoint ts{};            // capture time
    float temperature_c{};     // degrees Celsius
    float pressure_kpa{};      // kiloPascals
};

} // namespace industrial
