cmake_minimum_required(VERSION 3.0)

project(juggler VERSION 0.0.1 DESCRIPTION "Packet Juggling framework in C++" LANGUAGES CXX C)

# Include testing support (disabled for third-party modules)
include(CTest)
set(BUILD_TESTING OFF) # Disable testing for third-party modules

# Find and configure GFlags
set(BUILD_gflags_nothreads_LIBS ON)
set(BUILD_gflags_LIBS ON)
add_subdirectory(third_party/gflags EXCLUDE_FROM_ALL) # Exclude from all builds
include_directories(SYSTEM "${CMAKE_CURRENT_BINARY_DIR}/third_party/gflags/include/")
find_package(gflags REQUIRED)

# Configure GLog
add_subdirectory(third_party/glog EXCLUDE_FROM_ALL) # Exclude from all builds
include_directories(SYSTEM third_party/glog/src)
include_directories(SYSTEM ${CMAKE_BINARY_DIR}/third_party/glog)

# Configure GoogleTest
add_subdirectory(third_party/googletest EXCLUDE_FROM_ALL) # Exclude from all builds
include_directories(SYSTEM third_party/googletest/googlemock/include)

# Configure Google Benchmark
set(BENCHMARK_ENABLE_INSTALL OFF)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Suppressing benchmark's tests" FORCE)
add_subdirectory(third_party/googlebench EXCLUDE_FROM_ALL) # Exclude from all builds
include_directories(SYSTEM third_party/googlebench/include)

# Configure HdrHistogram
set(HDR_HISTOGRAM_BUILD_PROGRAMS OFF CACHE BOOL "Minimize HDR histogram build")
set(HDR_LOG_REQUIRED OFF CACHE BOOL "Disable HDR histogram's log to avoid zlib dependency")
add_subdirectory(third_party/HdrHistogram_c EXCLUDE_FROM_ALL) # Exclude from all builds
include_directories(SYSTEM third_party/HdrHistogram_c/include/)

# Configure xxHash (only include headers as needed)
include_directories(SYSTEM third_party/xxHash)

# Enable main project testing only
set(BUILD_TESTING ON)

# Add core source directories
add_subdirectory(src)

# Add wrk2 (conditionally included if required)
if(EXISTS "${PROJECT_SOURCE_DIR}/src/apps/wrk2")
    add_subdirectory(src/apps/wrk2) # Ensure wrk2 gets added if it exists
    message(STATUS "Added wrk2 target.")
else()
    message(WARNING "wrk2 directory not found. Skipping wrk2 target.")
endif()

# Re-adding third_party/gflags for safety (if required explicitly)
if(EXISTS "${PROJECT_SOURCE_DIR}/third_party/gflags")
    add_subdirectory(third_party/gflags EXCLUDE_FROM_ALL)
endif()

# Add wrk2 executable
if(TARGET wrk2)
    target_link_libraries(wrk2 PRIVATE gflags glog HdrHistogram_c)
    include_directories(${GFLAGS_INCLUDE_DIRS})
endif()
