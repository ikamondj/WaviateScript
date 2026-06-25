[CmdletBinding()]
param(
    [string]$BuildDir = "",
    [string]$Config = "Release",
    [string]$LLVM_DIR = "",
    [string]$Clang_DIR = "",
    [string]$JuceDir = "",
    [string]$Generator = ""
)

$ErrorActionPreference = "Stop"

$BackendRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$NativeDir = Join-Path $BackendRoot "native"

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $NativeDir "build"
} else {
    $BuildDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($BuildDir)
}

$configureArgs = @(
    "-S", $NativeDir,
    "-B", $BuildDir,
    "-DCMAKE_BUILD_TYPE=$Config"
)

if (-not [string]::IsNullOrWhiteSpace($Generator)) {
    $configureArgs = @("-G", $Generator) + $configureArgs
}

if (-not [string]::IsNullOrWhiteSpace($LLVM_DIR)) {
    $LLVM_DIR = (Resolve-Path $LLVM_DIR).Path
    $configureArgs += "-DLLVM_DIR=$LLVM_DIR"
}

if (-not [string]::IsNullOrWhiteSpace($Clang_DIR)) {
    $Clang_DIR = (Resolve-Path $Clang_DIR).Path
    $configureArgs += "-DClang_DIR=$Clang_DIR"
}

if (-not [string]::IsNullOrWhiteSpace($JuceDir)) {
    $JuceDir = (Resolve-Path $JuceDir).Path
    $configureArgs += "-DWAVIATESCRIPT_JUCE_DIR=$JuceDir"
}

cmake @configureArgs
cmake --build $BuildDir --config $Config --target waviate_marketplace_compiler

if ($env:OS -eq "Windows_NT") {
    cmake --build $BuildDir --config $Config --target waviate_marketplace_compiler_runtime
}

Write-Host "Native compiler static library built under $BuildDir/lib."
