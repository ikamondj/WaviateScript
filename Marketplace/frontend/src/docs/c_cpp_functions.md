# Available C++ Functions

WaviateScript exposes an audited subset of common scalar math functions to shaders. This includes trigonometric, hyperbolic, exponential, logarithmic, power, root, rounding, remainder, classification, and min/max functions such as `sin`, `cos`, `tan`, `exp`, `log`, `log2`, `pow`, `sqrt`, `floor`, `ceil`, `round`, `fmod`, `fmin`, and `fmax`, with supported float variants.

The Waviate façade adds waveform, envelope, timing, noise, audio-loading, and frame-scoped container helpers. See the [C++ Shader API](input_api_cpp.md) for context access.

This is not a general standard-library environment. File I/O, processes, networking, arbitrary allocation, unsafe external symbols, and unaudited system APIs are not exposed to shader code.

Complex frequency values support arithmetic operators and the pure `waviate_complex` helpers `abs`, `arg`, `conj`, `polar`, `exp`, `log`, `pow`, `sin`, `cos`, and `tan`.
