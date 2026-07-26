# TODO: SHA512 is a placeholder until the v1.1.0 release tarball exists; it
# cannot be computed before the tag is published.  To fill it in, run
#   vcpkg install mini-gmp-plus --overlay-ports=packaging/vcpkg
# and copy the hash from the "expected SHA512" error message (lowercase hex).
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO Nosenzor/mini-gmp-plus-plus
    REF "v${VERSION}"
    SHA512 0
    HEAD_REF main
)

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

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/mini-gmp-plus)

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
