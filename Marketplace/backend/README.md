# Marketplace Backend

Go scaffold for the WaviateScript marketplace API.

## Layout

```text
cmd/server        Local HTTP server mode.
cmd/serverless    Serverless handler scaffold.
internal/auth     OAuth provider and middleware placeholders.
internal/controller/http
internal/desktop  Desktop app handoff URI helpers.
internal/domain   Core request and response models.
internal/persistence
internal/service   Business logic shared by server and serverless modes.
internal/validation
native             C ABI shim/build files for optional Waviate compiler validation.
```

## Commands

```powershell
go run ./cmd/server
go run ./cmd/serverless
go test ./...
```

Native compiler validation is optional by default. See `native/README.md` for
building the Waviate desktop compiler shim and running with `-tags nativecompiler`.

## Notes

- The local server starts with an in-memory store.
- `internal/persistence/postgres` contains the first concrete SQL-backed store for the normalized marketplace schema.
- TODO comments mark real DB wiring, OAuth callback handling, upload package validation, and desktop install semantics.
- Server and serverless entry points both build the same handler through `internal/server`.
