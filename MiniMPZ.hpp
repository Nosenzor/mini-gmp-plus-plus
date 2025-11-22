// MiniMPZ.hpp
#ifndef MINIMPZ_HPP
#define MINIMPZ_HPP

#include "mini-gmp.h"
#include <string>
#include <stdexcept>
#include <ostream>

class MiniMPZ {
private:
    mpz_t value_;

public:
    // Constructors
    MiniMPZ() { mpz_init(value_); }

    explicit
    MiniMPZ(long val) { mpz_init_set_si(value_, val); }

    explicit
    MiniMPZ(unsigned long val) { mpz_init_set_ui(value_, val); }

    explicit
    MiniMPZ(double val) { mpz_init_set_d(value_, val); }

    explicit
    MiniMPZ(float val) : MiniMPZ(static_cast<double>(val)) {}

    MiniMPZ(const char* str, int base = 10) {
        if (mpz_init_set_str(value_, str, base) != 0) {
            mpz_clear(value_);
            throw std::invalid_argument("Invalid string for MiniMPZ");
        }
    }

    MiniMPZ(const std::string& str, int base = 10) : MiniMPZ(str.c_str(), base) {}

    // Copy constructor
    MiniMPZ(const MiniMPZ& other) { mpz_init_set(value_, other.value_); }

    // Move constructor
    MiniMPZ(MiniMPZ&& other) noexcept {
        *value_ = *other.value_;
        mpz_init(other.value_);
    }

    // Destructor
    ~MiniMPZ() { mpz_clear(value_); }

    // Copy assignment
    MiniMPZ& operator=(const MiniMPZ& other) {
        if (this != &other) {
            mpz_set(value_, other.value_);
        }
        return *this;
    }

    // Move assignment
    MiniMPZ& operator=(MiniMPZ&& other) noexcept {
        if (this != &other) {
            mpz_clear(value_);
            *value_ = *other.value_;
            mpz_init(other.value_);
        }
        return *this;
    }

    // Arithmetic operators
    MiniMPZ operator+(const MiniMPZ& other) const {
        MiniMPZ result;
        mpz_add(result.value_, value_, other.value_);
        return result;
    }

    MiniMPZ operator-(const MiniMPZ& other) const {
        MiniMPZ result;
        mpz_sub(result.value_, value_, other.value_);
        return result;
    }

    MiniMPZ operator*(const MiniMPZ& other) const {
        MiniMPZ result;
        mpz_mul(result.value_, value_, other.value_);
        return result;
    }

    MiniMPZ operator/(const MiniMPZ& other) const {
        MiniMPZ result;
        mpz_tdiv_q(result.value_, value_, other.value_);
        return result;
    }

    MiniMPZ operator%(const MiniMPZ& other) const {
        MiniMPZ result;
        mpz_mod(result.value_, value_, other.value_);
        return result;
    }

    MiniMPZ operator-() const {
        MiniMPZ result;
        mpz_neg(result.value_, value_);
        return result;
    }

    // Compound assignment operators
    MiniMPZ& operator+=(const MiniMPZ& other) {
        mpz_add(value_, value_, other.value_);
        return *this;
    }

    MiniMPZ& operator-=(const MiniMPZ& other) {
        mpz_sub(value_, value_, other.value_);
        return *this;
    }

    MiniMPZ& operator*=(const MiniMPZ& other) {
        mpz_mul(value_, value_, other.value_);
        return *this;
    }

    MiniMPZ& operator/=(const MiniMPZ& other) {
        mpz_tdiv_q(value_, value_, other.value_);
        return *this;
    }

    MiniMPZ& operator%=(const MiniMPZ& other) {
        mpz_mod(value_, value_, other.value_);
        return *this;
    }

    // Comparison operators
    bool operator==(const MiniMPZ& other) const {
        return mpz_cmp(value_, other.value_) == 0;
    }

    bool operator!=(const MiniMPZ& other) const {
        return mpz_cmp(value_, other.value_) != 0;
    }

    bool operator<(const MiniMPZ& other) const {
        return mpz_cmp(value_, other.value_) < 0;
    }

    bool operator<=(const MiniMPZ& other) const {
        return mpz_cmp(value_, other.value_) <= 0;
    }

    bool operator>(const MiniMPZ& other) const {
        return mpz_cmp(value_, other.value_) > 0;
    }

    bool operator>=(const MiniMPZ& other) const {
        return mpz_cmp(value_, other.value_) >= 0;
    }

    // Conversion methods
    long to_long() const { return mpz_get_si(value_); }

    unsigned long to_ulong() const { return mpz_get_ui(value_); }

    double to_double() const { return mpz_get_d(value_); }

    std::string to_string(int base = 10) const {
        char* str = mpz_get_str(nullptr, base, value_);
        std::string result(str);
        free(str);
        return result;
    }

    // Utility methods
    MiniMPZ abs() const {
        MiniMPZ result;
        mpz_abs(result.value_, value_);
        return result;
    }

    MiniMPZ pow(unsigned long exp) const {
        MiniMPZ result;
        mpz_pow_ui(result.value_, value_, exp);
        return result;
    }

    MiniMPZ sqrt() const {
        MiniMPZ result;
        mpz_sqrt(result.value_, value_);
        return result;
    }

    int sign() const { return mpz_sgn(value_); }

    bool is_even() const { return mpz_even_p(value_); }

    bool is_odd() const { return mpz_odd_p(value_); }

    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const MiniMPZ& num) {
        return os << num.to_string();
    }

    // Access to underlying mpz_t
    mpz_t& get_mpz() { return value_; }
    const mpz_t& get_mpz() const { return value_; }
};

#endif // MINIMPZ_HPP

