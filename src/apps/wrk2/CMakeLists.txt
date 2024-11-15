cmake_minimum_required(VERSION 3.0)

project(juggler VERSION 0.0.1 DESCRIPTION "Packet Juggling framework in C++" LANGUAGES CXX C)

# Include testing support (disabled for third-party modules)
include(CTest)
set(BUILD_TESTING OFF) # Disable testing for third-party modules

# Find and configure GFlags
set(BUILD_gflags_nothreads_LIBS ON)
set(BUILD_gflags_LIBS ON)

# Check if third_party/gflags exists and include it
if(EXISTS "${PROJECT_SOURCE_DIR}/third_party/gflags")
    add_subdirectory(third_party/gflags EXCLUDE_FROM_ALL)
    include_directories(SYSTEM "${PROJECT_SOURCE_DIR}/third_party/gflags/include")
    message(STATUS "Configured GFlags from third_party.")
else()
    message(FATAL_ERROR "third_party/gflags directory not found.")
endif()

# Configure GLog
if(EXISTS "${PROJECT_SOURCE_DIR}/third_party/glog")
    add_subdirectory(third_party/glog EXCLUDE_FROM_ALL)
    include_directories(SYSTEM third_party/glog/src)
    include_directories(SYSTEM ${CMAKE_BINARY_DIR}/third_party/glog)
    message(STATUS "Configured GLog from third_party.")
else()
    message(FATAL_ERROR "third_party/glog directory not found.")
endif()

# Configure GoogleTest
if(EXISTS "${PROJECT_SOURCE_DIR}/third_party/googletest")
    add_subdirectory(third_party/googletest EXCLUDE_FROM_ALL)
    include_directories(SYSTEM third_party/googletest/googlemock/include)
    message(STATUS "Configured GoogleTest from third_party.")
else()
    message(WARNING "third_party/googletest directory not found. Skipping.")
endif()

# Configure Google Benchmark
set(BENCHMARK_ENABLE_INSTALL OFF)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Suppressing benchmark's tests" FORCE)
if(EXISTS "${PROJECT_SOURCE_DIR}/third_party/googlebench")
    add_subdirectory(third_party/googlebench EXCLUDE_FROM_ALL)
    include_directories(SYSTEM third_party/googlebench/include)
    message(STATUS "Configured Google Benchmark from third_party.")
else()
    message(WARNING "third_party/googlebench directory not found. Skipping.")
endif()

# Configure HdrHistogram
set(HDR_HISTOGRAM_BUILD_PROGRAMS OFF CACHE BOOL "Minimize HDR histogram build")
set(HDR_LOG_REQUIRED OFF CACHE BOOL "Disable HDR histogram's log to avoid zlib dependency")
if(EXISTS "${PROJECT_SOURCE_DIR}/third_party/HdrHistogram_c")
    add_subdirectory(third_party/HdrHistogram_c EXCLUDE_FROM_ALL)
    include_directories(SYSTEM third_party/HdrHistogram_c/include/)
    message(STATUS "Configured HdrHistogram from third_party.")
else()
    message(WARNING "third_party/HdrHistogram_c directory not found. Skipping.")
endif()

# Configure xxHash (only include headers as needed)
if(EXISTS "${PROJECT_SOURCE_DIR}/third_party/xxHash")
    include_directories(SYSTEM third_party/xxHash)
    message(STATUS "Configured xxHash headers.")
else()
    message(WARNING "third_party/xxHash directory not found. Skipping.")
endif()

# Enable main project testing only
set(BUILD_TESTING ON)

# Add core source directories
if(EXISTS "${PROJECT_SOURCE_DIR}/src")
    add_subdirectory(src)
    message(STATUS "Added core source directories.")
else()
    message(FATAL_ERROR "Core source directory src not found.")
endif()

# Add wrk2 target conditionally
if(EXISTS "${PROJECT_SOURCE_DIR}/src/apps/wrk2")
    add_subdirectory(src/apps/wrk2)
    message(STATUS "Added wrk2 target.")
else()
    message(WARNING "wrk2 directory not found. Skipping wrk2 target.")
endif()

# Link libraries for wrk2
if(TARGET wrk2)
    target_link_libraries(wrk2 PRIVATE gflags glog HdrHistogram_c)
    include_directories(SYSTEM "${PROJECT_SOURCE_DIR}/src/apps/wrk2")
    message(STATUS "Linked gflags, glog, and HdrHistogram_c to wrk2.")
endif()
