# vcpkg Integration

This directory contains files for integrating mini-gmp-plus with vcpkg.

## Files Overview

- **vcpkg.json**: Manifest file describing the package and its dependencies
- **portfile.cmake**: Build instructions for vcpkg
- **usage**: Instructions for users on how to consume the package
- **mini-gmp-plus-config.cmake.in**: CMake package configuration template

## Using mini-gmp-plus with vcpkg

### Option 1: Manifest Mode (Recommended)

Add to your project's `vcpkg.json`:

```json
{
  "dependencies": [
    "mini-gmp-plus"
  ]
}
```

### Option 2: Classic Mode

```bash
vcpkg install mini-gmp-plus
```

### In Your CMakeLists.txt

```cmake
find_package(mini-gmp-plus CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE mini-gmp-plus::mini-gmp-plus)
```

## Contributing to vcpkg Registry

To add this package to the official vcpkg registry:

1. Fork the [vcpkg repository](https://github.com/microsoft/vcpkg)
2. Create a new port directory: `ports/mini-gmp-plus/`
3. Copy these files to the port directory:
   - `portfile.cmake`
   - `vcpkg.json`
   - `usage`
4. Update the SHA512 in `portfile.cmake` with the actual hash of your release tarball
5. Test the port:
   ```bash
   vcpkg install mini-gmp-plus --overlay-ports=/path/to/your/ports
   ```
6. Submit a pull request to the vcpkg repository

## Local Testing

To test the port locally before submission:

```bash
# From your vcpkg installation directory
./vcpkg install mini-gmp-plus --overlay-ports=/path/to/mini-gmp-plus-plus
```

## Notes

- The package is configured to skip building tests when installed via vcpkg
- All public headers are installed to `include/mini-gmp-plus/`
- The library supports dynamic linking on all platforms

