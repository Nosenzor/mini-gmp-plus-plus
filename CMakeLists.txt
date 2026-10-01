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

cmake_minimum_required(VERSION 3.16...3.30)
project(mini-gmp-plus-plus VERSION 1.1.0 LANGUAGES C CXX)

include(CTest)

# Build the C regression test suite (needs system gmp via pkg-config).
option(MINI_GMP_PLUS_WITH_TESTS "Build the C regression tests (requires system gmp)" OFF)

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
# Linkage follows BUILD_SHARED_LIBS (shared by default, matching historical
# behaviour).  Package managers such as vcpkg drive this to build both variants.
if(NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ON)
endif()
add_library(mini-gmp-plus-plus ${MINI_GMP_SOURCES})
set_target_properties(mini-gmp-plus-plus PROPERTIES
    PUBLIC_HEADER "mini-gmp.h;mini-mpq.h;MiniMPZ.hpp;MiniMPF.hpp;mini-gmp-plus-plus-config.hpp;bitops64.h"
    POSITION_INDEPENDENT_CODE ON
)
if(BUILD_SHARED_LIBS)
    set_target_properties(mini-gmp-plus-plus PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
else()
    # Static build: the API macros must expand to nothing, both here and in
    # consumers, so publish MINI_GMP_PLUS_STATIC as a usage requirement
    # (see the MINI_GMP_PLUS_API logic in mini-gmp.h).
    target_compile_definitions(mini-gmp-plus-plus PUBLIC MINI_GMP_PLUS_STATIC)
endif()

# Set include directories for building and installing
target_include_directories(mini-gmp-plus-plus
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

if(MINI_GMP_ENABLE_SIMD)
    target_compile_definitions(mini-gmp-plus-plus PRIVATE MINI_GMP_SIMD)
    target_compile_features(mini-gmp-plus-plus PRIVATE cxx_std_17)

    # Handle different xsimd installation methods.
    #
    # xsimd is a header-only, build-time-only dependency: it is used solely by
    # mini-gmp-simd.cpp and appears in no installed header.  The dependency is
    # therefore wrapped in $<BUILD_INTERFACE:...> so it is NOT exported.  This
    # matters for static builds, where CMake would otherwise record PRIVATE
    # dependencies as transitive link requirements and consumers would fail with
    # "library 'xsimd' not found" (nothing resolves the name without a
    # find_dependency(xsimd) that we deliberately do not want to impose).
    if(TARGET xsimd::xsimd)
        # CONFIG mode with namespace (vcpkg, modern CMake installations)
        target_link_libraries(mini-gmp-plus-plus PRIVATE $<BUILD_INTERFACE:xsimd::xsimd>)
        message(STATUS "Using xsimd::xsimd target")
    elseif(TARGET xsimd)
        # CONFIG mode without namespace (some package managers)
        target_link_libraries(mini-gmp-plus-plus PRIVATE $<BUILD_INTERFACE:xsimd>)
        message(STATUS "Using xsimd target")
    elseif(xsimd_INCLUDE_DIRS)
        # Manual or module mode
        target_include_directories(mini-gmp-plus-plus PRIVATE ${xsimd_INCLUDE_DIRS})
        message(STATUS "Using xsimd from: ${xsimd_INCLUDE_DIRS}")
    else()
        message(FATAL_ERROR "xsimd found but could not determine how to link it")
    endif()
endif()

# On GCC/Clang, disable PLT-based symbol interposition within the DSO so the
# compiler can generate direct intra-library calls, reducing overhead on Linux.
# (No-op on Mach-O, where it only produces an unused-argument warning.)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang" AND NOT APPLE)
    target_compile_options(mini-gmp-plus-plus PRIVATE -fno-semantic-interposition)
endif()

# Optional: build the library without stack-smashing protection.  Selected
# hot functions already opt out individually (see MINI_GMP_NO_STACK_PROTECTOR
# in mini-gmp.c); disabling it library-wide is worth a further ~5-10% on
# small-operand workloads, at the cost of stack-smash hardening.
option(MINI_GMP_NO_STACK_PROTECTOR "Build mini-gmp-plus-plus with -fno-stack-protector (fastest, less hardening)" OFF)
if(MINI_GMP_NO_STACK_PROTECTOR AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mini-gmp-plus-plus PRIVATE -fno-stack-protector)
endif()

# Optional link-time optimization for the library (opt-in; lets the linker
# inline across mini-gmp.c / mini-mpq.c / mini-gmp-simd.cpp).
option(MINI_GMP_ENABLE_IPO "Enable interprocedural optimization (LTO) for mini-gmp-plus-plus" OFF)
if(MINI_GMP_ENABLE_IPO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT mini_gmp_ipo_ok OUTPUT mini_gmp_ipo_msg)
    if(mini_gmp_ipo_ok)
        set_target_properties(mini-gmp-plus-plus PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "IPO not supported: ${mini_gmp_ipo_msg}")
    endif()
endif()

# Add C++ wrapper test executable (only if not being built by vcpkg)
if(NOT DEFINED VCPKG_TARGET_TRIPLET AND NOT CMAKE_CROSSCOMPILING)
    add_executable(test_MiniMPZ tests/test_MiniMPZ.cpp)
    target_link_libraries(test_MiniMPZ mini-gmp-plus-plus)
    add_test(NAME test_MiniMPZ COMMAND test_MiniMPZ)
    set_tests_properties(test_MiniMPZ PROPERTIES TIMEOUT 30)

    add_executable(test_MiniMPF tests/test_MiniMPF.cpp)
    target_link_libraries(test_MiniMPF mini-gmp-plus-plus)
    add_test(NAME test_MiniMPF COMMAND test_MiniMPF)
    set_tests_properties(test_MiniMPF PROPERTIES TIMEOUT 30)

    add_executable(test_MiniMPF_stress tests/test_MiniMPF_stress.cpp)
    target_link_libraries(test_MiniMPF_stress mini-gmp-plus-plus)
    add_test(NAME test_MiniMPF_stress COMMAND test_MiniMPF_stress)
    set_tests_properties(test_MiniMPF_stress PROPERTIES TIMEOUT 30)

    add_executable(test_MiniMPF_double_ctor tests/test_MiniMPF_double_ctor.cpp)
    target_link_libraries(test_MiniMPF_double_ctor mini-gmp-plus-plus)
    add_test(NAME test_MiniMPF_double_ctor COMMAND test_MiniMPF_double_ctor)
    set_tests_properties(test_MiniMPF_double_ctor PROPERTIES TIMEOUT 30)

    add_executable(benchmark_geometry EXCLUDE_FROM_ALL benchmarks/benchmark_geometry.cpp)
    target_include_directories(benchmark_geometry PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    target_link_libraries(benchmark_geometry mini-gmp-plus-plus)
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

install(TARGETS mini-gmp-plus-plus
    EXPORT mini-gmp-plus-plus-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mini-gmp-plus-plus
)

# Install the export set
install(EXPORT mini-gmp-plus-plus-targets
    FILE mini-gmp-plus-plus-targets.cmake
    NAMESPACE mini-gmp-plus-plus::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mini-gmp-plus-plus
)

# Create and install package config file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/mini-gmp-plus-plus-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/mini-gmp-plus-plus-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mini-gmp-plus-plus
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/mini-gmp-plus-plus-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/mini-gmp-plus-plus-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/mini-gmp-plus-plus-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mini-gmp-plus-plus
)
