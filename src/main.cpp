// NOTE: These standard facilities are convenient for a host demo but are often
// not embedded-friendly on bare-metal targets. Prefer lightweight/log-free builds,
// hardware timers/RTOS ticks, and RTOS tasks or a single-threaded main loop.
#include <iostream>   // std::cout: replace with UART/log ring or disable in firmware
#include <chrono>     // std::chrono clocks/durations: prefer HW timers or tick counters
#include <thread>     // std::thread/sleep: use RTOS delay or WFI/idle hooks instead

#include "industrial/Status.hpp"
#include "industrial/SensorSample.hpp"
#include "industrial/SimSensor.hpp"
#include "industrial/SpscRing.hpp"

// Minimal producer: read N samples from SimSensor at a fixed period and try_push to the SPSC.
static void producer_task(industrial::SpscRing<industrial::SensorSample, 256>& q,
                          industrial::SimSensor& sensor,
                          std::size_t count,
                          std::chrono::milliseconds period)
{
    using clock = std::chrono::steady_clock;
    auto next = clock::now(); // timestamp via std::chrono; on MCU prefer a monotonic HW timer
    for (std::size_t i = 0; i < count; ++i) {
        industrial::SensorSample sample = sensor.read();
        (void)q.try_push(sample); // drop-on-full for now
        next += period;
        std::this_thread::sleep_until(next); // replace with RTOS delay-until or timer-driven ISR
    }
}

// Minimal consumer: drain until 'stop_after' items consumed or 'timeout' expires.
static void consumer_task(industrial::SpscRing<industrial::SensorSample, 256>& q,
                          std::size_t stop_after,
                          std::chrono::milliseconds timeout)
{
    using clock = std::chrono::steady_clock;
    std::size_t consumed = 0;
    auto deadline = clock::now() + timeout; // avoid wall-clock math; use ticks or a timeout counter
    industrial::SensorSample s{};
    while (consumed < stop_after && clock::now() < deadline) { // polling now(); prefer event/ISR or RTOS wait
        if (q.try_pop(s)) {
            ++consumed;
            std::cout << "consumer: T=" << s.temperature_c
                      << " C, P=" << s.pressure_kpa << '\n'; // std::cout is heavy; use lightweight logging
        } else {
            // Backoff a bit to avoid busy spin; in embedded, this might be a wait instruction.
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    std::cout << "consumer: total consumed=" << consumed << '\n';
}

int main() {
    using namespace industrial;

    SpscRing<SensorSample, 256> q; // no-heap buffer
    SimSensor sensor;

    // Run producer and consumer concurrently: host-only demo using std::thread.
    // On embedded, prefer RTOS tasks or a cooperative main loop plus ISRs.
    std::thread prod([&]{ producer_task(q, sensor, 50, std::chrono::milliseconds(50)); });
    std::thread cons([&]{ consumer_task(q, 50, std::chrono::milliseconds(5000)); });
    prod.join(); // thread join is a host primitive; on RTOS use task sync or semaphores
    cons.join();

    return 0;
}
