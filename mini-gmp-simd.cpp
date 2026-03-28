/* mini-gmp-simd.cpp — SIMD-accelerated mpn_ functions using xsimd.
 *
 * Compiled only when MINI_GMP_ENABLE_SIMD=ON (defines MINI_GMP_SIMD).
 * Requires xsimd ≥ 14 and C++17.
 *
 * xsimd 14 element-access API used here:
 *   batch.get(i)          — extract element at runtime index i → T
 *   batch_bool.get(i)     — extract element at runtime index i → bool
 *   xsimd::insert(v, val, xsimd::index<I>{}) — return v with lane I replaced by val
 *   xsimd::slide_left<N>(v)  — shift elements toward higher indices by N bytes, fill index 0 with 0
 *                              result: [0, v[0], v[1], ..., v[W-2]]
 *   xsimd::slide_right<N>(v) — shift elements toward lower  indices by N bytes, fill last with 0
 *                              result: [v[1], v[2], ..., v[W-1], 0]
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
    static constexpr std::size_t LAST = W - 1;

    /* Byte distance between adjacent uint64_t lanes. */
    static constexpr std::size_t STRIDE = sizeof(uint64_t);

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

/* ── comparison (MSB-first lexicographic) ───────────────────────────────── */

int mpn_cmp(mp_srcptr ap, mp_srcptr bp, mp_size_t n)
{
    mp_size_t i = n;

    while (i >= static_cast<mp_size_t>(W)) {
        i -= static_cast<mp_size_t>(W);
        const batch_t va = batch_t::load_unaligned(ap + i);
        const batch_t vb = batch_t::load_unaligned(bp + i);
        if (!xsimd::all(va == vb)) {
            for (int j = static_cast<int>(W) - 1; j >= 0; j--) {
                const uint64_t a = va.get(static_cast<std::size_t>(j));
                const uint64_t b = vb.get(static_cast<std::size_t>(j));
                if (a != b)
                    return a > b ? 1 : -1;
            }
        }
    }

    while (--i >= 0) {
        if (ap[i] != bp[i])
            return ap[i] > bp[i] ? 1 : -1;
    }
    return 0;
}

/* ── addition with carry (carry-select parallel prefix) ─────────────────── *
 *
 * The carry chain is the only serial dependency. We break it by processing W
 * limbs per SIMD block:
 *
 *   sum_nc[j] = a[j] + b[j]          (no carry input)
 *   gen[j]    = sum_nc[j] < a[j]     (carry generated regardless of c_in)
 *   prop[j]   = sum_nc[j] == MAX64   (carry propagates if c_in == 1)
 *
 * W-step serial prefix resolves carry into each lane:
 *   c[0] = carry_in
 *   c[j] = gen[j-1] | (prop[j-1] & c[j-1])
 *
 * carry out of block: gen[W-1] | (prop[W-1] & c[W-1])
 * result[j] = sum_nc[j] + c[j]
 *
 * The carry crosses block boundaries serially (one carry per W limbs
 * instead of one per limb): ~W× fewer serial steps on the bulk.
 */
mp_limb_t mpn_add_n(mp_ptr rp, mp_srcptr ap, mp_srcptr bp, mp_size_t n)
{
    mp_size_t i = 0;
    mp_limb_t carry = 0;

    for (; i + static_cast<mp_size_t>(W) <= n; i += static_cast<mp_size_t>(W)) {
        const batch_t va     = batch_t::load_unaligned(ap + i);
        const batch_t vb     = batch_t::load_unaligned(bp + i);
        const batch_t sum_nc = va + vb;

        const auto gen  = sum_nc < va;                       /* batch_bool */
        const auto prop = sum_nc == batch_t(~uint64_t(0));  /* batch_bool */

        /* W-step carry prefix; always fully unrolled by the compiler. */
        uint64_t c[W];
        c[0] = carry;
        for (std::size_t j = 1; j < W; j++)
            c[j] = (gen.get(j - 1) | (prop.get(j - 1) & static_cast<bool>(c[j - 1]))) ? 1u : 0u;
        carry = (gen.get(W - 1) | (prop.get(W - 1) & static_cast<bool>(c[W - 1]))) ? 1u : 0u;

        (sum_nc + batch_t::load_unaligned(c)).store_unaligned(rp + i);
    }

    for (; i < n; i++) {
        const mp_limb_t a = ap[i], b = bp[i];
        mp_limb_t r = a + carry; carry  = (r < carry);
        r += b;                  carry += (r < b);
        rp[i] = r;
    }
    return carry;
}

/* ── subtraction with borrow (mirror of mpn_add_n) ─────────────────────── */

