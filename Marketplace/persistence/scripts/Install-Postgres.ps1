[CmdletBinding()]
param(
    [string]$Version = "16"
)

$ErrorActionPreference = "Stop"

function Test-Command {
    param([Parameter(Mandatory = $true)][string]$Name)
    return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

if (Test-Command "psql") {
    Write-Host "Postgres client already available."
    exit 0
}

if (Test-Command "winget") {
    Write-Host "Postgres was not found. Attempting winget install for PostgreSQL $Version."
    winget install --id "PostgreSQL.PostgreSQL.$Version" --silent --accept-package-agreements --accept-source-agreements
    Write-Host "Postgres install requested. Open a new shell if psql is not on PATH yet."
    exit 0
}

throw "Postgres is not installed and winget is unavailable. Install PostgreSQL manually, then rerun initialization."
