// mini-gmp-plus-config.hpp
// Compatibility configurations for mini-gmp-plus-plus
//
// This header provides:
// - Detection of __uint128_t availability
// - Likely/Unlikely branch prediction macros
//
// Copyright 2025 Romain Nosenzo

#ifndef MINI_GMP_PLUS_CONFIG_HPP
#define MINI_GMP_PLUS_CONFIG_HPP

// ============================================================================
// __uint128_t availability detection
// ============================================================================

#if defined(__SIZEOF_INT128__)
// GCC and Clang define __SIZEOF_INT128__ when __int128 is available
#  define MINI_GMP_PLUS_HAS_UINT128 1
#  define MINI_GMP_PLUS_UINT128_T unsigned __int128
#elif defined(_MSC_VER)
// MSVC has __uint128_t in newer versions (2019+ with /std:c++latest or /arch:AVX2)
#  if defined(__cpp_size_t) && __cpp_size_t >= 201806L
#    include <cstdint>
#    ifdef __uint128_t
#      define MINI_GMP_PLUS_HAS_UINT128 1
#      define MINI_GMP_PLUS_UINT128_T __uint128_t
#    else
#      define MINI_GMP_PLUS_HAS_UINT128 0
#    endif
#  else
#    define MINI_GMP_PLUS_HAS_UINT128 0
#  endif
#else
// Other compilers - try to detect
#  ifdef __has_include
#    if __has_include(<cstdint>) && defined(__UINT128_TYPE__)
#      define MINI_GMP_PLUS_HAS_UINT128 1
#      define MINI_GMP_PLUS_UINT128_T __UINT128_TYPE__
#    else
#      define MINI_GMP_PLUS_HAS_UINT128 0
#    endif
#  else
#    define MINI_GMP_PLUS_HAS_UINT128 0
#  endif
#endif

#ifndef MINI_GMP_PLUS_HAS_UINT128
#  define MINI_GMP_PLUS_HAS_UINT128 0
#endif

// ============================================================================
// Branch prediction macros
// ============================================================================

#if defined(__cplusplus) && __cplusplus >= 202002L
// C++20 and later: use standard attributes
#  define MINI_GMP_PLUS_LIKELY [[likely]]
#  define MINI_GMP_PLUS_UNLIKELY [[unlikely]]
#elif defined(__GNUC__) || defined(__clang__)
// GCC and Clang: use __builtin_expect
#  define MINI_GMP_PLUS_LIKELY __builtin_expect(!!(true), true)
#  define MINI_GMP_PLUS_UNLIKELY __builtin_expect(!!(false), false)
#  define MINI_GMP_PLUS_EXPECT(cond, val) __builtin_expect(!!(cond), val)
#elif defined(_MSC_VER)
// MSVC: use __assume for branch prediction hints
// Note: MSVC doesn't have direct likely/unlikely, but we can use conditional __assume
// For simplicity, we define to nothing (no-op) for MSVC
// Alternatively, use /analyze:predict or profile-guided optimization
#  define MINI_GMP_PLUS_LIKELY
#  define MINI_GMP_PLUS_UNLIKELY
#  define MINI_GMP_PLUS_EXPECT(cond, val) (cond)
#else
// Fallback: no branch prediction hints
#  define MINI_GMP_PLUS_LIKELY
#  define MINI_GMP_PLUS_UNLIKELY
#  define MINI_GMP_PLUS_EXPECT(cond, val) (cond)
#endif

// Backward compatibility macro for existing __builtin_expect usage
// Usage: if (MINI_GMP_PLUS_EXPECT(cond, 1)) { ... }
// MINI_GMP_PLUS_EXPECT(cond, 1) -> likely, MINI_GMP_PLUS_EXPECT(cond, 0) -> unlikely
#ifndef MINI_GMP_PLUS_EXPECT
#  if defined(__GNUC__) || defined(__clang__)
#    define MINI_GMP_PLUS_EXPECT(cond, expected) __builtin_expect((cond), (expected))
#  else
#    define MINI_GMP_PLUS_EXPECT(cond, expected) (cond)
#  endif
#endif

#endif // MINI_GMP_PLUS_CONFIG_HPP
