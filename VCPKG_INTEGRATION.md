# vcpkg Integration

`mini-gmp-plus-plus` can be consumed with [vcpkg](https://vcpkg.io). The port files live in
[`packaging/vcpkg/mini-gmp-plus-plus/`](packaging/vcpkg/mini-gmp-plus-plus):

| File | Role |
|---|---|
| `vcpkg.json` | Port manifest — version, license, dependencies, features |
| `portfile.cmake` | Build recipe used by vcpkg |
| `usage` | Text shown to users after a successful install |

The CMake package config template `mini-gmp-plus-plus-config.cmake.in` lives at the repo root
and is part of the library's own install rules, not of the port.

## Consuming the package

Add a `vcpkg-configuration.json` next to your `vcpkg.json`:

```json
{
  "default-registry": {
    "kind": "git",
    "repository": "https://github.com/microsoft/vcpkg",
    "baseline": "<a microsoft/vcpkg commit sha>"
  },
  "registries": [
    {
      "kind": "git",
      "repository": "https://github.com/Nosenzor/vcpkg-registry",
      "baseline": "<a vcpkg-registry commit sha>",
      "packages": ["mini-gmp-plus-plus"]
    }
  ]
}
```

then depend on it as usual:

```json
{
  "dependencies": ["mini-gmp-plus-plus"]
}
```

### In your CMakeLists.txt

```cmake
find_package(mini-gmp-plus-plus CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE mini-gmp-plus-plus::mini-gmp-plus-plus)
```

## What gets installed

Headers are installed under `include/mini-gmp-plus-plus/`:

```cpp
#include <mini-gmp-plus-plus/mini-gmp.h>                    // C API, multiprecision integers
#include <mini-gmp-plus-plus/mini-mpq.h>                    // C API, rationals
#include <mini-gmp-plus-plus/MiniMPZ.hpp>                   // C++ integer wrapper
#include <mini-gmp-plus-plus/MiniMPF.hpp>                   // C++ float wrapper
#include <mini-gmp-plus-plus/mini-gmp-plus-plus-config.hpp> // compile-time configuration
#include <mini-gmp-plus-plus/bitops64.h>                    // 64-bit bit-operation helpers
```

## Features and linkage

- Feature **`simd`** (enabled by default) builds the xsimd-accelerated `mpn_*` primitives. To
  install without it, depend on `{ "name": "mini-gmp-plus-plus", "default-features": false }`.
  `xsimd` is a private build-time dependency; it is not needed to compile against the package.
- Both **static and dynamic** linkage are supported. When linking the static library, CMake
  defines `MINI_GMP_PLUS_STATIC` for you as a usage requirement; if you build without CMake,
  define it yourself so the API macros expand correctly.
- The C regression tests are not built when installed through vcpkg, so no system `gmp` is
  needed.

## Platform support

The port declares `"supports": "!(windows & arm)"`. The MSVC path in `mini-gmp.c` uses the
x64-only `_umul128` intrinsic, which is unavailable on ARM64 MSVC.

## Local testing

To test the port from a checkout of this repository:

```bash
vcpkg install mini-gmp-plus-plus --overlay-ports=packaging/vcpkg
```

Add `--triplet x64-windows-static` (or any other triplet) to exercise a specific configuration.
