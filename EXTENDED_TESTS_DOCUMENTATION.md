# MiniMPZ Extended Test Suite Documentation

## Overview

The extended test suite for MiniMPZ includes comprehensive tests for extreme values, edge cases, and special numeric scenarios.

## New Test Functions

### 1. `test_extreme_double_values()`

Tests MiniMPZ handling of extreme floating-point values:

#### Denormalized (Subnormal) Numbers
- **Smallest positive denormalized**: `4.9406564584124654e-324`
- **Other denormalized numbers**: `2.225e-320`
- **Negative denormalized**: `-4.9406564584124654e-324`
- All truncate to 0 as expected

#### Near DBL_MIN and DBL_MAX
- **Near DBL_MAX**: `1.7976931348623157e308` (largest representable double)
- **Near DBL_MIN**: `2.2250738585072014e-308` (smallest normalized positive)
- **Large negative**: `-1.7976931348623157e308`

#### Boundary Cases
- Doubles near LONG_MAX: `9.223372036854775e18`
- Doubles near LONG_MIN: `-9.223372036854775e18`
- Tiny fractions: `0.0000000001` → 0
- Large fractions: `999999999.999999` → 999999999
- Negative fractions with truncation towards zero

### 2. `test_extreme_integer_values()`

Tests MiniMPZ handling of extreme integer values:

#### Native Type Boundaries
- **LONG_MAX**: `9223372036854775807L` (2^63 - 1)
- **LONG_MIN**: `-9223372036854775808L` (-2^63)
- **ULONG_MAX**: `18446744073709551615UL` (2^64 - 1)

#### Beyond Native Range
- Values larger than LONG_MAX: `"9223372036854775808"`
- Very large numbers: `"99999999999999999999999999999999"` (32 digits)
- Values smaller than LONG_MIN: `"-9223372036854775809"`

#### Large Powers and Products
- **2^64**: `"18446744073709551616"`
- **2^100**: `"1267650600228229401496703205376"`
- **2^128**: `"340282366920938463463374607431768211456"`
- **20!**: `"2432902008176640000"`

#### Extreme Arithmetic
- Multiplication of 30-digit numbers
- Result: `"121932631137021795226185032733622923332237463801111263526900"` (60 digits)
- Tests proper handling of large integer multiplication

### 3. `test_edge_case_operations()`

Tests edge cases in arithmetic operations:

#### Division and Modulo
- Division of huge numbers by themselves: result = 1
- Division of small by huge: result = 0
- Modulo with very large numbers: `"1000000000000000000007" % 1000000007`

#### Sign Operations
- Negation of large numbers
- Addition of positive and negative large numbers
- Sign checks on extreme values

#### Square Root
- Perfect squares of large numbers
- `√(1000000000000^2) = 1000000000000`

#### Zero Handling
- All operations with zero
- Zero sign, parity checks
- Identity properties: `x + 0 = x`, `x * 0 = 0`

#### Even/Odd with Large Numbers
- Even: `"1000000000000000000000"`
- Odd: `"1000000000000000000001"`

## Test Coverage

### Denormalized Numbers
✅ Smallest positive denormalized (4.94e-324)
✅ Various subnormal values
✅ Negative denormalized numbers
✅ All correctly truncate to 0

### Extreme Doubles
✅ Near DBL_MAX (±1.798e308)
✅ Near DBL_MIN (2.225e-308)
✅ Fractional values near boundaries
✅ Proper truncation towards zero

### Extreme Integers
✅ All native type boundaries (LONG_MAX, LONG_MIN, ULONG_MAX)
✅ Values beyond 64-bit range
✅ Very large numbers (up to 60+ digits)
✅ Large powers of 2 (up to 2^128)
✅ Factorial computations

### Edge Cases
✅ Division and modulo with extreme values
✅ Sign operations on large numbers
✅ Square root of large perfect squares
✅ All operations with zero
✅ Parity checks on huge numbers

## Running the Tests

### Using CMake
```bash
cmake -Bcmake-build-debug
cmake --build cmake-build-debug
./cmake-build-debug/test_MiniMPZ
```

### Direct Compilation
```bash
c++ -std=c++11 -I. tests/test_MiniMPZ.cpp mini-gmp.c -o test_MiniMPZ
./test_MiniMPZ
```

### Using Test Script
```bash
./run_extreme_tests.sh
```

## Expected Output

```
Construction tests passed
Arithmetic tests passed
Compound assignment tests passed
Comparison tests passed
Utility tests passed
Large number tests passed
Float construction tests passed
Extreme double values tests passed
Extreme integer values tests passed
Edge case operations tests passed

All tests passed!
```

## Technical Details

### Denormalized Numbers
Denormalized (subnormal) numbers are IEEE 754 floating-point numbers smaller than the smallest normalized number. They have reduced precision but allow gradual underflow. MiniMPZ correctly handles these by truncating them to 0.

### Truncation Behavior
All float/double to integer conversions in MiniMPZ truncate towards zero:
- `3.9` → `3`
- `-2.7` → `-2`
- `0.999` → `0`

### Large Number Support
MiniMPZ can handle arbitrarily large integers limited only by available memory. The tests verify:
- Numbers beyond 64-bit range
- Multi-digit arithmetic operations
- Correct string conversions for very large numbers

## Notes

1. All tests use explicit type suffixes (`L`, `UL`) to avoid constructor ambiguity
2. Denormalized number handling is platform-dependent but consistently truncates to 0
3. Large number operations may take more time but are guaranteed to be accurate
4. String construction is used for numbers beyond native integer range

## Test Statistics

- **Total test functions**: 10
- **Total assertions**: 100+
- **Coverage areas**: 
  - Construction: 4 types
  - Arithmetic: 5 operators
  - Comparisons: 6 operators
  - Utilities: 6 methods
  - Edge cases: Multiple scenarios
  - Extreme values: Comprehensive coverage

