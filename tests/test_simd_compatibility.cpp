// SIMD/non-SIMD compatibility test for mini-gmp
// This test verifies that SIMD-accelerated functions produce identical results to non-SIMD versions

#include "mini-gmp.h"
#include "test_nonsimd_wrappers.h"
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cassert>

// Test configuration
const int TEST_ITERATIONS = 100;
const int MAX_SIZE = 50;
const mp_limb_t MAX_LIMB = 0xFFFFFFFFFFFFFFFFULL;

// Random number generator
mp_limb_t random_limb() {
    mp_limb_t val = 0;
    for (int i = 0; i < 8; i++) {
        val = (val << 8) | (rand() % 256);
    }
    return val;
}

// Test helper functions
template<typename T>
bool compare_arrays(const T* a, const T* b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            std::cerr << "Array mismatch at index " << i << ": " << a[i] << " vs " << b[i] << std::endl;
            return false;
        }
    }
    return true;
}

bool test_mpn_copyi() {
    std::vector<mp_limb_t> src(MAX_SIZE);
    std::vector<mp_limb_t> dst_simd(MAX_SIZE);
    std::vector<mp_limb_t> dst_nonsimd(MAX_SIZE);
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        int size = 1 + rand() % MAX_SIZE;
        
        // Fill source with random data
        for (int j = 0; j < size; j++) {
            src[j] = random_limb();
        }
        
        // Test SIMD version
        mpn_copyi(dst_simd.data(), src.data(), size);
        
        // Test non-SIMD version
        mpn_copyi_nonsimd(dst_nonsimd.data(), src.data(), size);
        
        if (!compare_arrays(dst_simd.data(), dst_nonsimd.data(), size)) {
            std::cerr << "mpn_copyi test failed at iteration " << i << std::endl;
            return false;
        }
    }
    return true;
}

bool test_mpn_copyd() {
    std::vector<mp_limb_t> src(MAX_SIZE);
    std::vector<mp_limb_t> dst_simd(MAX_SIZE);
    std::vector<mp_limb_t> dst_nonsimd(MAX_SIZE);
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        int size = 1 + rand() % MAX_SIZE;
        
        // Fill source with random data
        for (int j = 0; j < size; j++) {
            src[j] = random_limb();
        }
        
        // Test SIMD version
        mpn_copyd(dst_simd.data(), src.data(), size);
        
        // Test non-SIMD version
        mpn_copyd_nonsimd(dst_nonsimd.data(), src.data(), size);
        
        if (!compare_arrays(dst_simd.data(), dst_nonsimd.data(), size)) {
            std::cerr << "mpn_copyd test failed at iteration " << i << std::endl;
            return false;
        }
    }
    return true;
}

bool test_mpn_zero() {
    std::vector<mp_limb_t> dst_simd(MAX_SIZE);
    std::vector<mp_limb_t> dst_nonsimd(MAX_SIZE);
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        int size = 1 + rand() % MAX_SIZE;
        
        // Test SIMD version
        mpn_zero(dst_simd.data(), size);
        
        // Test non-SIMD version
        mpn_zero_nonsimd(dst_nonsimd.data(), size);
        
        if (!compare_arrays(dst_simd.data(), dst_nonsimd.data(), size)) {
            std::cerr << "mpn_zero test failed at iteration " << i << std::endl;
            return false;
        }
    }
    return true;
}

bool test_mpn_com() {
    std::vector<mp_limb_t> src(MAX_SIZE);
    std::vector<mp_limb_t> dst_simd(MAX_SIZE);
    std::vector<mp_limb_t> dst_nonsimd(MAX_SIZE);
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        int size = 1 + rand() % MAX_SIZE;
        
        // Fill source with random data
        for (int j = 0; j < size; j++) {
            src[j] = random_limb();
        }
        
        // Test SIMD version
        mpn_com(dst_simd.data(), src.data(), size);
        
        // Test non-SIMD version
        mpn_com_nonsimd(dst_nonsimd.data(), src.data(), size);
        
        if (!compare_arrays(dst_simd.data(), dst_nonsimd.data(), size)) {
            std::cerr << "mpn_com test failed at iteration " << i << std::endl;
            return false;
        }
    }
    return true;
}

