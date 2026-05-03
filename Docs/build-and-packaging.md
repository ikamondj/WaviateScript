# Build and Packaging

WaviateScript builds directly with CMake. Projucer and the generated Visual
Studio exporter are no longer required for normal local or CI builds.

## Dependencies

- CMake 3.24 or newer
- Ninja or another CMake generator
- A C++20-capable compiler
- JUCE 8.0.12
- LLVM/Clang development packages with CMake config files
- FFTW headers and libraries
- Windows installer builds: WiX CLI (`wix`)
- macOS installer builds: Xcode command line tools for `pkgbuild` and `productbuild`

JUCE can be supplied from a local checkout:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DWAVIATESCRIPT_JUCE_DIR="C:/Users/ikamo/OneDrive/Documents/JuceInstalls/JUCE"
cmake --build build --config Release
```

If `WAVIATESCRIPT_JUCE_DIR` is omitted, CMake fetches JUCE from the official JUCE
repository pinned to tag `8.0.12`.

## Public Build

Windows example:

```powershell
cmake -S . -B build -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DWAVIATESCRIPT_JUCE_DIR="C:/Users/ikamo/OneDrive/Documents/JuceInstalls/JUCE" `
  -DWAVIATESCRIPT_FFTW_DIR="C:/path/to/fftw" `
  -DLLVM_DIR="C:/Program Files/LLVM/lib/cmake/llvm" `
  -DClang_DIR="C:/Program Files/LLVM/lib/cmake/clang"

cmake --build build --config Release
```

macOS example:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DWAVIATESCRIPT_JUCE_DIR="$HOME/JUCE" \
  -DWAVIATESCRIPT_FFTW_DIR="$(brew --prefix fftw)" \
  -DLLVM_DIR="$(brew --prefix llvm)/lib/cmake/llvm" \
  -DClang_DIR="$(brew --prefix llvm)/lib/cmake/clang"

cmake --build build --config Release
```

CMake/JUCE places plugin artifacts under `build/WaviateScript_artefacts/`, with
Standalone and VST3 subfolders for the package scripts to consume.

## Premium Build

Premium builds require WSPremium source but public builds do not. If
`WAVIATESCRIPT_PREMIUM=ON`, provide `WAVIATESCRIPT_PREMIUM_SOURCE_DIR`:

```powershell
cmake -S . -B build-premium -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DWAVIATESCRIPT_PREMIUM=ON `
  -DWAVIATESCRIPT_PREMIUM_SOURCE_DIR="C:/path/to/WSPremium" `
  -DWAVIATESCRIPT_JUCE_DIR="C:/Users/ikamo/OneDrive/Documents/JuceInstalls/JUCE" `
  -DWAVIATESCRIPT_FFTW_DIR="C:/path/to/fftw" `
  -DLLVM_DIR="C:/Program Files/LLVM/lib/cmake/llvm" `
  -DClang_DIR="C:/Program Files/LLVM/lib/cmake/clang"

cmake --build build-premium --config Release
```

If `Source/WSPremium` exists locally, CMake uses it as the default premium source
directory. Public checkouts can omit that folder entirely.

## Installers

The installer scripts do not rebuild the plugin. Run them after a CMake build.

Windows public installer:

```powershell
scripts/package/windows/build-installer.ps1 -BuildDir build -OutputDir dist -Configuration Release -Edition Public
```

Windows premium installer:

```powershell
scripts/package/windows/build-installer.ps1 -BuildDir build-premium -OutputDir dist -Configuration Release -Edition Premium
```

macOS public installer:

```bash
bash scripts/package/macos/build-installer.sh --build-dir build --output-dir dist --configuration Release --edition Public
```

macOS premium installer:

```bash
bash scripts/package/macos/build-installer.sh --build-dir build-premium --output-dir dist --configuration Release --edition Premium
```

Expected outputs:

- `dist/WaviateScript-1.0.0-windows.msi`
- `dist/WaviateScript-Premium-1.0.0-windows.msi`
- `dist/WaviateScript-1.0.0-macos.pkg`
- `dist/WaviateScript-Premium-1.0.0-macos.pkg`

The public and premium installer filenames and CI artifact names are distinct.
They install to the same Standalone and VST3 locations and are intended to
replace each other rather than install side by side, because the plugin bundle
identity remains `com.yourcompany.WaviateScript` with plugin code `Lh86`.

Windows install locations:

- Standalone: `C:\Program Files\WaviateScript`
- VST3: `C:\Program Files\Common Files\VST3`

macOS install locations:

- Standalone: `/Applications`
- VST3: `/Library/Audio/Plug-Ins/VST3/`

For macOS signing, set `MACOS_INSTALLER_SIGN_IDENTITY` before running the
package script. Notarization credentials are intentionally not required for
unsigned local packages.

## CI

The public workflow is `.github/workflows/build-installer.yml`. It has a manual
`platform` dropdown with `all`, `windows`, and `macos`.

The premium workflow is `.github/workflows/build-premium-installers.yml`. It uses
the same platform dropdown and expects these repository secrets:

- `WAVIATESCRIPT_PREMIUM_REPOSITORY`: the private WSPremium repository in
  `owner/name` form
- `WAVIATESCRIPT_PREMIUM_TOKEN`: a token with read access to that repository

No private source, private repo URL, or token value is committed.
