# WaviateScript Installer Guide

## Overview

This setup includes:
1. **WaviateScript.wxs** - WiX installer source file for creating MSI installer
2. **.github/workflows/build-installer.yml** - GitHub Actions workflow for automated builds

## Installer Features

### Installation Locations

- **Standalone Executable**: `Program Files\Waviate\Script\bin\WaviateScript.exe`
- **VST3 Plugin**: `Program Files\Common Files\VST3\WaviateScript.vst3`
- **Header Files**: `Program Files\Waviate\Script\include\`
  - `Waviate.h`
  - `Waviate.hpp`

### Best Practices Implemented

- **Per-machine Installation**: Clean, centralized installation requiring admin privileges
- **Automatic Overwrite**: New versions automatically replace old installations
- **VST3 Standard Location**: Installs to the Windows standard VST3 folder recognized by all major DAWs
- **Version Management**: Version-based upgrade handling with no downgrade allowed
- **Embedded CAB**: All files compressed into single MSI for efficient distribution
- **x64 Support**: 64-bit architecture targeting

## GitHub Actions Workflow

### Trigger Events

The workflow automatically builds when:
- Pushes to `main` branch
- Pushes with tags matching `v*` (e.g., `v1.0.0`) → creates GitHub Release
- Pull requests to `main`
- Manual trigger via Actions tab

### Build Process

1. Checkout repository with submodules
2. Setup MSBuild
3. Install WiX Toolset via Chocolatey
4. Build solution in Release configuration (non-premium)
5. Verify build outputs exist
6. Create installer working directory
7. Generate RTF license file
8. Compile WiX source with candle.exe
9. Link into MSI with light.exe
10. Upload artifact (30-day retention)
11. Create GitHub Release if on version tag

### Output Artifacts

- **Artifact Name**: `waviate-script-installer-x64`
- **File**: `WaviateScript-Installer.msi`
- **Available**: Actions → Select workflow run → Artifacts section

## Manual Build Instructions

If building locally without GitHub Actions:

```powershell
# 1. Build the solution
msbuild "Builds/VisualStudio2026/WaviateScript.sln" `
  /p:Configuration=Release /p:Platform=x64 /m

# 2. Install WiX Toolset
choco install wixtoolset -y --force

# 3. Prepare files
mkdir installer_build
copy "Builds/VisualStudio2026/x64/Release/Standalone Plugin/WaviateScript.exe" `
     "installer_build/WaviateScript_Standalone.exe"
copy "Builds/VisualStudio2026/x64/Release/VST3/WaviateScript.vst3" `
     "installer_build/"
copy "Source/Waviate.h" "installer_build/"
copy "Source/Waviate.hpp" "installer_build/"

# 4. Compile and link
candle.exe -o WaviateScript.wixobj WaviateScript.wxs
light.exe -o WaviateScript-Installer.msi WaviateScript.wixobj
```

## Configuration Details

### WiX Installer Settings

| Setting | Value | Purpose |
|---------|-------|---------|
| UpgradeCode | 12345678-1234-1234-1234-123456789012 | ⚠️ CUSTOMIZE: Must be unique per product |
| Platform | x64 | 64-bit Windows support |
| InstallScope | perMachine | Requires admin, installs system-wide |
| Version | 1.0.0.0 | Update per release |

### Important Customization

**⚠️ UPDATE THE UPGRADE CODE**: The UpgradeCode in `WaviateScript.wxs` must be unique and consistent across all versions. Update it to something like:
```xml
UpgradeCode="your-unique-guid-here"
```

You can generate a GUID in PowerShell:
```powershell
[guid]::NewGuid()
```

### VST3 Compatibility

The installer places VST3 plugins in the Windows standard location (`Program Files\Common Files\VST3`), automatically discovered by:
- Studio One
- Ableton Live 12+
- Reaper
- FL Studio
- Cubase
- Logic Pro (via wrapper)
- And all other DAWs using VST3

## Customization Guide

### Change Installation Paths

Edit these directories in `WaviateScript.wxs`:
- `<Directory Id="WaviateFolder" Name="Waviate">` → Change folder name
- `<Directory Id="BinFolder" Name="bin">` → Change bin folder name

### Add More Files

Add new files to `WaviateScript.wxs`:
```xml
<DirectoryRef Id="IncludeFolder">
  <Component Id="MyNewFile" Guid="new-unique-guid">
    <File Id="MyFile" Source="path/to/file" KeyPath="yes"/>
  </Component>
</DirectoryRef>
```

### Change Version Number

Update in workflow and WiX:
1. Add version tag: `git tag v1.1.0`
2. Update Version in WiX: `<Product Id="*" Name="..." Version="1.1.0.0"`

## Troubleshooting

### Build Path Issues

If the workflow fails with "not found" errors, verify:
1. Standalone output: `Builds/VisualStudio2026/x64/Release/Standalone Plugin/`
2. VST3 output: `Builds/VisualStudio2026/x64/Release/VST3/`

Update paths in `build-installer.yml` if different.

### WiX Version

Current workflow uses WiX v3.14. If using v3.11 or v4.0, update paths:
- v3.11: `C:\Program Files (x86)\WiX Toolset v3.11\bin\`
- v4.0: Different namespace and syntax required

### Permission Issues

- Installer requires admin rights (by design)
- MSI creation may require admin in PowerShell

### File Not Found in MSI

Verify file paths in WiX source are absolute and correct:
```powershell
Test-Path "path/to/file"
```

## Security Considerations

1. **Code Signing**: Consider adding digital signatures to MSI:
   ```powershell
   signtool sign /f cert.pfx /p password WaviateScript-Installer.msi
   ```

2. **License Agreement**: Customize `LICENSE.rtf` in workflow step

3. **Registry Protection**: Installer writes to `HKCU\Software\Waviate\WaviateScript`

## Release/Version Management

### Creating a Release

```bash
# Tag a commit
git tag v1.0.0
git push origin v1.0.0
```

Workflow automatically:
1. Builds the installer
2. Creates GitHub Release
3. Attaches MSI file

### Version Format

Use semantic versioning: `vMAJOR.MINOR.PATCH`
- `v1.0.0` - Initial release
- `v1.1.0` - Feature update
- `v1.0.1` - Bug fix

