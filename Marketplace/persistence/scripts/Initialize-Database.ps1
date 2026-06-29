[CmdletBinding()]
param(
    [string]$DatabaseName = "waviatescript_marketplace",
    [string]$HostName = "localhost",
    [int]$Port = 5432,
    [string]$UserName = "postgres",
    [string]$Password = "admin"
)

$ErrorActionPreference = "Stop"

$PreviousPgPassword = $env:PGPASSWORD
$env:PGPASSWORD = $Password

try {
    $Root = Resolve-Path (Join-Path $PSScriptRoot "..")
    $SchemaDir = Join-Path $Root "schema\one-time"

    # Change this to 16 if you installed PostgreSQL 16
    $PgBin = "C:\Program Files\PostgreSQL\18\bin"

    $Psql = Join-Path $PgBin "psql.exe"
    $Createdb = Join-Path $PgBin "createdb.exe"

    if (-not (Test-Path $Psql)) {
        Write-Host "psql not found at $Psql - attempting install..."
        & (Join-Path $PSScriptRoot "Install-Postgres.ps1")
    }

    if (-not (Test-Path $Psql)) {
        throw "psql is still unavailable at $Psql. Verify PostgreSQL is installed."
    }

    if (-not (Test-Path $SchemaDir)) {
        throw "Schema directory not found: $SchemaDir"
    }

    $DatabaseNameSql = $DatabaseName.Replace("'", "''")

    $dbExists = & $Psql `
        -h $HostName `
        -p $Port `
        -U $UserName `
        -d postgres `
        -tAc "SELECT CASE WHEN EXISTS (SELECT 1 FROM pg_database WHERE datname = '$DatabaseNameSql') THEN 1 ELSE 0 END;"

    $dbExistsText = if ($null -eq $dbExists) { "" } else { $dbExists.ToString().Trim() }

    if ($dbExistsText -ne "1") {
        if (-not (Test-Path $Createdb)) {
            throw "Database $DatabaseName does not exist and createdb is unavailable at $Createdb."
        }

        Write-Host "Creating database $DatabaseName."

        & $Createdb `
            -h $HostName `
            -p $Port `
            -U $UserName `
            $DatabaseName
    }

    $CreateMigrationsTableSql = @"
CREATE TABLE IF NOT EXISTS schema_migrations (
    filename text PRIMARY KEY,
    applied_at timestamptz NOT NULL DEFAULT now()
);
"@

    & $Psql `
        -h $HostName `
        -p $Port `
        -U $UserName `
        -d $DatabaseName `
        -v ON_ERROR_STOP=1 `
        -c $CreateMigrationsTableSql

    Get-ChildItem -Path $SchemaDir -Filter "*.sql" | Sort-Object Name | ForEach-Object {
        $script = $_
        $ScriptNameSql = $script.Name.Replace("'", "''")

        $applied = & $Psql `
            -h $HostName `
            -p $Port `
            -U $UserName `
            -d $DatabaseName `
            -tAc "SELECT CASE WHEN EXISTS (SELECT 1 FROM schema_migrations WHERE filename = '$ScriptNameSql') THEN 1 ELSE 0 END;"

        $appliedText = if ($null -eq $applied) { "" } else { $applied.ToString().Trim() }

        if ($appliedText -eq "1") {
            Write-Host "Skipping already applied script $($script.Name)."
        }
        else {
            Write-Host "Applying schema script $($script.Name)."

            & $Psql `
                -h $HostName `
                -p $Port `
                -U $UserName `
                -d $DatabaseName `
                -v ON_ERROR_STOP=1 `
                -f $script.FullName

            & $Psql `
                -h $HostName `
                -p $Port `
                -U $UserName `
                -d $DatabaseName `
                -v ON_ERROR_STOP=1 `
                -c "INSERT INTO schema_migrations (filename) VALUES ('$ScriptNameSql');"
        }
    }

    Write-Host "Marketplace database initialization complete."
}
finally {
    $env:PGPASSWORD = $PreviousPgPassword
}