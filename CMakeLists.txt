# A CMakeFile for mini-gmp
# Bruno Levy, November 2025
#
# === Using CMake presets (CMake >= 3.21, requires Ninja) ===
#
# Configure:
#   cmake --preset release              # Release, SIMD on (needs vcpkg/xsimd)
#   cmake --preset release-nosimd      # Release, SIMD off
#   cmake --preset debug               # Debug, SIMD on
#   cmake --preset debug-nosimd        # Debug, SIMD off
#   cmake --preset release-tests       # Release + SIMD + C regression tests
#   cmake --preset release-nosimd-tests # Release, no SIMD + C regression tests
#   cmake --preset asan                # ASan
#   cmake --preset ubsan               # UBSan
#   cmake --preset asan-ubsan          # ASan + UBSan
#   cmake --preset tsan                # TSan
#
# Build:   cmake --build --preset <name>
# Test:    ctest --preset <name>
#
# === Manual (no preset, legacy) ===
#
# To compile:
#   cmake -Bbuild
#   cmake --build build
#
# To compile and run tests: (needs gmp, see comments in tests/CMakeLists.txt)
#   cmake -Bbuild -DMINI_GMP_PLUS_WITH_TESTS=1
#   cmake --build build
#   ctest --test-dir build --output-on-failure
#
# To disable SIMD acceleration (enabled by default, requires xsimd e.g. brew install xsimd):
#   cmake -Bbuild -DMINI_GMP_ENABLE_SIMD=OFF
#   cmake --build build
#
# To build the standalone geometry benchmark:
#   cmake -Bbuild -DMINI_GMP_ENABLE_SIMD=ON|OFF
#   cmake --build build --target benchmark_geometry
#   ./build/benchmark_geometry

cmake_minimum_required(VERSION 3.5...3.30)
project(mini-gmp-plus VERSION 1.0.0 LANGUAGES C CXX)

include(CTest)

# Optional SIMD acceleration via xsimd
option(MINI_GMP_ENABLE_SIMD "Enable SIMD acceleration using xsimd" ON)
if(MINI_GMP_ENABLE_SIMD)
    # Try to find xsimd with CONFIG mode first
    find_package(xsimd QUIET CONFIG)
    
    # If CONFIG mode fails, try module mode
    if(NOT xsimd_FOUND)
        find_package(xsimd QUIET MODULE)
    endif()
    
    # If still not found, try pkg-config
    if(NOT xsimd_FOUND)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(xsimd QUIET xsimd)
        endif()
    endif()
    
    # If xsimd is still not found, try to find it manually
    if(NOT xsimd_FOUND)
        find_path(xsimd_INCLUDE_DIRS
            NAMES xsimd/xsimd.hpp
            PATHS
            /usr/local/include
            /usr/include
            /opt/homebrew/include
            ${CMAKE_INSTALL_PREFIX}/include
        )
        
        if(xsimd_INCLUDE_DIRS)
            set(xsimd_FOUND TRUE)
            message(STATUS "Found xsimd manually at: ${xsimd_INCLUDE_DIRS}")
        endif()
    endif()
    
    if(NOT xsimd_FOUND)
        message(FATAL_ERROR "xsimd not found. Please install xsimd (e.g., brew install xsimd) or disable SIMD with -DMINI_GMP_ENABLE_SIMD=OFF")
    else()
        message(STATUS "Found xsimd: ${xsimd_INCLUDE_DIRS}")
    endif()
endif()

# Library target
set(MINI_GMP_SOURCES mini-gmp.c mini-mpq.c)
if(MINI_GMP_ENABLE_SIMD)
    list(APPEND MINI_GMP_SOURCES mini-gmp-simd.cpp)
endif()
add_library(mini-gmp-plus SHARED ${MINI_GMP_SOURCES})
set_target_properties(mini-gmp-plus PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    PUBLIC_HEADER "mini-gmp.h;mini-mpq.h;MiniMPZ.hpp;bitops64.h"
)

# Set include directories for building and installing
target_include_directories(mini-gmp-plus
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

if(MINI_GMP_ENABLE_SIMD)
    target_compile_definitions(mini-gmp-plus PRIVATE MINI_GMP_SIMD)
    target_compile_features(mini-gmp-plus PRIVATE cxx_std_17)
    
    # Handle different xsimd installation methods
    if(TARGET xsimd::xsimd)
        # CONFIG mode with namespace (vcpkg, modern CMake installations)
        target_link_libraries(mini-gmp-plus PRIVATE xsimd::xsimd)
        message(STATUS "Using xsimd::xsimd target")
    elseif(TARGET xsimd)
        # CONFIG mode without namespace (some package managers)
        target_link_libraries(mini-gmp-plus PRIVATE xsimd)
        message(STATUS "Using xsimd target")
    elseif(xsimd_INCLUDE_DIRS)
        # Manual or module mode
        target_include_directories(mini-gmp-plus PRIVATE ${xsimd_INCLUDE_DIRS})
        message(STATUS "Using xsimd from: ${xsimd_INCLUDE_DIRS}")
    else()
        message(FATAL_ERROR "xsimd found but could not determine how to link it")
    endif()
endif()

# Add C++ wrapper test executable (only if not being built by vcpkg)
if(NOT DEFINED VCPKG_TARGET_TRIPLET AND NOT CMAKE_CROSSCOMPILING)
    add_executable(test_MiniMPZ tests/test_MiniMPZ.cpp)
    target_link_libraries(test_MiniMPZ mini-gmp-plus)
    add_test(NAME test_MiniMPZ COMMAND test_MiniMPZ)
    set_tests_properties(test_MiniMPZ PROPERTIES TIMEOUT 30)

    add_executable(benchmark_geometry EXCLUDE_FROM_ALL benchmarks/benchmark_geometry.cpp)
    target_include_directories(benchmark_geometry PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    target_link_libraries(benchmark_geometry mini-gmp-plus)
    target_compile_features(benchmark_geometry PRIVATE cxx_std_11)
    if(MINI_GMP_ENABLE_SIMD)
        target_compile_definitions(benchmark_geometry PRIVATE MINI_GMP_PLUS_BENCHMARK_VARIANT=\"SIMD\")
    else()
        target_compile_definitions(benchmark_geometry PRIVATE MINI_GMP_PLUS_BENCHMARK_VARIANT=\"non-SIMD\")
    endif()
endif()

if(MINI_GMP_PLUS_WITH_TESTS)
   add_subdirectory(tests)
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS mini-gmp-plus
    EXPORT mini-gmp-plus-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mini-gmp-plus
)

# Install the export set
install(EXPORT mini-gmp-plus-targets
    FILE mini-gmp-plus-targets.cmake
    NAMESPACE mini-gmp-plus::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mini-gmp-plus
)

# Create and install package config file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/mini-gmp-plus-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/mini-gmp-plus-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mini-gmp-plus
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/mini-gmp-plus-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/mini-gmp-plus-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/mini-gmp-plus-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mini-gmp-plus
)
