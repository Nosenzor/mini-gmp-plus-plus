#include "geometry_workloads.hpp"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifndef MINI_GMP_PLUS_BENCHMARK_VARIANT
#define MINI_GMP_PLUS_BENCHMARK_VARIANT "unknown"
#endif

namespace {

using mini_gmp_plus_geometry::Matrix2;
using mini_gmp_plus_geometry::Matrix3;
using mini_gmp_plus_geometry::Matrix4;
using mini_gmp_plus_geometry::Vector4;

struct Options {
    std::size_t dataset_size = 256;
    unsigned min_time_ms = 250;
};

struct SplitMix64 {
    explicit SplitMix64(uint64_t seed) : state(seed) {}

    uint64_t next() {
        uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30U)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27U)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31U);
    }

    uint64_t state;
};

struct Dot4Input {
    Vector4 lhs;
    Vector4 rhs;
};

struct Det2Input {
    Matrix2 matrix;
};

struct Det3Input {
    Matrix3 matrix;
};

struct Det4Input {
    Matrix4 matrix;
};

struct SqrtInput {
    MiniMPZ value;
};

struct GcdInput {
    MiniMPZ lhs;
    MiniMPZ rhs;
};

struct BenchmarkResult {
    std::string name;
    std::size_t operations;
    double elapsed_ms;
    double ns_per_op;
    uint64_t checksum;
};

volatile uint64_t benchmark_sink = 0;

bool parse_unsigned_argument(const std::string& text, unsigned long* value) {
    char* end = nullptr;
    *value = std::strtoul(text.c_str(), &end, 10);
    return end != nullptr && *end == '\0';
}

bool parse_size_argument(const std::string& text, std::size_t* value) {
    unsigned long parsed = 0;
    if (!parse_unsigned_argument(text, &parsed)) {
        return false;
    }
    *value = static_cast<std::size_t>(parsed);
    return true;
}

void print_usage(const char* argv0) {
    std::cout << "Usage: " << argv0
              << " [--dataset-size=N] [--min-time-ms=N]\n";
}

Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--help") {
            print_usage(argv[0]);
            std::exit(0);
        }
        if (arg.find("--dataset-size=") == 0) {
            if (!parse_size_argument(arg.substr(15), &options.dataset_size) || options.dataset_size == 0) {
                throw std::runtime_error("Invalid --dataset-size value");
            }
            continue;
        }
        if (arg.find("--min-time-ms=") == 0) {
            unsigned long parsed = 0;
            if (!parse_unsigned_argument(arg.substr(14), &parsed) || parsed == 0) {
                throw std::runtime_error("Invalid --min-time-ms value");
            }
            options.min_time_ms = static_cast<unsigned>(parsed);
            continue;
        }
        throw std::runtime_error("Unknown argument: " + arg);
    }
    return options;
}

MiniMPZ make_random_value(SplitMix64& rng, unsigned bit_count, bool allow_negative) {
    const std::size_t limb_count = (bit_count + 63U) / 64U;
    std::vector<uint64_t> limbs(limb_count);
    for (std::size_t i = 0; i < limb_count; ++i) {
        limbs[i] = rng.next();
    }

    const unsigned top_bits = bit_count - static_cast<unsigned>(64U * (limb_count - 1U));
    if (top_bits < 64U) {
        limbs.back() &= ((uint64_t(1) << top_bits) - 1U);
    }
    limbs.back() |= uint64_t(1) << (top_bits - 1U);

    MiniMPZ value;
    mpz_import(value.get_mpz(), limb_count, -1, sizeof(uint64_t), 0, 0, limbs.data());
    if (allow_negative && (rng.next() & 1U) != 0U) {
        mpz_neg(value.get_mpz(), value.get_mpz());
    }
    return value;
}

MiniMPZ make_positive_odd_value(SplitMix64& rng, unsigned bit_count) {
    MiniMPZ value = make_random_value(rng, bit_count, false);
    mpz_setbit(value.get_mpz(), 0U);
    return value;
}

std::vector<Dot4Input> make_dot4_inputs(std::size_t count, SplitMix64& rng) {
    std::vector<Dot4Input> inputs;
    inputs.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        Dot4Input input;
        input.lhs = Vector4{{
            make_random_value(rng, 80U, true),
            make_random_value(rng, 80U, true),
            make_random_value(rng, 80U, true),
            make_random_value(rng, 80U, true)
        }};
        input.rhs = Vector4{{
            make_random_value(rng, 80U, true),
            make_random_value(rng, 80U, true),
            make_random_value(rng, 80U, true),
            make_random_value(rng, 80U, true)
        }};
        inputs.push_back(std::move(input));
    }
    return inputs;
}

