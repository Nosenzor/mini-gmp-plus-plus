# A CMakeFile for mini-gmp
# Bruno Levy, November 2025
#
# To compile:
#   cmake -Bbuild
#   cmake --build build
#
# To compile and run tests: (needs gmp, see comments in tests/CMakeLists.txt)
#   cmake -Bbuild -DMINI_GMP_PLUS_WITH_TESTS=1
#   cmake --build build
#   cd build/tests; ctest -C config
#     where config is one of Release, Debug

cmake_minimum_required(VERSION 3.5...3.30)
project(mini-gmp-plus)

add_library(mini-gmp-plus SHARED mini-gmp.c mini-mpq.c)
set_target_properties(mini-gmp-plus PROPERTIES VERSION 6.2)
set_target_properties(mini-gmp-plus PROPERTIES SOVERSION 6.2)

# Add C++ wrapper test executable
add_executable(test_MiniMPZ tests/test_MiniMPZ.cpp)
target_link_libraries(test_MiniMPZ mini-gmp-plus)

if(MINI_GMP_PLUS_WITH_TESTS)
   add_subdirectory(tests)
endif()
