# WaviateScript Tests

This folder is the CMake-driven unit test entry point for the core compile pipeline.

## Structure

- `TestMain.cpp`: JUCE-based console test runner used by `ctest`.
- `TestSupport/WaviateUnitTest.h`: small macros for quick test case setup.
- `Compile/`: compile-engine-focused helpers, fixtures, and tests.
- `Serialization/`: serializer smoke tests and future round-trip coverage.

## Writing compile tests

Use the helper in `Compile/CompileTestHelpers.h`:

- `compileSource("short_name", sourceString)`
- `compileFixture("fixture_name.wlsl")`
- `invokeSample(result, { .incomingSample = 0.25f })`
- `expectCompileSuccess(*this, result)`
- `expectCompileFailure(*this, result)`

The helper builds the same default compile pipeline that the app uses and keeps the JIT/compiler objects alive so returned function pointers stay valid inside the test.

## Typical commands

```powershell
cmake -S . -B build-cmake-vs
cmake --build build-cmake-vs --config Release --target WaviateScriptTests
ctest --test-dir build-cmake-vs -C Release --output-on-failure
```

On this Windows setup, the installed LLVM/Clang libraries are release-runtime builds, so `Release` is the supported test configuration.
