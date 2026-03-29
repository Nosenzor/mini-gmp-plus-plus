// Debug test for SIMD vs non-SIMD
#include "mini-gmp.h"
#include "test_nonsimd_wrappers.h"
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

int main() {
    srand(time(0));
    
    // Simple test case
    const int size = 3;
    mp_limb_t src[size] = {0x123456789ABCDEF0, 0xFEDCBA9876543210, 0x0123456789ABCDEF};
    mp_limb_t dst_simd[size] = {0};
    mp_limb_t dst_nonsimd[size] = {0};
    
    unsigned int shift = 16;
    
    std::cout << "Testing mpn_lshift with shift=" << shift << std::endl;
    std::cout << "Input: ";
    for (int i = 0; i < size; i++) {
        std::cout << std::hex << src[i] << " ";
    }
    std::cout << std::dec << std::endl;
    
    // Test SIMD version
    mp_limb_t carry_simd = mpn_lshift(dst_simd, src, size, shift);
    std::cout << "SIMD result: ";
    for (int i = 0; i < size; i++) {
        std::cout << std::hex << dst_simd[i] << " ";
    }
    std::cout << std::dec << "(carry: " << carry_simd << ")" << std::endl;
    
    // Test non-SIMD version
    mp_limb_t carry_nonsimd = mpn_lshift_nonsimd(dst_nonsimd, src, size, shift);
    std::cout << "Non-SIMD result: ";
    for (int i = 0; i < size; i++) {
        std::cout << std::hex << dst_nonsimd[i] << " ";
    }
    std::cout << std::dec << "(carry: " << carry_nonsimd << ")" << std::endl;
    
    // Compare
    bool match = true;
    if (carry_simd != carry_nonsimd) {
        std::cout << "Carry mismatch: " << carry_simd << " vs " << carry_nonsimd << std::endl;
        match = false;
    }
    for (int i = 0; i < size; i++) {
        if (dst_simd[i] != dst_nonsimd[i]) {
            std::cout << "Mismatch at index " << i << ": " << std::hex << dst_simd[i] << " vs " << dst_nonsimd[i] << std::dec << std::endl;
            match = false;
        }
    }
    
    std::cout << "Test " << (match ? "PASSED" : "FAILED") << std::endl;
    
    return match ? 0 : 1;
}