[CmdletBinding()]
param(
    [string]$Version = "18.4"
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

# Set your desired variables
$Version = "18.4"
$DbPassword = "admin"  # <-- Set your password here

if (Get-Command "winget" -ErrorAction SilentlyContinue) {
    Write-Host "PostgreSQL was not found. Attempting silent install..."
    
    # Use --override to pass the master password argument directly to the EDB installer
    winget install --id "PostgreSQL.PostgreSQL.$Version" --silent --accept-package-agreements --accept-source-agreements --override "--mode unattended --serverpassword `"$DbPassword`""
    
    Write-Host "PostgreSQL installation requested with your custom password."
    exit 0
}


throw "Postgres is not installed and winget is unavailable. Install PostgreSQL manually, then rerun initialization."
