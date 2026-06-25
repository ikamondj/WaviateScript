# Marketplace Backend

Go scaffold for the WaviateScript marketplace API.

## Layout

```text
cmd/server        Traditional long-running HTTP server.
cmd/serverless    Endpoint-scoped Lambda bootstrap selected by MARKETPLACE_ENDPOINT_NAME.
internal/auth     OAuth/session provider and reusable auth middleware.
internal/controller/http
internal/desktop  Desktop app handoff URI helpers.
internal/domain   Core request and response models.
internal/endpoints Explicit route/auth/deployment metadata registry.
internal/persistence
internal/service   Business logic shared by server and serverless modes.
internal/validation
native             C ABI shim/build files for optional Waviate compiler validation.
```

## Commands

```powershell
go run ./cmd/server
$env:MARKETPLACE_ENDPOINT_NAME="health"; go run ./cmd/serverless
go test ./...
```

`cmd/serverless` is not a catch-all router. It mounts exactly one endpoint from
`internal/endpoints` based on `MARKETPLACE_ENDPOINT_NAME`. Terraform deploys one
logical Lambda function per endpoint while reusing the same bootstrap artifact.

Native compiler validation is optional by default. See `native/README.md` for
building the Waviate desktop compiler shim and running with `-tags nativecompiler`.

## Notes

- `MARKETPLACE_MODE=server` and `MARKETPLACE_MODE=serverless` use Postgres as
  the source of truth. Other modes use the in-memory store for quick tests/dev.
- `internal/persistence/postgres` contains the first concrete SQL-backed store for the normalized marketplace schema.
- Server and serverless entry points share service, persistence, auth, and
  endpoint handler code through `internal/server`.
- Terraform modules live under `../infra/server` and `../infra/serverless`.
