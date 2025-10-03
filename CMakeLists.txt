cmake_minimum_required(VERSION 3.14)
project(industrial-sensor-sim VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Globally disable exceptions (and RTTI) for GCC/Clang builds.
# This applies to all targets in this project (and subdirectories).
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
	add_compile_options(
		-fno-exceptions
		-fno-rtti
		# Optional size-reducing options
		# -fno-unwind-tables
		# -fno-asynchronous-unwind-tables
	)
endif()

add_subdirectory(src)