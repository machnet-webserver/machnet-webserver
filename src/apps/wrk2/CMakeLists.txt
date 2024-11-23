cmake_minimum_required(VERSION 3.0)

project(wrk2 VERSION 0.0.1 DESCRIPTION "High-performance HTTP benchmarking tool" LANGUAGES C)

# Default to Debug build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Include testing support (disabled for third-party modules)
include(CTest)
set(BUILD_TESTING OFF) # Disable testing for third-party modules

# Enable debug flags for better gdb support
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# Add defines
add_definitions( -DMACHNET )
#add_definitions( -DMACHNET_DEBUG )

# Unique binary directory variables
set(WRK2_BINARY_DIR "${CMAKE_BINARY_DIR}/src/apps/wrk2_build")

# Debugging messages for paths
message(STATUS "Source dir for wrk2: ${CMAKE_SOURCE_DIR}/src/apps/wrk2")
message(STATUS "Binary dir for wrk2: ${WRK2_BINARY_DIR}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

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
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/hdr_histogram.c" # Use the local hdr_histogram.c
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/http_parser.c"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/net.c"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/script.c"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/ssl.c"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/stats.c"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/tinymt64.c"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/units.c"
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/wrk.c" # Entry point
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/zmalloc.c"
    "${CMAKE_SOURCE_DIR}/src/ext/machnet.c" # Add machnet.c directly
)

# Define executable
add_executable(wrk2 ${WRK2_SOURCES})

# Locate Lua library and include directory
set(LUA_LIB "${CMAKE_SOURCE_DIR}/src/apps/wrk2/deps/luajit/src/libluajit.so")
set(LUA_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src/apps/wrk2/deps/luajit/src")

# Debugging messages for Lua paths
message(STATUS "Lua library found at: ${LUA_LIB}")
message(STATUS "Lua include directory found at: ${LUA_INCLUDE_DIR}")

# Link libraries
target_link_libraries(wrk2 PRIVATE gflags glog ssl crypto ${LUA_LIB} machnet_shim)

# Include directories
target_include_directories(wrk2 PRIVATE
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src"          # Source directory for wrk2
    "${CMAKE_SOURCE_DIR}/src/include"               # For pause.h
    "${CMAKE_SOURCE_DIR}/src/ext"                   # For machnet.h and related headers
    "${LUA_INCLUDE_DIR}"                            # Lua headers
    "${CMAKE_SOURCE_DIR}/third_party/gflags/include"
    "${CMAKE_SOURCE_DIR}/third_party/glog/src"
)

# Debug message
message(STATUS "wrk2 target successfully created.")
