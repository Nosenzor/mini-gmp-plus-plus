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

add_library(mini-gmp-plus SHARED mini-gmp.c mini-mpq.c
        BigInt.hpp
        tests/test_BigInt.cpp)
set_target_properties(mini-gmp-plus PROPERTIES VERSION 6.2)
set_target_properties(mini-gmp-plus PROPERTIES SOVERSION 6.2)

if(MINI_GMP_PLUS_WITH_TESTS)
   add_subdirectory(tests)
endif()
