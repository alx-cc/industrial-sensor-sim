#include <iostream>
#include <chrono>
#include <thread>

#include "industrial/Status.hpp"
#include "industrial/sensor_sample.hpp"
#include "industrial/SimSensor.hpp"
#include "industrial/SimpleSPSC.hpp"

// Minimal producer: read N samples from SimSensor at a fixed period and try_push to the SPSC.
static void producer_task(industrial::SimpleSPSC& q,
                          industrial::SimSensor& sensor,
                          std::size_t count,
                          std::chrono::milliseconds period)
{
    using clock = std::chrono::steady_clock;
    auto next = clock::now();
    for (std::size_t i = 0; i < count; ++i) {
        industrial::SensorSample sample = sensor.read();
    const bool enq = q.try_push(sample); // drop-on-full for now
    std::cout << "producer: T=" << sample.temperature_c
          << " C, P=" << sample.pressure_kpa
          << (enq ? " [enqueued]" : " [dropped]") << '\n';
        next += period;
        std::this_thread::sleep_until(next);
    }
}

int main() {
    using namespace industrial;

    // Example: run producer for 20 samples at 100ms
    SimpleSPSC q;
    SimSensor sensor;
    producer_task(q, sensor, 20, std::chrono::milliseconds(100));

    return 0;
}
