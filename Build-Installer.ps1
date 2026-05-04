#!/usr/bin/env powershell
<#
.SYNOPSIS
    Build WaviateScript installer locally
.DESCRIPTION
    This script builds the WaviateScript solution and creates an MSI installer.
    Requires: Visual Studio with C++ workload, WiX Toolset, .NET Build Tools
.PARAMETER Configuration
    Build configuration: Debug or Release (default: Release)
.PARAMETER OnlyInstaller
    Skip build, only create installer from existing artifacts (default: $false)
.EXAMPLE
    .\Build-Installer.ps1
    .\Build-Installer.ps1 -Configuration Release
    .\Build-Installer.ps1 -OnlyInstaller
#>
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    
    [switch]$OnlyInstaller = $false
)

$ErrorActionPreference = 'Stop'
$VerbosePreference = 'Continue'

# Constants
$SOLUTION_PATH = "Builds\VisualStudio2026\WaviateScript.sln"
$WXS_SOURCE = "WaviateScript.wxs"
$INSTALLER_DIR = "installer_build"
$OUTPUT_MSI = "WaviateScript-Installer.msi"
$MSBUILD_PATH = $null

function Resolve-MSBuildPath {
    $msbuild = Get-Command msbuild -ErrorAction SilentlyContinue
    if ($msbuild) {
        return $msbuild.Source
    }

    $programFiles = [Environment]::GetFolderPath('ProgramFiles')
    $programFilesX86 = [Environment]::GetFolderPath('ProgramFilesX86')
    $vswhere = Join-Path $programFilesX86 "Microsoft Visual Studio\Installer\vswhere.exe"

    if (Test-Path $vswhere) {
        $latest = & $vswhere -latest -requires Microsoft.Component.MSBuild -find "MSBuild\Current\Bin\amd64\MSBuild.exe" |
            Select-Object -First 1
        if ($latest -and (Test-Path $latest)) {
            return $latest
        }
    }

    $candidateRoots = @(
        (Join-Path $programFiles "Microsoft Visual Studio"),
        (Join-Path $programFilesX86 "Microsoft Visual Studio")
    ) | Where-Object { Test-Path $_ }

    foreach ($root in $candidateRoots) {
        $candidate = Get-ChildItem -Path $root -Recurse -Filter MSBuild.exe -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -like "*\MSBuild\Current\Bin\amd64\MSBuild.exe" } |
            Sort-Object FullName -Descending |
            Select-Object -First 1

        if ($candidate) {
            return $candidate.FullName
        }
    }

    return $null
}

function Test-Prerequisites {
    Write-Verbose "Checking prerequisites..."
    
    # Check MSBuild
    $script:MSBUILD_PATH = Resolve-MSBuildPath
    if (-not $script:MSBUILD_PATH) {
        Write-Error "MSBuild not found. Please install Visual Studio with C++ workload."
        return $false
    }
    Write-Verbose "MSBuild found: $script:MSBUILD_PATH"
    
    # Check WiX (optional if only building without installer)
    if (-not $OnlyInstaller) {
        $candle = Get-Command candle.exe -ErrorAction SilentlyContinue
        if (-not $candle) {
            Write-Warning "candle.exe (WiX Toolset) not found in PATH"
            Write-Warning "Trying default WiX 3.14 installation path..."
            $wixPath = "C:\Program Files (x86)\WiX Toolset v3.14\bin"
            if (-not (Test-Path $wixPath)) {
                Write-Error "WiX Toolset not found. Install with: choco install wixtoolset -y"
                return $false
            }
            $env:PATH = "$wixPath;$env:PATH"
        }
    }
    
    Write-Verbose "All prerequisites met"
    return $true
}