mp_limb_t mpn_sub_n(mp_ptr rp, mp_srcptr ap, mp_srcptr bp, mp_size_t n)
{
    mp_size_t i = 0;
    mp_limb_t borrow = 0;

    for (; i + static_cast<mp_size_t>(W) <= n; i += static_cast<mp_size_t>(W)) {
        const batch_t va      = batch_t::load_unaligned(ap + i);
        const batch_t vb      = batch_t::load_unaligned(bp + i);
        const batch_t diff_nb = va - vb;

        const auto gen  = va < vb;
        const auto prop = va == vb;

        uint64_t c[W];
        c[0] = borrow;
        for (std::size_t j = 1; j < W; j++)
            c[j] = (gen.get(j - 1) | (prop.get(j - 1) & static_cast<bool>(c[j - 1]))) ? 1u : 0u;
        borrow = (gen.get(W - 1) | (prop.get(W - 1) & static_cast<bool>(c[W - 1]))) ? 1u : 0u;

        (diff_nb - batch_t::load_unaligned(c)).store_unaligned(rp + i);
    }

    for (; i < n; i++) {
        const mp_limb_t a = ap[i], b = bp[i];
        mp_limb_t bx = b + borrow; borrow  = (bx < borrow);
        borrow += (a < bx);
        rp[i] = a - bx;
    }
    return borrow;
}

/* ── left shift (SIMD: carry flows low→high) ────────────────────────────── *
 *
 * rp[j] = (up[j] << cnt) | (up[j-1] >> tnc)
 *
 * Process batches low→high. For each W-lane batch:
 *   sl = va << cnt  (each lane shifted left by cnt bits)
 *   sr = va >> tnc  (bits spilling from lane j into lane j+1)
 *
 * slide_left<STRIDE>(sr) shifts sr toward higher indices by one element:
 *   [0, sr[0], sr[1], ..., sr[W-2]]
 * Then inject the inter-batch carry into lane 0.
 *
 * carry_out = sr[W-1] of the last batch = up[n-1] >> tnc.
 */
mp_limb_t mpn_lshift(mp_ptr rp, mp_srcptr up, mp_size_t n, unsigned int cnt)
{
    assert(n >= 1);
    assert(cnt >= 1);
    assert(cnt < GMP_LIMB_BITS);

    unsigned int tnc = GMP_LIMB_BITS - cnt;
    mp_limb_t carry = 0;

    mp_size_t i = 0;
    for (; i + static_cast<mp_size_t>(W) <= n; i += static_cast<mp_size_t>(W)) {
        const batch_t va = batch_t::load_unaligned(up + i);
        const batch_t sl = va << static_cast<int>(cnt);
        const batch_t sr = va >> static_cast<int>(tnc);

        /* spill[j] = sr[j-1]: shift sr toward higher indices by one lane, inject carry at 0. */
        batch_t spill = xsimd::slide_left<STRIDE>(sr);
        spill = xsimd::insert(spill, carry, xsimd::index<0>{});

        (sl | spill).store_unaligned(rp + i);
        carry = sr.get(LAST);
    }

    /* Scalar tail */
    for (; i < n; i++) {
        mp_limb_t s = up[i];
        rp[i] = (s << cnt) | carry;
        carry = s >> tnc;
    }

    return carry;
}

/* ── right shift (SIMD: carry flows high→low) ───────────────────────────── *
 *
 * rp[j] = (up[j] >> cnt) | (up[j+1] << tnc)
 *
 * Process the scalar tail (top n%W limbs) first, then SIMD batches high→low.
 * For each W-lane batch:
 *   sr = va >> cnt  (each lane shifted right by cnt bits)
 *   sl = va << tnc  (bits spilling from lane j into lane j-1)
 *
 * slide_right<STRIDE>(sl) shifts sl toward lower indices by one element:
 *   [sl[1], sl[2], ..., sl[W-1], 0]
 * Then inject the inter-batch carry into lane W-1.
 *
 * carry_out (return value) = sl[0] of the lowest batch = up[0] << tnc.
 */
mp_limb_t mpn_rshift(mp_ptr rp, mp_srcptr up, mp_size_t n, unsigned int cnt)
{
    assert(n >= 1);
    assert(cnt >= 1);
    assert(cnt < GMP_LIMB_BITS);

    unsigned int tnc = GMP_LIMB_BITS - cnt;

    mp_size_t simd_limit = (static_cast<mp_size_t>(n) / W) * W;
    mp_limb_t carry = 0;

    /* Scalar tail: top (n % W) limbs, processed high→low. */
    mp_size_t j = static_cast<mp_size_t>(n);
    while (j-- > simd_limit) {
        rp[j] = (up[j] >> cnt) | carry;
        carry = up[j] << tnc;
    }

    /* SIMD batches, high→low. */
    for (mp_size_t b = simd_limit / W; b-- > 0; ) {
        mp_size_t i = b * W;
        const batch_t va = batch_t::load_unaligned(up + i);
        const batch_t sr = va >> static_cast<int>(cnt);
        const batch_t sl = va << static_cast<int>(tnc);

        /* spill[j] = sl[j+1]: shift sl toward lower indices by one lane, inject carry at LAST. */
        batch_t spill = xsimd::slide_right<STRIDE>(sl);
        spill = xsimd::insert(spill, carry, xsimd::index<LAST>{});

        (sr | spill).store_unaligned(rp + i);
        carry = sl.get(0);
    }

    return carry;
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
