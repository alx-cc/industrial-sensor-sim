#include <iostream>
#include <chrono>

#include "industrial/Status.hpp"
#include "industrial/sensor_sample.hpp"

int main() {
    using namespace industrial;
    using clock = std::chrono::steady_clock;

    SensorSample sample{
        .ts = clock::now(),
        .temperature_c = 23.5f,
        .pressure_kpa = 101.3f,
    };

    // Print a simple line (we won't format time_point here)
    std::cout << "Sample: T=" << sample.temperature_c
              << " C, P=" << sample.pressure_kpa << " kPa\n";

    // Exercise Status
    Status s_ok = Status::ok();
    if (s_ok.is_ok()) {
        std::cout << "Status OK" << std::endl;
    }

    Status s_bad = Status::invalid();
    if (!s_bad.is_ok()) {
        std::cout << "Status invalid (code=" << static_cast<int>(s_bad.code()) << ")" << std::endl;
    }

    return 0;
}
