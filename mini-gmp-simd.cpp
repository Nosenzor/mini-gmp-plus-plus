/* mini-gmp-simd.cpp — SIMD-accelerated mpn_ functions using xsimd.
 *
 * Compiled only when MINI_GMP_ENABLE_SIMD=ON (defines MINI_GMP_SIMD).
 * Requires xsimd ≥ 14 and C++17.
 *
 * xsimd 14 element-access API used here:
 *   batch.get(i)          — extract element at runtime index i → T
 *   batch_bool.get(i)     — extract element at runtime index i → bool
 *   xsimd::insert(v, val, xsimd::index<I>{}) — return v with lane I replaced by val
 *   xsimd::slide_left<N>(v)  — shift register left  by N bytes, fill right with 0
 *   xsimd::slide_right<N>(v) — shift register right by N bytes, fill left  with 0
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

/* ── left shift (rolling-window, fully in xsimd) ────────────────────────── *
 *
 * rp[i] = (up[i] << cnt) | (up[i-1] >> tnc)
 *
 * The per-lane spill is built entirely with xsimd operations:
 *
 *   slide_right<STRIDE>(sr)            →  [0, sr[0], …, sr[W-2]]
 *   insert(…, prev_spill, index<0>{})  →  [prev_spill, sr[0], …, sr[W-2]]
 *
 * No inner loops, no intermediate scalar arrays.
 */
mp_limb_t mpn_lshift(mp_ptr rp, mp_srcptr up, mp_size_t n, unsigned int cnt)
{
    assert(n >= 1);
    assert(cnt >= 1 && cnt < static_cast<unsigned>(GMP_LIMB_BITS));
    const unsigned int tnc    = static_cast<unsigned>(GMP_LIMB_BITS) - cnt;
    const mp_limb_t    retval = up[n - 1] >> tnc;

    mp_limb_t prev_spill = 0;
    mp_size_t i = 0;

    for (; i + static_cast<mp_size_t>(W) <= n; i += static_cast<mp_size_t>(W)) {
        const batch_t v    = batch_t::load_unaligned(up + i);
        const batch_t sl   = v << static_cast<int>(cnt);
        const batch_t sr   = v >> static_cast<int>(tnc);

        /* [prev_spill, sr[0], sr[1], …, sr[W-2]] */
        const batch_t spill = xsimd::insert(
            xsimd::slide_right<STRIDE>(sr), prev_spill, xsimd::index<0>{});

        (sl | spill).store_unaligned(rp + i);
        prev_spill = sr.get(LAST);
    }

    /* Scalar tail. */
    for (; i < n; i++) {
        const mp_limb_t limb = up[i];
        rp[i]      = (limb << cnt) | prev_spill;
        prev_spill  = limb >> tnc;
    }
    return retval;
}

/* ── right shift (rolling-window, fully in xsimd) ───────────────────────── *
 *
 * rp[i] = (up[i] >> cnt) | (up[i+1] << tnc)
 *
 * Processed MSB→LSB so next_high flows naturally between blocks:
 *
 *   slide_left<STRIDE>(sl)               →  [sl[1], …, sl[W-1], 0]
 *   insert(…, next_high, index<LAST>{})  →  [sl[1], …, sl[W-1], next_high]
 */
mp_limb_t mpn_rshift(mp_ptr rp, mp_srcptr up, mp_size_t n, unsigned int cnt)
{
    assert(n >= 1);
    assert(cnt >= 1 && cnt < static_cast<unsigned>(GMP_LIMB_BITS));
    const unsigned int tnc = static_cast<unsigned>(GMP_LIMB_BITS) - cnt;

    mp_limb_t next_high = 0;
    mp_size_t i = n;

    while (i >= static_cast<mp_size_t>(W)) {
        i -= static_cast<mp_size_t>(W);
        const batch_t v    = batch_t::load_unaligned(up + i);
        const batch_t sr   = v >> static_cast<int>(cnt);
        const batch_t sl   = v << static_cast<int>(tnc);

        /* [sl[1], sl[2], …, sl[W-1], next_high] */
        const batch_t spill = xsimd::insert(
            xsimd::slide_left<STRIDE>(sl), next_high, xsimd::index<LAST>{});

        (sr | spill).store_unaligned(rp + i);
        next_high = sl.get(0);
    }

    /* Scalar tail. */
    while (i > 0) {
        i--;
        rp[i]     = (up[i] >> cnt) | next_high;
        next_high  = up[i] << tnc;
    }
    return next_high;
}

} /* extern "C" */
