cmake_minimum_required(VERSION 3.22)
project(lock-free-queues-benchmark)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CXX_STANDARD_REQUIRED 20)
set(CMAKE_CXX_STANDARD 20)

find_package(Threads REQUIRED)
find_package(Catch2 3 REQUIRED)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fconcepts-diagnostics-depth=6")

include(CTest)
include(Catch)

add_subdirectory(test)
add_subdirectory(thirdparty/mgark/test)
add_subdirectory(benchmark)