std::vector<Det2Input> make_det2_inputs(std::size_t count, SplitMix64& rng) {
    std::vector<Det2Input> inputs;
    inputs.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        Det2Input input;
        input.matrix = Matrix2{{
            make_random_value(rng, 96U, true),
            make_random_value(rng, 96U, true),
            make_random_value(rng, 96U, true),
            make_random_value(rng, 96U, true)
        }};
        inputs.push_back(std::move(input));
    }
    return inputs;
}

std::vector<Det3Input> make_det3_inputs(std::size_t count, SplitMix64& rng) {
    std::vector<Det3Input> inputs;
    inputs.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        Det3Input input;
        input.matrix = Matrix3{{
            make_random_value(rng, 64U, true),
            make_random_value(rng, 64U, true),
            make_random_value(rng, 64U, true),
            make_random_value(rng, 64U, true),
            make_random_value(rng, 64U, true),
            make_random_value(rng, 64U, true),
            make_random_value(rng, 64U, true),
            make_random_value(rng, 64U, true),
            make_random_value(rng, 64U, true)
        }};
        inputs.push_back(std::move(input));
    }
    return inputs;
}

std::vector<Det4Input> make_det4_inputs(std::size_t count, SplitMix64& rng) {
    std::vector<Det4Input> inputs;
    inputs.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        Det4Input input;
        input.matrix = Matrix4{{
            make_random_value(rng, 48U, true),
            make_random_value(rng, 48U, true),
            make_random_value(rng, 48U, true),
            make_random_value(rng, 48U, true),
            make_random_value(rng, 48U, true),
            make_random_value(rng, 48U, true),
            make_random_value(rng, 48U, true),
            make_random_value(rng, 48U, true),
            make_random_value(rng, 48U, true),
            make_random_value(rng, 48U, true),
            make_random_value(rng, 48U, true),
            make_random_value(rng, 48U, true),
            make_random_value(rng, 48U, true),
            make_random_value(rng, 48U, true),
            make_random_value(rng, 48U, true),
            make_random_value(rng, 48U, true)
        }};
        inputs.push_back(std::move(input));
    }
    return inputs;
}

std::vector<SqrtInput> make_sqrt_inputs(std::size_t count, SplitMix64& rng) {
    std::vector<SqrtInput> inputs;
    inputs.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        MiniMPZ root = make_random_value(rng, 192U, false);
        SqrtInput input;
        input.value = root * root;
        if ((i & 1U) != 0U) {
            input.value += MiniMPZ(static_cast<unsigned long>((rng.next() % 1024U) + 1U));
        }
        inputs.push_back(std::move(input));
    }
    return inputs;
}

std::vector<GcdInput> make_gcd_inputs(std::size_t count, SplitMix64& rng) {
    std::vector<GcdInput> inputs;
    inputs.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const MiniMPZ common = make_positive_odd_value(rng, 64U);
        GcdInput input;
        input.lhs = common * make_positive_odd_value(rng, 160U);
        input.rhs = common * make_positive_odd_value(rng, 160U);
        inputs.push_back(std::move(input));
    }
    return inputs;
}

uint64_t update_checksum(uint64_t checksum, const MiniMPZ& value) {
    checksum ^= static_cast<uint64_t>(mpz_get_ui(value.get_mpz())) + 0x9e3779b97f4a7c15ULL + (checksum << 6U) + (checksum >> 2U);
    return checksum * 1099511628211ULL;
}

template <typename Input, typename Callable>
BenchmarkResult run_benchmark(const std::string& name, const std::vector<Input>& inputs, Callable callable, unsigned min_time_ms) {
    typedef std::chrono::steady_clock clock_type;

    std::size_t rounds = 1;
    double elapsed_ms = 0.0;
    uint64_t runtime_checksum = 1469598103934665603ULL;
    uint64_t reference_checksum = 1469598103934665603ULL;

    for (std::size_t i = 0; i < inputs.size(); ++i) {
        const MiniMPZ result = callable(inputs[i]);
        reference_checksum = update_checksum(reference_checksum, result);
    }

    do {
        runtime_checksum = 1469598103934665603ULL;
        const clock_type::time_point start = clock_type::now();
        for (std::size_t round = 0; round < rounds; ++round) {
            for (std::size_t i = 0; i < inputs.size(); ++i) {
                const MiniMPZ result = callable(inputs[i]);
                runtime_checksum = update_checksum(runtime_checksum, result);
            }
        }
        const std::chrono::duration<double, std::milli> elapsed = clock_type::now() - start;
        elapsed_ms = elapsed.count();
        if (elapsed_ms < static_cast<double>(min_time_ms)) {
            rounds *= 2;
        }
    } while (elapsed_ms < static_cast<double>(min_time_ms));

    benchmark_sink ^= runtime_checksum;

    const std::size_t operations = rounds * inputs.size();
    return BenchmarkResult{
        name,
        operations,
        elapsed_ms,
        (elapsed_ms * 1000000.0) / static_cast<double>(operations),
        reference_checksum
    };
}

