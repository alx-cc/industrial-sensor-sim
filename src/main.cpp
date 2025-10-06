/**
 * @file main.cpp
 * @brief Host-side demo entry point for the Industrial Sensor Simulator
 *
 * This file wires together a simulated sensor, a single-producer/single-consumer (SPSC) ring buffer,
 * a runtime-configurable moving average filter, and an optional MQTT publisher to form a simple
 * end-to-end data path suitable for host testing and demonstration.
 *
 * Data flow:
 *   SimSensor -> producer_task(SensorSample) -> SpscRing -> consumer_task -> M.A Filter
 *   -> console logging and optional MQTT CSV publishing
 *
 * Responsibilities:
 * - Initializes a no-heap SPSC ring buffer (capacity 256) for SensorSample transport.
 * - Spawns two std::thread tasks:
 *   - Producer: samples SimSensor at a fixed period and try_push-es into the ring (drop-on-full).
 *   - Consumer: drains the ring with a deadline, computes moving averages (temperature/pressure),
 *     logs results, and optionally publishes compact CSV payloads over MQTT.
 * - Configures MQTT from environment variables or compile-time macros:
 *   - MQTT_BROKER_URL (default: tcp://127.0.0.1:1883)
 *   - MQTT_TOPIC       (default: sensors/demo/readings)
 *   Uses client-id "sensor-sim" and keep-alive 60 s. If connection fails, runs without publishing.
 * - Parses optional CLI arguments:
 *   - window: moving average window size (default 8, clamped to [1, 256])
 *   - count: total samples to produce/consume (default 50)
 *
 * Output and payloads:
 * - Console: per-sample raw and averaged values plus a final consumed count
 * - MQTT: CSV "tempC,avgTempC,pressKPa,avgPressKPa" with three decimal places (QoS 0, retain=false)
 *
 * Timing and threading notes:
 * - Uses std::chrono steady_clock and sleep_until/for for host convenience.
 * - SpscRing is single-producer/single-consumer safe; producer drops on full to avoid blocking.
 * - Consumer uses a polling loop with a short sleep and a fixed 5 s timeout; may terminate early
 *   if the producer runs too slowly.
 *
 * Limitations:
 * - Drop-on-full behavior may lose samples under backpressure.
 * - Polling-based consumer is not real-time deterministic.
 * - Moving average window is capped at 256 samples.
 * - MQTT errors are not retried.
 *
 * Embedded considerations (what would be done differently on an MCU platform):
 * - Replace std::thread and sleeps with RTOS tasks and delay-until/timers or ISR-driven producers.
 * - Replace std::chrono with hardware timers or RTOS tick counters.
 * - Replace std::cout/stdio with lightweight logging or disable logs entirely.
 * - Prefer event/notification-driven consumption over polling.
 */

#include <iostream> // std::cout: on embedded, replace with UART/log ring or disable in firmware
#include <cstdlib>  // std::strtoul/getenv for simple CLI parsing (host-only)
#include <cstdio>   // std::snprintf for tiny payload formatting
#include <cstring>  // std::strlen
#include <chrono>   // std::chrono clocks/durations: prefer HW timers or tick counters
#include <thread>   // std::thread/sleep: use RTOS delay or WFI/idle hooks instead

#include "industrial/Config.hpp"
#include "industrial/Status.hpp"
#include "industrial/SensorSample.hpp"
#include "industrial/SimSensor.hpp"
#include "industrial/SpscRing.hpp"
#include "industrial/MovingAverageFloat.hpp"
#include "industrial/MqttPublisher.hpp"

/**
 * @brief Minimal producer: read N samples from SimSensor at a fixed period and try_push to the SPSC ring.
 */
using RingBuf = industrial::SpscRing<industrial::SensorSample, industrial::kRingCapacity>;
using MovingAvg = industrial::MovingAverageFloat<industrial::kMaxAvgWindow>;

static void producer_task(RingBuf &q,
                          industrial::SimSensor &sensor,
                          std::size_t count,
                          std::chrono::milliseconds period)
{
    using clock = std::chrono::steady_clock;
    auto next = clock::now(); // timestamp via std::chrono; on MCU prefer a monotonic HW timer
    for (std::size_t i = 0; i < count; ++i)
    {
        industrial::SensorSample sample{};
        sensor.read(sample);
        (void)q.try_push(sample); // drop-on-full for now
        next += period;
        std::this_thread::sleep_until(next); // replace with RTOS delay-until or timer-driven ISR
    }
}

/**
 * @brief Minimal consumer: try_pop samples from the ring, compute moving average on them and publish to MQTT broker (if used)
 */
