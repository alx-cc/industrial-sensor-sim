# industrial-sensor-sim

A micro C++17/20 project simulating temperature and pressure readings, buffering them in a lock-free ring buffer, processing with a moving average filter, and publishing results to an MQTT broker.

## Features
- Simulated temperature & pressure sensors
- Lock-free SPSC ring buffer
- Moving average filter
- MQTT publishing
- Minimal JSON config
- Unit tests (GoogleTest)

## Directory Structure
- src/        (main source)
- include/    (headers)
- test/       (unit tests)

## Build
Requires CMake >=3.14, C++17 or newer, GoogleTest, and an MQTT client library (e.g., Eclipse Paho MQTT C++).

```sh
mkdir build
cd build
cmake ..
make
```

## Run
See main.cpp for usage. MQTT broker must be running (e.g., Eclipse Mosquitto).

## License
MIT