bool test_mpn_cmp() {
    std::vector<mp_limb_t> a(MAX_SIZE);
    std::vector<mp_limb_t> b(MAX_SIZE);
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        int size = 1 + rand() % MAX_SIZE;
        
        // Fill arrays with random data
        for (int j = 0; j < size; j++) {
            a[j] = random_limb();
            b[j] = random_limb();
        }
        
        // Test SIMD version
        int result_simd = mpn_cmp(a.data(), b.data(), size);
        
        // Test non-SIMD version
        int result_nonsimd = mpn_cmp_nonsimd(a.data(), b.data(), size);
        
        if (result_simd != result_nonsimd) {
            std::cerr << "mpn_cmp test failed at iteration " << i 
                      << ": SIMD=" << result_simd 
                      << " vs non-SIMD=" << result_nonsimd << std::endl;
            return false;
        }
    }
    return true;
}

bool test_mpn_add_n() {
    std::vector<mp_limb_t> a(MAX_SIZE);
    std::vector<mp_limb_t> b(MAX_SIZE);
    std::vector<mp_limb_t> dst_simd(MAX_SIZE);
    std::vector<mp_limb_t> dst_nonsimd(MAX_SIZE);
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        int size = 1 + rand() % MAX_SIZE;
        
        // Fill arrays with random data
        for (int j = 0; j < size; j++) {
            a[j] = random_limb();
            b[j] = random_limb();
        }
        
        // Test SIMD version
        mp_limb_t carry_simd = mpn_add_n(dst_simd.data(), a.data(), b.data(), size);
        
        // Test non-SIMD version
        mp_limb_t carry_nonsimd = mpn_add_n_nonsimd(dst_nonsimd.data(), a.data(), b.data(), size);
        
        if (carry_simd != carry_nonsimd || 
            !compare_arrays(dst_simd.data(), dst_nonsimd.data(), size)) {
            std::cerr << "mpn_add_n test failed at iteration " << i << std::endl;
            return false;
        }
    }
    return true;
}

bool test_mpn_sub_n() {
    std::vector<mp_limb_t> a(MAX_SIZE);
    std::vector<mp_limb_t> b(MAX_SIZE);
    std::vector<mp_limb_t> dst_simd(MAX_SIZE);
    std::vector<mp_limb_t> dst_nonsimd(MAX_SIZE);
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        int size = 1 + rand() % MAX_SIZE;
        
        // Fill arrays with random data
        for (int j = 0; j < size; j++) {
            a[j] = random_limb();
            b[j] = random_limb();
        }
        
        // Test SIMD version
        mp_limb_t borrow_simd = mpn_sub_n(dst_simd.data(), a.data(), b.data(), size);
        
        // Test non-SIMD version
        mp_limb_t borrow_nonsimd = mpn_sub_n_nonsimd(dst_nonsimd.data(), a.data(), b.data(), size);
        
        if (borrow_simd != borrow_nonsimd || 
            !compare_arrays(dst_simd.data(), dst_nonsimd.data(), size)) {
            std::cerr << "mpn_sub_n test failed at iteration " << i << std::endl;
            return false;
        }
    }
    return true;
}

bool test_mpn_lshift() {
    std::vector<mp_limb_t> src(MAX_SIZE);
    std::vector<mp_limb_t> dst_simd(MAX_SIZE);
    std::vector<mp_limb_t> dst_nonsimd(MAX_SIZE);
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        int size = 1 + rand() % MAX_SIZE;
        unsigned int shift = 1 + rand() % (sizeof(mp_limb_t) * 8 - 1);
        
        // Fill source with random data
        for (int j = 0; j < size; j++) {
            src[j] = random_limb();
        }
        
        // Test SIMD version
        mp_limb_t carry_simd = mpn_lshift(dst_simd.data(), src.data(), size, shift);
        
        // Test non-SIMD version
        mp_limb_t carry_nonsimd = mpn_lshift_nonsimd(dst_nonsimd.data(), src.data(), size, shift);
        
        if (carry_simd != carry_nonsimd || 
            !compare_arrays(dst_simd.data(), dst_nonsimd.data(), size)) {
            std::cerr << "mpn_lshift test failed at iteration " << i << std::endl;
            return false;
        }
    }
    return true;
}

