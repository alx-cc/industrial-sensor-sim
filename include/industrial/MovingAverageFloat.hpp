/**
 * @file industrial/MovingAverageFloat.hpp
 * @brief Fixed-capacity, no-heap moving average filter for float samples.
 *
 * @tparam MAX_N Compile-time maximum capacity/window (>= 1).
 *
 * Features:
 *  - O(1) updates via running sum and circular buffer.
 *  - Runtime window size clamped to [1, MAX_N].
 *  - No exceptions; no dynamic allocation; minimal std use
 *
 * API:
 *  - set_window(n): clamps n; resets internal state.
 *  - window(), capacity(), size(): query configuration/state.
 *  - push(x): insert sample; returns current average.
 *  - get(): current average (0.0f if empty).
 *  - reset(): clear buffer and accumulators.
 *
 * @note:
 *  - While filling, average uses count of received samples; once full, uses window size.
 *  - Not thread-safe.
 *  - Time: O(1) per push/get; Memory: MAX_N floats + small metadata.
 */
#pragma once

#include <cstdint>

namespace industrial {

template <uint32_t MAX_N>
class MovingAverageFloat {
public:
	static_assert(MAX_N >= 1, "MAX_N must be >= 1");

	MovingAverageFloat() = default;

	void set_window(uint32_t n) {
		if (n < 1u) n = 1u;
		if (n > MAX_N) n = MAX_N;
		reset();
		win_ = n;
	}

	uint32_t window() const { return win_; }
	uint32_t capacity() const { return MAX_N; }
	uint32_t size() const { return count_; }
	void reset() { head_ = 0; count_ = 0; sum_ = 0.0f; }

	// Push a sample and return the current average.
	float push(float x) {
		if (count_ < win_) {
			buf_[head_] = x;
			sum_ += x;
			head_ = (head_ + 1u) % win_;
			count_ += 1u;
			return sum_ / count_;
		} else {
			float old = buf_[head_];
			sum_ += x - old;
			buf_[head_] = x;
			head_ = (head_ + 1u) % win_;
			return sum_ / win_;
		}
	}

	float get() const {
		if (count_ == 0u) return 0.0f;
		float denom = (count_ < win_ ? count_ : win_);
		return sum_ / denom;
	}

private:
	float buf_[MAX_N]{}; // no-heap storage
	uint32_t head_{0};
	uint32_t count_{0};
	uint32_t win_{1};
	float sum_{0.0f};
};

} // namespace industrial
