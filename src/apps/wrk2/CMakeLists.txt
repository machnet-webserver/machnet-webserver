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

# Add LuaJIT
set(LUAJIT_SRC_DIR "${CMAKE_SOURCE_DIR}/src/apps/wrk2/deps/luajit/src")
set(LUAJIT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src/apps/wrk2/deps/luajit/src")
set(LUAJIT_LIB_DIR "${CMAKE_BINARY_DIR}/src/apps/wrk2/deps/luajit_build")
include_directories(${LUAJIT_INCLUDE_DIR})

# Define LuaJIT build target
add_custom_command(
    OUTPUT ${LUAJIT_LIB_DIR}/libluajit.a
    COMMAND make -C ${LUAJIT_SRC_DIR}
    WORKING_DIRECTORY ${LUAJIT_SRC_DIR}
    COMMENT "Building LuaJIT..."
)

add_custom_target(build_luajit ALL DEPENDS ${LUAJIT_LIB_DIR}/libluajit.a)

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

# Define executable
add_executable(wrk2 ${WRK2_SOURCES})

# Ensure LuaJIT is built before wrk2
add_dependencies(wrk2 build_luajit)

# Link libraries
target_link_libraries(wrk2 PRIVATE gflags glog HdrHistogram_c ssl crypto ${LUAJIT_LIB_DIR}/libluajit.a)

# Include directories
target_include_directories(wrk2 PRIVATE
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src"
    "${CMAKE_SOURCE_DIR}/third_party/gflags/include"
    "${CMAKE_SOURCE_DIR}/third_party/glog/src"
    "${CMAKE_SOURCE_DIR}/third_party/HdrHistogram_c/include"
    "${CMAKE_SOURCE_DIR}/src/ext" # Include machnet.h directory
    ${LUAJIT_INCLUDE_DIR}
)

# Debug message to confirm the target creation
message(STATUS "wrk2 target successfully created.")
message(STATUS "Using LuaJIT include directory: ${LUAJIT_INCLUDE_DIR}")
message(STATUS "LuaJIT library path: ${LUAJIT_LIB_DIR}/libluajit.a")
