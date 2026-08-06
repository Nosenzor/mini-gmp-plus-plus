vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO Nosenzor/mini-gmp-plus-plus
    REF "v${VERSION}"
    SHA512 9507479f9c553999228bfa010517a2873c9b3511cd948ed780ebbb5bb78250898e0144cfc4d97dff3bfd530146c62e33d40bf2e4f7640f2e7f8542c401d6657a
    HEAD_REF main
)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        simd MINI_GMP_ENABLE_SIMD
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DMINI_GMP_PLUS_WITH_TESTS=OFF
        ${FEATURE_OPTIONS}
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/mini-gmp-plus-plus)

vcpkg_copy_pdbs()

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/share"
)

# mini-gmp is dual-licensed: LGPL-3.0-or-later or GPL-2.0-or-later, at the
# recipient's option.  LGPL-3.0 is defined as a supplement to GPL-3.0, so that
# text is required as well.
vcpkg_install_copyright(FILE_LIST
    "${SOURCE_PATH}/LICENSE"
    "${SOURCE_PATH}/COPYING.GPL2"
    "${SOURCE_PATH}/COPYING.GPL3"
)
