const e=`# WaviateScript \`rules.md\`\r
\r
This document defines the **minimum rules** for writing and compiling a WaviateScript shader module.\r
\r
A WaviateScript module is a **single entry file** (C, C++, or Rust) that **exports one or both** of:\r
\r
- \`sample_process\` (time-domain, per-sample or per-frame processing)\r
- \`frequency_process\` (frequency-domain, per-bin processing)\r
\r
If **at least one** of those functions is present, the shader is considered **valid**.\r
\r
---\r
\r
## 1) Required entry file\r
\r
### 1.1 Single entry file\r
You provide exactly **one** entry file to WaviateScript:\r
\r
- C: \`*.c\`\r
- C++: \`*.cpp\`, \`*.cc\`, or \`*.cxx\`\r
- Rust: \`*.rs\`\r
\r
WaviateScript only *parses* this entry file to detect exported shader functions. Your entry file may include or reference other files, but function detection is entry-file-only.
\r
### 1.2 Required exported symbol names\r
The shader functions must be named **exactly**:\r
\r
- \`sample_process\`\r
- \`frequency_process\`\r
\r
---\r
\r
## 2) C interface (authoritative ABI)\r
\r
The C interface is the **authoritative ABI**; however, C++ and Rust idiomatic interfaces will be automatically converted by Waviate's internal compiler toolchain. Both C and Rust provide an immutable state reader structure and a mutable state writer. C++ instead uses a single argument with managed read-only fields and mutable fields. Unlike GPU shaders which act in parallel on all vertices/pixels, any state changes written to the Writer object in the waviate shader function will be available to the following sample/frequency-bin call. This allows users to perform time dependent calculations.\r
\r
### 2.1 Sample processing\r
\`\`\`c\r
float sample_process(const WaviateSampleInput* in, WaviateSampleStateWriter* out);\r
\`\`\`\r
\`\`\`cpp\r
float sample_process(const WaviateSampleInputCpp& in);\r
\`\`\`\r
\`\`\`rust\r
pub fn sample_process(\r
    input: &WaviateSampleInput,\r
    state: &mut WaviateSampleStateWriter,\r
) -> f32 {\r
    ...\r
}\r
\`\`\`\r
### 2.2 Frequency processing\r
\`\`\`c\r
float frequency_process(const WaviateFrequencyInput* in, WaviateFrequencyStateWriter* out);\r
\`\`\`\r
\`\`\`cpp\r
float frequency_process(const WaviateFrequencyInputCpp& in);\r
\`\`\`\r
\`\`\`rust\r
pub fn frequency_process(\r
    input: &WaviateFrequencyInput,\r
    state: &mut WaviateFrequencyStateWriter,\r
) -> f32 {\r
    ...\r
}\r
\`\`\`\r
\r
### 2.3 Input APIs\r
- [Inputs](input_apis.md)\r
\r
## 3) Function presence and validity\r
\r
A module is valid if at least one is defined:\r
\r
sample_process(...) or\r
\r
frequency_process(...)\r
\r
If one is missing:\r
\r
WaviateScript will skip that stage.\r
\r
If frequency_process is missing, the engine may skip FFT entirely (preserving performance and sample-accurate pass-through).

## 4) Execution order

Stage order is not configurable within a single plugin instance.

If only \`sample_process\` is present, WaviateScript runs the sample stage.

If only \`frequency_process\` is present, WaviateScript runs the frequency stage.

If both \`sample_process\` and \`frequency_process\` are present, WaviateScript always runs \`sample_process\` first and \`frequency_process\` second.

Function declaration order, compiler symbol order, linker order, source file layout, and shader configuration do not change this stage order.

## 5) Includes / modules / project layout
### 5.1 C and C++
\r
Supported by default:\r
\r
Local includes relative to the entry file directory:\r
\`\`\`c\r
#include "helpers.h"\r
\r
#include "./dsp/filters.h"\r
\`\`\`\r
Absolute-path includes may work depending on OS permissions and sandbox rules, but are discouraged.\r
\r
Not supported by default:\r
\r
Custom include directories passed by user configuration. Linking external third-party libraries (unless enabled in an advanced/premium toolchain). These may become available to premium users or in advanced settings as a stretch goal for the project.\r
\r
You are responsible for include guards or #pragma once in your headers.\r
\r
### 5.2 Rust (idiomatic structure without boilerplate)
\r
Rust does not have headers. Use modules.\r
\r
Goal: keep it “single-file simple,” with optional helpers.\r
\r
By default:\r
\r
The entry *.rs file is treated as the crate root.\r
\r
You may add sibling *.rs files and reference them using mod.\r
\r
Example:\r
\`\`\`rust\r
mod helpers; // loads helpers.rs in the same folder\r
\`\`\`\r
\r
No Cargo project required. No forced main.rs naming. Your entry file can be descriptively named.\r
\r
## 6) Pass-through expectations
### 6.1 “No-op” shader
\r
A no-op implementation should behave as pass-through:\r
\r
In sample stage: write outputs equivalent to inputs.\r
\r
In frequency stage: if bins are unmodified, the stage should not introduce audible artifacts if the engine bypasses FFT when frequency stage is absent.\r
\r
### 6.2 FFT windowing note
\r
If the engine performs windowed STFT/overlap-add, then a frequency stage that runs (even pass-through) may still slightly shape audio. If sample-accurate pass-through is required, the recommended behavior is:\r
\r
If frequency_process is not present → skip FFT path entirely\r
\r
If frequency_process is present → accept that windowing/OLA may affect samples\r
\r
(Exact FFT policy is engine-defined and configurable; the rule here is about what authors should expect.)\r
\r
## 7) Compilation rules (default toolchain)
### 7.1 Output formats
\r
Depending on platform/toolchain, the output may be:\r
\r
Shared library: .dll / .so / .dylib\r
\r
WebAssembly: .wasm (when targeting wasm)\r
\r
These formats are handled internally within the engine's toolchain, so users should only be concerned with source code. \r
\r
As a project stretch goal, users may be able to package files to encrypted web-assembly with shader usage metadata. This allows safe sandboxed code execution and opens the door for an instrument/shader marketplace\r
\r
### 7.2 No-config defaults
\r
Default pipeline aims for “drop in a file and compile”:\r
\r
C/C++:\r
\r
Compiles entry file and any included local headers\r
\r
Links only against the WaviateScript SDK runtime\r
\r
Rust:\r
\r
Compiles crate rooted at the entry file\r
\r
Includes sibling modules via mod\r
\r
No external crates unless explicitly allowed by the toolchain tier\r
\r
### 7.3 Determinism requirements
\r
Do not rely on:\r
\r
Compiler symbol ordering\r
\r
Linker ordering\r
\r
Address layout\r
\r
Unspecified initialization order\r
\r
Execution order is defined only by section 4.
`;export{e as default};
