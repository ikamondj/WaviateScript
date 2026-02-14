# Waviate Script Docs
Waviate Script is a real-time audio scripting system designed for deterministic, sample-accurate signal processing. It allows users to describe audio behavior using small, shader-like programs that are compiled and executed inside a managed audio engine.

The system is intended for situations where traditional DAW workflows are too rigid, but raw DSP code is too low-level or fragmented. Waviate Script sits between those extremes: high-performance, explicit, and predictable, while still being expressive and modular.

## About
Waviate Script exists to reduce the amount of boilerplate required to connect custom audio logic to real-time audio systems. Rather than forcing each host or language to expose its own bespoke DSP interface, Waviate Script defines a common execution and data interface that can be implemented consistently across multiple systems languages.

### Programming Details
The project is designed around a shared ABI-level contract. Audio shaders are authored in languages such as C, C++, or Rust and compiled into binaries that conform to this contract. The runtime is responsible for wiring those binaries into the surrounding audio environment, handling buffer exchange, parameter binding, and host integration. For standard users this integrates audio inputs, sidechain, midi and keyboard inputs. For premium users, we also include common inter-process communication and network protocols, integration with DAW features, support for HID devices and more. 

### Shader Language Information
The core guiding principle of waviate script is to write shader-like code to process and output audio signals. If you aren't familiar with the graphics world, shader code is a common function interface that acts, say, per pixel or per geometric point. The arguments to the function carry data about the context of the function call (like enough information to know which pixel you're operating on) and the system automatically renders graphics at each pixel based on your function's output. Think of it as a function mapping your pixel coordinate to a color. 

Similarly, waviate flow acts on audio samples and frequency bins. You can define a function and it will have a context input describing a large amount of context: "what audio sample am I on? How long has the app been running in seconds? Is a certain midi note currently being pressed?" are just a few examples. Waviate flow then processes your logic and plays back in real-time or records in an offline rendering mode. These functions can process per sample, per frequency bin, or both in either order.

### Note about shader domains
-Per-sample functions are sample-accurate and local.
-Per-frequency stages operate on windowed frames and are sample-aligned but not sample-local. Temporal resolution is bounded by the transform size and hop interval. These are configurable and have tradeoffs depending on the user's needs: 
"better time response, worse frequency detail" <-> "better frequency detail, more latency/smear"

Execution behavior is intentionally flexible rather than strictly deterministic. While sample-accurate processing is possible, Waviate Script does not impose hard guarantees around scheduling, threading, or host timing beyond what the embedding system provides. This allows the same shader code to be reused across different engines, platforms, and integration contexts without rewriting interface glue.

Waviate Script is not a replacement for full audio frameworks or DAWs. It is a connective layer: a way to express audio processing logic once, and embed it into many systems with minimal friction.

## Getting started
- [Guide](guide.md)
