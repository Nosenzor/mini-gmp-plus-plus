#include "../MiniMPF.hpp"

#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>

namespace {

bool almost_equal(double a, double b, double eps = 1e-12) {
    return std::fabs(a - b) <= eps;
}

void test_default_and_sign() {
    MiniMPF z;
    assert(z.IsZero());
    assert(!z.IsPositive());
    assert(!z.IsNegative());
    assert(z.Sign() == ZERO);

    MiniMPF p(MiniMPZ(7L), 0);
    assert(p.IsPositive());
    assert(!p.IsNegative());
    assert(p.Sign() == POSITIVE);

    MiniMPF n(MiniMPZ(-11L), 0);
    assert(!n.IsPositive());
    assert(n.IsNegative());
    assert(n.Sign() == NEGATIVE);

    n.FlipSign();
    assert(n.IsPositive());

    std::cout << "Default/sign tests passed\n";
}

void test_construction_and_normalization() {
    MiniMPF x(MiniMPZ(8L), -1);
    assert(x.Mantisse() == MiniMPZ(1L));
    assert(x.Exponent() == 2);
    assert(almost_equal(x.Estimate(), 4.0));

    MiniMPF y(1.5);
    assert(!y.IsZero());
    assert(almost_equal(y.Estimate(), 1.5));

    MiniMPF n(-1.5);
    assert(n.IsNegative());
    assert(almost_equal(n.Estimate(), -1.5));

    std::cout << "Construction/normalization tests passed\n";
}

void test_copy_move_and_swap() {
    MiniMPF a(MiniMPZ(5L), 3);
    MiniMPF b(a);
    assert(a == b);

    MiniMPF c(std::move(b));
    assert(c == a);

    MiniMPF d(MiniMPZ(3L), 1);
    MiniMPF e(MiniMPZ(9L), -2);
    const double d_before = d.Estimate();
    const double e_before = e.Estimate();
    d.swap(e);
    assert(almost_equal(d.Estimate(), e_before));
    assert(almost_equal(e.Estimate(), d_before));

    std::cout << "Copy/move/swap tests passed\n";
}

void test_comparison() {
    MiniMPF a(MiniMPZ(10L), 0);
    MiniMPF b(MiniMPZ(3L), 0);
    MiniMPF c(MiniMPZ(10L), 0);

    assert(a.compare(b) > 0);
    assert(b.compare(a) < 0);
    assert(a.compare(c) == 0);

    assert(a > b);
    assert(b < a);
    assert(a >= c);
    assert(a <= c);
    assert(a == c);
    assert(a != b);

    std::cout << "Comparison tests passed\n";
}

void test_arithmetic_same_exponent() {
    MiniMPF a(MiniMPZ(7L), 0);
    MiniMPF b(MiniMPZ(5L), 0);

    MiniMPF s = a + b;
    MiniMPF d = a - b;
    MiniMPF m = a * b;

    assert(almost_equal(s.Estimate(), 12.0));
    assert(almost_equal(d.Estimate(), 2.0));
    assert(almost_equal(m.Estimate(), 35.0));

    MiniMPF c(MiniMPZ(9L), 0);
    c += MiniMPF(MiniMPZ(1L), 0);
    assert(almost_equal(c.Estimate(), 10.0));
    c -= MiniMPF(MiniMPZ(4L), 0);
    assert(almost_equal(c.Estimate(), 6.0));
    c *= MiniMPF(MiniMPZ(3L), 0);
    assert(almost_equal(c.Estimate(), 18.0));

    std::cout << "Arithmetic (same exponent) tests passed\n";
}

void test_unary_minus_and_fma() {
    MiniMPF a(MiniMPZ(4L), 0);
    MiniMPF b = -a;
    assert(b == MiniMPF(MiniMPZ(-4L), 0));

    MiniMPF x(MiniMPZ(3L), 0);
    MiniMPF y(MiniMPZ(6L), 0);
    MiniMPF z(MiniMPZ(-5L), 0);
    MiniMPF r = fma(x, y, z);
    assert(almost_equal(r.Estimate(), 13.0));

    std::cout << "Unary minus/FMA tests passed\n";
}

void test_conversion_bounds_size_log() {
    MiniMPF a(MiniMPZ(3L), 4);
    double v = 0.0;
    int e = 0;
    a.ConvertToDouble(v, e);
    assert(almost_equal(v, 3.0));
    assert(e == 4);
    assert(almost_equal(a.Estimate(), 48.0));

    const double lb = a.LowerBound();
    const double ub = a.UpperBound();
    const double est = a.Estimate();
    assert(lb <= est);
    assert(est <= ub);
    assert(a.size() > 0U);
    assert(almost_equal(a.GetLog2(), std::log2(48.0)));

    std::cout << "Conversion/bounds/size/log tests passed\n";
}

void test_exactness_beyond_double_precision() {
    const MiniMPZ two_pow_53 = MiniMPZ(2L).pow(53);
    const MiniMPZ two_pow_53_plus_one = two_pow_53 + MiniMPZ(1L);
    MiniMPF a(two_pow_53_plus_one, 0);
    MiniMPF b(two_pow_53, 0);

    assert(a > b);
    assert(a.compare(b) > 0);

    MiniMPF c(MiniMPZ(1L), 200);
    MiniMPF d(MiniMPZ(1L), 0);
    MiniMPF e = c + d;
    assert(e > c);
    assert(e.compare(c) > 0);

    MiniMPF x(MiniMPZ(1L), 100);
    MiniMPF y(MiniMPZ(1L), 100);
    MiniMPF z = x - y;
    assert(z.IsZero());

    std::cout << "Exactness-beyond-double tests passed\n";
}

void test_bounds_non_degenerate() {
    const MiniMPZ base = MiniMPZ(2L).pow(100);
    const MiniMPZ base_plus_one = base + MiniMPZ(1L);
    MiniMPF v(base_plus_one, 0);

    const double est = v.Estimate();
    const double lb = v.LowerBound();
    const double ub = v.UpperBound();

    assert(lb <= est);
    assert(est <= ub);
    assert(lb < ub);

    MiniMPF z;
    const double zlb = z.LowerBound();
    const double zub = z.UpperBound();
    assert(zlb < 0.0);
    assert(zub > 0.0);
    assert(zlb < zub);

    std::cout << "Bounds non-degenerate tests passed\n";
}

void test_hash_and_visu() {
    MiniMPF a(MiniMPZ(9L), -1);
    MiniMPF b(MiniMPZ(9L), -1);
    MiniMPF c(MiniMPZ(9L), 0);

    const std::size_t ha = a.hash();
    const std::size_t hb = b.hash();
    const std::size_t hc = c.hash();

    assert(ha == hb);
    assert(ha != hc);

    const std::string s = a.Visu();
    assert(!s.empty());
    assert(s.find("2^") != std::string::npos);

    std::hash<MiniMPF> hasher;
    assert(hasher(a) == ha);

    std::cout << "Hash/Visu tests passed\n";
}

void test_zero_stability() {
    MiniMPF z;
    MiniMPF a(MiniMPZ(17L), 0);

    assert(almost_equal((a + z).Estimate(), a.Estimate()));
    assert(almost_equal((a - z).Estimate(), a.Estimate()));
    assert((a * z).IsZero());

    MiniMPF t(MiniMPZ(5L), 0);
    t -= MiniMPF(MiniMPZ(5L), 0);
    assert(t.IsZero());
    assert(t.Exponent() == 0);

    std::cout << "Zero stability tests passed\n";
}

} // namespace

int main() {
    test_default_and_sign();
    test_construction_and_normalization();
    test_copy_move_and_swap();
    test_comparison();
    test_arithmetic_same_exponent();
    test_unary_minus_and_fma();
    test_conversion_bounds_size_log();
    test_exactness_beyond_double_precision();
    test_bounds_non_degenerate();
    test_hash_and_visu();
    test_zero_stability();

    std::cout << "\nAll MiniMPF tests passed!\n";
    return 0;
}
