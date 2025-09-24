add_executable(sensor_sim
    main.cpp
    SimpleSPSC.cpp
    SimSensor.cpp
)

target_include_directories(sensor_sim PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../include
)

target_compile_features(sensor_sim PRIVATE cxx_std_17)

# Install target and headers for packaging systems (Yocto, Debian, etc.)
install(TARGETS sensor_sim RUNTIME DESTINATION bin)
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../include/ DESTINATION include)
