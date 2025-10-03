/**
 * @file SimSensor.cpp
 * @brief Host-side simulator that synthesizes temperature and pressure samples.
 *
 * Implements SimSensor::read() using steady_clock time and a noisy sine generator (std::chrono, std::random).
 * Intended for simulation only (not embedded-friendly). Configuration is per instance (see SimSensor::Config).
 * Produces timestamped SensorSample with temperature (°C) and pressure (kPa); pressure includes a fast wave
 * and partial correlation to temperature deviation; noise is bounded uniform.
 *
 * @note: This is simulation code; uses std::chrono and std::random to synthesize data. Not embedded-friendly;
 * real firmware would read hardware sensors via drivers/ISRs and avoid host RNG/time APIs.
 */

#include "industrial/SensorSample.hpp"
#include "industrial/SimSensor.hpp"
#include <chrono>
#include <cmath>
#include <random>

namespace industrial
{

    using clock = std::chrono::steady_clock;

    /**
     * @brief Generate a sine value with added noise, given frequency, amplitude and current time.
     * 
     * @note Time (t0) starts at the first call to this function.
     */
    static float noisy_sine_gen(double freqHz, double amplitude, double offset, double noise_fraction, double phaseRad = 0.0)
    {
        constexpr float pi = 3.14159265358979323846;

        // fetch a static snapshot of time at first run of this function
        static auto t0 = clock::now();

        // get elapsed time (from t0 to now) in seconds
        auto now = clock::now();
        float t = std::chrono::duration<double>(now - t0).count();

        // Generate uniform noise in [-amplitude * noise_fraction, amplitude * noise_fraction]
        static std::mt19937 rng(std::random_device{}());
        float noise_range = amplitude * noise_fraction;
        std::uniform_real_distribution<float> dist(-noise_range, noise_range);
        float noise = dist(rng);

        // Get final calculation
        float y = offset + amplitude * std::sin(2.0 * pi * freqHz * t + phaseRad) + noise;
        return y;
    }

    /**
     * @brief Generate a simulated sensor sample with timestamp, temperature, and pressure.
     * Uses internal configuration and noisy sine waves; pressure is partially correlated with temperature.
     */
    SensorSample SimSensor::read() const
    { 
        SensorSample s{};
        s.ts = clock::now();

        auto &cfg = cfg_; // look into how much space/processing this is taking up, auto is 'pythonic' could be polluting things in the backend

        // Temperature: slow variation around baseline.
        float temperature_c = noisy_sine_gen(cfg.tempc_freq, cfg.tempc_amp, cfg.base_tempc, cfg.noise_fraction, 0.0);

        // Pressure: faster wave plus partial correlation to temperature deviation.
        float p_fast = noisy_sine_gen(cfg.pressure_freq, cfg.pressure_amp, 0.0, cfg.noise_fraction, cfg.press_phase);
        float pressure_kpa = cfg.base_press_kpa + p_fast + cfg.corr_kpa_per_c * (temperature_c - cfg.base_tempc);
        s.temperature_c = temperature_c;
        s.pressure_kpa = pressure_kpa;
        return s;
    }

} // namespace industrial
