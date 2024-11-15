cmake_minimum_required(VERSION 3.0)

project(juggler VERSION 0.0.1 DESCRIPTION "Packet Juggling framework in C++" LANGUAGES CXX C)

# Include testing support (disabled for third-party modules)
include(CTest)
set(BUILD_TESTING OFF) # Disable testing for third-party modules

# Unique binary directory variables
set(WRK2_BINARY_DIR "${CMAKE_BINARY_DIR}/src/apps/wrk2_build")

# Prevent duplicate `add_subdirectory`
if(TARGET wrk2)
    message(STATUS "wrk2 target already exists. Skipping redundant addition.")
else()
    if(EXISTS "${CMAKE_SOURCE_DIR}/src/apps/wrk2")
        add_subdirectory("${CMAKE_SOURCE_DIR}/src/apps/wrk2" "${WRK2_BINARY_DIR}")
        message(STATUS "Added wrk2 target.")
    else()
        message(WARNING "wrk2 directory not found. Skipping wrk2 target.")
    endif()
endif()

# Link libraries for wrk2
if(TARGET wrk2)
    target_link_libraries(wrk2 PRIVATE gflags glog HdrHistogram_c)
    include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/src/apps/wrk2")
    message(STATUS "Linked gflags, glog, and HdrHistogram_c to wrk2.")
endif()
