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

// 2×2 minor of the rows (r0, r1) in columns (ci, cj), i.e.
//   | r0[ci] r0[cj] |
//   | r1[ci] r1[cj] |   =  r0[ci]*r1[cj] - r0[cj]*r1[ci]
// One fused library call, no temporaries.
inline void minor2(MiniMPZ& out,
                   const MiniMPZ& r0ci, const MiniMPZ& r0cj,
                   const MiniMPZ& r1ci, const MiniMPZ& r1cj)
{
    mpz_mul_sub_mul(out.get_mpz(), r0ci.get_mpz(), r1cj.get_mpz(),
                                   r0cj.get_mpz(), r1ci.get_mpz());
}

}  // namespace detail

// --- Dot products -------------------------------------------------------

// Fused dot product of any fixed dimension: a single mpz_dot_product call,
// which accumulates every term in stack scratch and normalizes once.
template<int N>
inline MiniMPZ dot_product(const StaticVector<N>& lhs, const StaticVector<N>& rhs) {
    static_assert(N > 0, "dot_product requires N > 0");
    // A std::array of MiniMPZ is not an array of mpz_t, so pass limb pointers.
    mpz_srcptr u[N], v[N];
    for (int i = 0; i < N; ++i) {
        u[i] = lhs[i].get_mpz();
        v[i] = rhs[i].get_mpz();
    }
    MiniMPZ result;
    mpz_dot_product(result.get_mpz(), N, u, v);
    return result;
}

// 4D dot product.
inline MiniMPZ dot_product4(const Vector4& lhs, const Vector4& rhs) {
    return dot_product<4>(lhs, rhs);
}

// --- Determinants -------------------------------------------------------

// 2×2 determinant: exactly one fused mpz_mul_sub_mul.
inline MiniMPZ determinant2(const Matrix2& m) {
    MiniMPZ result;
    mpz_mul_sub_mul(result.get_mpz(), m[0].get_mpz(), m[3].get_mpz(),
                                      m[1].get_mpz(), m[2].get_mpz());
    return result;
}

// 3×3 determinant via cofactor expansion along row 0:
//   det = a00*(a11*a22 - a12*a21)
//       - a01*(a10*a22 - a12*a20)
//       + a02*(a10*a21 - a11*a20)
// 3 fused 2×2 minors + 1 fused combine + 1 addmul = 5 library calls, versus
// 13 for the Sarrus form (which expands 6 triple products separately).
inline MiniMPZ determinant3(const Matrix3& m) {
    MiniMPZ result, m0, m1, m2;

    detail::minor2(m0, m[4], m[5], m[7], m[8]);   // a11*a22 - a12*a21
    detail::minor2(m1, m[3], m[5], m[6], m[8]);   // a10*a22 - a12*a20
    detail::minor2(m2, m[3], m[4], m[6], m[7]);   // a10*a21 - a11*a20

    mpz_mul_sub_mul(result.get_mpz(), m[0].get_mpz(), m0.get_mpz(),
                                      m[1].get_mpz(), m1.get_mpz());
    mpz_addmul(result.get_mpz(), m[2].get_mpz(), m2.get_mpz());
    return result;
}

// 4×4 determinant via Laplace expansion by complementary 2×2 minors
// (expansion along rows 0-1).  With A_ij the minor of rows {0,1} in columns
// {i,j} and B_ij that of rows {2,3}:
//
//   det = A01*B23 - A02*B13 + A03*B12 + A12*B03 - A13*B02 + A23*B01
//
// 12 fused minors + 3 fused combines + 2 adds = 17 library calls, versus 56
// for cofactor expansion through four full 3×3 determinants.  The minors are
// also reused across terms, so far fewer limb products are formed overall.
inline MiniMPZ determinant4(const Matrix4& m) {
    MiniMPZ a01, a02, a03, a12, a13, a23;
    MiniMPZ b01, b02, b03, b12, b13, b23;

    // Minors of rows 0,1 (entries m[0..3] and m[4..7]).
    detail::minor2(a01, m[0], m[1], m[4], m[5]);
    detail::minor2(a02, m[0], m[2], m[4], m[6]);
    detail::minor2(a03, m[0], m[3], m[4], m[7]);
    detail::minor2(a12, m[1], m[2], m[5], m[6]);
    detail::minor2(a13, m[1], m[3], m[5], m[7]);
    detail::minor2(a23, m[2], m[3], m[6], m[7]);

    // Minors of rows 2,3 (entries m[8..11] and m[12..15]).
    detail::minor2(b01, m[8],  m[9],  m[12], m[13]);
    detail::minor2(b02, m[8],  m[10], m[12], m[14]);
    detail::minor2(b03, m[8],  m[11], m[12], m[15]);
    detail::minor2(b12, m[9],  m[10], m[13], m[14]);
    detail::minor2(b13, m[9],  m[11], m[13], m[15]);
    detail::minor2(b23, m[10], m[11], m[14], m[15]);

    MiniMPZ result, t2, t3;
    mpz_mul_sub_mul(result.get_mpz(), a01.get_mpz(), b23.get_mpz(),
                                      a02.get_mpz(), b13.get_mpz());
    mpz_mul_add_mul(t2.get_mpz(),     a03.get_mpz(), b12.get_mpz(),
                                      a12.get_mpz(), b03.get_mpz());
    mpz_mul_sub_mul(t3.get_mpz(),     a23.get_mpz(), b01.get_mpz(),
                                      a13.get_mpz(), b02.get_mpz());
    mpz_add(result.get_mpz(), result.get_mpz(), t2.get_mpz());
    mpz_add(result.get_mpz(), result.get_mpz(), t3.get_mpz());
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
