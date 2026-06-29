<#
.SYNOPSIS
  Build the Marketplace serverless endpoint bootstrap artifact for Terraform.

.DESCRIPTION
  Produces one reusable Lambda bootstrap zip. Terraform deploys one Lambda per
  endpoint and sets MARKETPLACE_ENDPOINT_NAME on each function, so the deployed
  functions stay endpoint-scoped instead of behaving as a catch-all dispatcher.

.PARAMETER OutputDir
  Directory where bootstrap and bootstrap.zip are written.

.EXAMPLE
  .\scripts\Build-Serverless.ps1
#>
[CmdletBinding()]
param(
    [string]$OutputDir = "dist/serverless"
)

$ErrorActionPreference = "Stop"
$MarketplaceRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$BackendRoot = Join-Path $MarketplaceRoot "backend"
$OutputRoot = Join-Path $MarketplaceRoot $OutputDir
$BootstrapPath = Join-Path $OutputRoot "bootstrap"
$ZipPath = Join-Path $OutputRoot "bootstrap.zip"

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

Push-Location $BackendRoot
try {
    Write-Host "`n=== Building serverless endpoint bootstrap ===" -ForegroundColor Cyan
    $env:GOOS = "linux"
    $env:GOARCH = "amd64"
    $env:CGO_ENABLED = "0"
    go build -tags lambda.norpc -o $BootstrapPath ./cmd/serverless
    if ($LASTEXITCODE -ne 0) {
        throw "go build failed (exit code $LASTEXITCODE)"
    }
}
finally {
    Remove-Item Env:\GOOS -ErrorAction SilentlyContinue
    Remove-Item Env:\GOARCH -ErrorAction SilentlyContinue
    Remove-Item Env:\CGO_ENABLED -ErrorAction SilentlyContinue
    Pop-Location
}

if (Test-Path $ZipPath) {
    Remove-Item $ZipPath -Force
}

Compress-Archive -Path $BootstrapPath -DestinationPath $ZipPath
Write-Host "`nServerless artifact written to $ZipPath" -ForegroundColor Green
Write-Host "Use it with Terraform variable: -var lambda_zip_path=$ZipPath" -ForegroundColor DarkGray
