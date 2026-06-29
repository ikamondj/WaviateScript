const r=`# Build and Packaging\r
\r
WaviateScript builds directly with CMake. Projucer and the generated Visual\r
Studio exporter are no longer required for normal local or CI builds.\r
\r
## Dependencies\r
\r
- CMake 3.24 or newer\r
- Ninja or another CMake generator\r
- A C++20-capable compiler\r
- JUCE 8.0.12\r
- LLVM/Clang development packages with CMake config files\r
- FFTW headers and libraries\r
- Windows installer builds: WiX CLI (\`wix\`)\r
- macOS installer builds: Xcode command line tools for \`pkgbuild\` and \`productbuild\`\r
\r
JUCE can be supplied from a local checkout. Example:\r
\r
\`\`\`powershell\r
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DWAVIATESCRIPT_JUCE_DIR="C:/Users/ikamo/OneDrive/Documents/JuceInstalls/JUCE"\r
cmake --build build --config Release\r
\`\`\`\r
\r
If \`WAVIATESCRIPT_JUCE_DIR\` is omitted, CMake fetches JUCE from the official JUCE\r
repository pinned to tag \`8.0.12\`.\r
\r
## Public Build\r
\r
Windows example:\r
\r
\`\`\`powershell\r
$cmake = "C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"\r
\r
& $cmake -S . -B build -G "Visual Studio 18 2026" -A x64 \`\r
  -DWAVIATESCRIPT_JUCE_DIR="C:/Users/ikamo/OneDrive/Documents/JuceInstalls/JUCE" \`\r
  -DLLVM_DIR="C:/Program Files/LLVM/lib/cmake/llvm" \`\r
  -DClang_DIR="C:/Program Files/LLVM/lib/cmake/clang"\r
\r
& $cmake --build build --config Release --target WaviateScript_Standalone\r
\`\`\`\r
\r
The standalone executable is emitted to\r
\`build/WaviateScript_artefacts/bin/Standalone/WaviateScript.exe\`.\r
On Windows, the CMake build also stages the required LLVM runtime DLLs\r
(\`LLVM-C.dll\`, \`libclang.dll\`, \`zlib1.dll\`, and \`zstd.dll\`) next to the\r
standalone executable and inside the VST3 bundle.\r
\r
\`WAVIATESCRIPT_FFTW_DIR\` is optional for the current public build. The source\r
tree no longer requires FFTW to configure unless \`WAVIATESCRIPT_REQUIRE_FFTW\`\r
is enabled.\r
\r
If you prefer Ninja on Windows, run CMake from a developer environment that has\r
the MSVC toolchain configured. Invoking plain \`clang.exe\` in GNU frontend mode\r
is not a supported Windows setup for this project.\r
\r
macOS example:\r
\r
\`\`\`bash\r
cmake -S . -B build -G Ninja \\\r
  -DCMAKE_BUILD_TYPE=Release \\\r
  -DWAVIATESCRIPT_JUCE_DIR="$HOME/JUCE" \\\r
  -DWAVIATESCRIPT_FFTW_DIR="$(brew --prefix fftw)" \\\r
  -DLLVM_DIR="$(brew --prefix llvm)/lib/cmake/llvm" \\\r
  -DClang_DIR="$(brew --prefix llvm)/lib/cmake/clang"\r
\r
cmake --build build --config Release\r
\`\`\`\r
\r
CMake/JUCE places plugin artifacts under \`build/WaviateScript_artefacts/\`, with\r
\`bin/Standalone\` for the Windows standalone executable and \`lib/VST3\` for the\r
Windows VST3 bundle. The packaging scripts also accept the older \`Standalone\`\r
and \`Standalone Plugin\` layout variants.\r
\r
## Premium Build\r
\r
Premium builds require WSPremium source but public builds do not. If\r
\`WAVIATESCRIPT_PREMIUM=ON\`, provide \`WAVIATESCRIPT_PREMIUM_SOURCE_DIR\`:\r
\r
\`\`\`powershell\r
$cmake = "C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"\r
\r
& $cmake -S . -B build-premium -G "Visual Studio 18 2026" -A x64 \`\r
  -DWAVIATESCRIPT_PREMIUM=ON \`\r
  -DWAVIATESCRIPT_PREMIUM_SOURCE_DIR="C:/path/to/WSPremium" \`\r
  -DWAVIATESCRIPT_JUCE_DIR="C:/Users/ikamo/OneDrive/Documents/JuceInstalls/JUCE" \`\r
  -DLLVM_DIR="C:/Program Files/LLVM/lib/cmake/llvm" \`\r
  -DClang_DIR="C:/Program Files/LLVM/lib/cmake/clang"\r
\r
& $cmake --build build-premium --config Release --target WaviateScript_Standalone\r
\`\`\`\r
\r
If \`Source/WSPremium\` exists locally, CMake uses it as the default premium source\r
directory. Public checkouts can omit that folder entirely.\r
\r
## Installers\r
\r
The installer scripts do not rebuild the plugin. Run them after a CMake build.\r
\r
Windows public installer:\r
\r
\`\`\`powershell\r
scripts/package/windows/build-installer.ps1 -BuildDir build -OutputDir dist -Configuration Release -Edition Public\r
\`\`\`\r
\r
Windows premium installer:\r
\r
\`\`\`powershell\r
scripts/package/windows/build-installer.ps1 -BuildDir build-premium -OutputDir dist -Configuration Release -Edition Premium\r
\`\`\`\r
\r
macOS public installer:\r
\r
\`\`\`bash\r
bash scripts/package/macos/build-installer.sh --build-dir build --output-dir dist --configuration Release --edition Public\r
\`\`\`\r
\r
macOS premium installer:\r
\r
\`\`\`bash\r
bash scripts/package/macos/build-installer.sh --build-dir build-premium --output-dir dist --configuration Release --edition Premium\r
\`\`\`\r
\r
Expected outputs:\r
\r
- \`dist/WaviateScript-1.0.0-windows.msi\`\r
- \`dist/WaviateScript-Premium-1.0.0-windows.msi\`\r
- \`dist/WaviateScript-1.0.0-macos.pkg\`\r
- \`dist/WaviateScript-Premium-1.0.0-macos.pkg\`\r
\r
The public and premium installer filenames and CI artifact names are distinct.\r
They install to the same Standalone and VST3 locations and are intended to\r
replace each other rather than install side by side, because the plugin bundle\r
identity remains \`com.yourcompany.WaviateScript\` with plugin code \`Lh86\`.\r
\r
Windows install locations:\r
\r
- Standalone: \`C:\\Program Files\\WaviateScript\`\r
- VST3: \`C:\\Program Files\\Common Files\\VST3\`\r
\r
macOS install locations:\r
\r
- Standalone: \`/Applications\`\r
- VST3: \`/Library/Audio/Plug-Ins/VST3/\`\r
\r
For macOS signing, set \`MACOS_INSTALLER_SIGN_IDENTITY\` before running the\r
package script. Notarization credentials are intentionally not required for\r
unsigned local packages.\r
\r
## CI\r
\r
The public workflow is \`.github/workflows/build-installer.yml\`. It has a manual\r
\`platform\` dropdown with \`all\`, \`windows\`, and \`macos\`.\r
\r
The premium workflow is \`.github/workflows/build-premium-installers.yml\`. It uses\r
the same platform dropdown and expects these repository secrets:\r
\r
- \`WAVIATESCRIPT_PREMIUM_REPOSITORY\`: the private WSPremium repository in\r
  \`owner/name\` form\r
- \`WAVIATESCRIPT_PREMIUM_TOKEN\`: a token with read access to that repository\r
\r
No private source, private repo URL, or token value is committed.\r
`;export{r as default};
