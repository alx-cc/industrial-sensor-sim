#pragma once
#include "sensor_sample.hpp"

namespace industrial {

class SimpleSPSC {
public:
    // For now a trivial single-slot placeholder; will evolve into ring buffer.
    bool try_push(const SensorSample& s);   // copy in
    bool try_pop(SensorSample& out);        // copy out

private:
    SensorSample slot_{}; // single element buffer for initial learning
    bool has_value_{false};
};

} // namespace industrial