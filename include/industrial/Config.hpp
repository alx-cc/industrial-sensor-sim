#pragma once
#include <cstdint>

namespace industrial {

// Centralized capacities to avoid magic numbers and multiple template instantiations.
constexpr std::uint32_t kRingCapacity   = 256;
constexpr std::uint32_t kMaxAvgWindow   = 256;

} // namespace industrial
