#pragma once
#include <cstdint>

namespace industrial {
    enum class StatusCode : std::uint8_t {
        OK = 0,
        InvalidArgument = 1,
        OutOfBounds = 2,
        UnknownError = 3
    };

    class Status {
    public:
        // Factory helpers (named constructors)
    static constexpr Status ok() { return Status(StatusCode::OK); }
    static constexpr Status invalid() { return Status(StatusCode::InvalidArgument); }

        // Read-only observers
    constexpr bool is_ok() const { return status_ == StatusCode::OK; }
    constexpr StatusCode code() const { return status_; }

    private:
        // Keep construction internal so invariants are enforced via factories
    constexpr explicit Status(StatusCode c) : status_(c) {}

        StatusCode status_{StatusCode::OK};
    };
}