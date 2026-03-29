# A CMakeFile for mini-gmp test programs
# Bruno Levy, November 2025

# GMP is needed by the test programs (that do the same computations
#  with gmp and mini-gmp-plus and compare the results)
# GMP is found by pkgconfig (works under Linux, Mac, and Windows)
# Under windows, GMP is installed by vcpkg,
#  For this to work under Windows, one needs to:
#   - add VCPKG_ROOT/installed/x64-windows/tools/pkgconf to the path
#   - set PKG_CONFIG_PATH to VCPKG_ROOT/installed/x64-windows/<conf>/lib/pkgconfig
#  where <conf> is debug for debug (as you may have guessed) and nothing for release
#  In addition, for the test programs to find their DLLs,
#  one needs to add to the path:
#   - VCPKG_ROOT/installed/x64-windows/<conf>/bin  (where vcpkg installs the DLLs)
#   - the directory where libmini-gmp-plus.dll will be generated
#        that is, ${{ github.workspace }}/build/${{ matrix.config }}
#  see the build.yml github action, 'Set paths' step

find_package(PkgConfig REQUIRED)
pkg_check_modules(gmp REQUIRED IMPORTED_TARGET gmp)
include_directories(${gmp_INCLUDE_DIRS})

# These three files are systematically used by each test, made it an object lib
add_library(mini-gmp-plus-test-lib OBJECT hex-random.c mini-random.c testutils.c)

set(MINI_GMP_TESTS
   t-add t-sub t-mul t-invert t-div t-div_2exp
   t-double t-cmp_d t-gcd t-lcm t-import t-comb t-signed
   t-sqrt t-root t-powm t-logops t-bitops t-scan t-str
   t-reuse t-aorsmul t-limbs t-cong t-pprime_p t-lucm
   t-mpq_addsub t-mpq_muldiv t-mpq_muldiv_2exp t-mpq_str
   t-mpq_double test_simd_compatibility
)

# Add debug test
add_executable(debug_simd_test debug_simd_test.cpp test_nonsimd_wrappers.c)
target_include_directories(debug_simd_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    debug_simd_test mini-gmp-plus PkgConfig::gmp
)
if(UNIX)
   target_link_libraries(debug_simd_test m)
endif()
add_test(NAME debug_simd_test COMMAND debug_simd_test)
set_tests_properties(debug_simd_test PROPERTIES TIMEOUT 30)

foreach(T ${MINI_GMP_TESTS})
   if(${T} STREQUAL "test_simd_compatibility")
       # Special handling for SIMD compatibility test
       add_executable(${T} ${T}.cpp test_nonsimd_wrappers.c)
       target_include_directories(${T} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
       target_link_libraries(
           ${T} mini-gmp-plus PkgConfig::gmp
       )
   else()
       add_executable(${T} ${T}.c)
       target_link_libraries(
           ${T} mini-gmp-plus mini-gmp-plus-test-lib PkgConfig::gmp
       )
   endif()
   
   if(UNIX)
      target_link_libraries(${T} m)
   endif()
   add_test(NAME ${T} COMMAND ${T})
   set_tests_properties(${T} PROPERTIES TIMEOUT 30)
endforeach()
