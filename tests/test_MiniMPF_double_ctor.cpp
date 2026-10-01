// Regression test for MiniMPF(double).
//
// Every finite double must round-trip exactly: MiniMPF(d).Estimate() == d.
// The double constructor used to build its 54-bit mantissa through `long`,
// which is 32 bits on LLP64 targets (Windows), so every non-zero input was
// corrupted there while LP64 targets were unaffected.
//
// Checks are explicit and counted rather than assert()-based, so the test
// still fails in Release (NDEBUG) builds.

#include "../MiniMPF.hpp"

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>

namespace {

int g_failures = 0;
int g_checks = 0;

std::uint64_t bits_of(double d) {
    std::uint64_t u = 0;
    std::memcpy(&u, &d, sizeof u);
    return u;
}

double from_bits(std::uint64_t u) {
    double d = 0.0;
    std::memcpy(&d, &u, sizeof d);
    return d;
}

// Returns true on success. Prints a diagnostic line on failure only when
// `verbose` is set, so callers can cap the output of a badly broken build.
bool check_round_trip(double input, const char* label, bool verbose) {
    ++g_checks;
    const MiniMPF f(input);
    const double est = f.Estimate();

    double mant = 0.0;
    int exp = 0;
    f.ConvertToDouble(mant, exp);
    const double rebuilt = std::ldexp(mant, exp);

    // MiniMPF has no signed zero: -0.0 legitimately comes back as +0.0,
    // so compare by value (==), which treats the two zeros as equal.
    const bool sign_ok = (input == 0.0) ? f.IsZero()
                         : (input < 0.0) ? f.IsNegative()
                                         : f.IsPositive();
    const bool ok = (est == input) && (rebuilt == input) && sign_ok;
    if (!ok) {
        ++g_failures;
        if (verbose) {
            std::cout << "FAIL " << label << ": input=" << std::hexfloat << input
                      << " Estimate()=" << est << " ConvertToDouble+ldexp=" << rebuilt
                      << std::defaultfloat << " repr=" << f.Visu() << "\n";
        }
    }
    return ok;
}

void test_representative_values() {
    struct Case {
        double value;
        const char* label;
    };
    const Case cases[] = {
        {0.1, "0.1"},
        {1.0, "1.0"},
        {-3.5, "-3.5"},
        {123456.789, "123456.789"},
        {-1e-300, "-1e-300"},
        {1e300, "1e300"},
        {4503599627370497.0, "2^52+1"},
        {DBL_MIN, "DBL_MIN"},
        {std::numeric_limits<double>::denorm_min(), "denorm_min"},
        {-0.0, "-0.0"},
        {0.0, "0.0"},
        {DBL_MAX, "DBL_MAX"},
        {-DBL_MAX, "-DBL_MAX"},
        {std::nextafter(1.0, 2.0), "nextafter(1,2)"},
        {std::nextafter(DBL_MIN, 0.0), "largest subnormal"},
    };

    int failed = 0;
    for (const Case& c : cases) {
        if (!check_round_trip(c.value, c.label, true)) {
            ++failed;
        }
    }

    // Also pin the exact representation for a couple of values, so a
    // corrupted mantissa is caught even if Estimate() were also broken.
    // 1.0 and -3.5 compared against MiniMPFs built from integer parts.
    ++g_checks;
    if (MiniMPF(1.0).compare(MiniMPF(MiniMPZ(1L), 0)) != 0) {
        ++g_failures;
        ++failed;
        std::cout << "FAIL compare: MiniMPF(1.0) != 1 * 2^0 (repr=" << MiniMPF(1.0).Visu() << ")\n";
    }
    ++g_checks;
    if (MiniMPF(-3.5).compare(MiniMPF(MiniMPZ(-7L), -1)) != 0) {
        ++g_failures;
        ++failed;
        std::cout << "FAIL compare: MiniMPF(-3.5) != -7 * 2^-1 (repr=" << MiniMPF(-3.5).Visu() << ")\n";
    }

    std::cout << "Representative values: " << failed << " failure(s)\n";
}

// splitmix64: fixed algorithm and seed, so the sequence is identical on
// every platform and compiler (unlike std:: distributions).
std::uint64_t splitmix64(std::uint64_t& state) {
    std::uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

void test_random_bit_patterns() {
    const int kSamples = 5000;
    const int kMaxReported = 5;
    std::uint64_t state = 0x4D696E694D5046ULL;  // fixed seed

    int tested = 0;
    int failed = 0;
    while (tested < kSamples) {
        const double d = from_bits(splitmix64(state));
        if (!std::isfinite(d)) {
            continue;
        }
        ++tested;
        if (!check_round_trip(d, "random", failed < kMaxReported)) {
            if (failed < kMaxReported) {
                std::cout << "     bits=0x" << std::hex << bits_of(d) << std::dec << "\n";
            }
            ++failed;
        }
    }
    std::cout << "Random bit patterns: " << failed << "/" << tested << " failure(s)\n";
}

}  // namespace

int main() {
    test_representative_values();
    test_random_bit_patterns();

    if (g_failures != 0) {
        std::cout << "\nMiniMPF(double) round-trip FAILED: " << g_failures << " of " << g_checks
                  << " checks\n";
        return 1;
    }
    std::cout << "\nAll MiniMPF(double) round-trip tests passed (" << g_checks << " checks)\n";
    return 0;
}
