# Toolchain wrapper for Emscripten (WebAssembly) builds of mini-gmp-plus-plus.
#
# Point CMAKE_TOOLCHAIN_FILE at this file (the 'wasm-release' CMake preset
# does) instead of including Emscripten.cmake directly, so the Emscripten
# SDK is located the same way whether the emsdk was activated with
# emsdk_env.sh (EMSDK set), Emscripten's own variables are set
# (EMSCRIPTEN_ROOT), or emcc is simply on PATH.

if(DEFINED ENV{EMSCRIPTEN_ROOT})
    set(EMSCRIPTEN_ROOT "$ENV{EMSCRIPTEN_ROOT}")
elseif(DEFINED ENV{EMSDK})
    set(EMSCRIPTEN_ROOT "$ENV{EMSDK}/upstream/emscripten")
else()
    find_path(EMSCRIPTEN_ROOT emcc)
endif()

if(NOT EMSCRIPTEN_ROOT OR NOT EXISTS "${EMSCRIPTEN_ROOT}/cmake/Modules/Platform/Emscripten.cmake")
    message(FATAL_ERROR
        "Emscripten.cmake toolchain file not found. Install the emsdk and "
        "activate it (source emsdk_env.sh) so EMSDK is set or emcc is in PATH, "
        "then configure with: cmake --preset wasm-release")
endif()

include("${EMSCRIPTEN_ROOT}/cmake/Modules/Platform/Emscripten.cmake")
