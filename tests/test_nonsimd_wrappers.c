// Implementation of non-SIMD wrappers
// This file includes the original non-SIMD implementations from mini-gmp.c

#include "test_nonsimd_wrappers.h"
#include <string.h>
#include <limits.h>
#include <assert.h>

#ifndef GMP_LIMB_BITS
#define GMP_LIMB_BITS (sizeof(mp_limb_t) * CHAR_BIT)
#endif

void mpn_copyi_nonsimd(mp_ptr d, mp_srcptr s, mp_size_t n) {
    mp_size_t i;
    for (i = 0; i < n; i++)
        d[i] = s[i];
}

void mpn_copyd_nonsimd(mp_ptr d, mp_srcptr s, mp_size_t n) {
    while (--n >= 0)
        d[n] = s[n];
}

void mpn_zero_nonsimd(mp_ptr rp, mp_size_t n) {
    while (--n >= 0)
        rp[n] = 0;
}

void mpn_com_nonsimd(mp_ptr rp, mp_srcptr up, mp_size_t n) {
    while (--n >= 0)
        *rp++ = ~*up++;
}

int mpn_cmp_nonsimd(mp_srcptr ap, mp_srcptr bp, mp_size_t n) {
    while (--n >= 0) {
        if (ap[n] != bp[n])
            return ap[n] > bp[n] ? 1 : -1;
    }
    return 0;
}

mp_limb_t mpn_add_n_nonsimd(mp_ptr rp, mp_srcptr ap, mp_srcptr bp, mp_size_t n) {
    mp_size_t i;
    mp_limb_t cy;

    for (i = 0, cy = 0; i < n; i++) {
        mp_limb_t a, b, r;
        a = ap[i];
        b = bp[i];
        r = a + cy;
        cy = (r < cy);
        r += b;
        cy += (r < b);
        rp[i] = r;
    }
    return cy;
}

mp_limb_t mpn_sub_n_nonsimd(mp_ptr rp, mp_srcptr ap, mp_srcptr bp, mp_size_t n) {
    mp_size_t i;
    mp_limb_t cy;

    for (i = 0, cy = 0; i < n; i++) {
        mp_limb_t a, b;
        a = ap[i];
        b = bp[i];
        b += cy;
        cy = (b < cy);
        cy += (a < b);
        rp[i] = a - b;
    }
    return cy;
}

mp_limb_t mpn_lshift_nonsimd(mp_ptr rp, mp_srcptr up, mp_size_t n, unsigned int cnt) {
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

mp_limb_t mpn_rshift_nonsimd(mp_ptr rp, mp_srcptr up, mp_size_t n, unsigned int cnt) {
    mp_limb_t high_limb, low_limb;
    unsigned int tnc;
    mp_limb_t retval;

    assert(n >= 1);
    assert(cnt >= 1);
    assert(cnt < GMP_LIMB_BITS);

    tnc = GMP_LIMB_BITS - cnt;
    high_limb = *up++;
    retval = high_limb << tnc;
    low_limb = high_limb >> cnt;

    while (--n != 0) {
        high_limb = *up++;
        *rp++ = low_limb | (high_limb << tnc);
        low_limb = high_limb >> cnt;
    }
    *rp = low_limb;

    return retval;
}

static unsigned gmp_popcount_limb(mp_limb_t x) {
    unsigned c;
    for (c = 0; x > 0;) {
        unsigned w = x - ((x >> 1) & 0x5555);
        w = ((w >> 2) & 0x3333) + (w & 0x3333);
        w = (w >> 4) + w;
        w = ((w >> 8) & 0x000f) + (w & 0x000f);
        c += w;
        if (GMP_LIMB_BITS > 16)
            x >>= 16;
        else
            x = 0;
    }
    return c;
}

mp_bitcnt_t mpn_popcount_nonsimd(mp_srcptr p, mp_size_t n) {
    mp_size_t i;
    mp_bitcnt_t c;
    for (c = 0, i = 0; i < n; i++)
        c += gmp_popcount_limb(p[i]);
    return c;
}

mp_bitcnt_t mpn_hamdist_nonsimd(mp_srcptr ap, mp_srcptr bp, mp_size_t n) {
    mp_size_t i;
    mp_bitcnt_t c;
    for (c = 0, i = 0; i < n; i++)
        c += gmp_popcount_limb(ap[i] ^ bp[i]);
    return c;
}

void mpn_and_n_nonsimd(mp_ptr rp, mp_srcptr ap, mp_srcptr bp, mp_size_t n) {
    while (--n >= 0)
        rp[n] = ap[n] & bp[n];
}

void mpn_ior_n_nonsimd(mp_ptr rp, mp_srcptr ap, mp_srcptr bp, mp_size_t n) {
    while (--n >= 0)
        rp[n] = ap[n] | bp[n];
}

void mpn_xor_n_nonsimd(mp_ptr rp, mp_srcptr ap, mp_srcptr bp, mp_size_t n) {
    while (--n >= 0)
        rp[n] = ap[n] ^ bp[n];
}

int mpn_zero_p_nonsimd(mp_srcptr rp, mp_size_t n) {
    while (--n >= 0) {
        if (rp[n] != 0)
            return 0;
    }
    return 1;
}
