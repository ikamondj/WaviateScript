# 17b Server Refactor Progress

Updated: 2026-06-25

## Completed in this pass

- Added explicit route metadata covering path, method, auth requirement, and deployment name.
- Refactored traditional server route registration to use the shared endpoint registry.
- Added reusable auth middleware application based on endpoint metadata.
- Kept auth checks cross-cutting:
  - Token validation.
  - Expired/invalid token rejection.
  - User/account context attachment.
  - Endpoint auth requirement enforcement.
  - Optional active premium plan/creator gates for future protected endpoints.
- Preserved upload limit enforcement in service logic:
  - Standard users: 5 uploads.
  - Active premium users: 200 uploads.
  - Creator role: unlimited and separate from premium.
- Refactored server construction so dependency setup returns errors instead of calling `log.Fatal`.
- Added graceful shutdown for the traditional Go server.
- Added structured JSON-style startup/shutdown/store logging helpers.
- Added AWS App Runner Terraform for stateless traditional server deployment:
  - Container image deployment.
  - Managed HTTPS ingress.
  - `/healthz` health check.
  - Environment variables.
  - Secret references.
  - Optional VPC connector.
  - Autoscaling configuration.
- Updated backend and marketplace README files with the new deployment shape.

## Verification

- `go test ./...` passed from `Marketplace/backend`.
- `go build ./cmd/server` passed from `Marketplace/backend`.

## Notes / Limits

- Terraform was not installed in the environment, so `terraform fmt` and `terraform validate` could not be run.
- `MARKETPLACE_MODE=server` remains Postgres-backed and stateless. Non-server modes still use memory storage for local tests/dev only.
- OAuth/session/account/role logic remains in Go rather than cloud ingress because marketplace-specific permissions must not depend on a single cloud provider.
