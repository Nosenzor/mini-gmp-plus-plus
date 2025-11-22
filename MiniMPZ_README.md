# MiniMPZ - Modern C++ Wrapper for mini-gmp

`MiniMPZ` is a modern C++ wrapper around the mini-gmp library that provides an intuitive interface for arbitrary-precision integer arithmetic with support for floating-point conversions.

## Features

- **RAII Memory Management**: Automatic resource management with proper copy/move semantics
- **Operator Overloading**: Natural arithmetic syntax (`+`, `-`, `*`, `/`, `%`, etc.)
- **Multiple Constructors**: Initialize from `long`, `unsigned long`, `double`, `float`, or string
- **Type Conversions**: Convert to native types or strings in any base (2-62)
- **Utility Methods**: `abs()`, `pow()`, `sqrt()`, `sign()`, parity checks
- **Stream Support**: Easy printing with `std::cout << number`

## Basic Usage

```cpp
#include "MiniMPZ.hpp"
#include <iostream>

int main() {
    // Construction from different types
    MiniMPZ a(42L);                    // from long
    MiniMPZ b(3.14);                   // from double (truncates to 3)
    MiniMPZ c(2.5f);                   // from float (truncates to 2)
    MiniMPZ d("123456789012345678");   // from string
    
    // Arithmetic operations
    MiniMPZ sum = a + b;               // 42 + 3 = 45
    MiniMPZ diff = a - b;              // 42 - 3 = 39
    MiniMPZ prod = a * b;              // 42 * 3 = 126
    MiniMPZ quot = a / b;              // 42 / 3 = 14
    MiniMPZ rem = a % b;               // 42 % 3 = 0
    
    // Compound assignments
    a += MiniMPZ(10L);                 // a = 52
    a *= MiniMPZ(2L);                  // a = 104
    
    // Comparisons
    if (a > b) {
        std::cout << "a is greater than b\n";
    }
    
    // Utility methods
    MiniMPZ neg(-100L);
    std::cout << "abs(-100) = " << neg.abs() << "\n";
    std::cout << "2^10 = " << MiniMPZ(2L).pow(10) << "\n";
    std::cout << "sqrt(16) = " << MiniMPZ(16L).sqrt() << "\n";
    
    // String conversion (base 2-62)
    std::cout << "42 in hex: " << a.to_string(16) << "\n";
    
    return 0;
}
```

## API Reference

### Constructors

```cpp
MiniMPZ()                              // Initialize to 0
MiniMPZ(long val)                      // From signed long
MiniMPZ(unsigned long val)             // From unsigned long
MiniMPZ(double val)                    // From double (truncates)
MiniMPZ(float val)                     // From float (truncates)
MiniMPZ(const char* str, int base=10)  // From string
MiniMPZ(const std::string& str, int base=10)
```

### Arithmetic Operators

```cpp
MiniMPZ operator+(const MiniMPZ& other) const
MiniMPZ operator-(const MiniMPZ& other) const
MiniMPZ operator*(const MiniMPZ& other) const
MiniMPZ operator/(const MiniMPZ& other) const
MiniMPZ operator%(const MiniMPZ& other) const
MiniMPZ operator-() const                      // Unary negation

MiniMPZ& operator+=(const MiniMPZ& other)
MiniMPZ& operator-=(const MiniMPZ& other)
MiniMPZ& operator*=(const MiniMPZ& other)
MiniMPZ& operator/=(const MiniMPZ& other)
MiniMPZ& operator%=(const MiniMPZ& other)
```

### Comparison Operators

```cpp
bool operator==(const MiniMPZ& other) const
bool operator!=(const MiniMPZ& other) const
bool operator<(const MiniMPZ& other) const
bool operator<=(const MiniMPZ& other) const
bool operator>(const MiniMPZ& other) const
bool operator>=(const MiniMPZ& other) const
```

### Conversion Methods

```cpp
long to_long() const
unsigned long to_ulong() const
double to_double() const
std::string to_string(int base = 10) const
```

### Utility Methods

```cpp
MiniMPZ abs() const                    // Absolute value
MiniMPZ pow(unsigned long exp) const   // Power
MiniMPZ sqrt() const                   // Integer square root
int sign() const                       // -1, 0, or 1
bool is_even() const                   // Check if even
bool is_odd() const                    // Check if odd
```

### Stream Output

```cpp
std::ostream& operator<<(std::ostream& os, const MiniMPZ& num)
```

### Direct Access to Underlying Type

```cpp
mpz_t& get_mpz()                       // Get mutable reference
const mpz_t& get_mpz() const           // Get const reference
```

## Building

### Direct Compilation

```bash
c++ -std=c++11 -I. your_program.cpp mini-gmp.c -o your_program
```

### Using CMake

The project includes CMake support. Build the test executable:

```bash
cmake -Bcmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug
./cmake-build-debug/test_MiniMPZ
```

## Notes

- All constructors from numeric types are marked `explicit` to prevent implicit conversions
- Integer literals should be suffixed with `L` or `UL` to avoid ambiguity: `MiniMPZ(42L)`
- Division and modulo operations use truncation towards zero (GMP's `tdiv` functions)
- Floating-point conversions truncate towards zero (e.g., `3.9` becomes `3`, `-2.7` becomes `-2`)
- String constructors support bases from 2 to 62

## License

This wrapper follows the same licensing as mini-gmp (LGPL v3+ or GPL v2+).

