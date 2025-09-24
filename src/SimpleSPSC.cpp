#include "industrial/SimpleSPSC.hpp"

namespace industrial {

bool SimpleSPSC::try_push(const SensorSample& s) {
    if (has_value_) {
        return false; // buffer full (single slot version)
    }
    slot_ = s;
    has_value_ = true;
    return true;
}

bool SimpleSPSC::try_pop(SensorSample& out) {
    if (!has_value_) {
        return false; // empty
    }
    out = slot_;
    has_value_ = false;
    return true;
}

} // namespace industrial