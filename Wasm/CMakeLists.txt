# WebAssembly bindings for mini-gmp-plus-plus.
#
# Only reachable when the build is driven by the Emscripten toolchain
# (see the 'wasm-release' CMake preset). Produces in ${CMAKE_BINARY_DIR}/bin:
#   mini-gmp-plus-plus.js   - ES6 module loader (MODULARIZE + EXPORT_ES6)
#   mini-gmp-plus-plus.wasm - the compiled library and bindings

if(NOT EMSCRIPTEN)
  message(FATAL_ERROR
      "Wasm/ requires the Emscripten toolchain. Install and activate the "
      "emsdk (source emsdk_env.sh), then configure with the 'wasm-release' "
      "CMake preset.")
endif()

add_executable(mini-gmp-plus-plus-wasm src/mini_gmp_wasm.cpp)

target_compile_features(mini-gmp-plus-plus-wasm PRIVATE cxx_std_17)
target_link_libraries(mini-gmp-plus-plus-wasm PRIVATE mini-gmp-plus-plus)

set_target_properties(mini-gmp-plus-plus-wasm PROPERTIES
    OUTPUT_NAME "mini-gmp-plus-plus"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_link_options(mini-gmp-plus-plus-wasm PRIVATE
    "-lembind"
    "-sMODULARIZE=1"
    "-sEXPORT_ES6=1"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sENVIRONMENT=web,worker,node"
    "-sFILESYSTEM=0"
    "-sEXIT_RUNTIME=0"
    "-sEXPORT_NAME=createMiniGmpPlusPlus"
)
