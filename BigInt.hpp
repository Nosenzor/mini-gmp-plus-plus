// BigInt.hpp
#ifndef BIGINT_HPP
#define BIGINT_HPP

#include "mini-gmp.h"
#include <string>
#include <stdexcept>
#include <ostream>

class BigInt {
private:
    mpz_t value_;

public:
    // Constructors
    BigInt() { mpz_init(value_); }

    explicit
    BigInt(long val) { mpz_init_set_si(value_, val); }

    explicit
    BigInt(unsigned long val) { mpz_init_set_ui(value_, val); }

    explicit
    BigInt(double val) { mpz_init_set_d(value_, val); }

    explicit
    BigInt(float val) : BigInt(static_cast<double>(val)) {}

    BigInt(const char* str, int base = 10) {
        if (mpz_init_set_str(value_, str, base) != 0) {
            mpz_clear(value_);
            throw std::invalid_argument("Invalid string for BigInt");
        }
    }

    BigInt(const std::string& str, int base = 10) : BigInt(str.c_str(), base) {}

    // Copy constructor
    BigInt(const BigInt& other) { mpz_init_set(value_, other.value_); }

    // Move constructor
    BigInt(BigInt&& other) noexcept {
        *value_ = *other.value_;
        mpz_init(other.value_);
    }

    // Destructor
    ~BigInt() { mpz_clear(value_); }

    // Copy assignment
    BigInt& operator=(const BigInt& other) {
        if (this != &other) {
            mpz_set(value_, other.value_);
        }
        return *this;
    }

    // Move assignment
    BigInt& operator=(BigInt&& other) noexcept {
        if (this != &other) {
            mpz_clear(value_);
            *value_ = *other.value_;
            mpz_init(other.value_);
        }
        return *this;
    }

    // Arithmetic operators
    BigInt operator+(const BigInt& other) const {
        BigInt result;
        mpz_add(result.value_, value_, other.value_);
        return result;
    }

    BigInt operator-(const BigInt& other) const {
        BigInt result;
        mpz_sub(result.value_, value_, other.value_);
        return result;
    }

    BigInt operator*(const BigInt& other) const {
        BigInt result;
        mpz_mul(result.value_, value_, other.value_);
        return result;
    }

    BigInt operator/(const BigInt& other) const {
        BigInt result;
        mpz_tdiv_q(result.value_, value_, other.value_);
        return result;
    }

    BigInt operator%(const BigInt& other) const {
        BigInt result;
        mpz_mod(result.value_, value_, other.value_);
        return result;
    }

    BigInt operator-() const {
        BigInt result;
        mpz_neg(result.value_, value_);
        return result;
    }

    // Compound assignment operators
    BigInt& operator+=(const BigInt& other) {
        mpz_add(value_, value_, other.value_);
        return *this;
    }

    BigInt& operator-=(const BigInt& other) {
        mpz_sub(value_, value_, other.value_);
        return *this;
    }

    BigInt& operator*=(const BigInt& other) {
        mpz_mul(value_, value_, other.value_);
        return *this;
    }

    BigInt& operator/=(const BigInt& other) {
        mpz_tdiv_q(value_, value_, other.value_);
        return *this;
    }

    BigInt& operator%=(const BigInt& other) {
        mpz_mod(value_, value_, other.value_);
        return *this;
    }

    // Comparison operators
    bool operator==(const BigInt& other) const {
        return mpz_cmp(value_, other.value_) == 0;
    }

    bool operator!=(const BigInt& other) const {
        return mpz_cmp(value_, other.value_) != 0;
    }

    bool operator<(const BigInt& other) const {
        return mpz_cmp(value_, other.value_) < 0;
    }

    bool operator<=(const BigInt& other) const {
        return mpz_cmp(value_, other.value_) <= 0;
    }

    bool operator>(const BigInt& other) const {
        return mpz_cmp(value_, other.value_) > 0;
    }

    bool operator>=(const BigInt& other) const {
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
    BigInt abs() const {
        BigInt result;
        mpz_abs(result.value_, value_);
        return result;
    }

    BigInt pow(unsigned long exp) const {
        BigInt result;
        mpz_pow_ui(result.value_, value_, exp);
        return result;
    }

    BigInt sqrt() const {
        BigInt result;
        mpz_sqrt(result.value_, value_);
        return result;
    }

    int sign() const { return mpz_sgn(value_); }

    bool is_even() const { return mpz_even_p(value_); }

    bool is_odd() const { return mpz_odd_p(value_); }

    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const BigInt& num) {
        return os << num.to_string();
    }

    // Access to underlying mpz_t
    mpz_t& get_mpz() { return value_; }
    const mpz_t& get_mpz() const { return value_; }
};

#endif // BIGINT_HPP
