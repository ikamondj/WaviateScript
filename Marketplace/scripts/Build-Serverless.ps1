<#
.SYNOPSIS
  Build and optionally deploy the Marketplace serverless stack via SAM CLI.

.DESCRIPTION
  Runs `sam build` (and `sam deploy --guided` if -Deploy is set) from the
  marketplace root so the template.yaml and backend CodeUri are resolved
  correctly.

.PARAMETER Deploy
  When set, runs `sam deploy --guided` after a successful build.

.EXAMPLE
  # Build only
  .\scripts\Build-Serverless.ps1

  # Build and deploy
  .\scripts\Build-Serverless.ps1 -Deploy
#>
[CmdletBinding()]
param(
    [switch]$Deploy
)

$ErrorActionPreference = "Stop"
$MarketplaceRoot = Resolve-Path (Join-Path $PSScriptRoot "..")

Push-Location $MarketplaceRoot
try {
    Write-Host "`n=== SAM Build ===" -ForegroundColor Cyan
    sam build --use-container
    if ($LASTEXITCODE -ne 0) {
        throw "sam build failed (exit code $LASTEXITCODE)"
    }

    if ($Deploy) {
        Write-Host "`n=== SAM Deploy (guided) ===" -ForegroundColor Cyan
        sam deploy --guided
        if ($LASTEXITCODE -ne 0) {
            throw "sam deploy failed (exit code $LASTEXITCODE)"
        }
    } else {
        Write-Host "`nBuild complete. Run with -Deploy to push to AWS." -ForegroundColor Green
        Write-Host "Or invoke locally with:  sam local start-api" -ForegroundColor DarkGray
    }
} finally {
    Pop-Location
}
