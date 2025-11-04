/**
 * @file test_spsc_ring.cpp
 * @brief Unit tests for SpscRing drop-oldest behavior.
 *
 * Tests verify:
 * - Basic push/pop operations
 * - Drop-oldest (overwrite) behavior when ring is full
 * - Size and capacity tracking
 * - Empty/full state detection
 */

#include "industrial/SpscRing.hpp"
#include <cassert>
#include <iostream>

using TestRing = industrial::SpscRing<int, 4>;

void test_basic_push_pop() {
    TestRing ring;
    
    assert(ring.empty());
    assert(!ring.full());
    assert(ring.size() == 0);
    assert(ring.capacity() == 4);
    
    ring.push(10);
    assert(ring.size() == 1);
    assert(!ring.empty());
    
    ring.push(20);
    ring.push(30);
    assert(ring.size() == 3);
    
    int val;
    assert(ring.try_pop(val));
    assert(val == 10);
    assert(ring.size() == 2);
    
    assert(ring.try_pop(val));
    assert(val == 20);
    
    assert(ring.try_pop(val));
    assert(val == 30);
    
    assert(ring.empty());
    assert(!ring.try_pop(val));  // Should fail on empty
    
    std::cout << "✓ test_basic_push_pop passed\n";
}

void test_drop_oldest() {
    TestRing ring;
    
    // Fill the ring to capacity (4 items)
    ring.push(1);
    ring.push(2);
    ring.push(3);
    ring.push(4);
    
    assert(ring.full());
    assert(ring.size() == 4);
    
    // Push more items - should overwrite oldest
    ring.push(5);  // Overwrites 1
    assert(ring.size() == 4);  // Still full
    
    ring.push(6);  // Overwrites 2
    ring.push(7);  // Overwrites 3
    
    // Pop all items - should get the newest 4: [4, 5, 6, 7]
    int val;
    assert(ring.try_pop(val));
    assert(val == 4);  // Item 1 was dropped
    
    assert(ring.try_pop(val));
    assert(val == 5);  // Item 2 was dropped
    
    assert(ring.try_pop(val));
    assert(val == 6);  // Item 3 was dropped
    
    assert(ring.try_pop(val));
    assert(val == 7);
    
    assert(ring.empty());
    
    std::cout << "✓ test_drop_oldest passed\n";
}

void test_continuous_overwrite() {
    TestRing ring;
    
    // Simulate producer running ahead of consumer
    // Push 10 items into a capacity-4 ring
    for (int i = 0; i < 10; ++i) {
        ring.push(i);
    }
    
    // Ring should contain the last 4 items: [6, 7, 8, 9]
    assert(ring.full());
    assert(ring.size() == 4);
    
    int val;
    assert(ring.try_pop(val));
    assert(val == 6);  // First 6 items (0-5) were overwritten
    
    assert(ring.try_pop(val));
    assert(val == 7);
    
    assert(ring.try_pop(val));
    assert(val == 8);
    
    assert(ring.try_pop(val));
    assert(val == 9);
    
    assert(ring.empty());
    
    std::cout << "✓ test_continuous_overwrite passed\n";
}

void test_clear() {
    TestRing ring;
    
    ring.push(100);
    ring.push(200);
    ring.push(300);
    
    assert(ring.size() == 3);
    
    ring.clear();
    
    assert(ring.empty());
    assert(ring.size() == 0);
    
    // Should be able to use ring normally after clear
    ring.push(999);
    int val;
    assert(ring.try_pop(val));
    assert(val == 999);
    
    std::cout << "✓ test_clear passed\n";
}

int main() {
    std::cout << "Running SpscRing tests...\n\n";
    
    test_basic_push_pop();
    test_drop_oldest();
    test_continuous_overwrite();
    test_clear();
    
    std::cout << "\n✓ All tests passed!\n";
    return 0;
}
