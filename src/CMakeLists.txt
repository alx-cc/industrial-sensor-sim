add_executable(sensor_sim
    main.cpp
    SimSensor.cpp
    MqttPublisher.cpp
)

target_include_directories(sensor_sim PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../include
)

target_compile_features(sensor_sim PRIVATE cxx_std_17)

# Install target and headers for packaging systems (Yocto, Debian, etc.)
install(TARGETS sensor_sim RUNTIME DESTINATION bin)
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../include/ DESTINATION include)

# Threads for std::thread
find_package(Threads REQUIRED)
target_link_libraries(sensor_sim PRIVATE Threads::Threads)

# Optional: Link Eclipse Paho MQTT C (synchronous client) if available
find_library(PAHO_MQTT3C paho-mqtt3c)
if(PAHO_MQTT3C)
    message(STATUS "Found Paho MQTT C: ${PAHO_MQTT3C}")
    target_compile_definitions(sensor_sim PRIVATE PAHO_MQTT_C_AVAILABLE=1)
    target_link_libraries(sensor_sim PRIVATE ${PAHO_MQTT3C})
else()
    message(STATUS "Paho MQTT C not found; MQTT disabled at build time")
endif()

# Optional compile-time defaults for MQTT connection
# Pass at configure time, e.g.:
#   cmake -S . -B build -DMQTT_BROKER_URL=tcp://127.0.0.1:1883 -DMQTT_TOPIC=sensors/demo/readings
if(DEFINED MQTT_BROKER_URL)
    target_compile_definitions(sensor_sim PRIVATE MQTT_BROKER_URL="${MQTT_BROKER_URL}")
endif()
if(DEFINED MQTT_TOPIC)
    target_compile_definitions(sensor_sim PRIVATE MQTT_TOPIC="${MQTT_TOPIC}")
endif()
