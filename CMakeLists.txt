cmake_minimum_required(VERSION 3.16)
project(drone_telemetry CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(telemetry src/main.cpp)

target_include_directories(telemetry PRIVATE ${CMAKE_SOURCE_DIR}/external/c_library_v2)

target_compile_options(telemetry PRIVATE -Wall -Wno-address-of-packed-member)