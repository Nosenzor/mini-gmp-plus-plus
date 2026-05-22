# Plan for Implementing Arbitrary Precision Floating Point Arithmetic

## Goal
Create a floating-point arithmetic class in the style of MiniMPZ, based on FloatRingExtension.h, offering arbitrary precision floating point arithmetic.

## Source Reference
/Users/romainnosenzo/CLionProjects/PhotoTools/Mathematics/SharedInterfaces/FloatRingExtension.h

## Target Location
mini-gmp-plus-plus project, likely as a new header file (e.g., MiniMPF.hpp)

## Implementation Plan

### 1. Class Structure
- Create a new header file `MiniMPF.hpp` (Mini Multi-Precision Floating Point)
- Follow MiniMPZ codestyle: header guards, includes, class definition
- Class name: `MiniMPF` (to match MiniMPZ naming convention)
- No namespace (following MiniMPZ convention)

### 2. Core Components to Implement

#### Private Members
- `MiniMPZ m_Mantisse;` // mantissa (significand)
- `int m_Exponant{};`   // exponent (power of 2)

#### Public Interface
- Constructors:
  - Default constructor
  - Explicit constructor from double
  - Copy constructor
  - Move constructor
  - Constructor from MiniMPZ and exponent
  
- Assignment operators:
  - Copy assignment
  - Move assignment
  
- Swap method
  
- Sign methods:
  - IsPositive(), IsNegative(), IsZero()
  - Sign() returning Sign_t enum
  - FlipSign()
  
- Arithmetic operators:
  - Compound assignment: +=, -=, *=
  - Binary operators: +, -, * (as friends)
  
- FMA function (fused multiply-add)
  
- Comparison:
  - compare() method returning int (-1,0,1)
  - Comparison operators: ==, !=, <, >, <=, >=
  
- Utility methods:
  - Visu() for visualization/debugging
  - ConvertToDouble(double& value, int& exponent)
  - Estimate(), LowerBound(), UpperBound() for double approximations
  - size() for approximate memory size
  - GetLog2() for approximate log base 2
  
- Hash support:
  - hash() method
  - Specialization of std::hash

### 3. Implementation Approach
- Follow MiniMPZ patterns for:
  - Constructor explicitness (explicit for numeric types)
  - Operator implementation (binary ops return by value, compound ops modify in-place)
  - Move semantics (raw-copy then reset source)
  - Inline methods where appropriate
  - Friend declarations for operators
  - Stream output operator (if needed)

### 4. Dependencies
- Include "MiniMPZ.hpp" (since we're using MiniMPZ for mantissa)
- Include <cstdint> for int types
- Include <functional> or <utility> for pair if needed
- Include <string> for to_string methods
- May need to define Sign_t enum if not already available

### 5. Files to Create/Modify
- New file: `MiniMPF.hpp` (main implementation)
- Possibly update CMakeLists.txt if needed for compilation
- Add tests in `tests/test_MiniMPF.cpp` following the pattern of `test_MiniMPZ.cpp`

### 6. Verification Steps
- Ensure code compiles without warnings
- Run existing tests to ensure no regression
- Create and run new tests for MiniMPF functionality
- Verify adherence to MiniMPZ codestyle through inspection

## Subtasks for Vibe
1. Create MiniMPF.hpp header with basic class structure
2. Implement constructors and destructor
3. Implement swap and assignment operators
4. Implement sign methods and FlipSign
5. Implement compound assignment operators
6. Implement binary arithmetic operators (+, -, *)
7. Implement FMA function
8. Implement comparison methods and operators
9. Implement utility methods (Visu, ConvertToDouble, etc.)
10. Implement hash function and std::hash specialization
11. Create test file and implement basic tests
12. Verify compilation and run tests