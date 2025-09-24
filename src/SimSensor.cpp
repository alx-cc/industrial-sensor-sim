#include "industrial/sensor_sample.hpp"
#include "industrial/SimpleSPSC.hpp"
#include <chrono>
#include <thread>
#include <cstdlib>

namespace industrial {

static SimpleSPSC g_queue; // temporary global for learning phase

void sample_generator_once() {
    SensorSample sample;
    sample.ts = std::chrono::steady_clock::now();
    sample.temperature_c = static_cast<float>(std::rand() % 100);
    sample.pressure_kpa = static_cast<float>(std::rand() % 200);
    g_queue.try_push(sample); // ignore full for now (single slot)
}

} // namespace industrial
