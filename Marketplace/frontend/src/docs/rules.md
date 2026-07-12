# Shader Rules

## Supported language

C++ is the only first-class shader language currently supported. C is retained as an internal ABI for future language frontends, not as the primary user interface. Rust is not currently supported or under active development; it is a stretch goal.

## Entry points

A shader is valid when it defines one or both of these functions:

```cpp
float SampleProcess(WaviateSample& wav);
WaviateComplex FrequencyProcess(WaviateFrequency& wav);
```

If an entry point is absent, its processing stage is skipped. When both are present, `SampleProcess` always runs before `FrequencyProcess`.

## Context access

Use the `wav` façade to read audio, timing, MIDI, sidechain, and frequency context. Raw engine context fields are not part of the supported C++ API.

- Read-only accessors and pure helpers are `const` and do not use a `get` prefix.
- Functions beginning with `set` are non-const and mutate frame-local processing state.
- Mutation happens only while shader code executes on the audio thread.
- Engine-owned pointers and internal ABI structures must not be retained or accessed directly.

See the [C++ Shader API](input_api_cpp.md) for the current function list.

## Real-time safety

Shader code runs in the real-time audio path. Do not perform file access, networking, process creation, unrestricted allocation, blocking synchronization, or other operating-system work. WaviateScript validates and meters compiled code, but shader authors should still keep work bounded and predictable.

Temporary Waviate containers use frame-scoped engine storage. Do not assume their contents survive into a later block or FFT frame.

## Processing order and pass-through

Sample processing is sample-wise and channel-wise. Frequency processing operates on FFT frames and complex bins. FFT window size, processed-bin limit, and window function are application settings.

If no frequency entry point exists, the FFT path is skipped. A sample-stage pass-through shader should return `wav.incomingSample()`. A frequency-stage pass-through shader should return `wav.incomingSample()` as a `WaviateComplex`.

## Source layout

The current workflow compiles a C++ shader source through WaviateScript's managed Clang pipeline. Only the audited embedded API and approved math/runtime symbols are available. Arbitrary system headers, external libraries, mutable global storage, raw allocation, inline assembly, and unsafe external symbols are rejected.

WebAssembly packaging and sandboxed marketplace distribution are roadmap goals, not current shader guarantees.
