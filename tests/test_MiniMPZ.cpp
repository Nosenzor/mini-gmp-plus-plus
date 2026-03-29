//
// Created by Romain Nosenzo on 21/11/2025.
//
// test_MiniMPZ.cpp
#include "../MiniMPZ.hpp"
#include "geometry_workloads.hpp"
#include <iostream>
#include <cassert>
#include <limits>

void test_construction() {
    MiniMPZ a(42ul);
    MiniMPZ b(3.14);
    MiniMPZ c(2.5f);
    MiniMPZ d("12345678901234567890");

    assert(a.to_long() == 42);
    assert(b.to_long() == 3);
    assert(c.to_long() == 2);
    assert(d.to_string() == "12345678901234567890");

    std::cout << "Construction tests passed\n";
}

void test_arithmetic() {
    MiniMPZ a(100ul);
    MiniMPZ b(25ul);

    assert((a + b).to_long() == 125);
    assert((a - b).to_long() == 75);
    assert((a * b).to_long() == 2500);
    assert((a / b).to_long() == 4);
    assert((a % b).to_long() == 0);
    assert((-a).to_long() == -100);

    std::cout << "Arithmetic tests passed\n";
}

void test_compound_assignment() {
    MiniMPZ a(100ul);

    a += MiniMPZ(50ul);
    assert(a.to_long() == 150);

    a -= MiniMPZ(30ul);
    assert(a.to_long() == 120);

    a *= MiniMPZ(2ul);
    assert(a.to_long() == 240);

    a /= MiniMPZ(4ul);
    assert(a.to_long() == 60);

    a %= MiniMPZ(17ul);
    assert(a.to_long() == 9);

    std::cout << "Compound assignment tests passed\n";
}

void test_comparison() {
    MiniMPZ a(100ul);
    MiniMPZ b(50ul);
    MiniMPZ c(100ul);

    assert(a > b);
    assert(b < a);
    assert(a >= c);
    assert(a <= c);
    assert(a == c);
    assert(a != b);

    std::cout << "Comparison tests passed\n";
}

void test_utilities() {
    MiniMPZ a(-42l);
    assert(a.abs().to_long() == 42);
    assert(a.sign() == -1);

    MiniMPZ b(2l);
    assert(b.pow(10).to_long() == 1024);

    MiniMPZ c(16l);
    assert(c.sqrt().to_long() == 4);

    MiniMPZ d(4l);
    assert(d.is_even());
    assert(!d.is_odd());

    std::cout << "Utility tests passed\n";
}

void test_large_numbers() {
    MiniMPZ a("123456789012345678901234567890");
    MiniMPZ b("987654321098765432109876543210");

    MiniMPZ sum = a + b;
    assert(sum.to_string() == "1111111110111111111011111111100");

    std::cout << "Large number tests passed\n";
}

void test_float_construction() {
    MiniMPZ from_double(123.456);
    MiniMPZ from_float(78.9f);

    assert(from_double.to_long() == 123);
    assert(from_float.to_long() == 78);

    std::cout << "Float construction tests passed\n";
}

