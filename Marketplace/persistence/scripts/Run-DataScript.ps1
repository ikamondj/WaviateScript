[CmdletBinding()]
param(
    [string]$DatabaseName = "waviatescript_marketplace",
    [string]$HostName = "localhost",
    [int]$Port = 5432,
    [string]$UserName = $env:USERNAME,
    [string]$ScriptName
)

$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$DataDir = Join-Path $Root "data"

if ([string]::IsNullOrWhiteSpace($ScriptName)) {
    Write-Host "Available data scripts:"
    Get-ChildItem -Path $DataDir -Filter "*.sql" | Sort-Object Name | ForEach-Object {
        Write-Host " - $($_.Name)"
    }
    exit 0
}

$ScriptPath = Join-Path $DataDir $ScriptName
if (-not (Test-Path $ScriptPath)) {
    throw "Data script not found: $ScriptName"
}

& psql -h $HostName -p $Port -U $UserName -d $DatabaseName -v ON_ERROR_STOP=1 -f $ScriptPath
Write-Host "Applied data script $ScriptName."