function Build-Solution {
    if ($OnlyInstaller) {
        Write-Verbose "Skipping solution build (OnlyInstaller mode)"
        return $true
    }
    
    Write-Host "Building solution in $Configuration configuration..."
    
    if (-not (Test-Path $SOLUTION_PATH)) {
        Write-Error "Solution not found at: $SOLUTION_PATH"
        return $false
    }
    
    try {
        # The generated VS2026 solution can fail when MSBuild schedules its dependency projects in parallel.
        & $script:MSBUILD_PATH $SOLUTION_PATH `
            /p:Configuration=$Configuration `
            /p:Platform=x64 `
            /m:1 `
            /nologo
        
        if ($LASTEXITCODE -ne 0) {
            Write-Error "MSBuild failed with exit code $LASTEXITCODE"
            return $false
        }
        
        Write-Verbose "Solution built successfully"
        return $true
    }
    catch {
        Write-Error "Build error: $_"
        return $false
    }
}

function Verify-BuildArtifacts {
    $standaloneExe = "Builds\VisualStudio2026\x64\$Configuration\Standalone Plugin\WaviateScript.exe"
    $vst3Plugin = "Builds\VisualStudio2026\x64\$Configuration\VST3\WaviateScript.vst3"
    $waviateH = "Source\Waviate.h"
    $waviateHpp = "Source\Waviate.hpp"
    
    Write-Verbose "Verifying build artifacts..."
    
    $missingFiles = @()
    
    if (-not (Test-Path $standaloneExe)) {
        $missingFiles += $standaloneExe
    }
    if (-not (Test-Path $vst3Plugin)) {
        $missingFiles += $vst3Plugin
    }
    if (-not (Test-Path $waviateH)) {
        $missingFiles += $waviateH
    }
    if (-not (Test-Path $waviateHpp)) {
        $missingFiles += $waviateHpp
    }
    
    if ($missingFiles.Count -gt 0) {
        Write-Error "Missing build artifacts:`n$($missingFiles -join "`n")"
        return $false
    }
    
    Write-Verbose "OK: All build artifacts found"
    return $true
}

function Prepare-InstallerDirectory {
    Write-Verbose "Preparing installer directory..."
    
    if (Test-Path $INSTALLER_DIR) {
        Remove-Item -Recurse -Force $INSTALLER_DIR | Out-Null
    }
    New-Item -ItemType Directory -Path $INSTALLER_DIR | Out-Null
    
    $standaloneSrc = "Builds\VisualStudio2026\x64\$Configuration\Standalone Plugin\WaviateScript.exe"
    $vst3Src = "Builds\VisualStudio2026\x64\$Configuration\VST3\WaviateScript.vst3"
    
    Copy-Item $standaloneSrc "$INSTALLER_DIR\WaviateScript_Standalone.exe"
    
    # Handle VST3 plugin (may be a folder or single file)
    if ((Get-Item $vst3Src) -is [System.IO.DirectoryInfo]) {
        # Copy entire VST3 folder
        Copy-Item -Recurse $vst3Src "$INSTALLER_DIR\WaviateScript.vst3"
    } else {
        # Copy single VST3 file
        Copy-Item $vst3Src "$INSTALLER_DIR\WaviateScript.vst3"
    }
    
    Copy-Item "Source\Waviate.h" "$INSTALLER_DIR\Waviate.h"
    Copy-Item "Source\Waviate.hpp" "$INSTALLER_DIR\Waviate.hpp"
    
    Write-Verbose "OK: Files copied to installer directory"
}

function Create-LicenseFile {
    Write-Verbose "Creating license file..."
    
    $rtfContent = @"
{\rtf1\ansi\ansicpg1252\deff0\nouicompat
{\fonttbl{\f0\fnil\fcharset0 Calibri;}}
{\*\generator Riched20 10.0.19041}
\viewkind4\uc1 
\pard\f0\fs20\par
Waviate Script License\par
\par
This software is provided AS-IS.\par
\par
}
"@
    
    Set-Content -Path "$INSTALLER_DIR\LICENSE.rtf" -Value $rtfContent
    Write-Verbose "OK: License file created"
}

function Update-WixSourceFile {
    Write-Verbose "Updating WiX source file with correct paths..."
    
    if (-not (Test-Path $WXS_SOURCE)) {
        Write-Error "WiX source file not found: $WXS_SOURCE"
        return $false
    }
    
    $wxsContent = Get-Content $WXS_SOURCE
    $installerPath = (Get-Location).Path
    
    # Update paths with absolute paths
    $wxsContent = $wxsContent -replace 'SourceWaviateScript_Standalone\.exe', `
        "$installerPath\$INSTALLER_DIR\WaviateScript_Standalone.exe"
    $wxsContent = $wxsContent -replace 'SourceWaviateScript\.vst3', `
        "$installerPath\$INSTALLER_DIR\WaviateScript.vst3"
    $wxsContent = $wxsContent -replace 'SourceWaviate\.h', `
        "$installerPath\$INSTALLER_DIR\Waviate.h"
    $wxsContent = $wxsContent -replace 'SourceWaviate\.hpp', `
        "$installerPath\$INSTALLER_DIR\Waviate.hpp"
    $wxsContent = $wxsContent -replace 'LICENSE\.rtf', `
        "$installerPath\$INSTALLER_DIR\LICENSE.rtf"
    
    $wxsContent | Set-Content "$env:TEMP\WaviateScript.wxs"
    Write-Verbose "OK: WiX source file updated"
    return $true
}

function Build-Installer {
    Write-Host "Building installer..."
    
    $wxsPath = "$env:TEMP\WaviateScript.wxs"
    $wixobjPath = "WaviateScript.wixobj"
    
    try {
        # Compile
        Write-Verbose "Compiling WiX source (candle)..."
        & candle.exe -o $wixobjPath $wxsPath
        
        if ($LASTEXITCODE -ne 0) {
            Write-Error "candle.exe failed with exit code $LASTEXITCODE"
            return $false
        }
        
        # Link
        Write-Verbose "Linking WiX object (light)..."
        & light.exe -o $OUTPUT_MSI $wixobjPath
        
        if ($LASTEXITCODE -ne 0) {
            Write-Error "light.exe failed with exit code $LASTEXITCODE"
            return $false
        }
        
        if (-not (Test-Path $OUTPUT_MSI)) {
            Write-Error "MSI file was not created"
            return $false
        }
        
        $msiSize = (Get-Item $OUTPUT_MSI).Length / 1MB
        Write-Host "OK: Installer created: $OUTPUT_MSI ($($msiSize.ToString('F2')) MB)"
        return $true
    }
    catch {
        Write-Error "Installer build error: $_"
        return $false
    }
}

function Main {
    Write-Host "=== WaviateScript Installer Builder ===" -ForegroundColor Cyan
    Write-Host "Configuration: $Configuration"
    Write-Host ""
    
    # Test prerequisites
    if (-not (Test-Prerequisites)) {
        exit 1
    }
    
    # Build solution
    if (-not (Build-Solution)) {
        exit 1
    }
    
    # Verify artifacts
    if (-not (Verify-BuildArtifacts)) {
        exit 1
    }
    
    # Prepare installer
    Prepare-InstallerDirectory
    Create-LicenseFile
    
    if (-not (Update-WixSourceFile)) {
        exit 1
    }
    
    # Build installer
    if (-not (Build-Installer)) {
        exit 1
    }
    
    Write-Host ""
    Write-Host "=== Build Complete ===" -ForegroundColor Green
    Write-Host "Installer: $OUTPUT_MSI"
    Write-Host ""
    Write-Host "To test installation:"
    Write-Host "  msiexec /i $OUTPUT_MSI"
    Write-Host ""
    Write-Host "To test silently or with logging:"
    Write-Host "  msiexec /i $OUTPUT_MSI /quiet"
    Write-Host "  msiexec /i $OUTPUT_MSI /l*v install.log"
}

# Run main
Main
