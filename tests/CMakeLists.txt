add_executable(test_spsc_ring test_spsc_ring.cpp)
target_include_directories(test_spsc_ring PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_spsc_ring PRIVATE)

# Add the test to CTest
enable_testing()
add_test(NAME SpscRingTest COMMAND test_spsc_ring)
