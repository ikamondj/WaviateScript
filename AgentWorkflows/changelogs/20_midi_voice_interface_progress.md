# MIDI voice interface

## 2026-07-12

- Extended both sample and frequency C ABI contexts with MIDI note/CC state, 64-bit note press/release timestamps, newest-first unique press/release order arrays, and a newest-first unique voice order.
- Kept all storage fixed at 128 entries and updated it on the audio thread without allocation.
- Made MIDI state advance even for frequency-only shaders.
- Added shared `WaviateCore` queries for absolute event samples, elapsed samples, ordering, voice count, 12-TET frequency, phase, custom compile-time tuning, and note-aware ADSR.
- Added `midiVoices(maximumVoices)`, an allocation-free iterable view whose voices expose note, held state, sample timing, frequency, phase, custom tuning, and ADSR.
- Stabilized the public/premium ABI layout by keeping OSC pointer slots present in both editions (null when unavailable).
- Updated embedded shader code, autocomplete metadata, test invocation support, and Marketplace C++/C ABI documentation.
- Added compiled shader coverage for bounded voice iteration, custom tuning, sample-precise timing, held ADSR, release ADSR, and frequency-context MIDI access.
- All `WaviateScriptTests` pass.
- Public and premium Release standalone targets build successfully.
