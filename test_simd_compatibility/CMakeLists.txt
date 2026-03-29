cmake_minimum_required(VERSION 3.5)
project(test_simd_compatibility)

# Find the mini-gmp-plus library
find_package(mini-gmp-plus REQUIRED)

# Add test executable
add_executable(test_simd_compatibility test_simd_compatibility.cpp test_nonsimd_wrappers.c)

target_link_libraries(test_simd_compatibility mini-gmp-plus)

target_include_directories(test_simd_compatibility PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${mini-gmp-plus_INCLUDE_DIRS}
)