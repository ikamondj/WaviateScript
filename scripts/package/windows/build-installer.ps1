#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Package CMake-built WaviateScript artifacts into a Windows MSI.

.DESCRIPTION
    Expects an existing CMake Release build. The script does not rebuild the
    plugin. It packages the Standalone executable into Program Files and the
    VST3 bundle into Program Files\Common Files\VST3.
#>
param(
    [string]$BuildDir = "build",
    [string]$OutputDir = "dist",
    [string]$Configuration = "Release",
    [string]$Version = "1.0.0",
    [ValidateSet("Public", "Premium")]
    [string]$Edition = "Public"
)

$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
$BuildRoot = if ([System.IO.Path]::IsPathRooted($BuildDir)) { $BuildDir } else { Join-Path $RepoRoot $BuildDir }
$DistRoot = if ([System.IO.Path]::IsPathRooted($OutputDir)) { $OutputDir } else { Join-Path $RepoRoot $OutputDir }

$ProductName = if ($Edition -eq "Premium") { "WaviateScript Premium" } else { "WaviateScript" }
$PackageStem = if ($Edition -eq "Premium") { "WaviateScript-Premium" } else { "WaviateScript" }
$Manufacturer = "yourcompany"
$BundleName = "WaviateScript.vst3"
$StandaloneName = "WaviateScript.exe"
$UpgradeCode = "7E22EA1C-9171-4E86-BC88-5B13017274A1"
$PackageVersion = $Version

function ConvertTo-WixId {
    param([string]$Value)
    $id = [regex]::Replace($Value, "[^A-Za-z0-9_\.]", "_")
    if ($id.Length -eq 0 -or $id[0] -notmatch "[A-Za-z_]") {
        $id = "I_$id"
    }
    return $id
}

function New-StableGuid {
    param([string]$Seed)
    $md5 = [System.Security.Cryptography.MD5]::Create()
    $bytes = $md5.ComputeHash([System.Text.Encoding]::UTF8.GetBytes($Seed))
    $bytes[6] = ($bytes[6] -band 0x0f) -bor 0x30
    $bytes[8] = ($bytes[8] -band 0x3f) -bor 0x80
    return ([guid]::new($bytes)).ToString().ToUpperInvariant()
}

function Escape-Xml {
    param([string]$Value)
    return [System.Security.SecurityElement]::Escape($Value)
}

function Find-FirstExistingPath {
    param([string[]]$Candidates)
    foreach ($candidate in $Candidates) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }
    return $null
}

function Require-Path {
    param([string]$Path, [string]$Message)
    if (-not $Path -or -not (Test-Path $Path)) {
        Write-Error $Message
    }
}

New-Item -ItemType Directory -Force -Path $DistRoot | Out-Null

$StandalonePath = Find-FirstExistingPath @(
    (Join-Path $BuildRoot "WaviateScript_artefacts\$Configuration\Standalone\$StandaloneName"),
    (Join-Path $BuildRoot "WaviateScript_artefacts\Standalone\$StandaloneName"),
    (Join-Path $BuildRoot "WaviateScript_artefacts\$Configuration\Standalone Plugin\$StandaloneName"),
    (Join-Path $BuildRoot "WaviateScript_artefacts\Standalone Plugin\$StandaloneName")
)

$Vst3Path = Find-FirstExistingPath @(
    (Join-Path $BuildRoot "WaviateScript_artefacts\$Configuration\VST3\$BundleName"),
    (Join-Path $BuildRoot "WaviateScript_artefacts\VST3\$BundleName"),
    (Join-Path $BuildRoot "$Configuration\VST3\$BundleName"),
    (Join-Path $BuildRoot "VST3\$BundleName")
)

Require-Path $StandalonePath "Standalone executable not found under '$BuildRoot'. Build with CMake first."
Require-Path $Vst3Path "VST3 bundle not found under '$BuildRoot'. Build with CMake first."

