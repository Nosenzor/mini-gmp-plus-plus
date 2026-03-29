# AGENTS.md

## Scope and Intent
- `mini-gmp-plus` is a fork of GNU `mini-gmp` optimized for numerical geometry workloads with mostly small integers.
- Core design choices: fixed 64-bit limbs (`mp_limb_t` in `mini-gmp.h`), stack buffer for small `mpz_t` values (`MINI_GMP_PLUS_BUFF_SIZE`, default 5), and compiler-specific 64-bit intrinsics (`bitops64.h`).
- The library is built as a shared library (`mini-gmp-plus`), with a header-only C++ wrapper class `MiniMPZ` layered on top.

## Codebase Map (Read These First)
- `mini-gmp.c` + `mini-gmp.h`: C big-integer core (`mpn_*`, `mpz_*` APIs, memory model, export macros).
- `mini-mpq.c` + `mini-mpq.h`: rational arithmetic (`mpq_*`) implemented on top of `mpz`.
- `mini-gmp-simd.cpp`: optional SIMD implementations of selected `mpn_*` primitives (enabled by `MINI_GMP_SIMD`).
- `MiniMPZ.hpp`: inline C++ wrapper in global scope; owns one `mpz_t` and forwards operations to `mpz_*`.
- `tests/t-*.c` + `tests/CMakeLists.txt`: regression suite comparing mini-gmp-plus behavior with system GMP.
- `tests/test_MiniMPZ.cpp`: C++ wrapper tests using only `<cassert>` and `main()`.

## Build and Test Workflows
- Default build (SIMD ON by default, requires `xsimd`): `cmake -Bbuild && cmake --build build`.
- Disable SIMD if `xsimd` is unavailable: `cmake -Bbuild -DMINI_GMP_ENABLE_SIMD=OFF && cmake --build build`.
- Enable C regression tests (needs system `gmp` discoverable via `pkg-config`): `cmake -Bbuild -DMINI_GMP_PLUS_WITH_TESTS=1 && cmake --build build`.
- Run wrapper test executable: `./build/test_MiniMPZ`.
- Run C regression tests: `cd build/tests && ctest -C Release` (or `ctest -R t-mul` for one test).
- CI reference for platform quirks (especially Windows `pkgconf`/`PKG_CONFIG_PATH`): `.github/workflows/build.yml`.

## Project-Specific Coding Patterns
- Keep `MiniMPZ` constructor intent explicit for numeric types (`long`, `unsigned long`, `double`); string constructors remain implicit (`MiniMPZ.hpp`).
- Use integer literal suffixes when constructing `MiniMPZ` from literals in tests/examples (e.g., `42L`, `42UL`), matching existing tests.
- `MiniMPZ` binary operators return by value via local `result` and `mpz_*` call; compound ops mutate `*this` and return reference.
- Move operations in `MiniMPZ` raw-copy the underlying `__mpz_struct` then `mpz_init` the moved-from object.
- For C-level behavior changes, preserve GMP compatibility expectations validated by paired mini-gmp-plus vs GMP tests.

## Integration and Packaging
- Consumer contract is CMake package usage: `find_package(mini-gmp-plus CONFIG REQUIRED)` + `target_link_libraries(... mini-gmp-plus::mini-gmp-plus)` (see `usage`, `example_CMakeLists.txt`).
- Installed public headers: `mini-gmp.h`, `mini-mpq.h`, `MiniMPZ.hpp`, `bitops64.h`.
- `vcpkg.json` defaults to feature `simd`; disable with `default-features: false` when needed.

## Change Guidance for Agents
- If editing low-level `mpn_*` routines (`mini-gmp.c` / `mini-gmp-simd.cpp`), run at least targeted C regression tests affected (`ctest -R <name>`).
- If editing `MiniMPZ.hpp`, add/update a `test_*` function in `tests/test_MiniMPZ.cpp` and call it from `main()`.
- When touching cross-compiler bit operations, validate on at least one GCC/Clang path and keep MSVC branches in `bitops64.h` behaviorally equivalent.

