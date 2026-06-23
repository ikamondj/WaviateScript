[CmdletBinding()]
param(
    [string]$DatabaseName = "waviatescript_marketplace",
    [string]$HostName = "localhost",
    [int]$Port = 5432,
    [string]$UserName = $env:USERNAME
)

$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$SchemaDir = Join-Path $Root "schema/one-time"

function Test-Command {
    param([Parameter(Mandatory = $true)][string]$Name)
    return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

if (-not (Test-Command "psql")) {
    & (Join-Path $PSScriptRoot "Install-Postgres.ps1")
}

if (-not (Test-Command "psql")) {
    throw "psql is still unavailable. Open a new shell or add PostgreSQL bin to PATH."
}

$dbExists = & psql -h $HostName -p $Port -U $UserName -d postgres -tAc "SELECT 1 FROM pg_database WHERE datname = '$DatabaseName';"
if ($dbExists.Trim() -ne "1") {
    if (-not (Test-Command "createdb")) {
        throw "Database $DatabaseName does not exist and createdb is unavailable."
    }

    Write-Host "Creating database $DatabaseName."
    & createdb -h $HostName -p $Port -U $UserName $DatabaseName
}

& psql -h $HostName -p $Port -U $UserName -d $DatabaseName -v ON_ERROR_STOP=1 -c @"
CREATE TABLE IF NOT EXISTS schema_migrations (
    filename text PRIMARY KEY,
    applied_at timestamptz NOT NULL DEFAULT now()
);
"@

Get-ChildItem -Path $SchemaDir -Filter "*.sql" | Sort-Object Name | ForEach-Object {
    $script = $_
    $applied = & psql -h $HostName -p $Port -U $UserName -d $DatabaseName -tAc "SELECT 1 FROM schema_migrations WHERE filename = '$($script.Name)';"

    if ($applied.Trim() -eq "1") {
        Write-Host "Skipping already applied script $($script.Name)."
    } else {
        Write-Host "Applying schema script $($script.Name)."
        & psql -h $HostName -p $Port -U $UserName -d $DatabaseName -v ON_ERROR_STOP=1 -f $script.FullName
        & psql -h $HostName -p $Port -U $UserName -d $DatabaseName -v ON_ERROR_STOP=1 -c "INSERT INTO schema_migrations (filename) VALUES ('$($script.Name)');"
    }
}

Write-Host "Marketplace database initialization complete."
