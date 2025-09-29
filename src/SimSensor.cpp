#include "industrial/sensor_sample.hpp"
#include "industrial/SimSensor.hpp"
#include <chrono>
#include <cmath>
#include <random>

// All behavior is configured per-instance via SimSensor::Default_Config; no file-scope constants.

namespace industrial
{

    using clock = std::chrono::steady_clock;

    /**
     * Generate a sine value with added noise, given frequency, amplitude and current time.
     * Time (t0) starts at the first call to this function.
     */
    static double noisy_sine_gen(double freqHz, double amplitude, double offset, double noise_fraction, double phaseRad = 0.0)
    {
        // Prefer std::numbers::pi if building with C++20; literal is fine otherwise.
        constexpr double pi = 3.14159265358979323846;

        // fetch a static snapshot of time at first run of this function
        static const auto t0 = clock::now();

        // get elapsed time (from t0 to now) in seconds
        const auto now = clock::now();
        const double t = std::chrono::duration<double>(now - t0).count();

        // Generate uniform noise in [-amplitude * noise_fraction, amplitude * noise_fraction]
        static std::mt19937 rng(std::random_device{}());
        const double noise_range = amplitude * noise_fraction;
        std::uniform_real_distribution<double> dist(-noise_range, noise_range);
        const double noise = dist(rng);

        // Get final calculation
        const double y = offset + amplitude * std::sin(2.0 * pi * freqHz * t + phaseRad) + noise;
        return y;
    }

    // Define the member within the same namespace (class is declared in the header).
    SensorSample SimSensor::read() const noexcept
    {
        SensorSample s{};
        s.ts = clock::now();

        const auto &cfg = cfg_;

        // Temperature: slow variation around baseline.
    const double temp_c = noisy_sine_gen(cfg.tempc_freq, cfg.tempc_amp, cfg.base_tempc, cfg.noise_fraction, 0.0);

        // Pressure: faster wave plus partial correlation to temperature deviation.
    const double p_fast = noisy_sine_gen(cfg.pressure_freq, cfg.pressure_amp, 0.0, cfg.noise_fraction, cfg.press_phase);
        const double pressure_kpa = cfg.base_press_kpa + p_fast + cfg.corr_kpa_per_c * (temp_c - cfg.base_tempc);

        s.temperature_c = static_cast<float>(temp_c);
        s.pressure_kpa = static_cast<float>(pressure_kpa);
        return s;
    }

} // namespace industrial
