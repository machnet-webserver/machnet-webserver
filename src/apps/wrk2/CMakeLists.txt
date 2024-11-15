cmake_minimum_required(VERSION 3.0)

project(juggler VERSION 0.0.1 DESCRIPTION "Packet Juggling framework in C++" LANGUAGES CXX C)

# Include testing support (disabled for third-party modules)
include(CTest)
set(BUILD_TESTING OFF) # Disable testing for third-party modules

# Add unique subdirectory paths for dependencies
set(GFLAGS_BINARY_DIR "${CMAKE_BINARY_DIR}/third_party/gflags_build")
set(GLOG_BINARY_DIR "${CMAKE_BINARY_DIR}/third_party/glog_build")
set(GTEST_BINARY_DIR "${CMAKE_BINARY_DIR}/third_party/googletest_build")
set(BENCHMARK_BINARY_DIR "${CMAKE_BINARY_DIR}/third_party/googlebench_build")
set(HDRHISTOGRAM_BINARY_DIR "${CMAKE_BINARY_DIR}/third_party/HdrHistogram_c_build")
set(WRK2_BINARY_DIR "${CMAKE_BINARY_DIR}/src/apps/wrk2_build")

# Find and configure GFlags
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/gflags")
    if (NOT TARGET gflags)
        add_subdirectory("${CMAKE_SOURCE_DIR}/third_party/gflags" "${GFLAGS_BINARY_DIR}" EXCLUDE_FROM_ALL)
        include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/third_party/gflags/include")
        message(STATUS "Configured GFlags.")
    endif()
else()
    message(FATAL_ERROR "third_party/gflags directory not found.")
endif()

# Configure GLog
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/glog")
    if (NOT TARGET glog)
        add_subdirectory("${CMAKE_SOURCE_DIR}/third_party/glog" "${GLOG_BINARY_DIR}" EXCLUDE_FROM_ALL)
        include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/third_party/glog/src")
        include_directories(SYSTEM "${GLOG_BINARY_DIR}")
        message(STATUS "Configured GLog.")
    endif()
else()
    message(FATAL_ERROR "third_party/glog directory not found.")
endif()

# Configure GoogleTest
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/googletest")
    if (NOT TARGET gtest)
        add_subdirectory("${CMAKE_SOURCE_DIR}/third_party/googletest" "${GTEST_BINARY_DIR}" EXCLUDE_FROM_ALL)
        include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/third_party/googletest/googlemock/include")
        message(STATUS "Configured GoogleTest.")
    endif()
else()
    message(WARNING "third_party/googletest directory not found. Skipping.")
endif()

# Configure Google Benchmark
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/googlebench")
    if (NOT TARGET benchmark)
        add_subdirectory("${CMAKE_SOURCE_DIR}/third_party/googlebench" "${BENCHMARK_BINARY_DIR}" EXCLUDE_FROM_ALL)
        include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/third_party/googlebench/include")
        message(STATUS "Configured Google Benchmark.")
    endif()
else()
    message(WARNING "third_party/googlebench directory not found. Skipping.")
endif()

# Configure HdrHistogram
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/HdrHistogram_c")
    if (NOT TARGET hdr_histogram)
        add_subdirectory("${CMAKE_SOURCE_DIR}/third_party/HdrHistogram_c" "${HDRHISTOGRAM_BINARY_DIR}" EXCLUDE_FROM_ALL)
        include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/third_party/HdrHistogram_c/include/")
        message(STATUS "Configured HdrHistogram.")
    endif()
else()
    message(WARNING "third_party/HdrHistogram_c directory not found. Skipping.")
endif()

# Add core source directories
if(EXISTS "${CMAKE_SOURCE_DIR}/src")
    if (NOT TARGET core)
        add_subdirectory("${CMAKE_SOURCE_DIR}/src" "${CMAKE_BINARY_DIR}/src_build")
        message(STATUS "Added core source directories.")
    endif()
else()
    message(FATAL_ERROR "Core source directory src not found.")
endif()

# Add wrk2 target conditionally
if(EXISTS "${CMAKE_SOURCE_DIR}/src/apps/wrk2")
    if (NOT TARGET wrk2)
        add_subdirectory("${CMAKE_SOURCE_DIR}/src/apps/wrk2" "${WRK2_BINARY_DIR}")
        message(STATUS "Added wrk2 target.")
    endif()
else()
    message(WARNING "wrk2 directory not found. Skipping wrk2 target.")
endif()

# Link libraries for wrk2
if(TARGET wrk2)
    target_link_libraries(wrk2 PRIVATE gflags glog HdrHistogram_c)
    include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/src/apps/wrk2")
    message(STATUS "Linked gflags, glog, and HdrHistogram_c to wrk2.")
endif()
