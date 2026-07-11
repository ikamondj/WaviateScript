#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Configure and build WaviateScript with CMake on Windows.

.DESCRIPTION
    Presents a menu for these build modes:
      0 = Release
      1 = Release Premium
      2 = Debug
      3 = Debug Premium

    Public and premium builds use separate build directories because
    WAVIATESCRIPT_PREMIUM is a CMake configure-time option, not a build config.
    Debug and release also use separate build directories because they can point
    at different LLVM/Clang package trees.

.PARAMETER Choice
    Optional build choice. When omitted, the script prompts interactively.

.PARAMETER Target
    CMake target to build. Defaults to WaviateScript_Standalone.
#>
param(
    [ValidateSet("0", "1", "2", "3")]
    [string]$Choice,

    [string]$Target = "WaviateScript_Standalone"
)

$ErrorActionPreference = "Stop"

function Get-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Repair-PathValue {
    param([string]$PathValue)

    if ([string]::IsNullOrWhiteSpace($PathValue)) {
        return $PathValue
    }

    $seen = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    $cleanedEntries = New-Object System.Collections.Generic.List[string]

    foreach ($rawEntry in ($PathValue -split ';')) {
        $entry = $rawEntry.Trim()
        if ([string]::IsNullOrWhiteSpace($entry)) {
            continue
        }

        if ($entry -match '%[^%]+%') {
            continue
        }

        if ($seen.Add($entry)) {
            [void]$cleanedEntries.Add($entry)
        }
    }

    return ($cleanedEntries -join ';')
}

function Repair-BuildEnvironment {
    $originalPath = $env:PATH
    $cleanPath = Repair-PathValue -PathValue $originalPath

    if ($cleanPath -ne $originalPath) {
        $env:PATH = $cleanPath
        Write-Host "Sanitized PATH for this build session."
        Write-Host "  Original length: $($originalPath.Length)"
        Write-Host "  Cleaned length:  $($cleanPath.Length)"
    }

    if ($env:CMAKE_GENERATOR_PLATFORM -and -not $env:CMAKE_GENERATOR) {
        Remove-Item Env:CMAKE_GENERATOR_PLATFORM
        Write-Host "Removed orphaned CMAKE_GENERATOR_PLATFORM from the environment."
    }

    if ($env:CMAKE_GENERATOR_TOOLSET -and -not $env:CMAKE_GENERATOR) {
        Remove-Item Env:CMAKE_GENERATOR_TOOLSET
        Write-Host "Removed orphaned CMAKE_GENERATOR_TOOLSET from the environment."
    }
}

function Find-CMake {
    $candidates = @(
        "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $installPath = & $vswhere -latest -products * -format value -property installationPath | Select-Object -First 1
        if ($installPath) {
            $cmakePath = Join-Path $installPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
            if (Test-Path $cmakePath) {
                return $cmakePath
            }
        }
    }

    throw "cmake.exe was not found in the Visual Studio CMake install. Install Visual Studio C++ CMake tools."
}

function Get-MenuChoice {
    param([string]$ExistingChoice)

    if ($ExistingChoice) {
        return $ExistingChoice
    }

    Write-Host "Select build mode:"
    Write-Host "  0 - Release"
    Write-Host "  1 - Release Premium"
    Write-Host "  2 - Debug"
    Write-Host "  3 - Debug Premium"

    $selected = Read-Host "Enter 0, 1, 2, or 3"
    if ($selected -notin @("0", "1", "2", "3")) {
        throw "Invalid selection '$selected'. Expected 0, 1, 2, or 3."
    }

    return $selected
}

