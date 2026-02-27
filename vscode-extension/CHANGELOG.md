# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-02-27

### Added
- Initial release of Waviate Script VS Code extension
- Language association for `.wc` (C), `.wcpp` (C++), and `.wrs` (Rust) files
- Syntax highlighting for all three file types
- Code snippets for common Waviate patterns:
  - Function templates for `sample_process()` and `frequency_process()`
  - Quick access helpers for MIDI, controllers, and audio buffers
  - Input/output buffer access patterns
- Full autocomplete/IntelliSense support for Waviate API:
  - Structure types (WaviateSampleInput, etc.)
  - Function signatures
  - Field completions
- Hover documentation for API symbols
- Auto-generation of `.vscode/c_cpp_properties.json` when opening `.wc`/`.wcpp` files
- Proper include paths and forceInclude directives for IntelliSense

### Features Implemented
- ✅ Associate .wc files with c language
- ✅ Associate .wcpp files with cpp language
- ✅ Add snippets and autocomplete items for the Waviate API
- ✅ Associate .wrs files with rust language
- ✅ Auto-generate c_cpp_properties.json with WaviateInput.h forceInclude

### Planned
- Custom Waviate DSP snippet library (filters, oscillators, envelopes)
- Real-time compilation error feedback
- Debugging support integration
- Project templates for common shader types
- Rust crate bindings for Waviate
- Language server protocol (LSP) support for advanced features
