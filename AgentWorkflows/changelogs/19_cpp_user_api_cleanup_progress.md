# C++ user API cleanup

## 2026-07-12

- Renamed every `get...` function on `WaviateCore`, `WaviateSample`, and `WaviateFrequency` to a noun/query-style accessor.
- Confirmed all read accessors and pure façade helpers are `const`.
- Made `setCurrentSample` non-const and updated the recommended entry-point signature to accept a mutable façade reference for explicit frame-local setters.
- Made complex components private and exposed `real()` and `imaginary()` alongside the existing pure complex operations.
- Updated the embedded shader API, autocomplete metadata, starter template, fixtures, serializer test source, and compilation tests.
- Removed raw C ABI types and fields from C++ autocomplete. Embedded raw context fields are private to their façade classes.
- Reworked Marketplace documentation around the current C++-only user interface. C is documented as an internal future-language ABI; Rust is explicitly unsupported, not under active development, and a stretch goal.
- Public Release standalone builds successfully.
- Premium Release standalone builds successfully.
- All `WaviateScriptTests` pass, including a new frequency/complex accessor compilation test.
