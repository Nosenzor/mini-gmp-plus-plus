// MiniMPF.hpp
#ifndef MINIMPF_HPP
#define MINIMPF_HPP

#include "mini-gmp-plus-config.hpp"
#include "MiniMPZ.hpp"
#include <cmath>
#include <functional>
#include <limits>
#include <string>
#include <ostream>


class MiniMPF {
private:
    MiniMPZ m_Mantisse;
    int m_Exponant{};

     // Normalize the floating point representation
     void normalize() {
         if (MINI_GMP_PLUS_EXPECT(m_Mantisse.sign() == 0, 0)) {
             m_Exponant = 0;
             return;
         }
         // Use mpz_scan1 to find trailing zeros and shift them out in one operation
         mp_bitcnt_t trailing_zeros = mpz_scan1(m_Mantisse.get_mpz(), 0);
         if (MINI_GMP_PLUS_EXPECT(trailing_zeros > 0, 1)) {
             mpz_tdiv_q_2exp(m_Mantisse.get_mpz(), m_Mantisse.get_mpz(), trailing_zeros);
             m_Exponant += static_cast<int>(trailing_zeros);
         }
     }

public:
    // Constructors
    MiniMPF() : m_Mantisse(0L), m_Exponant(0) {}

    explicit
    MiniMPF(double val) {
        if (val == 0.0) {
            m_Mantisse = MiniMPZ(0L);
            m_Exponant = 0;
        } else {
            const bool negative = (val < 0.0);
            double abs_val = negative ? -val : val;
            int exp = 0;

            // Use frexp to extract exponent - faster than loops
            // frexp gives abs_val in [0.5, 1.0) and exp such that abs_val * 2^exp = original
            abs_val = std::frexp(abs_val, &exp);
            // Scale to [1.0, 2.0)
            abs_val *= 2.0;
            exp--;

            // Convert to integer representation: abs_val is in [1.0, 2.0)
            // We want mantissa in [2^52, 2^53) so that value = mantissa * 2^(exp-52)
            // This gives 53 bits of precision (52 explicit + 1 implicit)
            long mantissa = static_cast<long>(abs_val * (1LL << 53));
            if (negative) {
                mantissa = -mantissa;
            }

            m_Mantisse = MiniMPZ(mantissa);
            m_Exponant = exp - 53;
            // normalize() skipped: double constructor already produces normalized mantissa
        }
    }

    MiniMPF(const MiniMPZ& mantisse, int exponent = 0) : m_Mantisse(mantisse), m_Exponant(exponent) {
        normalize();
    }

    // Copy constructor
    MiniMPF(const MiniMPF& other) : m_Mantisse(other.m_Mantisse), m_Exponant(other.m_Exponant) {}

    // Move constructor
    MiniMPF(MiniMPF&& other) noexcept : m_Mantisse(std::move(other.m_Mantisse)), m_Exponant(other.m_Exponant) {
        other.m_Exponant = 0;
    }

    // Destructor - default is fine since MiniMPZ handles its own cleanup
    ~MiniMPF() = default;

    // Copy assignment
    MiniMPF& operator=(const MiniMPF& other) {
        if (this != &other) {
            m_Mantisse = other.m_Mantisse;
            m_Exponant = other.m_Exponant;
        }
        return *this;
    }

    // Move assignment
    MiniMPF& operator=(MiniMPF&& other) noexcept {
        if (this != &other) {
            m_Mantisse = std::move(other.m_Mantisse);
            m_Exponant = other.m_Exponant;
            other.m_Exponant = 0;
        }
        return *this;
    }

    // Swap method
    void swap(MiniMPF& other) noexcept {
        using std::swap;
        swap(m_Mantisse, other.m_Mantisse);
        swap(m_Exponant, other.m_Exponant);
    }

    // Sign methods
    bool IsPositive() const { return m_Mantisse.sign() > 0; }
    bool IsNegative() const { return m_Mantisse.sign() < 0; }
    bool IsZero() const { return m_Mantisse.sign() == 0; }
    int Sign() const { return m_Mantisse.sign(); }

