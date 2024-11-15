set(target_name msg_gen)

# Add the executable
add_executable(${target_name} main.cc)

# Add include directories
target_include_directories(${target_name} PUBLIC
    "${CMAKE_SOURCE_DIR}/src/ext"                               # For other includes
    "${CMAKE_SOURCE_DIR}/src/include"                           # General includes
    "${CMAKE_SOURCE_DIR}/third_party/HdrHistogram_c/include"    # For hdr/hdr_histogram.h
)

# Link the required libraries
target_link_libraries(${target_name} PUBLIC
    glog              # Logging library
    machnet_shim      # Machnet-specific library
    rt                # Real-time library
    gflags            # Command-line flags
)
