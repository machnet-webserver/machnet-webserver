set(target_name msg_gen)

# Add the executable
add_executable(${target_name} main.cc)

# Add the hdr_histogram source file directly
target_sources(${target_name} PRIVATE
    ../../../third_party/HdrHistogram_c/src/hdr_histogram.c
)

# Add the include directory for hdr_histogram.h
target_include_directories(${target_name} PRIVATE
    ../../../third_party/HdrHistogram_c/include
)

# Link libraries
target_link_libraries(${target_name} PUBLIC glog machnet_shim rt gflags)