    void FlipSign() { m_Mantisse = -m_Mantisse; }

    // Accessors
    const MiniMPZ& Mantisse() const { return m_Mantisse; }
    int Exponent() const { return m_Exponant; }

    // Comparison methods
    int compare(const MiniMPF& other) const {
        if (IsZero() && other.IsZero()) return 0;
        
        int s1 = Sign();
        int s2 = other.Sign();
        
        if (s1 != s2) {
            return (s1 > s2) ? 1 : -1;
        }
        
        if (s1 == 0) return 0;

        MiniMPZ lhs = m_Mantisse;
        MiniMPZ rhs = other.m_Mantisse;

        if (m_Exponant < other.m_Exponant) {
            const int shift = other.m_Exponant - m_Exponant;
            mpz_mul_2exp(rhs.get_mpz(), rhs.get_mpz(), static_cast<mp_bitcnt_t>(shift));
        } else if (m_Exponant > other.m_Exponant) {
            const int shift = m_Exponant - other.m_Exponant;
            mpz_mul_2exp(lhs.get_mpz(), lhs.get_mpz(), static_cast<mp_bitcnt_t>(shift));
        }

        if (lhs > rhs) return 1;
        if (lhs < rhs) return -1;
        return 0;
    }

    // Comparison operators
    bool operator==(const MiniMPF& other) const { return compare(other) == 0; }
    bool operator!=(const MiniMPF& other) const { return compare(other) != 0; }
    bool operator<(const MiniMPF& other) const { return compare(other) < 0; }
    bool operator<=(const MiniMPF& other) const { return compare(other) <= 0; }
    bool operator>(const MiniMPF& other) const { return compare(other) > 0; }
    bool operator>=(const MiniMPF& other) const { return compare(other) >= 0; }

    // Arithmetic operators - member functions
    MiniMPF& operator+=(const MiniMPF& other) {
        if (MINI_GMP_PLUS_EXPECT(IsZero(), 0)) {
            *this = other;
            return *this;
        }
        if (MINI_GMP_PLUS_EXPECT(other.IsZero(), 0)) {
            return *this;
        }

        if (MINI_GMP_PLUS_EXPECT(m_Exponant == other.m_Exponant, 1)) {
            mpz_add(m_Mantisse.get_mpz(), m_Mantisse.get_mpz(), other.m_Mantisse.get_mpz());
        } else if (MINI_GMP_PLUS_EXPECT(m_Exponant < other.m_Exponant, 0)) {
            // Scale other's mantissa up and add to this
            mpz_t temp;
            mpz_init(temp);
            const mp_bitcnt_t exp_diff = static_cast<mp_bitcnt_t>(other.m_Exponant - m_Exponant);
            mpz_mul_2exp(temp, other.m_Mantisse.get_mpz(), exp_diff);
            mpz_add(m_Mantisse.get_mpz(), m_Mantisse.get_mpz(), temp);
            mpz_clear(temp);
        } else {
            // Scale this's mantissa up
            mpz_t temp;
            mpz_init(temp);
            const mp_bitcnt_t exp_diff = static_cast<mp_bitcnt_t>(m_Exponant - other.m_Exponant);
            mpz_mul_2exp(temp, m_Mantisse.get_mpz(), exp_diff);
            mpz_add(m_Mantisse.get_mpz(), temp, other.m_Mantisse.get_mpz());
            mpz_clear(temp);
            m_Exponant = other.m_Exponant;
        }

        // Only normalize if result is even (has trailing zeros)
        if (MINI_GMP_PLUS_EXPECT(mpz_even_p(m_Mantisse.get_mpz()), 0)) {
            normalize();
        }
        return *this;
    }

