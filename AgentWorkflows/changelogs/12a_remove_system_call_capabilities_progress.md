# 12a Remove System Call Capabilities Progress

Updated: 2026-06-25

## Completed in this pass

- Hardened the LLVM compile path so safety validation runs against unoptimized IR.
  - This prevents LLVM optimization from erasing evidence of unsafe constructs before validation.
  - Dynamic stack allocation/VLA checks now see the original emitted IR.
- Kept unresolved external calls behind an explicit allowlist.
  - Waviate runtime capabilities remain explicit.
  - Pure C math calls are explicitly allowlisted as audited non-OS capabilities.
  - Arbitrary host/process/filesystem/network/thread/dynamic-linking calls remain blocked.
- Added a Waviate ephemeral arena allocation facade to the embedded C++ shader API:
  - `wav.newArray<T>(size)`
  - `wav.newVector<T>(initialCapacity)`
  - `wav.newString()`
  - `wav.newString(const char*)`
  - `wav.newMap<K, V>(initialCapacity)`
- Implemented minimal arena-backed wrapper types:
  - `WaviateArray<T>`
  - `WaviateVector<T>`
  - `WaviateString`
  - `WaviateMap<K, V>`
- Restricted Waviate arena container values to trivially destructible types.
- Bound the existing preallocated `EphemeralArena` to shader execution:
  - Tests now execute shaders inside a scoped arena pass.
  - The audio processor owns a preallocated shader arena.
  - Each sample shader invocation resets the arena via `ScopedArenaPass`.
- Wired runtime trap handling in the audio processor:
  - Fuel/arena exhaustion clears the current output block.
  - The active script is deactivated.
  - The processor marks the script as over budget.
- Added compile pipeline tests covering:
  - Waviate facade allocation success.
  - Arena exhaustion fallback behavior.
  - Rejection of unallowlisted external symbols.
  - Rejection of inline assembly.
  - Rejection of arbitrary function pointer calls.
  - Rejection of raw `new`.
  - Rejection of mutable global storage.
  - Rejection of dynamic stack allocation.

## Verification

- `cmake --build build-cmake-vs --config Release --target WaviateScriptTests` passed.
- `build-cmake-vs\tests\bin\Release\WaviateScriptTests.exe` passed.

## Notes / Limits

- The Waviate arena facade is intentionally minimal and headerless because user shaders currently compile without standard library includes.
- `WaviateMap` is a simple linear map for now; it avoids hashing and heap allocation.
- Pure math externals are allowlisted because Clang may lower builtins such as `__builtin_sinf` to CRT symbols at unoptimized IR. They are treated as audited math capabilities, not as a general standard-library boundary.
