# TODO

## Testing

- [ ] Add parser edge case tests (malformed input, Unicode, deeply nested expressions)
- [ ] Add VM opcode tests
- [ ] Add Array/List operation tests
- [ ] Add error handling path tests

## Documentation

- [ ] Add API documentation (Doxygen comments for public headers)
- [ ] Document naming conventions (camelCase methods, ALL_CAPS constants, etc.)
- [ ] Document Windows build status

## Performance Optimization

- [ ] Integrate libxsimd for Linux SIMD optimization
  - The ahihi/sapf fork uses xsimd for cross-platform SIMD support
  - Would provide vectorized math operations on Linux similar to Apple's Accelerate framework
  - Reference: https://github.com/xtensor-stack/xsimd

## Platform Support

- [ ] Investigate Windows build support (libedit dependency resolved with replxx, needs testing)

## Build System

- [ ] Consider enabling CI on push/PR (currently manual workflow_dispatch only)

## Code Quality

- [ ] Add const-correctness audit for method parameters
- [ ] Consider extracting VM opcodes into separate compilation units