    MiniMPF& operator-=(const MiniMPF& other) {
        if (MINI_GMP_PLUS_EXPECT(other.IsZero(), 0)) {
            return *this;
        }
        if (MINI_GMP_PLUS_EXPECT(IsZero(), 0)) {
            *this = -other;
            return *this;
        }

        if (MINI_GMP_PLUS_EXPECT(m_Exponant == other.m_Exponant, 1)) {
            mpz_sub(m_Mantisse.get_mpz(), m_Mantisse.get_mpz(), other.m_Mantisse.get_mpz());
        } else if (MINI_GMP_PLUS_EXPECT(m_Exponant < other.m_Exponant, 0)) {
            // Scale other's mantissa up and subtract from this
            mpz_t temp;
            mpz_init(temp);
            const mp_bitcnt_t exp_diff = static_cast<mp_bitcnt_t>(other.m_Exponant - m_Exponant);
            mpz_mul_2exp(temp, other.m_Mantisse.get_mpz(), exp_diff);
            mpz_sub(m_Mantisse.get_mpz(), m_Mantisse.get_mpz(), temp);
            mpz_clear(temp);
        } else {
            // Scale this's mantissa up
            mpz_t temp;
            mpz_init(temp);
            const mp_bitcnt_t exp_diff = static_cast<mp_bitcnt_t>(m_Exponant - other.m_Exponant);
            mpz_mul_2exp(temp, m_Mantisse.get_mpz(), exp_diff);
            mpz_sub(m_Mantisse.get_mpz(), temp, other.m_Mantisse.get_mpz());
            mpz_clear(temp);
            m_Exponant = other.m_Exponant;
        }

        // Only normalize if result is even (has trailing zeros)
        if (MINI_GMP_PLUS_EXPECT(mpz_even_p(m_Mantisse.get_mpz()), 0)) {
            normalize();
        }
        return *this;
    }

    MiniMPF& operator*=(const MiniMPF& other) {
        mpz_mul(m_Mantisse.get_mpz(), m_Mantisse.get_mpz(), other.m_Mantisse.get_mpz());
        m_Exponant += other.m_Exponant;
        // If mantissa is zero, clear exponent
        if (MINI_GMP_PLUS_EXPECT(m_Mantisse.sign() == 0, 0)) {
            m_Exponant = 0;
        }
        // Note: normalize not called because odd * odd = odd (no trailing zeros)
        // But this assumes both operands are normalized, which they should be
        return *this;
    }

    // Arithmetic operators - friend functions
    friend MiniMPF operator+(MiniMPF lhs, const MiniMPF& rhs) {
        lhs += rhs;
        return lhs;
    }

    friend MiniMPF operator-(MiniMPF lhs, const MiniMPF& rhs) {
        lhs -= rhs;
        return lhs;
    }

    friend MiniMPF operator*(MiniMPF lhs, const MiniMPF& rhs) {
        lhs *= rhs;
        return lhs;
    }

    friend MiniMPF operator-(const MiniMPF& val) {
        MiniMPF result(val);
        result.FlipSign();
        return result;
    }

    // FMA function (fused multiply-add): a * b + c
    friend MiniMPF fma(const MiniMPF& a, const MiniMPF& b, const MiniMPF& c) {
        MiniMPF result = a * b;
        result += c;
        return result;
    }

    // FUSED DOT PRODUCT: avoids intermediate MiniMPF allocations
    template<size_t N>
    friend MiniMPF dot_product_fused(const std::array<MiniMPF, N>& a, const std::array<MiniMPF, N>& b) {
        MiniMPF result;
        mpz_mul(result.m_Mantisse.get_mpz(), a[0].m_Mantisse.get_mpz(), b[0].m_Mantisse.get_mpz());
        result.m_Exponant = a[0].m_Exponant + b[0].m_Exponant;
        for (size_t i = 1; i < N; ++i) {
            mpz_t prod; mpz_init(prod);
            mpz_mul(prod, a[i].m_Mantisse.get_mpz(), b[i].m_Mantisse.get_mpz());
            int prod_exp = a[i].m_Exponant + b[i].m_Exponant;
            const int exp_diff = result.m_Exponant - prod_exp;
            if (exp_diff > 0) {
                mpz_t scaled; mpz_init(scaled);
                mpz_mul_2exp(scaled, prod, static_cast<mp_bitcnt_t>(exp_diff));
                mpz_add(result.m_Mantisse.get_mpz(), result.m_Mantisse.get_mpz(), scaled);
                mpz_clear(scaled);
            } else if (exp_diff < 0) {
                mpz_t scaled; mpz_init(scaled);
                mpz_mul_2exp(scaled, result.m_Mantisse.get_mpz(), static_cast<mp_bitcnt_t>(-exp_diff));
                result.m_Exponant = prod_exp;
                mpz_set(result.m_Mantisse.get_mpz(), scaled);
                mpz_add(result.m_Mantisse.get_mpz(), result.m_Mantisse.get_mpz(), prod);
                mpz_clear(scaled);
            } else {
                mpz_add(result.m_Mantisse.get_mpz(), result.m_Mantisse.get_mpz(), prod);
            }
            mpz_clear(prod);
        }
        result.normalize();
        return result;
    }

