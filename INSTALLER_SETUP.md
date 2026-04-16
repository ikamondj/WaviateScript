# Windows Installer Setup Documentation

## Quick Start

### Automated Build (GitHub Actions)

Every push to `main` and pull requests trigger an automatic installer build. Installers are available in the Actions artifacts.

To create a release:
```bash
git tag v1.0.0
git push origin v1.0.0
```

The workflow will automatically:
1. Build the Release configuration
2. Create the MSI installer
3. Publish to GitHub Releases

### Local Build

```powershell
# Ensure WiX Toolset is installed
choco install wixtoolset -y

# Run the build script
.\Build-Installer.ps1
```

## Installation Files Overview

### WaviateScript.wxs
**WiX Source File** - Defines the installer structure, layout, and components.

**Key Sections:**
- **Product**: Version 1.0.0.0, x64 platform, per-machine installation
- **Directories**: Installation paths for standalone, VST3, headers
- **Components**: Executable, plugin, headers, shortcuts
- **Features**: Allows selective component installation

**Customization Points:**
- Change `Version="1.0.0.0"` for version updates
- Update `UpgradeCode` if creating a new product variant
- Modify directory names to change installation paths
- Add/remove components for different features

### build-installer.yml (GitHub Actions Workflow)
**Automated Installer Builder** - Triggers on push/PR/tags, builds and publishes installers.

**Process:**
1. Checks out code with submodules
2. Installs MSBuild and WiX Toolset
3. Builds solution in Release configuration
4. Verifies build outputs exist
5. Prepares installer files
6. Compiles WiX source to object file
7. Links into MSI package
8. Uploads artifact (30-day retention)
9. Creates GitHub Release if tagged

**Triggers:**
- `push` to main branch
- `pull_request` to main branch  
- Git tags matching `v*`
- Manual workflow dispatch

### Build-Installer.ps1 (Local Build Script)
**PowerShell Helper** - Builds installer locally for testing and development.

**Usage:**
```powershell
# Standard Release build
.\Build-Installer.ps1

# Debug build
.\Build-Installer.ps1 -Configuration Debug

# Only create installer from existing artifacts
.\Build-Installer.ps1 -OnlyInstaller
```

**Features:**
- Prerequisite checking
- Detailed progress reporting
- Error handling with diagnostics
- Automatic WiX toolset detection
- Clean temporary files on completion

## Installation Details

### Default Locations

| Component | Location | Purpose |
|-----------|----------|---------|
| Standalone EXE | `Program Files\Waviate\Script\bin\` | Main application |
| VST3 Plugin | `Program Files\Common Files\VST3\` | DAW integration |
| Headers | `Program Files\Waviate\Script\include\` | Development |
| Shortcuts | Start Menu & Desktop | Quick access |

### Installation Features

**Installer Features:**
- Standard Windows MSI format
- x64 architecture only
- Per-machine installation (admin required)
- Automatic file overwriting on updates
- One-click uninstall
- Desktop and Start Menu shortcuts
- Registry entries for tracking

### Registry Entries

The installer creates registry entries for:
- Installation tracking: `HKCU\Software\Waviate\WaviateScript`
- Standalone path: `HKLM\Software\Waviate\WaviateScript\Standalone`
- VST3 path: `HKLM\Software\Waviate\WaviateScript\VST3`
- Include path: `HKLM\Software\Waviate\WaviateScript\Include`

## Versioning & Release Management

### Version Format

Use semantic versioning with tags:
- `v1.0.0` - Major release (breaking changes)
- `v1.1.0` - Minor release (new features)
- `v1.0.1` - Patch release (bug fixes)

### Release Process

1. **Update version in WaviateScript.wxs:**
   ```xml
   <Product ... Version="1.1.0.0" ...>
   ```

2. **Commit changes:**
   ```bash
   git add -A
   git commit -m "Release v1.1.0"
   ```

3. **Create tag:**
   ```bash
   git tag v1.1.0
   git push origin main
   git push origin v1.1.0
   ```

4. **GitHub Release:**
   - Workflow automatically creates release
   - MSI attached automatically
   - Edit release for release notes

### Version Update Behavior

- **New Installation**: Install to default location
- **Existing older version**: Automatically upgraded
- **Existing same version**: Repair installation offered
- **Existing newer version**: Installation blocked with error message

## Build Output Details

### Build Process Flow

```
MSBuild (Release|x64)
  ├─ WaviateScript_SharedCode → obj files
  ├─ WaviateScript_StandalonePlugin → WaviateScript.exe
  ├─ WaviateScript_VST3 → WaviateScript.vst3 (folder bundle)
  └─ WaviateScript_VST3ManifestHelper → manifest
