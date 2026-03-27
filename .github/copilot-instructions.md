# Copilot Instructions

## What This Project Is

`mini-gmp-plus` is a fork of GMP's `mini-gmp` (the standalone arbitrary-precision integer library) with two additions:
- Performance optimizations for numerical geometry: 64-bit-only limbs, stack allocation for numbers ≤ 5 limbs, and cross-compiler 64-bit intrinsics (`bitops64.h`)
- A modern C++ wrapper (`MiniMPZ.hpp`) over the C `mpz_t` type

## Build Commands

```bash
# Standard build
cmake -Bbuild
cmake --build build

# With SIMD acceleration via xsimd (ON by default, requires xsimd, e.g. brew install xsimd)
cmake -Bbuild -DMINI_GMP_ENABLE_SIMD=ON
cmake --build build

# Without SIMD (to disable)
cmake -Bbuild -DMINI_GMP_ENABLE_SIMD=OFF

# With C regression test suite (requires system gmp via pkg-config)
cmake -Bbuild -DMINI_GMP_PLUS_WITH_TESTS=1
cmake --build build
```

## Test Commands

```bash
# C++ wrapper test (always built, no external framework)
./build/test_MiniMPZ

# Run all C regression tests via CTest
cd build/tests && ctest -C Release

# Run a single C regression test by name
cd build/tests && ctest -R t-mul
# or directly:
./build/tests/t-mul
```

## Architecture

| File | Role |
|---|---|
| `mini-gmp.c` / `mini-gmp.h` | Core C library — arbitrary-precision integers (`mpz_t`). Forked from GMP's mini-gmp. |
| `mini-mpq.c` / `mini-mpq.h` | Rational number support (`mpq_t`) built on top of `mini-gmp`. |
| `MiniMPZ.hpp` | Header-only C++ wrapper class over `mpz_t`. All methods are inline. |
| `bitops64.h` | Cross-compiler intrinsics for 64-bit bit ops (GCC/Clang/MSVC). Not in upstream mini-gmp. |

The `.c` files compile into a **shared library** (`mini-gmp-plus`). `MiniMPZ.hpp` is header-only and depends on that shared library at link time.

CMake consumers use: `find_package(mini-gmp-plus)` + `target_link_libraries(... mini-gmp-plus::mini-gmp-plus)`.

## Key Conventions

### C++ Style
- C++11 minimum — uses `noexcept`, move semantics, `explicit` constructors, `nullptr`
- `MiniMPZ` is a plain class in **global scope** (no namespace)
- No templates — single concrete class wrapping `mpz_t`

### Constructor Rules
- Numeric constructors (`long`, `unsigned long`, `double`) are `explicit` — no implicit conversions
- String constructors (`const char*`, `std::string`) are **not** `explicit`
- When constructing from integer literals, use `L`/`UL` suffix: `MiniMPZ(42L)` — bare `42` is ambiguous

### Operator Overloading Pattern
- Binary operators (`+`, `-`, `*`, `/`, `%`) return by value: create a local `MiniMPZ result`, call the underlying `mpz_*` C function, return result
- Compound assignments (`+=`, `-=`, etc.) mutate in-place and return `*this`
- `operator<<` is a `friend` free function

### Move Semantics
Move ctor/assign raw-copies the `__mpz_struct` limb descriptor, then calls `mpz_init` on the source to reset it (avoids redundant heap allocation).

### Tests
- **C++ wrapper tests** (`tests/test_MiniMPZ.cpp`): pure `<cassert>` + `<iostream>`, no framework. Each `void test_*()` function prints a pass count. Add new tests as additional `test_*` functions called from `main()`.
- **C regression tests** (`tests/t-*.c`): link both `mini-gmp-plus` and system `gmp`, run the same computation on both, and compare results. Follow the same pattern when adding new C-level tests.
