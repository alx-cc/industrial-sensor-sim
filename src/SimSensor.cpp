#include "industrial/sensor_sample.hpp"
#include <chrono>
#include <cstdlib>
#include <cmath>
#include <numbers>

namespace industrial {

/**
 * Time-derived simulated sensor. Simulates an oscillating, somewhat noisey instrument 
 * reading by using a sine function with added noise to generate a value that can be fetched
 * by the application at any time for a constantly moving sensor reading.
 *
 * Values are a deterministic function of time, so sensor update cadence is decoupled
 * from the caller's read cadence. 
 *
 * Model:
 * Temperature = 27.5C   + 7.5C   * sin(10 * pi * t) + (noise)
 * Pressure    = 1400kPa + 200kPa * sin(12 * pi * t) + (50 * noise)
 * 
 */
class SimSensor {

    /**
     * Generate a sine value given frequency, amplitude and current time
     */
    double sine_gen(double freqHz, double amplitude, double offset, double phaseRad = 0.0)
    {
        // fetch a static snapshot of time at first run of this function
        using clock = std::chrono::steady_clock;
        static const auto t0 = clock::now();
        
        // get elapsed time (from t0 to now) in seconds
        const auto now = clock::now();
        const double t = std::chrono::duration<double>(now - t0).count(); 

        constexpr double pi = 3.14159265358979323846;
        const double y = offset + amplitude * std::sin(2.0 * pi * freqHz * t + phaseRad);
        return y;
        }
    };

} // namespace industrial