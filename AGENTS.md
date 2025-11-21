## fb-cpp Agent Guide

### Code style
- Classes names uses PascalCase.
- Method names uses camelCase.
- Variable names uses camelCase.
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

### Build
- If .cpp files are added, it's necessary to run `cmake --preset default` from the repo root.
- Use `cmake --build --preset default` from the repo root.

### Tests
- Test code can use Boost.Multiprecision without conditional compilation.
- Run the Boost.Test suite with `ctest --preset default --verbose`.
- Boost.Test options can be used with environments variables like `BOOST_TEST_LOG_LEVEL=all`.
