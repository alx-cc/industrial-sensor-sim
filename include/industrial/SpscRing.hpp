/**
 * @file industrial/SpscRing.hpp
 * @brief Fixed-capacity single-producer/single-consumer ring buffer with no dynamic allocation.
 *
 * @note:
 * - Template: SpscRing<T, N>
 * - Storage: internal array (no heap), constant-time operations
 * - Element type: T required to be trivially copyable when <type_traits> is available
 *   (define INDUSTRIAL_DISABLE_TRIVIALITY_GUARD to bypass on limited toolchains).
 * 
 * - API: try_push(const T&), try_pop(T&), size(), empty(), full(), clear(); all non-blocking.
 * - Concurrency: lock-free SPSC; exactly one producer thread and one consumer thread. Uses 
 *   std::memory_order to synchronize producer/consumer head/tail updates without the need for locks
 */
#pragma once

#include <atomic>
#include <cstdint>
#include <type_traits>

namespace industrial
{

    template <typename T, uint32_t N>
    class SpscRing
    {
    public:
#if !defined(INDUSTRIAL_DISABLE_TRIVIALITY_GUARD)
    static_assert(std::is_trivially_copyable_v<T>, "SpscRing requires trivially copyable T");
#endif
    static_assert(N >= 2, "Capacity must be >= 2");

        SpscRing() = default;

        uint32_t capacity() const { return N; }

        bool try_push(const T &v)
        {
            // Producer sees consumer's progress (pairs with consumer's release on tail)
            const uint32_t head = head_.load(std::memory_order_relaxed);
            uint32_t tail = tail_.load(std::memory_order_acquire); // acquire: observe latest consumed tail
            if ((head - tail) == N)
            {
                return false; // full
            }
            buf_[head % N] = v;
            // Publish produced item (pairs with consumer's acquire on head)
            head_.store(head + 1, std::memory_order_release); // release: make buf write visible before head advance
            return true;
        }

        bool try_pop(T &out)
        {
            // Consumer sees producer's progress (pairs with producer's release on head)
            const uint32_t head = head_.load(std::memory_order_acquire); // acquire: observe produced head and data
            uint32_t tail = tail_.load(std::memory_order_relaxed);
            if (head == tail)
            {
                return false; // empty
            }
            out = buf_[tail % N];
            // Publish consumption (pairs with producer's acquire on tail)
            tail_.store(tail + 1, std::memory_order_release); // release: allow producer to see freed slot
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
    T buf_[N]{};                    // no-heap storage
        std::atomic<uint32_t> head_{0}; // producer-only writer
        std::atomic<uint32_t> tail_{0}; // consumer-only writer
    };

} // namespace industrial
