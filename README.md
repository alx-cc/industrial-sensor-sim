# industrial-sensor-sim

A micro C++17/20 project simulating temperature and pressure readings, buffering them in a lock-free ring buffer, processing with a moving average filter, and publishing results to an MQTT broker.

## Features
- Simulated temperature & pressure sensors 
- Lock-free SPSC ring buffer (in-progress)
- Moving average filter (in-progress)
- MQTT publishing (in-progress)
- Minimal JSON config (in-progress)
- Unit tests (GoogleTest) (in-progress)

## Directory Structure
- src/        (main source)
- include/    (headers)
- test/       (unit tests)

## Build
Requires CMake >=3.14, C++17 or newer. Will require GoogleTest (when added), and an MQTT client library (e.g., Eclipse Paho MQTT C++) (when added).

```sh
mkdir build
cd build
cmake ..
make
make install DESTDIR=./_staging  # optional local install for packaging
```

## Run
See main.cpp for usage. MQTT broker must be running (e.g., Eclipse Mosquitto).

## Embedded vs Host notes
- SimSensor is a host-side simulator (std::chrono, std::random). On embedded, replace with hardware drivers/ISRs reading real sensors.
- SPSC ring buffers:
	- `SpscRing<T,N>`: fixed-cap, no-heap; embedded-friendly.
	- `SpscRing` (heap): allocate once in ctor; fine for host/tools.
- Time/timestamps: prefer HAL/RTOS tick counters or device timers over `std::chrono` on MCU.
- Concurrency: prefer RTOS tasks/semaphores or a cooperative main loop; avoid `std::thread` on bare metal.
- Logging: avoid `std::cout`; use a lightweight UART logger or disable logs in firmware builds.

## License
MIT. See LICENSE.