bool test_mpn_rshift() {
    std::vector<mp_limb_t> src(MAX_SIZE);
    std::vector<mp_limb_t> dst_simd(MAX_SIZE);
    std::vector<mp_limb_t> dst_nonsimd(MAX_SIZE);
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        int size = 1 + rand() % MAX_SIZE;
        unsigned int shift = 1 + rand() % (sizeof(mp_limb_t) * 8 - 1);
        
        // Fill source with random data
        for (int j = 0; j < size; j++) {
            src[j] = random_limb();
        }
        
        // Test SIMD version
        mp_limb_t carry_simd = mpn_rshift(dst_simd.data(), src.data(), size, shift);
        
        // Test non-SIMD version
        mp_limb_t carry_nonsimd = mpn_rshift_nonsimd(dst_nonsimd.data(), src.data(), size, shift);
        
        if (carry_simd != carry_nonsimd || 
            !compare_arrays(dst_simd.data(), dst_nonsimd.data(), size)) {
            std::cerr << "mpn_rshift test failed at iteration " << i << std::endl;
            return false;
        }
    }
    return true;
}

bool test_mpn_popcount() {
    std::vector<mp_limb_t> src(MAX_SIZE);
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        int size = 1 + rand() % MAX_SIZE;
        
        // Fill source with random data
        for (int j = 0; j < size; j++) {
            src[j] = random_limb();
        }
        
        // Test SIMD version
        mp_bitcnt_t count_simd = mpn_popcount(src.data(), size);
        
        // Test non-SIMD version
        mp_bitcnt_t count_nonsimd = mpn_popcount_nonsimd(src.data(), size);
        
        if (count_simd != count_nonsimd) {
            std::cerr << "mpn_popcount test failed at iteration " << i 
                      << ": SIMD=" << count_simd 
                      << " vs non-SIMD=" << count_nonsimd << std::endl;
            return false;
        }
    }
    return true;
}

bool test_mpn_hamdist() {
    std::vector<mp_limb_t> a(MAX_SIZE);
    std::vector<mp_limb_t> b(MAX_SIZE);
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        int size = 1 + rand() % MAX_SIZE;
        
        // Fill arrays with random data
        for (int j = 0; j < size; j++) {
            a[j] = random_limb();
            b[j] = random_limb();
        }
        
        // Test SIMD version
        mp_bitcnt_t dist_simd = mpn_hamdist(a.data(), b.data(), size);
        
        // Test non-SIMD version
        mp_bitcnt_t dist_nonsimd = mpn_hamdist_nonsimd(a.data(), b.data(), size);
        
        if (dist_simd != dist_nonsimd) {
            std::cerr << "mpn_hamdist test failed at iteration " << i 
                      << ": SIMD=" << dist_simd 
                      << " vs non-SIMD=" << dist_nonsimd << std::endl;
            return false;
        }
    }
    return true;
}

bool test_mpn_and_n() {
    std::vector<mp_limb_t> a(MAX_SIZE);
    std::vector<mp_limb_t> b(MAX_SIZE);
    std::vector<mp_limb_t> dst_simd(MAX_SIZE);
    std::vector<mp_limb_t> dst_nonsimd(MAX_SIZE);
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        int size = 1 + rand() % MAX_SIZE;
        
        // Fill arrays with random data
        for (int j = 0; j < size; j++) {
            a[j] = random_limb();
            b[j] = random_limb();
        }
        
        // Test SIMD version
        mpn_and_n(dst_simd.data(), a.data(), b.data(), size);
        
        // Test non-SIMD version
        mpn_and_n_nonsimd(dst_nonsimd.data(), a.data(), b.data(), size);
        
        if (!compare_arrays(dst_simd.data(), dst_nonsimd.data(), size)) {
            std::cerr << "mpn_and_n test failed at iteration " << i << std::endl;
            return false;
        }
    }
    return true;
}

bool test_mpn_ior_n() {
    std::vector<mp_limb_t> a(MAX_SIZE);
    std::vector<mp_limb_t> b(MAX_SIZE);
    std::vector<mp_limb_t> dst_simd(MAX_SIZE);
    std::vector<mp_limb_t> dst_nonsimd(MAX_SIZE);
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        int size = 1 + rand() % MAX_SIZE;
        
        // Fill arrays with random data
        for (int j = 0; j < size; j++) {
            a[j] = random_limb();
            b[j] = random_limb();
        }
        
        // Test SIMD version
        mpn_ior_n(dst_simd.data(), a.data(), b.data(), size);
        
        // Test non-SIMD version
        mpn_ior_n_nonsimd(dst_nonsimd.data(), a.data(), b.data(), size);
        
        if (!compare_arrays(dst_simd.data(), dst_nonsimd.data(), size)) {
            std::cerr << "mpn_ior_n test failed at iteration " << i << std::endl;
            return false;
        }
    }
    return true;
}