$Wix = Get-Command wix -ErrorAction SilentlyContinue
if (-not $Wix) {
    Write-Error "WiX CLI 'wix' was not found. Install with: dotnet tool install --global wix"
}

$WorkDir = Join-Path $DistRoot "_wix-$PackageStem"
if (Test-Path $WorkDir) {
    Remove-Item -Recurse -Force $WorkDir
}
New-Item -ItemType Directory -Force -Path $WorkDir | Out-Null

$StandaloneComponentGuid = New-StableGuid "$PackageStem/standalone"
$ShortcutComponentGuid = New-StableGuid "$PackageStem/start-menu"

$DirectoryXml = New-Object System.Collections.Generic.List[string]
$ComponentXml = New-Object System.Collections.Generic.List[string]
$ComponentRefs = New-Object System.Collections.Generic.List[string]

function Add-FileComponent {
    param(
        [string]$DirectoryId,
        [string]$SourcePath,
        [string]$ComponentSeed,
        [string]$IdSeed
    )

    $componentId = ConvertTo-WixId "Cmp_$IdSeed"
    $fileId = ConvertTo-WixId "File_$IdSeed"
    $guid = New-StableGuid $ComponentSeed
    $source = Escape-Xml $SourcePath
    $ComponentXml.Add("    <DirectoryRef Id=`"$DirectoryId`">")
    $ComponentXml.Add("      <Component Id=`"$componentId`" Guid=`"$guid`">")
    $ComponentXml.Add("        <File Id=`"$fileId`" Source=`"$source`" KeyPath=`"yes`" />")
    $ComponentXml.Add("      </Component>")
    $ComponentXml.Add("    </DirectoryRef>")
    $ComponentRefs.Add("      <ComponentRef Id=`"$componentId`" />")
}

$Vst3DirectoryTree = New-Object System.Collections.Generic.Dictionary[string,string]
$Vst3RootDirectoryId = "Dir_VST3Bundle"
$Vst3DirectoryTree[""] = $Vst3RootDirectoryId

if ((Get-Item $Vst3Path) -is [System.IO.DirectoryInfo]) {
    $directories = Get-ChildItem -Path $Vst3Path -Directory -Recurse | Sort-Object FullName
    foreach ($dir in $directories) {
        $relative = [System.IO.Path]::GetRelativePath($Vst3Path, $dir.FullName).Replace("\", "/")
        $id = ConvertTo-WixId ("Dir_VST3_" + $relative)
        $Vst3DirectoryTree[$relative] = $id
    }

    function Add-Vst3DirectoryXml {
        param([string]$Relative, [int]$Indent)
        $id = $Vst3DirectoryTree[$Relative]
        $name = if ($Relative -eq "") { $BundleName } else { Split-Path $Relative -Leaf }
        $spaces = " " * $Indent
        $DirectoryXml.Add("$spaces<Directory Id=`"$id`" Name=`"$(Escape-Xml $name)`">")
        $children = $Vst3DirectoryTree.Keys |
            Where-Object {
                if ($Relative -eq "") {
                    $_ -ne "" -and -not $_.Contains("/")
                } else {
                    $_.StartsWith("$Relative/") -and ($_.Substring($Relative.Length + 1) -notmatch "/")
                }
            } |
            Sort-Object
        foreach ($child in $children) {
            Add-Vst3DirectoryXml -Relative $child -Indent ($Indent + 2)
        }
        $DirectoryXml.Add("$spaces</Directory>")
    }

    Add-Vst3DirectoryXml -Relative "" -Indent 12

    $files = Get-ChildItem -Path $Vst3Path -File -Recurse | Sort-Object FullName
    foreach ($file in $files) {
        $relativeFile = [System.IO.Path]::GetRelativePath($Vst3Path, $file.FullName).Replace("\", "/")
        $relativeDir = Split-Path $relativeFile -Parent
        if ($relativeDir -eq ".") { $relativeDir = "" }
        $directoryId = $Vst3DirectoryTree[$relativeDir.Replace("\", "/")]
        Add-FileComponent -DirectoryId $directoryId -SourcePath $file.FullName -ComponentSeed "$PackageStem/vst3/$relativeFile" -IdSeed ("VST3_" + $relativeFile)
    }
} else {
    $DirectoryXml.Add("            <Directory Id=`"$Vst3RootDirectoryId`" Name=`"$BundleName`" />")
    Add-FileComponent -DirectoryId $Vst3RootDirectoryId -SourcePath $Vst3Path -ComponentSeed "$PackageStem/vst3-file" -IdSeed "VST3_File"
}

$StandaloneSource = Escape-Xml $StandalonePath
$WxsPath = Join-Path $WorkDir "$PackageStem.wxs"
$MsiPath = Join-Path $DistRoot "$PackageStem-$Version-windows.msi"

$wxs = @"
<Wix xmlns="http://wixtoolset.org/schemas/v4/wxs">
  <Package Name="$ProductName" Manufacturer="$Manufacturer" Version="$PackageVersion" UpgradeCode="$UpgradeCode" Scope="perMachine">
    <MajorUpgrade DowngradeErrorMessage="A newer version of $ProductName is already installed." />
    <MediaTemplate EmbedCab="yes" />

    <Feature Id="MainFeature" Title="$ProductName" Level="1">
      <ComponentRef Id="StandaloneExecutable" />
      <ComponentRef Id="StartMenuShortcutComponent" />
      <ComponentGroupRef Id="VST3Components" />
    </Feature>

    <StandardDirectory Id="ProgramFiles64Folder">
      <Directory Id="APPLICATIONFOLDER" Name="WaviateScript">
        <Component Id="StandaloneExecutable" Guid="$StandaloneComponentGuid">
          <File Id="StandaloneExe" Source="$StandaloneSource" KeyPath="yes" />
        </Component>
      </Directory>
    </StandardDirectory>

    <StandardDirectory Id="CommonFiles64Folder">
      <Directory Id="CommonVST3Folder" Name="VST3">
$($DirectoryXml -join "`n")
      </Directory>
    </StandardDirectory>

    <StandardDirectory Id="ProgramMenuFolder">
      <Directory Id="ApplicationProgramsFolder" Name="$ProductName">
        <Component Id="StartMenuShortcutComponent" Guid="$ShortcutComponentGuid">
          <Shortcut Id="StartMenuShortcut" Name="$ProductName" Target="[APPLICATIONFOLDER]WaviateScript.exe" WorkingDirectory="APPLICATIONFOLDER" />
          <RemoveFolder Id="ApplicationProgramsFolder" On="uninstall" />
          <RegistryValue Root="HKCU" Key="Software\WaviateScript\$Edition" Name="Installed" Type="integer" Value="1" KeyPath="yes" />
        </Component>
      </Directory>
    </StandardDirectory>
  </Package>

  <Fragment>
$($ComponentXml -join "`n")
  </Fragment>

  <Fragment>
    <ComponentGroup Id="VST3Components">
$($ComponentRefs -join "`n")
    </ComponentGroup>
  </Fragment>
</Wix>
"@

Set-Content -Path $WxsPath -Value $wxs -Encoding UTF8

Write-Host "Packaging $ProductName"
Write-Host "  Standalone: $StandalonePath"
Write-Host "  VST3:       $Vst3Path"
Write-Host "  Output:     $MsiPath"

& $Wix.Source build $WxsPath -arch x64 -out $MsiPath
if ($LASTEXITCODE -ne 0) {
    Write-Error "WiX build failed with exit code $LASTEXITCODE"
}

if (-not (Test-Path $MsiPath)) {
    Write-Error "MSI was not created: $MsiPath"
}

Write-Host "Created installer: $MsiPath"
