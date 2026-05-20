// MiniMPF.hpp
#ifndef MINIMPF_HPP
#define MINIMPF_HPP

#include "MiniMPZ.hpp"
#include <cmath>
#include <functional>
#include <limits>
#include <string>
#include <ostream>

enum Sign_t { NEGATIVE = -1, ZERO = 0, POSITIVE = 1 };

class MiniMPF {
private:
    MiniMPZ m_Mantisse;
    int m_Exponant{};

     // Normalize the floating point representation
     void normalize() {
         if (__builtin_expect(m_Mantisse.sign() == 0, 0)) {
             m_Exponant = 0;
             return;
         }
         // Use mpz_scan1 to find trailing zeros and shift them out in one operation
         mp_bitcnt_t trailing_zeros = mpz_scan1(m_Mantisse.get_mpz(), 0);
         if (__builtin_expect(trailing_zeros > 0, 1)) {
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
            // Multiply by 2^52 to get the fractional part, then add the implicit 1
            long mantissa = static_cast<long>(abs_val * (1LL << 52));
            // For values in [1.0, 2.0), the integer part is 1, so we add 2^52
            mantissa += (1L << 52);
            if (negative) {
                mantissa = -mantissa;
            }

            m_Mantisse = MiniMPZ(mantissa);
            m_Exponant = exp - 52;
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
    Sign_t Sign() const { return static_cast<Sign_t>(m_Mantisse.sign()); }
    void FlipSign() { m_Mantisse = -m_Mantisse; }

    // Accessors
    const MiniMPZ& Mantisse() const { return m_Mantisse; }
    int Exponent() const { return m_Exponant; }

    // Comparison methods
    int compare(const MiniMPF& other) const {
        if (IsZero() && other.IsZero()) return 0;
        
        Sign_t s1 = Sign();
        Sign_t s2 = other.Sign();
        
        if (s1 != s2) {
            return (s1 > s2) ? 1 : -1;
        }
        
        if (s1 == ZERO) return 0;

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
        if (__builtin_expect(IsZero(), 0)) {
            *this = other;
            return *this;
        }
        if (__builtin_expect(other.IsZero(), 0)) {
            return *this;
        }

        if (__builtin_expect(m_Exponant == other.m_Exponant, 1)) {
            mpz_add(m_Mantisse.get_mpz(), m_Mantisse.get_mpz(), other.m_Mantisse.get_mpz());
        } else if (__builtin_expect(m_Exponant < other.m_Exponant, 0)) {
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
        if (__builtin_expect(mpz_even_p(m_Mantisse.get_mpz()), 0)) {
            normalize();
        }
        return *this;
    }

    MiniMPF& operator-=(const MiniMPF& other) {
        if (__builtin_expect(other.IsZero(), 0)) {
            return *this;
        }
        if (__builtin_expect(IsZero(), 0)) {
            *this = -other;
            return *this;
        }

        if (__builtin_expect(m_Exponant == other.m_Exponant, 1)) {
            mpz_sub(m_Mantisse.get_mpz(), m_Mantisse.get_mpz(), other.m_Mantisse.get_mpz());
        } else if (__builtin_expect(m_Exponant < other.m_Exponant, 0)) {
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
        if (__builtin_expect(mpz_even_p(m_Mantisse.get_mpz()), 0)) {
            normalize();
        }
        return *this;
    }

    MiniMPF& operator*=(const MiniMPF& other) {
        mpz_mul(m_Mantisse.get_mpz(), m_Mantisse.get_mpz(), other.m_Mantisse.get_mpz());
        m_Exponant += other.m_Exponant;
        // If mantissa is zero, clear exponent
        if (__builtin_expect(m_Mantisse.sign() == 0, 0)) {
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
         // Hash the mantissa by converting to string (simple approach)
         std::hash<std::string> str_hash;
         size_t h1 = str_hash(m_Mantisse.to_string());
         size_t h2 = std::hash<int>()(m_Exponant);
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
