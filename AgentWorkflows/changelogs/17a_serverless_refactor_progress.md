# 17a Serverless Refactor Progress

Updated: 2026-06-25

## Completed in this pass

- Added explicit endpoint metadata in `Marketplace/backend/internal/endpoints`.
- Added `Marketplace/infra/endpoints.json` as the checked-in deployment manifest.
- Refactored the HTTP controller so route registration is generated from endpoint metadata.
- Added single-endpoint router support for serverless adapters.
- Changed `cmd/serverless` from a catch-all router into an endpoint-scoped Lambda bootstrap selected by `MARKETPLACE_ENDPOINT_NAME`.
- Removed the old SAM catch-all template.
- Updated `Marketplace/scripts/Build-Serverless.ps1` to build a reusable Lambda bootstrap zip for Terraform.
- Added AWS Terraform for endpoint-scoped serverless deployment:
  - One Lambda per endpoint.
  - One API Gateway HTTP route per endpoint.
  - Per-function `MARKETPLACE_ENDPOINT_NAME`.
  - Shared Go auth/account/role enforcement.
  - Optional VPC config.
  - CloudWatch log groups.
- Added registry/manifest drift tests so serverless deployment metadata cannot silently drift from Go endpoint definitions.
- Added route tests proving a single-endpoint router only serves the selected endpoint.
- Added auth middleware tests covering invalid sessions, authenticated context wiring, creator gates, and active premium plan gates.

## Verification

- `go test ./...` passed from `Marketplace/backend`.
- `go build ./cmd/serverless` passed from `Marketplace/backend`.

## Notes / Limits

- Terraform was not installed in the environment, so `terraform fmt` and `terraform validate` could not be run.
- The Lambda artifact is shared for build simplicity, but Terraform creates separate Lambda resources per logical endpoint.
- OAuth/token validation remains in Go. API Gateway is intentionally kept transport-only so marketplace account mapping, upload limits, premium checks, and creator permissions stay portable.
