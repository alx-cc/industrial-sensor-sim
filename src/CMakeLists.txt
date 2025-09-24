add_executable(sensor_sim
    main.cpp
)

target_include_directories(sensor_sim PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../include
)

target_compile_features(sensor_sim PRIVATE cxx_std_17)