function Get-BuildSettings {
    param(
        [string]$SelectedChoice,
        [string]$RepoRoot
    )

    $publicBuildDir = Join-Path $RepoRoot "build-cmake-vs"
    $premiumBuildDir = Join-Path $RepoRoot "build-cmake-vs-premium"
    $debugPublicBuildDir = Join-Path $RepoRoot "build-cmake-vs-debug"
    $debugPremiumBuildDir = Join-Path $RepoRoot "build-cmake-vs-debug-premium"
    $premiumSourceDir = Join-Path $RepoRoot "Source\WSPremium"

    switch ($SelectedChoice) {
        "0" {
            return @{
                Label = "Release"
                BuildDir = $publicBuildDir
                Configuration = "Release"
                Premium = $false
            }
        }
        "1" {
            return @{
                Label = "Release Premium"
                BuildDir = $premiumBuildDir
                Configuration = "Release"
                Premium = $true
                PremiumSourceDir = $premiumSourceDir
            }
        }
        "2" {
            return @{
                Label = "Debug"
                BuildDir = $debugPublicBuildDir
                Configuration = "Debug"
                Premium = $false
            }
        }
        "3" {
            return @{
                Label = "Debug Premium"
                BuildDir = $debugPremiumBuildDir
                Configuration = "Debug"
                Premium = $true
                PremiumSourceDir = $premiumSourceDir
            }
        }
    }
}

function Require-Path {
    param(
        [string]$Path,
        [string]$Message
    )

    if (-not (Test-Path $Path)) {
        throw $Message
    }
}

function Get-FirstExistingPath {
    param([string[]]$Candidates)

    foreach ($candidate in $Candidates) {
        if (-not [string]::IsNullOrWhiteSpace($candidate) -and (Test-Path $candidate)) {
            return $candidate
        }
    }

    return $null
}

$repoRoot = Get-RepoRoot
$cmake = Find-CMake
$selectedChoice = Get-MenuChoice -ExistingChoice $Choice
$settings = Get-BuildSettings -SelectedChoice $selectedChoice -RepoRoot $repoRoot

Repair-BuildEnvironment

$juceDir = "C:/Users/ikamo/OneDrive/Documents/JuceInstalls/JUCE"

if ($settings.Configuration -eq "Debug") {
    $llvmDir = Get-FirstExistingPath @(
        "C:/src/llvm-dbg/install/lib/cmake/llvm",
        "C:/src/llvm-dbg/build/lib/cmake/llvm"
    )
    $clangDir = Get-FirstExistingPath @(
        "C:/src/llvm-dbg/install/lib/cmake/clang",
        "C:/src/llvm-dbg/build/lib/cmake/clang"
    )
} else {
    $llvmDir = Get-FirstExistingPath @(
        "C:/Program Files/LLVM/lib/cmake/llvm"
    )
    $clangDir = Get-FirstExistingPath @(
        "C:/Program Files/LLVM/lib/cmake/clang"
    )
}

Require-Path -Path $juceDir -Message "JUCE checkout not found at '$juceDir'."
Require-Path -Path $llvmDir -Message "LLVM CMake package not found at '$llvmDir'."
Require-Path -Path $clangDir -Message "Clang CMake package not found at '$clangDir'."

$configureArgs = @(
    "-S", $repoRoot,
    "-B", $settings.BuildDir,
    "-G", "Visual Studio 18 2026",
    "-A", "x64",
    "-DWAVIATESCRIPT_JUCE_DIR=$juceDir",
    "-DLLVM_DIR=$llvmDir",
    "-DClang_DIR=$clangDir"
)

if ($settings.Premium) {
    Require-Path -Path $settings.PremiumSourceDir -Message "Premium source was not found at '$($settings.PremiumSourceDir)'."
    $configureArgs += @(
        "-DWAVIATESCRIPT_PREMIUM=ON",
        "-DWAVIATESCRIPT_PREMIUM_SOURCE_DIR=$($settings.PremiumSourceDir)"
    )
} else {
    $configureArgs += "-DWAVIATESCRIPT_PREMIUM=OFF"
}

$buildArgs = @(
    "--build", $settings.BuildDir,
    "--config", $settings.Configuration,
    "--target", $Target
)

Write-Host "Build mode: $($settings.Label)"
Write-Host "Target:     $Target"
Write-Host "Build dir:  $($settings.BuildDir)"
Write-Host ""

& $cmake @configureArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& $cmake @buildArgs
exit $LASTEXITCODE
