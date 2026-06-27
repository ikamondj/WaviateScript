# 12d Fix Standard Header Restrictions Progress

Updated: 2026-06-26

## Completed in this pass

- Added a shared audited runtime-capability module:
  - `waviate::runtime::isAuditedExternalFunction`
  - `waviate::runtime::registerAuditedRuntimeSymbols`
- Moved external-function safety checks to the shared audit predicate so validation and runtime registration use the same allowlist.
- Kept OS/process/filesystem/network/thread capabilities outside the allowlist.
- Explicitly registered deterministic C math functions for MCJIT resolution, including `sin` and `sinf`.
- Added basic memory/string primitives that are deterministic and do not create persistent storage:
  - `memcpy`
  - `memmove`
  - `memset`
  - `memcmp`
  - `strlen`
- Added a compile-pipeline regression for user shader calls to `sin`, `sinf`, `sqrtf`, and `fmaxf`.

## Verification

- Direct MSVC syntax check passed for `Source/WaviateRuntimeCapabilities.cpp`.
- Direct MSVC syntax check passed for `Source/WaviateSafetyValidator.cpp`.
- Direct MSVC syntax check passed for `Source/WaviateSafety.cpp` using the generated JUCE target defines.
- Direct MSVC syntax check passed for `Tests/Compile/CompilePipelineTests.cpp`.
- Full `cmake --build build-cmake-vs --config Release --target WaviateScriptTests` is currently blocked during CMake/JUCE regeneration by MSBuild `SetEnv` failing with `Environment variable name or value is too long` while building `juceaide`.

## Notes

- The allowlist remains explicit. This is intentionally not a general standard-library permission model.
- The safe set is limited to pure math/classification functions and simple memory/string primitives that users could otherwise implement inside shader code.
