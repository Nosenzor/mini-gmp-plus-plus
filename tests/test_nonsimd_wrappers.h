#ifndef TEST_NONSIMD_WRAPPERS_H
#define TEST_NONSIMD_WRAPPERS_H

#include "mini-gmp.h"

#ifdef __cplusplus
extern "C" {
#endif

void mpn_copyi_nonsimd(mp_ptr d, mp_srcptr s, mp_size_t n);
void mpn_copyd_nonsimd(mp_ptr d, mp_srcptr s, mp_size_t n);
void mpn_zero_nonsimd(mp_ptr rp, mp_size_t n);
void mpn_com_nonsimd(mp_ptr rp, mp_srcptr up, mp_size_t n);
int mpn_cmp_nonsimd(mp_srcptr ap, mp_srcptr bp, mp_size_t n);
mp_limb_t mpn_add_n_nonsimd(mp_ptr rp, mp_srcptr ap, mp_srcptr bp, mp_size_t n);
mp_limb_t mpn_sub_n_nonsimd(mp_ptr rp, mp_srcptr ap, mp_srcptr bp, mp_size_t n);
mp_limb_t mpn_lshift_nonsimd(mp_ptr rp, mp_srcptr up, mp_size_t n, unsigned int cnt);
mp_limb_t mpn_rshift_nonsimd(mp_ptr rp, mp_srcptr up, mp_size_t n, unsigned int cnt);
mp_bitcnt_t mpn_popcount_nonsimd(mp_srcptr p, mp_size_t n);
mp_bitcnt_t mpn_hamdist_nonsimd(mp_srcptr ap, mp_srcptr bp, mp_size_t n);
void mpn_and_n_nonsimd(mp_ptr rp, mp_srcptr ap, mp_srcptr bp, mp_size_t n);
void mpn_ior_n_nonsimd(mp_ptr rp, mp_srcptr ap, mp_srcptr bp, mp_size_t n);
void mpn_xor_n_nonsimd(mp_ptr rp, mp_srcptr ap, mp_srcptr bp, mp_size_t n);
int mpn_zero_p_nonsimd(mp_srcptr rp, mp_size_t n);

#ifdef __cplusplus
}
#endif

#endif
