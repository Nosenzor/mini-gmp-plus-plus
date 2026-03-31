# mini-gmp-plus

This is a fork of the mini version of the
[GNU Multi-Precision library](https://gmplib.org/)
with some patches/optimizations for usage in numerical geometry
and easy-to-use CMakeList (tested under Windows, Linux, Mac).

My changes are listed in [mini-gmp-plus-Changelog](mini-gmp-plus-Changelog),
and indicated by `[Bruno Levy]` tags in the sources, with the date and reason
for the modification.

Design rationale
----------------
In numerical geometry, one manipulates numbers that are relatively
small (a few 64-bit limbs), so Karatsuba multiplication is not
needed, hence `mini-gmp` suffices. On the other hand, it is interesting
to avoid dynamic allocation, hence numbers smaller than a certain threshold (that
corresponds to 5 64-bits numbers by default) are allocated on the
stack. Modern processors have some instructions that can significantly optimize
some multi-precision operations, such as finding the number of leading and
trailing zeroes in an integer, shifting 128 bit numbers, multiplying two 64-bit
numbers and obtaining the 128-bits result, or adding 64-bit numbers and
propagating the carry in multi-limb operations. While 64-bit add/subtract with
carry propagation is recognized and optimized by modern compilers, it is not the
case for the other operations, that are implemented by some compiler-dependent
intrinsics. For this reason, I needed to create a new [bitops64.h](bitops64.h)
file to abstract them. These two modifications (_local storage_ and
_64 bits intrinsics_) introduced a couple of very simple yet not completely
trivial changes (I must confess I did not have them right the first time), so it
is important - I would say _vital_ - to be able to run the testsuite. For this
reason I also created new `CMakeLists.txt` files as well as a github action that
runs the (already existing, thank you !) mini-gmp testsuite under Linux,Mac
and Windows.

Features and specificities
--------------------------
As compared to `mini-gmp`, `mini-gmp-plus` has the following differences:
- _limitation_: limb size is _fixed_ as 64 bits
- numbers smaller than a certain size (5 limbs) are stored in the `mpz_t`
  structure for better multithreading (avoids most dynamic allocations)
- the file [bitops64.h](bitops64.h), not part of mini-gmp, contains some
  wrappers for efficient bit operations on 64-bit numbers, for GCC, Clang and
  VisualC++ using these compiler's intrinsics
- `mini-gmp-plus` is compiled as a dynamic library
- [CMakeLists.txt](CMakeLists.txt) optionally builds and runs non-regression
  tests using CTest, use `cmake -DMINI_GMP_PLUS_WITH_TESTS=1` to compile and
  run the testsuite.
- The [github action](.github/workflows/build.yml) builds and runs the testsuite
  systematically. Under Windows, the github action installs `gmp` (through `vcpkg`)
  and declares it (through `pkgconf`) so that
  [tests/CMakeLists.txt](tests/CMakeLists.txt) sees it. Note that `gmp`
  is only needed if you want to run the non-regression testsuite.

Benchmarking geometry workloads
-------------------------------
A standalone `benchmark_geometry` target is available for native builds.
It measures deterministic batches of:
- 4D vector dot products
- 2x2, 3x3 and 4x4 determinants
- supplementary `sqrt` and `gcd` workloads

Build and run it in both variants to compare arithmetic throughput on the
same machine:

```bash
cmake -Bbuild-simd -DMINI_GMP_ENABLE_SIMD=ON
cmake --build build-simd --target benchmark_geometry
./build-simd/benchmark_geometry

cmake -Bbuild-nosimd -DMINI_GMP_ENABLE_SIMD=OFF
cmake --build build-nosimd --target benchmark_geometry
./build-nosimd/benchmark_geometry
```

Useful options:
- `--dataset-size=N` to control how many inputs are timed per benchmark case
- `--min-time-ms=N` to force a longer run for more stable measurements

Compare the reported `ns/op` numbers between SIMD and non-SIMD builds; they
reflect arithmetic workload timing much better than CI wall-clock duration.

To run the same benchmark matrix on GitHub-hosted runners, trigger the
`Benchmarks` workflow from the Actions tab. It runs Linux, macOS, and Windows
in both SIMD and non-SIMD modes, uploads raw per-job benchmark outputs, and
publishes an aggregate Markdown/CSV summary artifact.

The workflow accepts two manual inputs:
- `dataset_size`
- `min_time_ms`

Those hosted-runner numbers are excellent for spotting large regressions and
cross-platform trends, but they are still noisier than measurements collected
on a dedicated local machine.


_Bruno Lévy, November 2025_
