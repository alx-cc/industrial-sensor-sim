// NOTE: Using standard facilities for host demo - not embedded-friendly. 
// For embedded, prefer lightweight/log-free builds, hardware timers/RTOS ticks, and RTOS tasks. 

#include <iostream>   // std::cout: replace with UART/log ring or disable in firmware
#include <cstdlib>    // std::strtoul/getenv for simple CLI parsing (host-only)
#include <cstdio>     // std::snprintf for tiny payload formatting
#include <cstring>    // std::strlen
#include <chrono>     // std::chrono clocks/durations: prefer HW timers or tick counters
#include <thread>     // std::thread/sleep: use RTOS delay or WFI/idle hooks instead

#include "industrial/Status.hpp"
#include "industrial/SensorSample.hpp"
#include "industrial/SimSensor.hpp"
#include "industrial/SpscRing.hpp"
#include "industrial/MovingAverageFloat.hpp"
#include "industrial/MqttPublisher.hpp"

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

// Minimal consumer using a runtime-configurable moving average window to avoid template switching.
static void consumer_task_runtime(industrial::SpscRing<industrial::SensorSample, 256>& q,
                                 std::size_t stop_after,
                                 std::chrono::milliseconds timeout,
                                 uint32_t window,
                                 industrial::MqttPublisher* mqtt,
                                 const std::string& mqtt_topic)
{
    using clock = std::chrono::steady_clock;
    std::size_t consumed = 0;
    auto deadline = clock::now() + timeout; // avoid wall-clock math; use ticks or a timeout counter
    industrial::SensorSample s{};
    industrial::MovingAverageFloat<256> t_avg;
    industrial::MovingAverageFloat<256> p_avg;
    t_avg.set_window(window);
    p_avg.set_window(window);
    while (consumed < stop_after && clock::now() < deadline) { // polling now(); prefer event/ISR or RTOS wait
        if (q.try_pop(s)) {
            ++consumed;
            float t_smooth = t_avg.push(s.temperature_c);
            float p_smooth = p_avg.push(s.pressure_kpa);
            std::cout << "consumer: T=" << s.temperature_c
                      << " C (avg=" << t_smooth
                      << "), P=" << s.pressure_kpa
                      << " (avg=" << p_smooth << ")\n"; // std::cout is heavy; use lightweight logging

            // publish a tiny CSV payload to MQTT when available.
            if (mqtt && mqtt->is_connected()) {
                char buf[160];
                // CSV: temp,avgTemp,press,avgPress
                int n = std::snprintf(buf, sizeof(buf), "%.3f,%.3f,%.3f,%.3f", s.temperature_c, t_smooth, s.pressure_kpa, p_smooth);
                if (n > 0) {
                    (void)mqtt->publish(mqtt_topic, buf, (size_t)std::strlen(buf), 0, false);
                }
            }
        } else { 
            // idle poll at ~200Hz; using sleep_for in lieu of a platform-specific wait instruction
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    std::cout << "consumer: total consumed=" << consumed << '\n';
}

int main(int argc, char** argv) {
    using namespace industrial;

    SpscRing<SensorSample, 256> q; // no-heap buffer
    SimSensor sensor;

    // MQTT setup (host-only convenience): configure via environment variables.
    // MQTT_BROKER_URL example: tcp://127.0.0.1:1883 (or 18883 for the test config)
    // MQTT_TOPIC default: sensors/demo/readings
    const char* env_broker = std::getenv("MQTT_BROKER_URL");
    const char* env_topic  = std::getenv("MQTT_TOPIC");
#ifdef MQTT_BROKER_URL
    const char* def_broker = MQTT_BROKER_URL;
#else
    const char* def_broker = "tcp://127.0.0.1:1883";
#endif
#ifdef MQTT_TOPIC
    const char* def_topic = MQTT_TOPIC;
#else
    const char* def_topic = "sensors/demo/readings";
#endif
    std::string broker = env_broker ? env_broker : std::string(def_broker);
    std::string topic  = env_topic  ? env_topic  : std::string(def_topic);
    MqttPublisher mqtt;
    bool mqtt_on = mqtt.connect(broker, "sensor-sim", 60);
    if (mqtt_on) {
        std::cout << "mqtt: connected to " << broker << ", topic='" << topic << "'\n";
    } else {
        std::cout << "mqtt: disabled (library missing or connect failed)\n";
    }

    // Parse optional CLI args: [window] [count]
    // window: moving average window (default 8)
    // count: number of samples to produce/consume (default 50)
    unsigned long req = 8;
    if (argc > 1 && argv[1] != nullptr) {
        unsigned long v = std::strtoul(argv[1], nullptr, 10);
        if (v > 0) req = v;
    }
    uint32_t window = (uint32_t)req;
    if (window < 1u) window = 1u;
    if (window > 256u) window = 256u;
    std::cout << "moving average window set to " << window << "\n";

    unsigned long count_ul = 50;
    if (argc > 2 && argv[2] != nullptr) {
        unsigned long v = std::strtoul(argv[2], nullptr, 10);
        if (v > 0) count_ul = v;
    }
    std::size_t sample_count = static_cast<std::size_t>(count_ul);
    std::cout << "sample count set to " << sample_count << "\n";

    // Run producer and consumer concurrently: host-only demo using std::thread
    // On embedded, prefer RTOS tasks or a cooperative main loop plus ISRs
    std::thread prod([&]{ producer_task(q, sensor, sample_count, std::chrono::milliseconds(50)); });
    std::thread cons([&]{ consumer_task_runtime(q, sample_count, std::chrono::milliseconds(5000), window,
                                                mqtt_on ? &mqtt : nullptr, topic); });
    prod.join(); // thread join is a host primitive; on RTOS use task sync or semaphores
    cons.join();

    return 0;
}
