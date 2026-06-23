# Installer Quick Reference

## For Developers

### Quick Commands

```powershell
# Build installer locally (Release mode)
.\Build-Installer.ps1

# Build installer with Debug configuration
.\Build-Installer.ps1 -Configuration Debug

# Recreate installer from existing builds
.\Build-Installer.ps1 -OnlyInstaller

# Test installer (interactive)
msiexec /i WaviateScript-Installer.msi

# Test installer (quiet mode)
msiexec /i WaviateScript-Installer.msi /quiet

# Test installer with detailed logging
msiexec /i WaviateScript-Installer.msi /l*v install.log

# Uninstall silently
msiexec /x "{PRODUCT_CODE}" /quiet
```

## For Release Managers

### Create a Release

```bash
# Tag a commit
git tag v1.1.0
git push origin v1.1.0

# GitHub Actions automatically:
# 1. Builds Release configuration
# 2. Creates MSI installer
# 3. Publishes GitHub Release with MSI attached
```

### Version Update Checklist

- [ ] Update `Version="X.Y.Z.0"` in `WaviateScript.wxs`
- [ ] Update any build paths if they've changed
- [ ] Test locally: `.\Build-Installer.ps1`
- [ ] Commit and tag: `git tag vX.Y.Z`
- [ ] Push tag triggers automated release

## Installation Locations

| Item | Path | Notes |
|------|------|-------|
| Executable | `Program Files\Waviate\Script\bin\WaviateScript.exe` | Standalone |
| VST3 | `Program Files\Common Files\VST3\WaviateScript.vst3` | All DAWs |
| Headers | `Program Files\Waviate\Script\include\` | Dev headers |
| Start Menu | `Start Menu → Waviate Script` | Shortcuts |

## Key Files

| File | Purpose |
|------|---------|
| `WaviateScript.wxs` | Installer definition |
| `.github/workflows/build-installer.yml` | CI/CD automation |
| `Build-Installer.ps1` | Local build script |
| `INSTALLER_GUIDE.md` | Detailed user guide |
| `INSTALLER_SETUP.md` | Technical documentation |

## Workflow Status

Check GitHub Actions:
- Push to `main` → Builds automatically (artifact only)
- Tag commit `v*` → Creates Release with MSI
- Pull request → Builds to verify (artifact only)

## Troubleshooting

| Issue | Solution |
|-------|----------|
| MSBuild not found | Install Visual Studio with C++ workload |
| WiX not found | `choco install wixtoolset -y` |
| Build artifacts missing | Verify path matches VS version |
| MSI won't install | Check Event Viewer, run with `/l*v` logging |
| VST3 not in DAW | Restart DAW, rescan plugin folder |

## Important Notes

⚠️ **UpgradeCode in WaviateScript.wxs**
- Must be unique UUID per product
- DO NOT change without reason
- Used to identify product for upgrades

⚠️ **Version Format**
- Must be X.Y.Z.0 (four parts, last is 0)
- Increment for each release
- Controls upgrade behavior

⚠️ **Administrator Rights**
- Installation requires admin privileges
- MSI extracts to `%ProgramFiles%`
- Per-machine scope by design

## CI/CD Pipeline

```
Push/PR/Tag
    ↓
GitHub Actions
    ├─ MSBuild (Release|x64)
    ├─ Verify outputs
    ├─ Prepare files
    ├─ WiX compile (candle)
    ├─ WiX link (light)
    ├─ Upload artifact
    └─ Create Release (if tagged)
```

## Artifact Access

1. Go to GitHub Actions
2. Select "Build Installer" workflow
3. Click latest successful run
4. Download `waviate-script-installer-x64`
5. Extract and run the MSI

## Release Artifacts

When pushing a version tag:
- Automatic workflow activation
- ~5-10 minute build time
- MSI published to GitHub Releases
- 30-day artifact retention
- Can download from Releases page

## Signature/Verification

Current build: **Unsigned**

To add signing in future:
```powershell
signtool sign /f cert.pfx /p password /t http://timestamp.server WaviateScript-Installer.msi
```

## Support & Issues

- **Build issues**: Check GitHub Actions logs
- **Installation fails**: Enable logging with `/l*v`
- **Plugin issues**: Verify VST3 path and DAW compatibility
- **Questions**: See `INSTALLER_SETUP.md` troubleshooting

