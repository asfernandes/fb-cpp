## fb-cpp Agent Guide

### Code style
- Classes names uses PascalCase.
- Method names uses camelCase.
- Variable names uses camelCase.
- Do not use variables names starting or ending with underline.
- When using statements like `if`, `for`, `while`, etc, if the condition plus the sub-statement takes more than two
  lines, then the sub-statement should use braces. Otherwise, braces should be avoided.
- A C++ source file should be formatted with `clang-format`.
- Documentation uses doxygen with  `///` comments. Empty lines with only `///` should be used before and after the
  documentation.
- When scopes are introduced for RAII purposes, they should be commented as such:
  ```cpp
  {  // scope
  }
  ```
- When adding new source files, copy the header from the existing files, but update the Copyright line to reflect the current year and use `$(git config --get user.name)` as the original author.
- Use lowercase suffixes for numeric literals (e.g. `1u`, `3u`, not `1U`, `3U`).
- Do not use string literals in `assert()` expressions (e.g. `assert(ptr && "msg")` is not idiomatic C++).
- Do not explicitly define a destructor as `= default` unless it is necessary (e.g. to make it virtual or to
  control the definition translation unit).
- Prefer constructing `std::vector` directly from iterator ranges (constructor 5) instead of allocating and copying
  with `std::memcpy`.

### Build
- If .cpp files are added, it's necessary to run `cmake --preset default` from the repo root.
- Use `cmake --build --preset default` from the repo root.

### Tests
- Run the Boost.Test suite with `ctest --preset default --verbose`.
- Boost.Test options can be used with environments variables like `BOOST_TEST_LOG_LEVEL=all`.
- Prefer fewer, comprehensive test cases over many small single-assertion tests. A single test case should verify
  all related properties together (e.g., check the full error vector structure in one test instead of separate tests
  for each field).