void test_extreme_double_values() {
    // Test very large doubles
    MiniMPZ large_double(1.7976931348623157e308);  // Near DBL_MAX
    assert(large_double.sign() == 1);

    // Test very small positive double (near zero)
    MiniMPZ small_double(2.2250738585072014e-308);  // Near DBL_MIN (normalized minimum)
    assert(small_double.to_long() == 0);  // Truncates to 0

    // Test denormalized (subnormal) numbers
    MiniMPZ denorm1(4.9406564584124654e-324);  // Smallest positive denormalized
    assert(denorm1.to_long() == 0);  // Truncates to 0

    MiniMPZ denorm2(2.225e-320);  // Another denormalized number
    assert(denorm2.to_long() == 0);  // Truncates to 0

    // Test negative denormalized
    MiniMPZ neg_denorm(-4.9406564584124654e-324);
    assert(neg_denorm.to_long() == 0);  // Truncates to 0
    assert(neg_denorm.sign() == -1);

    // Test large negative double
    MiniMPZ large_neg(-1.7976931348623157e308);
    assert(large_neg.sign() == -1);

    // Test doubles near integer boundaries
    MiniMPZ near_max_long(9.223372036854775e18);  // Near LONG_MAX
    MiniMPZ near_min_long(-9.223372036854775e18); // Near LONG_MIN

    // Test special fractional cases
    MiniMPZ tiny_frac(0.0000000001);
    assert(tiny_frac.to_long() == 0);

    MiniMPZ large_frac(999999999.999999);
    assert(large_frac.to_long() == 999999999);

    // Test negative fractions
    MiniMPZ neg_frac(-0.99999);
    assert(neg_frac.to_long() == 0);  // Truncates towards zero

    MiniMPZ neg_large_frac(-999.99);
    assert(neg_large_frac.to_long() == -999);

    std::cout << "Extreme double values tests passed\n";
}

void test_extreme_integer_values() {
    // Test maximum values representable by different types
    const long long_max = (std::numeric_limits<long>::max)();
    const long long_min = (std::numeric_limits<long>::min)();
    const unsigned long ulong_max = (std::numeric_limits<unsigned long>::max)();

    MiniMPZ max_long(long_max);
    assert(max_long.to_long() == long_max);

    MiniMPZ min_long(long_min);
    assert(min_long.to_long() == long_min);

    MiniMPZ max_ulong(ulong_max);
    assert(max_ulong.to_ulong() == ulong_max);

    // Test values beyond native integer range (from string)
    MiniMPZ beyond_long("9223372036854775808");
    assert(beyond_long > max_long);

    MiniMPZ way_beyond("99999999999999999999999999999999");  // 32 nines
    assert(way_beyond.to_string() == "99999999999999999999999999999999");

    // Test negative values beyond native range
    MiniMPZ neg_beyond("-9223372036854775809");
    assert(neg_beyond < min_long);

    // Test arithmetic with extreme values
    MiniMPZ extreme1("340282366920938463463374607431768211455");  // 2^128 - 1
    MiniMPZ extreme2("340282366920938463463374607431768211456");  // 2^128
    assert(extreme2 > extreme1);
    assert((extreme2 - extreme1).to_long() == 1);

    // Test powers of 2
    MiniMPZ power2_64 = MiniMPZ(2L).pow(64);
    assert(power2_64.to_string() == "18446744073709551616");

    MiniMPZ power2_100 = MiniMPZ(2L).pow(100);
    assert(power2_100.to_string() == "1267650600228229401496703205376");

    MiniMPZ power2_128 = MiniMPZ(2L).pow(128);
    assert(power2_128.to_string() == "340282366920938463463374607431768211456");

    // Test factorials (large products)
    MiniMPZ factorial = MiniMPZ(1L);
    for (int i = 2; i <= 20; ++i) {
        factorial *= MiniMPZ((long)i);
    }
    assert(factorial.to_string() == "2432902008176640000");  // 20!

    // Test very large multiplication
    MiniMPZ large_a("123456789012345678901234567890");
    MiniMPZ large_b("987654321098765432109876543210");
    MiniMPZ large_product = large_a * large_b;
    assert(large_product.to_string() == "121932631137021795226185032733622923332237463801111263526900");

    std::cout << "Extreme integer values tests passed\n";
}

