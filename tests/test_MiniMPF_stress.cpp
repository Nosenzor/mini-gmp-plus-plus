#include "../MiniMPF.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>

namespace {

constexpr std::uint32_t kSeed = 0x5A17C3D1u;
constexpr int kIterations = 10000;
constexpr double kEps = 1e-9;

bool approx_equal(double a, double b, double eps = kEps) {
    return std::fabs(a - b) <= eps;
}

MiniMPF make_value(long mantissa, int exponent) {
    return MiniMPF(MiniMPZ(mantissa), exponent);
}

void stress_arithmetic_against_double() {
    std::mt19937 rng(kSeed);
    std::uniform_int_distribution<long> mant_dist(-100000L, 100000L);
    std::uniform_int_distribution<int> exp_dist(-16, 16);

    for (int i = 0; i < kIterations; ++i) {
        const long m1 = mant_dist(rng);
        const long m2 = mant_dist(rng);
        const int e1 = exp_dist(rng);
        const int e2 = exp_dist(rng);

        const MiniMPF a = make_value(m1, e1);
        const MiniMPF b = make_value(m2, e2);

        const double da = std::ldexp(static_cast<double>(m1), e1);
        const double db = std::ldexp(static_cast<double>(m2), e2);

        const MiniMPF sum = a + b;
        const MiniMPF diff = a - b;
        const MiniMPF prod = a * b;

        const double dsum = da + db;
        const double ddiff = da - db;
        const double dprod = da * db;

        assert(approx_equal(sum.Estimate(), dsum));
        assert(approx_equal(diff.Estimate(), ddiff));
        assert(approx_equal(prod.Estimate(), dprod));
    }

    std::cout << "Arithmetic stress tests passed\n";
}

void stress_compound_ops_against_double() {
    std::mt19937 rng(kSeed ^ 0xA5A5A5A5u);
    std::uniform_int_distribution<long> mant_dist(-50000L, 50000L);
    std::uniform_int_distribution<int> exp_dist(-12, 12);

    for (int i = 0; i < kIterations; ++i) {
        const MiniMPF x = make_value(mant_dist(rng), exp_dist(rng));
        const MiniMPF y = make_value(mant_dist(rng), exp_dist(rng));

        MiniMPF t_add = x;
        MiniMPF t_sub = x;
        MiniMPF t_mul = x;

        t_add += y;
        t_sub -= y;
        t_mul *= y;

        const double dx = x.Estimate();
        const double dy = y.Estimate();

        assert(approx_equal(t_add.Estimate(), dx + dy));
        assert(approx_equal(t_sub.Estimate(), dx - dy));
        assert(approx_equal(t_mul.Estimate(), dx * dy));
    }

    std::cout << "Compound-op stress tests passed\n";
}

void stress_ordering_consistency() {
    std::mt19937 rng(kSeed ^ 0x13579BDFu);
    std::uniform_int_distribution<long> mant_dist(-200000L, 200000L);
    std::uniform_int_distribution<int> exp_dist(-200, 200);

    for (int i = 0; i < kIterations; ++i) {
        const MiniMPF a = make_value(mant_dist(rng), exp_dist(rng));
        const MiniMPF b = make_value(mant_dist(rng), exp_dist(rng));

        assert((a == b) == (a.compare(b) == 0));
        assert((a < b) == (a.compare(b) < 0));
        assert((a > b) == (a.compare(b) > 0));
        assert((a <= b) == (a.compare(b) <= 0));
        assert((a >= b) == (a.compare(b) >= 0));

        if (a < b) {
            assert(!(b < a));
            assert(b > a);
        }
        if (a > b) {
            assert(!(b > a));
            assert(b < a);
        }
    }

    std::cout << "Ordering stress tests passed\n";
}

void stress_fma_and_bounds() {
    std::mt19937 rng(kSeed ^ 0x2468ACE0u);
    std::uniform_int_distribution<long> mant_dist(-30000L, 30000L);
    std::uniform_int_distribution<int> exp_dist(-8, 8);

    for (int i = 0; i < kIterations; ++i) {
        const MiniMPF a = make_value(mant_dist(rng), exp_dist(rng));
        const MiniMPF b = make_value(mant_dist(rng), exp_dist(rng));
        const MiniMPF c = make_value(mant_dist(rng), exp_dist(rng));

        const MiniMPF r = fma(a, b, c);
        const double dr = a.Estimate() * b.Estimate() + c.Estimate();
        assert(approx_equal(r.Estimate(), dr));

        const double lb = r.LowerBound();
        const double est = r.Estimate();
        const double ub = r.UpperBound();
        assert(lb <= est);
        assert(est <= ub);
    }

    std::cout << "FMA/bounds stress tests passed\n";
}

void stress_negative_double_constructor() {
    std::mt19937 rng(kSeed ^ 0xC0FFEEu);
    std::uniform_real_distribution<double> mag_dist(1e-12, 1e6);

    for (int i = 0; i < 2000; ++i) {
        const double magnitude = mag_dist(rng);
        const double value = -magnitude;
        MiniMPF x(value);
        assert(x.IsNegative() || x.IsZero());
        assert(approx_equal(x.Estimate(), value, 1e-7));
    }

    std::cout << "Negative-double constructor stress tests passed\n";
}

void stress_large_exponent_gap_addition() {
    std::mt19937 rng(kSeed ^ 0xFACEB00Cu);
    std::uniform_int_distribution<long> mant_dist(-1000L, 1000L);
    std::uniform_int_distribution<int> exp_lo(-50, 50);
    std::uniform_int_distribution<int> gap_dist(150, 300);

    for (int i = 0; i < 3000; ++i) {
        const long m1 = mant_dist(rng);
        const long m2 = mant_dist(rng);
        const int e1 = exp_lo(rng);
        const int e2 = e1 + gap_dist(rng);

        MiniMPF a = make_value(m1, e1);
        MiniMPF b = make_value(m2, e2);

        MiniMPF sum_ab = a + b;
        MiniMPF sum_ba = b + a;
        assert(sum_ab == sum_ba);

        if (!a.IsZero()) {
            MiniMPF back = sum_ab - a;
            assert(back == b);
        }
        if (!b.IsZero()) {
            MiniMPF back = sum_ab - b;
            assert(back == a);
        }
    }

    std::cout << "Large-exponent-gap addition stress tests passed\n";
}

void stress_hash_consistency() {
    std::mt19937 rng(kSeed ^ 0xDEADBEEFu);
    std::uniform_int_distribution<long> mant_dist(-10000L, 10000L);
    std::uniform_int_distribution<int> exp_dist(-20, 20);

    std::hash<MiniMPF> hasher;
    for (int i = 0; i < kIterations; ++i) {
        const long m = mant_dist(rng);
        const int e = exp_dist(rng);
        const MiniMPF a = make_value(m, e);
        const MiniMPF b = make_value(m, e);
        assert(a == b);
        assert(a.hash() == b.hash());
        assert(hasher(a) == hasher(b));
    }

    std::cout << "Hash consistency stress tests passed\n";
}

} // namespace

int main() {
    stress_arithmetic_against_double();
    stress_compound_ops_against_double();
    stress_ordering_consistency();
    stress_fma_and_bounds();
    stress_negative_double_constructor();
    stress_large_exponent_gap_addition();
    stress_hash_consistency();

    std::cout << "\nAll MiniMPF stress tests passed!\n";
    return 0;
}
