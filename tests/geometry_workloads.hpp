#ifndef MINI_GMP_PLUS_GEOMETRY_WORKLOADS_HPP
#define MINI_GMP_PLUS_GEOMETRY_WORKLOADS_HPP

#include "../MiniMPZ.hpp"
#include <array>

namespace mini_gmp_plus_geometry {

// Fixed-size vector and matrix types backed by MiniMPZ elements.
template<int N>          using StaticVector = std::array<MiniMPZ, N>;
template<int M, int N>   using StaticMatrix = std::array<MiniMPZ, M * N>;

using Vector4 = StaticVector<4>;
using Matrix2 = StaticMatrix<2, 2>;
using Matrix3 = StaticMatrix<3, 3>;
using Matrix4 = StaticMatrix<4, 4>;

namespace detail {

// Compute det3 of a 3×3 matrix into `out` using Sarrus rule.
// `out` is reset to zero on entry; `t` is a scratch buffer (content undefined).
// Both must be pre-initialised (default-constructed MiniMPZ is fine).
//
// Sarrus: +a00*a11*a22  +a01*a12*a20  +a02*a10*a21
//         -a02*a11*a20  -a00*a12*a21  -a01*a10*a22
inline void compute_det3_fused(
    MiniMPZ& out, MiniMPZ& t,
    const MiniMPZ& a00, const MiniMPZ& a01, const MiniMPZ& a02,
    const MiniMPZ& a10, const MiniMPZ& a11, const MiniMPZ& a12,
    const MiniMPZ& a20, const MiniMPZ& a21, const MiniMPZ& a22)
{
    mpz_set_ui(out.get_mpz(), 0);

    mpz_mul(t.get_mpz(), a11.get_mpz(), a22.get_mpz());
    mpz_addmul(out.get_mpz(), a00.get_mpz(), t.get_mpz());

    mpz_mul(t.get_mpz(), a12.get_mpz(), a20.get_mpz());
    mpz_addmul(out.get_mpz(), a01.get_mpz(), t.get_mpz());

    mpz_mul(t.get_mpz(), a10.get_mpz(), a21.get_mpz());
    mpz_addmul(out.get_mpz(), a02.get_mpz(), t.get_mpz());

    mpz_mul(t.get_mpz(), a11.get_mpz(), a20.get_mpz());
    mpz_submul(out.get_mpz(), a02.get_mpz(), t.get_mpz());

    mpz_mul(t.get_mpz(), a12.get_mpz(), a21.get_mpz());
    mpz_submul(out.get_mpz(), a00.get_mpz(), t.get_mpz());

    mpz_mul(t.get_mpz(), a10.get_mpz(), a22.get_mpz());
    mpz_submul(out.get_mpz(), a01.get_mpz(), t.get_mpz());
}

}  // namespace detail

// --- Dot products -------------------------------------------------------

// Fused 4D dot product: 1 mpz_mul + 3 mpz_addmul — zero C++ temporaries.
inline MiniMPZ dot_product4(const Vector4& lhs, const Vector4& rhs) {
    MiniMPZ result;
    mpz_mul   (result.get_mpz(), lhs[0].get_mpz(), rhs[0].get_mpz());
    mpz_addmul(result.get_mpz(), lhs[1].get_mpz(), rhs[1].get_mpz());
    mpz_addmul(result.get_mpz(), lhs[2].get_mpz(), rhs[2].get_mpz());
    mpz_addmul(result.get_mpz(), lhs[3].get_mpz(), rhs[3].get_mpz());
    return result;
}

// Generic fused dot product for any fixed dimension N.
template<int N>
inline MiniMPZ dot_product(const StaticVector<N>& lhs, const StaticVector<N>& rhs) {
    static_assert(N > 0, "dot_product requires N > 0");
    MiniMPZ result;
    mpz_mul(result.get_mpz(), lhs[0].get_mpz(), rhs[0].get_mpz());
    for (int i = 1; i < N; ++i)
        mpz_addmul(result.get_mpz(), lhs[i].get_mpz(), rhs[i].get_mpz());
    return result;
}

// --- Determinants -------------------------------------------------------

// 2×2 determinant: 1 mpz_mul + 1 mpz_submul — zero C++ temporaries.
inline MiniMPZ determinant2(const Matrix2& m) {
    MiniMPZ result;
    mpz_mul   (result.get_mpz(), m[0].get_mpz(), m[3].get_mpz());
    mpz_submul(result.get_mpz(), m[1].get_mpz(), m[2].get_mpz());
    return result;
}

// 3×3 determinant via Sarrus: 6 multiply-accumulate ops, 1 scratch temporary.
inline MiniMPZ determinant3(const Matrix3& m) {
    MiniMPZ result, t;
    detail::compute_det3_fused(result, t,
        m[0], m[1], m[2],
        m[3], m[4], m[5],
        m[6], m[7], m[8]);
    return result;
}

// 4×4 determinant via cofactor expansion along row 0.
// Uses one reusable 3×3 sub-result and one scratch temporary — 2 MiniMPZ objects total.
inline MiniMPZ determinant4(const Matrix4& m) {
    MiniMPZ result, sub, t;

    // C00: minor(0,0) — rows 1-3, cols 1-3  (sign +)
    detail::compute_det3_fused(sub, t, m[5], m[6], m[7], m[9], m[10], m[11], m[13], m[14], m[15]);
    mpz_addmul(result.get_mpz(), m[0].get_mpz(), sub.get_mpz());

    // C01: minor(0,1) — rows 1-3, cols 0,2,3  (sign -)
    detail::compute_det3_fused(sub, t, m[4], m[6], m[7], m[8], m[10], m[11], m[12], m[14], m[15]);
    mpz_submul(result.get_mpz(), m[1].get_mpz(), sub.get_mpz());

    // C02: minor(0,2) — rows 1-3, cols 0,1,3  (sign +)
    detail::compute_det3_fused(sub, t, m[4], m[5], m[7], m[8], m[9], m[11], m[12], m[13], m[15]);
    mpz_addmul(result.get_mpz(), m[2].get_mpz(), sub.get_mpz());

    // C03: minor(0,3) — rows 1-3, cols 0,1,2  (sign -)
    detail::compute_det3_fused(sub, t, m[4], m[5], m[6], m[8], m[9], m[10], m[12], m[13], m[14]);
    mpz_submul(result.get_mpz(), m[3].get_mpz(), sub.get_mpz());

    return result;
}

// Template dispatch API — call as determinant<2>(m), determinant<3>(m), etc.
template<int N> inline MiniMPZ determinant(const StaticMatrix<N,N>&);
template<> inline MiniMPZ determinant<2>(const Matrix2& m) { return determinant2(m); }
template<> inline MiniMPZ determinant<3>(const Matrix3& m) { return determinant3(m); }
template<> inline MiniMPZ determinant<4>(const Matrix4& m) { return determinant4(m); }

// --- Other geometry helpers ---------------------------------------------

inline MiniMPZ gcd_value(const MiniMPZ& lhs, const MiniMPZ& rhs) {
    MiniMPZ result;
    mpz_gcd(result.get_mpz(), lhs.get_mpz(), rhs.get_mpz());
    return result;
}

}  // namespace mini_gmp_plus_geometry

#endif  // MINI_GMP_PLUS_GEOMETRY_WORKLOADS_HPP
