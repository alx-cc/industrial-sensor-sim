#pragma once
#include "industrial/sensor_sample.hpp"

namespace industrial {

class SimSensor {
public:
    // Use a config with sensible defaults to group settings and allow non-breaking extensibility.
    struct Default_Config
    {
        // Units: Hz, °C, kPa, radians. Reasonable defaults for a simple demo.
        double pressure_freq = 0.8333;  // Frequency (Hz) at which pressure readings will oscillate
        double tempc_freq = 0.1;        // Frequency (Hz) at which temperature readings will oscillate
        double pressure_amp = 15.0;     // Amplitude (kPa) to which pressure readings will oscillate
        double tempc_amp = 400.0;       // Amplitude (deg C) to which temperature readings will oscillate
        double noise_fraction = 0.15;   // fraction of amplitude for noise e.g., 0.15 => +/-15% of amplitude
        double base_tempc = 27.5;       // ambient baseline temperature (deg C)
        double base_press_kpa = 1400.0; // nominal system pressure (kPa)
        double press_phase = 0.7;       // phase offset to de-sync waves (radians)
        double corr_kpa_per_c = 0.5;    // partial correlation P per delta T, creates a realistic coupling
                                        // between P and T. When T drifts, P drifts slightly but without
                                        // strict proportionality 
    };

    // Inline ctors using a member-initializer list; default ctor delegates to Default_Config{}.
    explicit SimSensor(const Default_Config &cfg) noexcept : cfg_{cfg} {}
    SimSensor() noexcept = default; // using 'default' so it is non user-defined, allowing triviality

    // Generate one sample at call time; no I/O, no allocation.
    [[nodiscard]] SensorSample read() const noexcept;

private:
    const Default_Config cfg_;
};
} // namespace industrial