bool test_mpn_xor_n() {
    std::vector<mp_limb_t> a(MAX_SIZE);
    std::vector<mp_limb_t> b(MAX_SIZE);
    std::vector<mp_limb_t> dst_simd(MAX_SIZE);
    std::vector<mp_limb_t> dst_nonsimd(MAX_SIZE);
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        int size = 1 + rand() % MAX_SIZE;
        
        // Fill arrays with random data
        for (int j = 0; j < size; j++) {
            a[j] = random_limb();
            b[j] = random_limb();
        }
        
        // Test SIMD version
        mpn_xor_n(dst_simd.data(), a.data(), b.data(), size);
        
        // Test non-SIMD version
        mpn_xor_n_nonsimd(dst_nonsimd.data(), a.data(), b.data(), size);
        
        if (!compare_arrays(dst_simd.data(), dst_nonsimd.data(), size)) {
            std::cerr << "mpn_xor_n test failed at iteration " << i << std::endl;
            return false;
        }
    }
    return true;
}

bool test_mpn_zero_p() {
    std::vector<mp_limb_t> src(MAX_SIZE);
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        int size = 1 + rand() % MAX_SIZE;
        
        // Fill source with random data
        for (int j = 0; j < size; j++) {
            src[j] = random_limb();
        }
        
        // Test SIMD version
        int result_simd = mpn_zero_p(src.data(), size);
        
        // Test non-SIMD version
        int result_nonsimd = mpn_zero_p_nonsimd(src.data(), size);
        
        if (result_simd != result_nonsimd) {
            std::cerr << "mpn_zero_p test failed at iteration " << i 
                      << ": SIMD=" << result_simd 
                      << " vs non-SIMD=" << result_nonsimd << std::endl;
            return false;
        }
    }
    return true;
}

int main() {
    srand(time(0));
    
    std::cout << "Testing SIMD vs non-SIMD function compatibility..." << std::endl;
    
    bool all_passed = true;
    
    // Run all tests
    all_passed &= test_mpn_copyi();
    std::cout << "mpn_copyi: " << (all_passed ? "PASSED" : "FAILED") << std::endl;
    
    all_passed &= test_mpn_copyd();
    std::cout << "mpn_copyd: " << (all_passed ? "PASSED" : "FAILED") << std::endl;
    
    all_passed &= test_mpn_zero();
    std::cout << "mpn_zero: " << (all_passed ? "PASSED" : "FAILED") << std::endl;
    
    all_passed &= test_mpn_com();
    std::cout << "mpn_com: " << (all_passed ? "PASSED" : "FAILED") << std::endl;
    
    all_passed &= test_mpn_cmp();
    std::cout << "mpn_cmp: " << (all_passed ? "PASSED" : "FAILED") << std::endl;
    
    all_passed &= test_mpn_add_n();
    std::cout << "mpn_add_n: " << (all_passed ? "PASSED" : "FAILED") << std::endl;
    
    all_passed &= test_mpn_sub_n();
    std::cout << "mpn_sub_n: " << (all_passed ? "PASSED" : "FAILED") << std::endl;
    
    all_passed &= test_mpn_lshift();
    std::cout << "mpn_lshift: " << (all_passed ? "PASSED" : "FAILED") << std::endl;
    
    all_passed &= test_mpn_rshift();
    std::cout << "mpn_rshift: " << (all_passed ? "PASSED" : "FAILED") << std::endl;
    
    all_passed &= test_mpn_popcount();
    std::cout << "mpn_popcount: " << (all_passed ? "PASSED" : "FAILED") << std::endl;
    
    all_passed &= test_mpn_hamdist();
    std::cout << "mpn_hamdist: " << (all_passed ? "PASSED" : "FAILED") << std::endl;
    
    all_passed &= test_mpn_and_n();
    std::cout << "mpn_and_n: " << (all_passed ? "PASSED" : "FAILED") << std::endl;
    
    all_passed &= test_mpn_ior_n();
    std::cout << "mpn_ior_n: " << (all_passed ? "PASSED" : "FAILED") << std::endl;
    
    all_passed &= test_mpn_xor_n();
    std::cout << "mpn_xor_n: " << (all_passed ? "PASSED" : "FAILED") << std::endl;
    
    all_passed &= test_mpn_zero_p();
    std::cout << "mpn_zero_p: " << (all_passed ? "PASSED" : "FAILED") << std::endl;
    
    std::cout << "\nOverall result: " << (all_passed ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << std::endl;
    
    return all_passed ? 0 : 1;
}