vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO BrunoLevy/mini-gmp-plus
    REF v${VERSION}
    SHA512 0
    HEAD_REF main
)

# This port builds a shared library only.
vcpkg_check_linkage(ONLY_DYNAMIC_LIBRARY)

# Map the 'simd' feature to the CMake option.
vcpkg_check_features(OUT_FEATURE_FLAGS FEATURE_FLAGS
    FEATURES
        simd MINI_GMP_ENABLE_SIMD
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DMINI_GMP_PLUS_WITH_TESTS=OFF
        ${FEATURE_FLAGS}
)

vcpkg_cmake_build()
vcpkg_cmake_install()

vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/mini-gmp-plus)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")

