cmake_minimum_required(VERSION 3.0)

project(juggler VERSION 0.0.1 DESCRIPTION "Packet Juggling framework in C++" LANGUAGES CXX C)

# Include testing support (disabled for third-party modules)
include(CTest)
set(BUILD_TESTING OFF) # Disable testing for third-party modules

# Find and configure GFlags
set(BUILD_gflags_nothreads_LIBS ON)
set(BUILD_gflags_LIBS ON)
add_subdirectory(third_party/gflags EXCLUDE_FROM_ALL)
include_directories(SYSTEM "${CMAKE_CURRENT_BINARY_DIR}/third_party/gflags/include")

find_package(gflags REQUIRED)

# Configure GLog
add_subdirectory(third_party/glog EXCLUDE_FROM_ALL)
include_directories(SYSTEM third_party/glog/src)
include_directories(SYSTEM ${CMAKE_BINARY_DIR}/third_party/glog)

# Configure GoogleTest
add_subdirectory(third_party/googletest EXCLUDE_FROM_ALL)
include_directories(SYSTEM third_party/googletest/googlemock/include)

# Configure Google Benchmark
set(BENCHMARK_ENABLE_INSTALL OFF)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Suppressing benchmark's tests" FORCE)
add_subdirectory(third_party/googlebench EXCLUDE_FROM_ALL)
include_directories(SYSTEM third_party/googlebench/include)

# Configure HdrHistogram
set(HDR_HISTOGRAM_BUILD_PROGRAMS OFF CACHE BOOL "Minimize HDR histogram build")
set(HDR_LOG_REQUIRED OFF CACHE BOOL "Disable HDR histogram's log to avoid zlib dependency")
add_subdirectory(third_party/HdrHistogram_c EXCLUDE_FROM_ALL)
include_directories(SYSTEM third_party/HdrHistogram_c/include/)

# Configure xxHash
include_directories(SYSTEM third_party/xxHash)

# Enable testing for main project only
set(BUILD_TESTING ON)

# Add core source directories
add_subdirectory(src)

# Add wrk2 project if it exists
set(WRK2_BINARY_DIR "${CMAKE_BINARY_DIR}/src/apps/wrk2_build")
if(EXISTS "${CMAKE_SOURCE_DIR}/src/apps/wrk2")
    if(NOT TARGET wrk2) # Ensure wrk2 is added only once
        add_subdirectory("${CMAKE_SOURCE_DIR}/src/apps/wrk2" "${WRK2_BINARY_DIR}")
        message(STATUS "wrk2 target added to the build.")
    else()
        message(STATUS "wrk2 target already exists. Skipping redundant addition.")
    endif()
else()
    message(WARNING "wrk2 directory not found. Skipping wrk2 target.")
endif()

# Link dependencies to wrk2
if(TARGET wrk2)
    target_link_libraries(wrk2 PRIVATE gflags glog HdrHistogram_c ssl crypto)
    include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/src/apps/wrk2")
    message(STATUS "Linked dependencies (gflags, glog, HdrHistogram_c) to wrk2.")
endif()
