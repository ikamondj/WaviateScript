# C ABI Status

The engine has a C-compatible internal ABI for sample and frequency entry points. It allows the runtime and future language frontends to exchange context without depending on a C++ object ABI.

This is not currently a first-class user authoring interface. Its raw structures are engine implementation details and may evolve as C++, Rust, or other frontends are developed. Shader authors should use the [C++ Shader API](input_api_cpp.md), which keeps context storage private and exposes it through `const` accessors and explicit non-const `set...` functions.

The internal sample and frequency context ABIs both carry MIDI state. The MIDI portion includes 128-entry note/CC state arrays, absolute `uint64_t` sample timestamps for the latest press, release, and CC change, newest-first unique note press/release order arrays with counts, and a newest-first unique voice order with its count. These integer sample encodings preserve timing precision for long-running sessions. The C++ façade is the supported ownership- and bounds-safe view over this data.