    // SIMD-INSPIRED DOT4 using 128-bit integers
    // For the common case where all values have single-limb mantissas (from doubles)
    // and equal exponents, we can use 128-bit integer arithmetic for speed.
    // Falls back to fused version if conditions are not met or __uint128_t is unavailable.
    friend MiniMPF dot_product_simd4(const std::array<MiniMPF, 4>& a, const std::array<MiniMPF, 4>& b) {
        // Check if all exponents are equal
        int ref_exp = a[0].m_Exponant + b[0].m_Exponant;
        for (size_t i = 1; i < 4; ++i) {
            if (a[i].m_Exponant + b[i].m_Exponant != ref_exp) {
                return dot_product_fused(a, b);
            }
        }
        
        // Check if all mantissas are single-limb
        for (size_t i = 0; i < 4; ++i) {
            const mpz_t& ma = a[i].m_Mantisse.get_mpz();
            const mpz_t& mb = b[i].m_Mantisse.get_mpz();
            if (ma->_mp_size < -1 || ma->_mp_size > 1 || mb->_mp_size < -1 || mb->_mp_size > 1) {
                return dot_product_fused(a, b);
            }
        }

#if MINI_GMP_PLUS_HAS_UINT128
        // Fast path: all single-limb, same exponent
        // Products are 53x53=106 bits, sum is up to 106+2=108 bits
        // Use 128-bit integer type to handle this precisely
        MINI_GMP_PLUS_UINT128_T products[4];
        int signs[4];  // 1 or -1 for each product's sign
        
        for (size_t i = 0; i < 4; ++i) {
            const mpz_t& ma = a[i].m_Mantisse.get_mpz();
            const mpz_t& mb = b[i].m_Mantisse.get_mpz();
            
            uint64_t a_limb = ma->_mp_d[0];
            uint64_t b_limb = mb->_mp_d[0];
            
            // Determine sign of product
            bool a_neg = (ma->_mp_size < 0);
            bool b_neg = (mb->_mp_size < 0);
            signs[i] = (a_neg == b_neg) ? 1 : -1;
            
            // Multiply absolute values
            products[i] = static_cast<MINI_GMP_PLUS_UINT128_T>(a_limb) * static_cast<MINI_GMP_PLUS_UINT128_T>(b_limb);
        }
        
        // Sum absolute values of products
        MINI_GMP_PLUS_UINT128_T abs_sum = products[0] + products[1] + products[2] + products[3];
        
        // Determine overall sign: XOR of all individual signs
        // If odd number of negative products, result is negative
        bool result_negative = false;
        for (int s : signs) {
            if (s < 0) result_negative = !result_negative;
        }
        
        // Convert 128-bit result to MiniMPF
        MiniMPF result;
        result.m_Exponant = ref_exp;
        
        if (abs_sum == 0) {
            result.m_Mantisse = MiniMPZ(0L);
            result.m_Exponant = 0;
        } else {
            // Split 128-bit value into limbs for mpz_t
            uint64_t lo = static_cast<uint64_t>(abs_sum);
            uint64_t hi = static_cast<uint64_t>(abs_sum >> 64);
            
            mpz_t& m = result.m_Mantisse.get_mpz();
            if (hi == 0) {
                // Fits in single limb
                m->_mp_size = result_negative ? -1 : 1;
                m->_mp_d[0] = lo;
            } else {
                // Needs 2 limbs
                // Check if it fits in the stack buffer
                m->_mp_size = result_negative ? -2 : 2;
                m->_mp_d[0] = lo;
                m->_mp_d[1] = hi;
            }
            
            // Normalize to remove trailing zeros
            result.normalize();
        }
        
        return result;
#else
        // __uint128_t not available, fall back to fused implementation
        return dot_product_fused(a, b);
#endif
    }

