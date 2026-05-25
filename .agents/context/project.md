# Project Context — Apeirogon

> Full reference: see `AGENTS.md` at the repository root.

## What This Project Is

C++20 library for exact 3D Boolean operations (union, intersection, difference, symmetric difference) on polyhedra. Vertices are the intersection of three planes — never XYZ floats. All geometric predicates are exact via `DoubleDynamicError → IntervalArithmetic → FloatRingExtension` cascade. **No epsilon/tolerance comparisons in geometric logic.**

## Build & Test

```bash
cmake --preset ninja-release-vcpkg && cmake --build --preset ninja-release-vcpkg
cd builds/ninja-release-vcpkg && ctest -E "SlowTests"
./bin/PB2Test --run_test=PlaneBasedGeometryTests/ExactArithmetic --logger=HRF,all --color_output=false
```

Sanitizer presets: `ninja-asan`, `ninja-ubsan`, `ninja-asan-ubsan` (best daily-driver), `ninja-tsan` (races, incompatible with ASan).

## Module Map

| Directory | Target | Purpose |
|---|---|---|
| `Infrastructure/` | `PB2Infra` | Logging, errors, macros |
| `Mathematics/` | `PB2Maths` | Exact number types + predicate engine |
| `PlaneBasedGeometry/` | `PB2Geo` | `Plane`, `Point`, `Polygon`, `Polyhedron` |
| `PlaneBasedOperators/` | `PB2Operators` | Boolean pipeline, spatial localizers |
| `PlaneBasedIO/` | `PB2IO` | Import/export |
| `PlaneBasedTests/` | `PB2Test` | Boost.Test suite |

Public headers: `<Module>/SharedInterfaces/`. Internal headers: `<Module>/LocalInterfaces/`.

## Key Conventions

- **Parameters**: `i` input, `o` output, `io` in/out. **Members**: `m_` prefix.
- **Never `auto` with Eigen** (aliasing bugs). Pass by `const&`, never by value.
- **Logging**: `LogError({}, "msg {}", val)` — never call spdlog directly.
- **Errors**: return `ErrorKinds` (`PB_OK`, `KO_*`). Null guards: `ThrowForNullPointer(ptr)`.
- **No tolerances** in geometric logic — only permitted during primitive creation (mesh import).
- **DLL export**: `ExportedByPlaneBasedGeometry` / `ExportedByPlaneBasedOperators` on all public classes.
- **Non-copyable**: `DISALLOW_COPYCTOR_ASSIGN_AND_MOVE(MyType)`.

## Mandatory Review

Every non-trivial code change **must** be reviewed by the `critical-review` agent before committing. Trivial changes (typos, comments, single-line renames) are exempt.
