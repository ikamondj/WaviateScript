# Marketplace Native Compiler

This directory builds the C ABI shim used by the Go backend when it is compiled
with `-tags nativecompiler`.

The shim keeps compiler implementation code in the desktop source tree. CMake
references `../../../Source` and builds a static library named
`waviate_marketplace_compiler`.

On Windows, the same C ABI is also built as
`waviate_marketplace_compiler_runtime.dll`. Go's Windows toolchain uses MinGW
for cgo, which does not link cleanly against the MSVC C++ static `.lib`, so the
local Windows native build loads the DLL at runtime. The Lambda/non-Windows path
uses cgo against the static library.

## Build

```powershell
powershell -ExecutionPolicy Bypass -File ./scripts/Build-NativeCompiler.ps1
```

The CMake file auto-detects the standard Windows LLVM install at
`C:/Program Files/LLVM` and the repo-local JUCE checkout at
`../../../_deps/juce-src`. You can still override them:

```powershell
powershell -ExecutionPolicy Bypass -File ./scripts/Build-NativeCompiler.ps1 `
  -LLVM_DIR "C:/Program Files/LLVM/lib/cmake/llvm" `
  -Clang_DIR "C:/Program Files/LLVM/lib/cmake/clang" `
  -JuceDir "../../../_deps/juce-src"
```

The Go cgo wrapper expects the non-Windows static library under
`native/build/lib`. The Windows runtime loader checks
`MARKETPLACE_NATIVE_COMPILER_DLL` first, then the default build output under
`native/build/bin/<Config>`.

## Go Build

```powershell
$env:CGO_ENABLED = "1"
go build -tags nativecompiler ./cmd/serverless
```

Without the `nativecompiler` build tag, the backend uses a stub compiler
validator. Set `MARKETPLACE_REQUIRE_NATIVE_COMPILER=1` to make the stub fail
closed instead of skipping native compilation.

The active validation endpoint is:

```http
POST /api/source/compile-check
Content-Type: application/json

{ "content": "float SampleProcess(const WaviateSample& wav) { return wav.getIncomingSample(); }" }
```