    // Utility methods
    std::string Visu() const {
        if (IsZero()) return "0";
        std::string mant_str = m_Mantisse.to_string();
        if (mant_str.empty()) mant_str = "0";
        return mant_str + " * 2^" + std::to_string(m_Exponant);
    }

    void ConvertToDouble(double& value, int& exponent) const {
        value = m_Mantisse.to_double();
        exponent = m_Exponant;
    }

    double Estimate() const {
        double result = m_Mantisse.to_double();
        // Scale by power of 2 using ldexp
        return std::ldexp(result, m_Exponant);
    }

    double LowerBound() const {
        const double val = Estimate();
        if (!std::isfinite(val)) {
            return val;
        }
        if (val == 0.0) {
            return -std::numeric_limits<double>::denorm_min();
        }
        const double ulp = std::fabs(std::nextafter(val, std::numeric_limits<double>::infinity()) - val);
        return val - ulp;
    }

    double UpperBound() const {
        const double val = Estimate();
        if (!std::isfinite(val)) {
            return val;
        }
        if (val == 0.0) {
            return std::numeric_limits<double>::denorm_min();
        }
        const double ulp = std::fabs(std::nextafter(val, std::numeric_limits<double>::infinity()) - val);
        return val + ulp;
    }

    size_t size() const {
        // Approximate memory size: base size + mantissa string length as proxy
        return sizeof(*this) + m_Mantisse.to_string().size();
    }

    double GetLog2() const {
        if (IsZero()) return -std::numeric_limits<double>::infinity();
        // log2(|mantisse|) + exponent
        double log_mant = std::log2(std::abs(m_Mantisse.to_double()));
        return log_mant + m_Exponant;
    }

     // Hash support
     size_t hash() const {
         if (IsZero()) {
             std::hash<std::string> str_hash;
             return str_hash("0") ^ (std::hash<int>()(0) << 1);
         }
         // Use canonical (normalized) representation so that mathematically
         // equal values hash the same, even if they have different internal
         // mantissa/exponent pairs (e.g. 1.0 from double ctor vs. 0.5+0.5).
         mp_bitcnt_t trailing_zeros = mpz_scan1(m_Mantisse.get_mpz(), 0);
         std::hash<std::string> str_hash;
         if (trailing_zeros == 0) {
             size_t h1 = str_hash(m_Mantisse.to_string());
             size_t h2 = std::hash<int>()(m_Exponant);
             return h1 ^ (h2 << 1);
         }
         mpz_t normalized;
         mpz_init(normalized);
         mpz_tdiv_q_2exp(normalized, m_Mantisse.get_mpz(), trailing_zeros);
         char* cstr = mpz_get_str(nullptr, 10, normalized);
         std::string normalized_str(cstr);
         free(cstr);
         mpz_clear(normalized);
         size_t h1 = str_hash(normalized_str);
         size_t h2 = std::hash<int>()(m_Exponant + static_cast<int>(trailing_zeros));
         return h1 ^ (h2 << 1);
     }

    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const MiniMPF& num) {
        return os << num.Visu();
    }
};

// std::hash specialization
namespace std {
    template<> struct hash<MiniMPF> {
        size_t operator()(const MiniMPF& val) const {
            return val.hash();
        }
    };
}

#endif // MINIMPF_HPP