void test_edge_case_operations() {
    // Test division by very large numbers
    MiniMPZ huge("999999999999999999999999999999");
    MiniMPZ small(1L);
    assert((huge / huge).to_long() == 1);
    assert((small / huge).to_long() == 0);

    // Test modulo with extreme values
    MiniMPZ mod_test = MiniMPZ("1000000000000000000007") % MiniMPZ(1000000007L);
    assert(mod_test.to_long() == 1000000000);

    // Test alternating signs with large numbers
    MiniMPZ pos_huge("999999999999999999");
    MiniMPZ neg_huge = -pos_huge;
    assert(neg_huge.sign() == -1);
    assert((pos_huge + neg_huge).to_long() == 0);

    // Test sqrt of large perfect squares
    MiniMPZ large_square = MiniMPZ("1000000000000") * MiniMPZ("1000000000000");
    assert(large_square.sqrt().to_string() == "1000000000000");

    // Test with zero
    MiniMPZ zero(0L);
    assert(zero.sign() == 0);
    assert(zero.is_even());
    assert(!zero.is_odd());
    assert((zero + MiniMPZ(42L)).to_long() == 42);
    assert((MiniMPZ(42L) * zero).to_long() == 0);

    // Test boundary between even/odd with large numbers
    MiniMPZ even_huge("1000000000000000000000");
    MiniMPZ odd_huge("1000000000000000000001");
    assert(even_huge.is_even());
    assert(!even_huge.is_odd());
    assert(odd_huge.is_odd());
    assert(!odd_huge.is_even());

    std::cout << "Edge case operations tests passed\n";
}

void test_geometry_workloads() {
    using namespace mini_gmp_plus_geometry;

    const Vector4 lhs = {{
        MiniMPZ(3L), MiniMPZ(-2L), MiniMPZ(5L), MiniMPZ(7L)
    }};
    const Vector4 rhs = {{
        MiniMPZ(-11L), MiniMPZ(13L), MiniMPZ(17L), MiniMPZ(-19L)
    }};
    assert(dot_product4(lhs, rhs).to_long() == -107);

    const Matrix2 matrix2 = {{
        MiniMPZ(3L), MiniMPZ(8L),
        MiniMPZ(4L), MiniMPZ(6L)
    }};
    assert(determinant2(matrix2).to_long() == -14);

    const Matrix3 matrix3 = {{
        MiniMPZ(6L), MiniMPZ(1L), MiniMPZ(1L),
        MiniMPZ(4L), MiniMPZ(-2L), MiniMPZ(5L),
        MiniMPZ(2L), MiniMPZ(8L), MiniMPZ(7L)
    }};
    assert(determinant3(matrix3).to_long() == -306);

    const Matrix4 matrix4 = {{
        MiniMPZ(3L), MiniMPZ(2L), MiniMPZ(0L), MiniMPZ(1L),
        MiniMPZ(4L), MiniMPZ(0L), MiniMPZ(1L), MiniMPZ(2L),
        MiniMPZ(3L), MiniMPZ(0L), MiniMPZ(2L), MiniMPZ(1L),
        MiniMPZ(9L), MiniMPZ(2L), MiniMPZ(3L), MiniMPZ(1L)
    }};
    assert(determinant4(matrix4).to_long() == 24);

    std::cout << "Geometry workload tests passed\n";
}

void test_sqrt_and_gcd_workloads() {
    using namespace mini_gmp_plus_geometry;

    const MiniMPZ root("12345678901234567890");
    const MiniMPZ square = root * root;
    assert(square.sqrt() == root);

    const MiniMPZ near_square = square + MiniMPZ(123456789UL);
    assert(near_square.sqrt() == root);

    const MiniMPZ lhs("12345678901234567890");
    const MiniMPZ rhs("98765432109876543210");
    const MiniMPZ expected_gcd("900000000090");
    assert(gcd_value(lhs, rhs) == expected_gcd);
    assert(gcd_value(-lhs, rhs) == expected_gcd);

    std::cout << "Sqrt and gcd workload tests passed\n";
}

int main() {
    try {
        test_construction();
        test_arithmetic();
        test_compound_assignment();
        test_comparison();
        test_utilities();
        test_large_numbers();
        test_float_construction();
        test_extreme_double_values();
        test_extreme_integer_values();
        test_edge_case_operations();
        test_geometry_workloads();
        test_sqrt_and_gcd_workloads();

        std::cout << "\nAll tests passed!\n";
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
