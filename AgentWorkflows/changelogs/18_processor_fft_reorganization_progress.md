# Processor and FFT reorganization

## 2026-07-12

- Reduced `processBlock` to coordination of block setup, sample processing, frequency setup/bin processing, accounting, and visualization.
- Added `AudioProcessingFlow.inl` with inlined common/standalone/DAW and premium/non-premium setup boundaries. Intentional future hooks are empty.
- Added preallocated FFT plans/work buffers for 256-8192 sample transforms and atomic runtime configuration publication.
- Added Tools > Frequency Domain menus for FFT size, processed-bin limit, and rectangular/Hann selection, persisted in user settings.
- Added `WaviateBasicComplex<T>`, `fcomplex`, `dcomplex`, and pure arithmetic/transcendental helpers while retaining `WaviateComplex` as the shader-facing float type.
- Public Release standalone compiles successfully with JUCE DSP linked.
- `WaviateScriptTests` compiles and all tests pass. Existing LLVM/JUCE warnings remain; no new build errors.
- Premium Release standalone also compiles successfully, confirming premium-only setup boundaries.
- Final regression after embedding the complex API: all `WaviateScriptTests` pass.