void print_result(const BenchmarkResult& result) {
    std::ios_base::fmtflags original_flags = std::cout.flags();
    char original_fill = std::cout.fill();

    std::cout << std::left << std::setw(24) << result.name
              << std::right << std::setw(12) << result.operations
              << std::setw(12) << std::fixed << std::setprecision(1) << result.elapsed_ms
              << std::setw(14) << std::fixed << std::setprecision(1) << result.ns_per_op
              << "  0x" << std::hex << std::setw(16) << std::setfill('0') << result.checksum
              << std::dec << std::setfill(' ') << '\n';

    std::cout.flags(original_flags);
    std::cout.fill(original_fill);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_options(argc, argv);
        SplitMix64 rng(0x6d696e692d676d70ULL);

        const std::vector<Dot4Input> dot4_inputs = make_dot4_inputs(options.dataset_size, rng);
        const std::vector<Det2Input> det2_inputs = make_det2_inputs(options.dataset_size, rng);
        const std::vector<Det3Input> det3_inputs = make_det3_inputs(options.dataset_size, rng);
        const std::vector<Det4Input> det4_inputs = make_det4_inputs(options.dataset_size, rng);
        const std::vector<SqrtInput> sqrt_inputs = make_sqrt_inputs(options.dataset_size, rng);
        const std::vector<GcdInput> gcd_inputs = make_gcd_inputs(options.dataset_size, rng);

        std::cout << "mini-gmp-plus geometry benchmark\n";
        std::cout << "Variant       : " << MINI_GMP_PLUS_BENCHMARK_VARIANT << '\n';
        std::cout << "Dataset size  : " << options.dataset_size << '\n';
        std::cout << "Min time/case : " << options.min_time_ms << " ms\n";
        std::cout << "Workloads     : dot4(80-bit coords), det2(96-bit entries), det3(64-bit entries), det4(48-bit entries), sqrt(~384-bit radicands), gcd(~224-bit inputs)\n\n";

        std::cout << std::left << std::setw(24) << "Benchmark"
                  << std::right << std::setw(12) << "ops"
                  << std::setw(12) << "ms"
                  << std::setw(14) << "ns/op"
                  << "  checksum\n";
        std::cout << std::string(78, '-') << '\n';

        print_result(run_benchmark("dot-product-4d", dot4_inputs,
                                   [](const Dot4Input& input) {
                                       return mini_gmp_plus_geometry::dot_product4(input.lhs, input.rhs);
                                   },
                                   options.min_time_ms));
        print_result(run_benchmark("determinant-2x2", det2_inputs,
                                   [](const Det2Input& input) {
                                       return mini_gmp_plus_geometry::determinant2(input.matrix);
                                   },
                                   options.min_time_ms));
        print_result(run_benchmark("determinant-3x3", det3_inputs,
                                   [](const Det3Input& input) {
                                       return mini_gmp_plus_geometry::determinant3(input.matrix);
                                   },
                                   options.min_time_ms));
        print_result(run_benchmark("determinant-4x4", det4_inputs,
                                   [](const Det4Input& input) {
                                       return mini_gmp_plus_geometry::determinant4(input.matrix);
                                   },
                                   options.min_time_ms));
        print_result(run_benchmark("sqrt", sqrt_inputs,
                                   [](const SqrtInput& input) {
                                       return input.value.sqrt();
                                   },
                                   options.min_time_ms));
        print_result(run_benchmark("gcd", gcd_inputs,
                                   [](const GcdInput& input) {
                                       return mini_gmp_plus_geometry::gcd_value(input.lhs, input.rhs);
                                   },
                                   options.min_time_ms));

        std::cout << "\nSink checksum : 0x" << std::hex << benchmark_sink << std::dec << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Benchmark setup failed: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
