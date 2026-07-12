# Shader Input API

The supported user-facing shader API is C++:

- [C++ Shader API](input_api_cpp.md)

WaviateScript internally uses a C-compatible ABI to connect compiled code to the audio engine. It exists to enable future language frontends and is not currently a first-class authoring interface.

Rust is not supported and is not under active development. Rust integration remains a stretch goal for the project.
