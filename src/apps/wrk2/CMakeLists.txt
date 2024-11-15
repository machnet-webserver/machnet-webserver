cmake_minimum_required(VERSION 3.0)

project(wrk2 VERSION 0.0.1 DESCRIPTION "High-performance HTTP benchmarking tool" LANGUAGES C)

# Include testing support (disabled for third-party modules)
include(CTest)
set(BUILD_TESTING OFF) # Disable testing for third-party modules

# Unique binary directory variables
set(WRK2_BINARY_DIR "${CMAKE_BINARY_DIR}/src/apps/wrk2_build")

# Debugging messages for paths
message(STATUS "Source dir for wrk2: ${CMAKE_SOURCE_DIR}/src/apps/wrk2")
message(STATUS "Binary dir for wrk2: ${WRK2_BINARY_DIR}")

# Ensure binary directory is not reused
if(EXISTS "${WRK2_BINARY_DIR}")
    file(REMOVE_RECURSE "${WRK2_BINARY_DIR}")
    message(STATUS "Cleaned existing binary directory: ${WRK2_BINARY_DIR}")
endif()

# Define source files
set(WRK2_SOURCES
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/ae.c"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/ae_epoll.c"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/aprintf.c"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/hdr_histogram.c"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/http_parser.c"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/net.c"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/script.c"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/ssl.c"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/stats.c"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/tinymt64.c"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/units.c"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/wrk.c" # Entry point
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/zmalloc.c"
)

# Define header files for clarity (optional, but improves visibility)
set(WRK2_HEADERS
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/ae.h"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/aprintf.h"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/config.h"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/hdr_histogram.h"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/http_parser.h"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/main.h"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/net.h"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/script.h"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/ssl.h"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/stats.h"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/tinymt64.h"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/units.h"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/wrk.h"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/zmalloc.h"
)

# Find Lua
find_package(Lua REQUIRED)

# Define executable
add_executable(wrk2 ${WRK2_SOURCES})

# Link libraries
target_link_libraries(wrk2 PRIVATE gflags glog HdrHistogram_c ssl crypto ${LUA_LIBRARIES})

# Include directories
target_include_directories(wrk2 PRIVATE
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src"
    "${CMAKE_SOURCE_DIR}/third_party/gflags/include"
    "${CMAKE_SOURCE_DIR}/third_party/glog/src"
    "${CMAKE_SOURCE_DIR}/third_party/HdrHistogram_c/include"
    ${LUA_INCLUDE_DIR}
)

# Debug message to confirm the target creation
message(STATUS "wrk2 target successfully created.")
message(STATUS "Using Lua include directory: ${LUA_INCLUDE_DIR}")
message(STATUS "Using Lua libraries: ${LUA_LIBRARIES}")