static void consumer_task_runtime(RingBuf &q,
                                  std::size_t stop_after,
                                  std::chrono::milliseconds timeout,
                                  uint32_t window,
                                  industrial::MqttPublisher *mqtt,
                                  const std::string &mqtt_topic)
{
    using clock = std::chrono::steady_clock;
    std::size_t consumed = 0;
    auto deadline = clock::now() + timeout;
    industrial::SensorSample s{};
    MovingAvg t_avg;
    MovingAvg p_avg;
    t_avg.set_window(window);
    p_avg.set_window(window);
    while (consumed < stop_after && clock::now() < deadline) // polling now(); for embedded, prefer event/ISR or RTOS wait
    { 
        if (q.try_pop(s))
        {
            ++consumed;
            float t_smooth = t_avg.push(s.temperature_c);
            float p_smooth = p_avg.push(s.pressure_kpa);
            std::cout << "consumer: T=" << s.temperature_c
                      << " C (avg=" << t_smooth
                      << "), P=" << s.pressure_kpa
                      << " (avg=" << p_smooth << ")\n"; // std::cout is heavy; for embedded, use lightweight logging

            // publish CSV to MQTT when available
            if (mqtt && mqtt->is_connected())
            {
                char buf[160];
                // CSV: temp,avgTemp,press,avgPress
                int n = std::snprintf(buf, sizeof(buf), "%.3f,%.3f,%.3f,%.3f", s.temperature_c, t_smooth, s.pressure_kpa, p_smooth);
                if (n > 0)
                {
                    (void)mqtt->publish(mqtt_topic, buf, (size_t)std::strlen(buf), 0, false);
                }
            }
        }
        else
        {
            // idle poll at ~200Hz; using sleep_for in lieu of a platform-specific wait instruction
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    std::cout << "consumer: total consumed=" << consumed << '\n';
}

/**
 * @brief Program entry for the industrial sensor simulator demo
 *
 * Initializes a single-producer single-consumer ring buffer, starts a simulated sensor
 * producer and a consumer that computes a moving average and optionally publishes results
 * to an MQTT broker. Designed for host demonstration with std::thread;
 */
int main(int argc, char **argv)
{
    using namespace industrial;

    RingBuf q;
    SimSensor sensor;

    // MQTT setup (host-only convenience): configure via environment variables.
    // MQTT_BROKER_URL example: tcp://127.0.0.1:1883 (or 18883 for the test config)
    // MQTT_TOPIC default: sensors/demo/readings
    char *env_broker = std::getenv("MQTT_BROKER_URL");
    char *env_topic = std::getenv("MQTT_TOPIC");
#ifdef MQTT_BROKER_URL
    char *def_broker = MQTT_BROKER_URL;
#else
    const char *def_broker = "tcp://127.0.0.1:1883";
#endif
#ifdef MQTT_TOPIC
    char *def_topic = MQTT_TOPIC;
#else
    const char *def_topic = "sensors/demo/readings";
#endif
    std::string broker = env_broker ? env_broker : std::string(def_broker);
    std::string topic = env_topic ? env_topic : std::string(def_topic);
    MqttPublisher mqtt;
    bool mqtt_on = mqtt.connect(broker, "sensor-sim", 60);
    if (mqtt_on)
    {
        std::cout << "mqtt: connected to " << broker << ", topic='" << topic << "'\n";
    }
    else
    {
        std::cout << "mqtt: disabled (library missing or connect failed)\n";
    }

    // Parse optional CLI args: [window] [count]
    // window: moving average window (default 8)
    // count: number of samples to produce/consume (default 50)
    unsigned long req = 8;
    if (argc > 1 && argv[1] != nullptr)
    {
        unsigned long v = std::strtoul(argv[1], nullptr, 10);
        if (v > 0)
            req = v;
    }
    uint32_t window = (uint32_t)req;
    if (window < 1u)
        window = 1u;
    if (window > industrial::kMaxAvgWindow)
        window = industrial::kMaxAvgWindow;
    std::cout << "moving average window set to " << window << "\n";

    unsigned long count_ul = 50;
    if (argc > 2 && argv[2] != nullptr)
    {
        unsigned long v = std::strtoul(argv[2], nullptr, 10);
        if (v > 0)
            count_ul = v;
    }
    std::size_t sample_count = static_cast<std::size_t>(count_ul);
    std::cout << "sample count set to " << sample_count << "\n";

    // Run producer and consumer concurrently: host-only demo using std::thread
    // On embedded, prefer RTOS tasks or a cooperative main loop plus ISRs
    std::thread prod([&]
                     { producer_task(q, sensor, sample_count, std::chrono::milliseconds(50)); });
    std::thread cons([&]
                     { consumer_task_runtime(q, sample_count, std::chrono::milliseconds(5000), window,
                                             mqtt_on ? &mqtt : nullptr, topic); });
    prod.join(); // thread join is a host primitive; on RTOS use task sync or semaphores
    cons.join();

    return 0;
}