```

### Expected Outputs

**Standalone Plugin Directory:**
```
Builds/VisualStudio2026/x64/Release/Standalone Plugin/
  └─ WaviateScript.exe
```

**VST3 Plugin Directory:**
```
Builds/VisualStudio2026/x64/Release/VST3/
  └─ WaviateScript.vst3/                    # Folder bundle
     ├─ Contents/
     │  ├─ Info.plist
     │  └─ x86_64-win/
     │     └─ WaviateScript.vst3.dll
     └─ (other VST3 structure)
```

## Troubleshooting

### Build Failures

**"MSBuild not found"**
- Install Visual Studio 2022+ with C++ workload
- Add to PATH: `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin`

**"WiX Toolset not found"**
```powershell
choco install wixtoolset -y --force
[Environment]::SetEnvironmentVariable("PATH", $env:PATH + ";C:\Program Files (x86)\WiX Toolset v3.14\bin", "User")
```

**BUILD FILES NOT FOUND**
1. Verify Release build completed successfully
2. Check paths match your VS version (e.g., VS2026 vs VS2022)
3. Run full clean rebuild:
   ```powershell
   msbuild ... /p:Configuration=Release /t:Clean,Build
   ```

### Installer Creation Issues

**"candle.exe failed"**
- Verify WiX source file syntax: `candle.exe WaviateScript.wxs`
- Check file paths in WiX use forward slashes or are escaped

**"light.exe failed"**
- Check for missing component GUIDs (must be unique)
- Verify all referenced files exist

**MSI Won't Install**
- Check Event Viewer for detailed error
- Run: `msiexec /i installer.msi /l*v install.log`
- Review installation requirements (admin, x64)

### Runtime Issues

**Standalone won't launch**
- Verify dependencies installed (LLVM, FFTW)
- Run from `Program Files\Waviate\Script\bin\`

**VST3 plugin not detected**
- Verify installed to: `Program Files\Common Files\VST3\WaviateScript.vst3`
- Rescan DAW plugin list
- Restart DAW

## Advanced Configuration

### Custom Installation Paths

Edit `WaviateScript.wxs`:

```xml
<Directory Id="INSTALLFOLDER" Name="Script">
  <!-- Change 'Script' to custom folder name -->
</Directory>
```

### Add Registry Keys

Insert into component:
```xml
<RegistryValue Root="HKLM" Key="Software\Waviate\MyKey" 
               Name="ValueName" Type="string" Value="ValueData"/>
```

### Digital Code Signing

Sign the MSI for authentic distribution:
```powershell
# Install SignTool (part of Windows SDK)
signtool sign /f certificate.pfx /p password /t http://timestamp.server WaviateScript-Installer.msi
```

### Custom UI

Replace default UI:
```xml
<!-- Replace: -->
<UIRef Id="WixUI_InstallDir"/>

<!-- With custom WXS file: -->
<DialogRef Id="MyCustomDialog"/>
```

## DAW VST3 Compatibility

The VST3 plugin is installed to the standard Windows location and compatible with:

| DAW | Version | Notes |
|-----|---------|-------|
| Studio One | 6.0+ | Native VST3 |
| Ableton Live | 12+ | Native VST3 |
| Reaper | 6.82+ | Native VST3 with wrapper |
| FL Studio | 21.8+ | Native VST3 |
| Cubase | 12+ | Native VST3 |
| Nuendo | 13+ | Native VST3 |
| Logic Pro | 10.7+ | VST3 via wrapper |
| ProTools | 2024+ | VST3 (limited) |

## Best Practices Used

✓ **Per-Machine Installation** - Centralized, admin-controlled
✓ **Standard Paths** - Uses Windows recommended locations
✓ **Automatic Overwrite** - Seamless upgrades without cleanup
✓ **Registry Integration** - Proper Windows registry usage
✓ **Embedded CAB** - Single-file distribution
✓ **x64 Target** - Modern 64-bit architecture
✓ **MajorUpgrade** - Version management built-in
✓ **Error Handling** - Prevents downgrade scenarios
✓ **Uninstall Support** - Clean removal through Add/Remove Programs

## Files Modified

- ✓ `WaviateScript.wxs` - Created WiX installer configuration
- ✓ `.github/workflows/build-installer.yml` - Created GitHub Actions workflow
- ✓ `Build-Installer.ps1` - Created local build helper script
- ✓ `.gitignore` - Added installer build artifacts
- ✓ `INSTALLER_GUIDE.md` - Created user-friendly guide

## Support

For issues or questions:
1. Check troubleshooting section above
2. Review WiX documentation: https://wixtoolset.github.io/
3. Check workflow logs in GitHub Actions
4. Enable MSI logging: `msiexec /i file.msi /l*v log.txt`

