/* mini-gmp-simd.cpp — SIMD-accelerated mpn_ functions using xsimd.
 *
 * Compiled only when MINI_GMP_ENABLE_SIMD=ON (defines MINI_GMP_SIMD).
 * Requires xsimd ≥ 14 and C++17.
 *
 * xsimd 14 element-access API used here:
 *   batch.get(i)      — extract element at runtime index i → T
 *   batch_bool.get(i) — extract element at runtime index i → bool
 */

#include "mini-gmp.h"
#include <xsimd/xsimd.hpp>
#include <climits>   /* CHAR_BIT */
#include <cstdint>

/* GMP_LIMB_BITS is defined inside mini-gmp.c, not in the header. */
#ifndef GMP_LIMB_BITS
#  define GMP_LIMB_BITS (sizeof(mp_limb_t) * CHAR_BIT)
#endif

namespace {
    using batch_t = xsimd::batch<uint64_t>;

    /* Lane count: 8 on AVX-512, 4 on AVX2, 2 on SSE2/NEON.
     * Constexpr so it can appear in template arguments. */
    static constexpr std::size_t W    = batch_t::size;
    /* ── SWAR popcount on a batch of uint64_t ──────────────────────────── *
     *
     * xsimd 14 has no batch-level popcount, so we implement the classic
     * parallel bit-count using batch arithmetic.  Each lane independently
     * computes popcount(x) via:
     *   x -= (x >> 1) & 0x5555…
     *   x = (x & 0x3333…) + ((x >> 2) & 0x3333…)
     *   x = (x + (x >> 4)) & 0x0f0f…
     *   return (x * 0x0101…) >> 56
     *
     * This processes W limbs in parallel (2 on NEON, 4 on AVX2, 8 on
     * AVX-512), which is still much faster than the scalar 16-bit nibble
     * approach used in gmp_popcount_limb.
     */
    static inline batch_t popcount_batch(batch_t x)
    {
        const batch_t m1  = batch_t(0x5555555555555555ULL);
        const batch_t m2  = batch_t(0x3333333333333333ULL);
        const batch_t m4  = batch_t(0x0f0f0f0f0f0f0f0fULL);
        const batch_t h01 = batch_t(0x0101010101010101ULL);

        x -= (x >> 1) & m1;
        x = (x & m2) + ((x >> 2) & m2);
        x = (x + (x >> 4)) & m4;
        return (x * h01) >> 56;
    }

} // anonymous namespace

extern "C" {

/* The small scalar primitives (mpn_copyi/copyd/zero/cmp/zero_p/add_n/sub_n/
 * lshift/rshift) are compiled in mini-gmp.c even in SIMD builds: they are
 * dependency-heavy, called from hot mpz_* paths, and benchmark dramatically
 * slower when outlined into this separate translation unit (the C callers can
 * no longer inline them).  This file only defines the routines that genuinely
 * benefit from vectorization. */

/* ── bitwise complement ─────────────────────────────────────────────────── */

void mpn_com(mp_ptr rp, mp_srcptr up, mp_size_t n)
{
    const batch_t ones = batch_t(~uint64_t(0));
    mp_size_t i = 0;
    for (; i + static_cast<mp_size_t>(W) <= n; i += static_cast<mp_size_t>(W))
        (batch_t::load_unaligned(up + i) ^ ones).store_unaligned(rp + i);
    for (; i < n; i++)
        rp[i] = ~up[i];
}

/* ── population count (batch popcount + horizontal reduce) ───────────────── *
 *
 * popcount_batch gives per-lane popcount; reduce_add sums all lanes.
 * Processes W limbs per iteration (2 on NEON, 4 on AVX2, 8 on AVX-512),
 * which is much faster than the scalar 16-bit nibble approach in
 * gmp_popcount_limb.
 */
mp_bitcnt_t mpn_popcount(mp_srcptr p, mp_size_t n)
{
    mp_size_t i = 0;
    batch_t acc = batch_t(uint64_t(0));

    for (; i + static_cast<mp_size_t>(W) <= n; i += static_cast<mp_size_t>(W))
        acc += popcount_batch(batch_t::load_unaligned(p + i));

    mp_bitcnt_t c = static_cast<mp_bitcnt_t>(xsimd::reduce_add(acc));

    for (; i < n; i++)
    {
#if defined(__GNUC__) || defined(__clang__)
        c += static_cast<mp_bitcnt_t>(__builtin_popcountll(p[i]));
#elif defined(_MSC_VER)
        c += static_cast<mp_bitcnt_t>(__popcnt64(p[i]));
#else
        uint64_t x = p[i];
        x -= (x >> 1) & 0x5555555555555555ULL;
        x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
        x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
        c += static_cast<mp_bitcnt_t>((x * 0x0101010101010101ULL) >> 56);
#endif
    }
    return c;
}

/* ── Hamming distance (XOR + popcount in one pass) ──────────────────────── */

mp_bitcnt_t mpn_hamdist(mp_srcptr ap, mp_srcptr bp, mp_size_t n)
{
    mp_size_t i = 0;
    batch_t acc = batch_t(uint64_t(0));

    for (; i + static_cast<mp_size_t>(W) <= n; i += static_cast<mp_size_t>(W))
        acc += popcount_batch(
            batch_t::load_unaligned(ap + i) ^ batch_t::load_unaligned(bp + i));

    mp_bitcnt_t c = static_cast<mp_bitcnt_t>(xsimd::reduce_add(acc));

    for (; i < n; i++)
    {
#if defined(__GNUC__) || defined(__clang__)
        c += static_cast<mp_bitcnt_t>(__builtin_popcountll(ap[i] ^ bp[i]));
#elif defined(_MSC_VER)
        c += static_cast<mp_bitcnt_t>(__popcnt64(ap[i] ^ bp[i]));
#else
        uint64_t x = ap[i] ^ bp[i];
        x -= (x >> 1) & 0x5555555555555555ULL;
        x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
        x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
        c += static_cast<mp_bitcnt_t>((x * 0x0101010101010101ULL) >> 56);
#endif
    }
    return c;
}

/* ── bulk logical operations (embarrassingly parallel) ──────────────────── */

void mpn_and_n(mp_ptr rp, mp_srcptr ap, mp_srcptr bp, mp_size_t n)
{
    mp_size_t i = 0;
    for (; i + static_cast<mp_size_t>(W) <= n; i += static_cast<mp_size_t>(W))
        (batch_t::load_unaligned(ap + i) & batch_t::load_unaligned(bp + i))
            .store_unaligned(rp + i);
    for (; i < n; i++)
        rp[i] = ap[i] & bp[i];
}

void mpn_ior_n(mp_ptr rp, mp_srcptr ap, mp_srcptr bp, mp_size_t n)
{
    mp_size_t i = 0;
    for (; i + static_cast<mp_size_t>(W) <= n; i += static_cast<mp_size_t>(W))
        (batch_t::load_unaligned(ap + i) | batch_t::load_unaligned(bp + i))
            .store_unaligned(rp + i);
    for (; i < n; i++)
        rp[i] = ap[i] | bp[i];
}

void mpn_xor_n(mp_ptr rp, mp_srcptr ap, mp_srcptr bp, mp_size_t n)
{
    mp_size_t i = 0;
    for (; i + static_cast<mp_size_t>(W) <= n; i += static_cast<mp_size_t>(W))
        (batch_t::load_unaligned(ap + i) ^ batch_t::load_unaligned(bp + i))
            .store_unaligned(rp + i);
    for (; i < n; i++)
        rp[i] = ap[i] ^ bp[i];
}

} /* extern "C" */
