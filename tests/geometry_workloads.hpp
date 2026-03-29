#ifndef MINI_GMP_PLUS_GEOMETRY_WORKLOADS_HPP
#define MINI_GMP_PLUS_GEOMETRY_WORKLOADS_HPP

#include "../MiniMPZ.hpp"
#include <array>

namespace mini_gmp_plus_geometry {

using Vector4 = std::array<MiniMPZ, 4>;
using Matrix2 = std::array<MiniMPZ, 4>;
using Matrix3 = std::array<MiniMPZ, 9>;
using Matrix4 = std::array<MiniMPZ, 16>;

inline MiniMPZ dot_product4(const Vector4& lhs, const Vector4& rhs) {
    MiniMPZ result = lhs[0] * rhs[0];
    result += lhs[1] * rhs[1];
    result += lhs[2] * rhs[2];
    result += lhs[3] * rhs[3];
    return result;
}

inline MiniMPZ determinant2(const Matrix2& m) {
    return m[0] * m[3] - m[1] * m[2];
}

inline MiniMPZ determinant3(const Matrix3& m) {
    return m[0] * (m[4] * m[8] - m[5] * m[7])
         - m[1] * (m[3] * m[8] - m[5] * m[6])
         + m[2] * (m[3] * m[7] - m[4] * m[6]);
}

inline MiniMPZ determinant4(const Matrix4& m) {
    const MiniMPZ subfactor00 = m[10] * m[15] - m[11] * m[14];
    const MiniMPZ subfactor01 = m[9] * m[15] - m[11] * m[13];
    const MiniMPZ subfactor02 = m[9] * m[14] - m[10] * m[13];
    const MiniMPZ subfactor03 = m[8] * m[15] - m[11] * m[12];
    const MiniMPZ subfactor04 = m[8] * m[14] - m[10] * m[12];
    const MiniMPZ subfactor05 = m[8] * m[13] - m[9] * m[12];

    return m[0] * (m[5] * subfactor00 - m[6] * subfactor01 + m[7] * subfactor02)
         - m[1] * (m[4] * subfactor00 - m[6] * subfactor03 + m[7] * subfactor04)
         + m[2] * (m[4] * subfactor01 - m[5] * subfactor03 + m[7] * subfactor05)
         - m[3] * (m[4] * subfactor02 - m[5] * subfactor04 + m[6] * subfactor05);
}

inline MiniMPZ gcd_value(const MiniMPZ& lhs, const MiniMPZ& rhs) {
    MiniMPZ result;
    mpz_gcd(result.get_mpz(), lhs.get_mpz(), rhs.get_mpz());
    return result;
}

}  // namespace mini_gmp_plus_geometry

#endif  // MINI_GMP_PLUS_GEOMETRY_WORKLOADS_HPP
