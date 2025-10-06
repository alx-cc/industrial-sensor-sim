cmake_minimum_required(VERSION 3.14)
project(industrial-sensor-sim VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable exceptions and RTTI for GCC/Clang builds.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
	add_compile_options(
		-fno-exceptions
		-fno-rtti
	)
endif()

add_subdirectory(src)
