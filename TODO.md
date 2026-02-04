# TODO

## Testing

- [x] Add parser edge case tests (malformed input, deeply nested expressions)
- [x] Add VM opcode tests (arithmetic, stack manipulation, conditionals)
- [x] Add Array/List operation tests (construction, indexing, fold/scan)
- [x] Add error handling path tests (syntax, type, stack errors)

### Features

- [ ] Design and implement plugin architecture: for third-party extensibility needed
- [ ] Add configuration file support - For complex setups

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
- [ ] Consider unified image format across platforms
- [ ] Plan C++20 migration path for cleaner compiler intrinsics
- [ ] Migrate to standard exceptions - Better debugging experience
- [ ] Consider native Windows audio backend (optional enhancement given RtAudio)

## Build System

- [ ] Consider enabling CI on push/PR (currently manual workflow_dispatch only)

## Code Quality

- [ ] Add more unit tests: enable testing components in isolation
- [ ] Add const-correctness audit for method parameters
- [ ] Consider extracting VM opcodes into separate compilation units

