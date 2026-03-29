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
#include <cassert>
#include <cstring>
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

    /* These dependency-heavy primitives are hot in mpz_gcd/mpz_gcdext and
     * benchmark dramatically slower than the original scalar loops on current
     * AppleClang/NEON builds, so SIMD builds keep scalar implementations for
     * them and reserve xsimd for the routines that actually benefit. */
    static inline int scalar_mpn_cmp(mp_srcptr ap, mp_srcptr bp, mp_size_t n)
    {
        while (--n >= 0) {
            if (ap[n] != bp[n])
                return ap[n] > bp[n] ? 1 : -1;
        }
        return 0;
    }

    static inline mp_limb_t scalar_mpn_add_n(mp_ptr rp, mp_srcptr ap, mp_srcptr bp, mp_size_t n)
    {
        mp_size_t i;
        mp_limb_t carry;

        for (i = 0, carry = 0; i < n; i++) {
            mp_limb_t a = ap[i];
            mp_limb_t b = bp[i];
            mp_limb_t r = a + carry;
            carry = (r < carry);
            r += b;
            carry += (r < b);
            rp[i] = r;
        }
        return carry;
    }

    static inline mp_limb_t scalar_mpn_sub_n(mp_ptr rp, mp_srcptr ap, mp_srcptr bp, mp_size_t n)
    {
        mp_size_t i;
        mp_limb_t borrow;

        for (i = 0, borrow = 0; i < n; i++) {
            mp_limb_t a = ap[i];
            mp_limb_t b = bp[i];
            b += borrow;
            borrow = (b < borrow);
            borrow += (a < b);
            rp[i] = a - b;
        }
        return borrow;
    }

    static inline mp_limb_t scalar_mpn_lshift(mp_ptr rp, mp_srcptr up, mp_size_t n, unsigned int cnt)
    {
        mp_limb_t high_limb, low_limb;
        unsigned int tnc;
        mp_limb_t retval;

        assert(n >= 1);
        assert(cnt >= 1);
        assert(cnt < GMP_LIMB_BITS);

        up += n;
        rp += n;

        tnc = GMP_LIMB_BITS - cnt;
        low_limb = *--up;
        retval = low_limb >> tnc;
        high_limb = (low_limb << cnt);

        while (--n != 0) {
            low_limb = *--up;
            *--rp = high_limb | (low_limb >> tnc);
            high_limb = (low_limb << cnt);
        }
        *--rp = high_limb;

        return retval;
    }

    static inline mp_limb_t scalar_mpn_rshift(mp_ptr rp, mp_srcptr up, mp_size_t n, unsigned int cnt)
    {
        mp_limb_t high_limb, low_limb;
        unsigned int tnc;
        mp_limb_t retval;

        assert(n >= 1);
        assert(cnt >= 1);
        assert(cnt < GMP_LIMB_BITS);

        tnc = GMP_LIMB_BITS - cnt;
        high_limb = *up++;
        retval = (high_limb << tnc);
        low_limb = high_limb >> cnt;

        while (--n != 0) {
            high_limb = *up++;
            *rp++ = low_limb | (high_limb << tnc);
            low_limb = high_limb >> cnt;
        }
        *rp = low_limb;

        return retval;
    }
} // anonymous namespace

extern "C" {

/* ── trivial memory operations ─────────────────────────────────────────── */

void mpn_copyi(mp_ptr d, mp_srcptr s, mp_size_t n)
{
    std::memcpy(d, s, static_cast<std::size_t>(n) * sizeof(mp_limb_t));
}

void mpn_copyd(mp_ptr d, mp_srcptr s, mp_size_t n)
{
    std::memmove(d, s, static_cast<std::size_t>(n) * sizeof(mp_limb_t));
}

void mpn_zero(mp_ptr rp, mp_size_t n)
{
    std::memset(rp, 0, static_cast<std::size_t>(n) * sizeof(mp_limb_t));
}

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

/* ── comparison (scalar fallback) ───────────────────────────────────────── */

int mpn_cmp(mp_srcptr ap, mp_srcptr bp, mp_size_t n)
{
    return scalar_mpn_cmp(ap, bp, n);
}

/* ── addition with carry (scalar fallback) ──────────────────────────────── */
mp_limb_t mpn_add_n(mp_ptr rp, mp_srcptr ap, mp_srcptr bp, mp_size_t n)
{
    return scalar_mpn_add_n(rp, ap, bp, n);
}

/* ── subtraction with borrow (scalar fallback) ──────────────────────────── */

mp_limb_t mpn_sub_n(mp_ptr rp, mp_srcptr ap, mp_srcptr bp, mp_size_t n)
{
    return scalar_mpn_sub_n(rp, ap, bp, n);
}

/* ── left shift (scalar fallback, overlap-safe like upstream) ───────────── */
mp_limb_t mpn_lshift(mp_ptr rp, mp_srcptr up, mp_size_t n, unsigned int cnt)
{
    return scalar_mpn_lshift(rp, up, n, cnt);
}

/* ── right shift (scalar fallback, overlap-safe like upstream) ──────────── */
mp_limb_t mpn_rshift(mp_ptr rp, mp_srcptr up, mp_size_t n, unsigned int cnt)
{
    return scalar_mpn_rshift(rp, up, n, cnt);
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

/* ── vectorised zero-test ───────────────────────────────────────────────── *
 *
 * Scan MSB→LSB: high limbs are most likely non-zero in normalised numbers,
 * so we reject early.  Uses batch OR to detect any non-zero lane.
 */
int mpn_zero_p(mp_srcptr rp, mp_size_t n)
{
    mp_size_t i = n;
    const batch_t zero = batch_t(uint64_t(0));

    while (i >= static_cast<mp_size_t>(W)) {
        i -= static_cast<mp_size_t>(W);
        if (xsimd::any(batch_t::load_unaligned(rp + i) != zero))
            return 0;
    }
    while (i > 0) {
        if (rp[--i] != 0)
            return 0;
    }
    return 1;
}

} /* extern "C" */
