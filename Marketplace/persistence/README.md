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

## Schema Notes

Tags are normalized into `tags` plus `waviate_script_tags` instead of being stored as an array on `waviate_scripts`. That keeps tag names unique, supports tag rename/merge flows, and allows indexed tag search without rewriting script rows.

Authors are separate from login users: `authors.user_id` points at `marketplace_users`, and OAuth-style identities live in `user_credentials`.
