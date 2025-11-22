//
// Created by Romain Nosenzo on 21/11/2025.
//
// test_BigInt.cpp
#include "../BigInt.hpp"
#include <iostream>
#include <cassert>

void test_construction() {
    BigInt a(42ul);
    BigInt b(3.14);
    BigInt c(2.5f);
    BigInt d("12345678901234567890");

    assert(a.to_long() == 42);
    assert(b.to_long() == 3);
    assert(c.to_long() == 2);
    assert(d.to_string() == "12345678901234567890");

    std::cout << "Construction tests passed\n";
}

void test_arithmetic() {
    BigInt a(100ul);
    BigInt b(25ul);

    assert((a + b).to_long() == 125);
    assert((a - b).to_long() == 75);
    assert((a * b).to_long() == 2500);
    assert((a / b).to_long() == 4);
    assert((a % b).to_long() == 0);
    assert((-a).to_long() == -100);

    std::cout << "Arithmetic tests passed\n";
}

void test_compound_assignment() {
    BigInt a(100ul);

    a += BigInt(50ul);
    assert(a.to_long() == 150);

    a -= BigInt(30ul);
    assert(a.to_long() == 120);

    a *= BigInt(2ul);
    assert(a.to_long() == 240);

    a /= BigInt(4ul);
    assert(a.to_long() == 60);

    a %= BigInt(17ul);
    assert(a.to_long() == 9);

    std::cout << "Compound assignment tests passed\n";
}

void test_comparison() {
    BigInt a(100ul);
    BigInt b(50ul);
    BigInt c(100ul);

    assert(a > b);
    assert(b < a);
    assert(a >= c);
    assert(a <= c);
    assert(a == c);
    assert(a != b);

    std::cout << "Comparison tests passed\n";
}

void test_utilities() {
    BigInt a(-42l);
    assert(a.abs().to_long() == 42);
    assert(a.sign() == -1);

    BigInt b(2l);
    assert(b.pow(10).to_long() == 1024);

    BigInt c(16l);
    assert(c.sqrt().to_long() == 4);

    BigInt d(4l);
    assert(d.is_even());
    assert(!d.is_odd());

    std::cout << "Utility tests passed\n";
}

void test_large_numbers() {
    BigInt a("123456789012345678901234567890");
    BigInt b("987654321098765432109876543210");

    BigInt sum = a + b;
    assert(sum.to_string() == "1111111110111111111011111111100");

    std::cout << "Large number tests passed\n";
}

void test_float_construction() {
    BigInt from_double(123.456);
    BigInt from_float(78.9f);

    assert(from_double.to_long() == 123);
    assert(from_float.to_long() == 78);

    std::cout << "Float construction tests passed\n";
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

        std::cout << "\nAll tests passed!\n";
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
