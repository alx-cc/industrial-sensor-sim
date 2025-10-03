/**
 * @file industrial/SpscRing.hpp
 * @brief Fixed-capacity single-producer/single-consumer ring buffer with no dynamic allocation.
 *
 * @note:
 * - Template: SpscRing<T, N>; N is a power-of-two >= 2.
 * - Storage: internal array (no heap), constant-time operations.
 * - Element type: T required to be trivially copyable when <type_traits> is available
 *   (define INDUSTRIAL_DISABLE_TRIVIALITY_GUARD to bypass on limited toolchains).
 * 
 * - API: try_push(const T&), try_pop(T&), size(), empty(), full(), clear(); all non-blocking.
 * - Concurrency: lock-free SPSC; exactly one producer thread and one consumer thread
 */
#pragma once

#include <atomic>
#include <cassert>
#include <stdint.h>
#include <type_traits>

namespace industrial
{

    template <typename T, uint32_t N>
    class SpscRing
    {
    public:
#if !defined(INDUSTRIAL_DISABLE_TRIVIALITY_GUARD) && defined(INDUSTRIAL_HAS_TYPE_TRAITS)
        static_assert(std::is_trivially_copyable_v<T>, "SpscRing requires trivially copyable T");
#endif
        static_assert(N >= 2, "Capacity must be >= 2");
        static_assert((N & (N - 1)) == 0, "Capacity must be a power of two");

        SpscRing() = default;

        uint32_t capacity() const { return N; }

        bool try_push(const T &v)
        {
            const uint32_t head = head_.load();
            uint32_t tail = tail_.load();
            if ((head - tail) == N)
            {
                return false; // full
            }
            buf_[(head)&mask_] = v;
            head_.store(head + 1);
            return true;
        }

        bool try_pop(T &out)
        {
            const uint32_t head = head_.load();
            uint32_t tail = tail_.load();
            if (head == tail)
            {
                return false; // empty
            }
            out = buf_[(tail)&mask_];
            tail_.store(tail + 1);
            return true;
        }

        bool empty() const { return size() == 0; }
        bool full() const { return size() == N; }

        uint32_t size() const
        {
            const uint32_t head = head_.load();
            uint32_t tail = tail_.load();
            return (head - tail);
        }

        void clear()
        {
            uint32_t h = head_.load();
            tail_.store(h);
        }

    private:
        static constexpr uint32_t mask_ = N - 1;
        T buf_[N]{};                    // no-heap storage
        std::atomic<uint32_t> head_{0}; // producer-only writer
        std::atomic<uint32_t> tail_{0}; // consumer-only writer
    };

} // namespace industrial
