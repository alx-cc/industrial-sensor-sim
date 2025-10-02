#pragma once

#include <stdint.h> // replace with platform types if needed on embedded

// Embedded-minded, no-heap moving average for float
// - Runtime-configurable window size (1..MAX_N) with fixed maximum capacity
// - O(1) update using running sum and circular buffer
// - No exceptions; no dynamic allocation; minimal std use

namespace industrial {

template <uint32_t MAX_N> // using template for no-heap storage while still exposing as a compile-time option
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
			return sum_ / (float)count_;
		} else {
			float old = buf_[head_];
			sum_ += x - old;
			buf_[head_] = x;
			head_ = (head_ + 1u) % win_;
			return sum_ / (float)win_;
		}
	}

	float get() const {
		if (count_ == 0u) return 0.0f;
		float denom = (float)(count_ < win_ ? count_ : win_);
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
