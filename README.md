# industrial-sensor-sim

A micro C++17/20 project simulating temperature and pressure readings, buffering them in a lock-free ring buffer, processing with a moving average filter, and publishing results to an MQTT broker.

## Features
- Simulated temperature & pressure sensors 
- Lock-free SPSC ring buffer 
- Moving average filter 
- MQTT publishing 
- Minimal JSON config 

## Directory Structure
- src/        (main source)
- include/    (headers)
- test/       (unit tests)

## Build
Requires CMake >=3.14, C++17 or newer, Eclipse Paho MQTT C++.

```sh
mkdir build
cd build
cmake ..
make
make install DESTDIR=./_staging  # optional local install for packaging
```

## Run
See main.cpp for usage. MQTT broker must be running.

## Embedded vs Host notes
- SimSensor is a host-side simulator (std::chrono, std::random). On embedded, replace with hardware drivers/ISRs reading real sensors.
- SPSC ring buffers:
	- `SpscRing<T,N>`: fixed-cap, no-heap; embedded-friendly.
- Time/timestamps: prefer HAL/RTOS tick counters or device timers over `std::chrono` on MCU.
- Concurrency: prefer RTOS tasks/semaphores or a cooperative main loop; avoid `std::thread` on bare metal.
- Logging: avoid `std::cout`; use a lightweight UART logger or disable logs in firmware builds.

## License
MIT. See LICENSE.
