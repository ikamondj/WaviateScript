# Marketplace Persistence Simulation

Local Postgres helper scripts and SQL for simulating the marketplace database.

## Folders

- `scripts`: PowerShell helpers for installing Postgres, initializing schema, and running manual data scripts.
- `schema/one-time`: Ordered idempotent schema scripts. These represent the newest empty database shape.
- `data`: Manual seed/demo data scripts.

## Typical Flow

```powershell
cd Marketplace/persistence
./scripts/Initialize-Database.ps1
./scripts/Run-DataScript.ps1 -ScriptName 001_seed_demo_marketplace.sql
```

The schema runner records applied one-time scripts in `schema_migrations` so repeated runs are safe.
