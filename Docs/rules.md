# WaviateScript `rules.md`

This document defines the **minimum rules** for writing and compiling a WaviateScript shader module.

A WaviateScript module is a **single entry file** (C, C++, or Rust) that **exports one or both** of:

- `sample_process` (time-domain, per-sample or per-frame processing)
- `frequency_process` (frequency-domain, per-bin processing)

If **at least one** of those functions is present, the shader is considered **valid**.

---

## 1) Required entry file

### 1.1 Single entry file
You provide exactly **one** entry file to WaviateScript:

- C: `*.c`
- C++: `*.cpp`, `*.cc`, or `*.cxx`
- Rust: `*.rs`

WaviateScript only *parses* this entry file to infer execution order (see section 4). Your entry file may include or reference other files (see section 5), but order inference is entry-file-only.

### 1.2 Required exported symbol names
The shader functions must be named **exactly**:

- `sample_process`
- `frequency_process`

---

## 2) C interface (authoritative ABI)

The C interface is the **authoritative ABI**; however, C++ and Rust idiomatic interfaces will be automatically converted by Waviate's internal compiler toolchain. Both C and Rust provide an immutable state reader structure and a mutable state writer. C++ instead uses a single argument with managed read-only fields and mutable fields. Unlike GPU shaders which act in parallel on all vertices/pixels, any state changes written to the Writer object in the waviate shader function will be available to the following sample/frequency-bin call. This allows users to perform time dependent calculations.

### 2.1 Sample processing
```c
float sample_process(const WaviateSampleInput* in, WaviateSampleStateWriter* out);
```
```cpp
float sample_process(const WaviateSampleInputCpp& in);
```
```rust
pub fn sample_process(
    input: &WaviateSampleInput,
    state: &mut WaviateSampleStateWriter,
) -> f32 {
    ...
}
```
### 2.2 Frequency processing
```c
float frequency_process(const WaviateFrequencyInput* in, WaviateFrequencyStateWriter* out);
```
```cpp
float frequency_process(const WaviateFrequencyInputCpp& in);
```
```rust
pub fn frequency_process(
    input: &WaviateFrequencyInput,
    state: &mut WaviateFrequencyStateWriter,
) -> f32 {
    ...
}
```

### 2.3 Input APIs
- [Inputs](input_apis.md)

## 3) Function presence and validity

A module is valid if at least one is defined:

sample_process(...) or

frequency_process(...)

If one is missing:

WaviateScript will skip that stage.

If frequency_process is missing, the engine may skip FFT entirely (preserving performance and sample-accurate pass-through).


## 4) Execution order rules
### 4.1 Primary rule: order in entry file

If WaviateScript can see both definitions in the entry file text, then:

The function defined first in the entry file runs first.

The function defined second runs second.

This is based on a simple text search match. If the first appearance of "frequency_process("
 appears before the first of "sample_process(" as an exact text match within the file, that will determine the execution order. This means function definitions can be declared in included files, and a user can actually use function declarations or even comments in the entry file text to define the order. 

### 4.2 Fallback rule: default order

If both frequency and sample shaders are defined and WaviateScript cannot determine order because:

#### 4.2.1: One or both function bodies are defined in headers / other files only

#### 4.2.2: No text match from section 4.1 appears in the entry file

#### 4.2.3: Macros obscure the function body such that it is not parseable

…then WaviateScript defaults to:

sample_process first

frequency_process second

### 4.3 Practical guidance

If you care about custom order:

Put the actual function bodies in the entry file, in the order you want.

Keep wrappers thin and obvious.

## 5) Includes / modules / project layout
### 5.1 C and C++

Supported by default:

Local includes relative to the entry file directory:
```c
#include "helpers.h"

#include "./dsp/filters.h"
```
Absolute-path includes may work depending on OS permissions and sandbox rules, but are discouraged.

Not supported by default:

Custom include directories passed by user configuration. Linking external third-party libraries (unless enabled in an advanced/premium toolchain). These may become available to premium users or in advanced settings as a stretch goal for the project.

You are responsible for include guards or #pragma once in your headers.

### 5.2 Rust (idiomatic structure without boilerplate)

Rust does not have headers. Use modules.

Goal: keep it “single-file simple,” with optional helpers.

By default:

The entry *.rs file is treated as the crate root.

You may add sibling *.rs files and reference them using mod.

Example:
```rust
mod helpers; // loads helpers.rs in the same folder
```

No Cargo project required. No forced main.rs naming. Your entry file can be descriptively named.

## 6) Pass-through expectations
### 6.1 “No-op” shader

A no-op implementation should behave as pass-through:

In sample stage: write outputs equivalent to inputs.

In frequency stage: if bins are unmodified, the stage should not introduce audible artifacts if the engine bypasses FFT when frequency stage is absent.

### 6.2 FFT windowing note

If the engine performs windowed STFT/overlap-add, then a frequency stage that runs (even pass-through) may still slightly shape audio. If sample-accurate pass-through is required, the recommended behavior is:

If frequency_process is not present → skip FFT path entirely

If frequency_process is present → accept that windowing/OLA may affect samples

(Exact FFT policy is engine-defined and configurable; the rule here is about what authors should expect.)

## 7) Compilation rules (default toolchain)
### 7.1 Output formats

Depending on platform/toolchain, the output may be:

Shared library: .dll / .so / .dylib

WebAssembly: .wasm (when targeting wasm)

These formats are handled internally within the engine's toolchain, so users should only be concerned with source code. 

As a project stretch goal, users may be able to package files to encrypted web-assembly with shader usage metadata. This allows safe sandboxed code execution and opens the door for an instrument/shader marketplace

### 7.2 No-config defaults

Default pipeline aims for “drop in a file and compile”:

C/C++:

Compiles entry file and any included local headers

Links only against the WaviateScript SDK runtime

Rust:

Compiles crate rooted at the entry file

Includes sibling modules via mod

No external crates unless explicitly allowed by the toolchain tier

### 7.3 Determinism requirements

Do not rely on:

Compiler symbol ordering

Linker ordering

Address layout

Unspecified initialization order

Execution order is defined only by section 4.