set(target_name msg_gen)

# Add the executable
add_executable(${target_name} main.cc)

# Include directories for HdrHistogram
target_include_directories(${target_name} PUBLIC
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src"  # Path to hdr_histogram.h
)

# Link the required libraries
target_link_libraries(${target_name} PUBLIC
    glog              # Logging library
    machnet_shim      # Machnet-specific library
    rt                # Real-time library
    gflags            # Command-line flags
)

# Add hdr_histogram.c as a source file if needed
target_sources(${target_name} PRIVATE
    "${CMAKE_SOURCE_DIR}/src/apps/wrk2/src/hdr_histogram.c"
)

# Debugging message for confirmation
message(STATUS "Configured ${target_name} with HdrHistogram support.")
