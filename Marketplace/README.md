# WaviateScript Marketplace

This directory contains an isolated scaffold for the future WaviateScript marketplace. It is split into three parts:

- `frontend`: Vue/Vite marketplace UI scaffold.
- `backend`: Go service scaffold with server and serverless entry points.
- `persistence`: local Postgres simulation scripts, schema scripts, and manual data scripts.
- `infra`: Terraform for traditional server and endpoint-scoped serverless deployment.

The marketplace entries are expected to be Waviate scripts created by users. The upload payload may eventually be raw text, compressed, encrypted, or another packaged format. The current scaffold keeps that decision abstract.

## Local Shape

```text
Marketplace/
  frontend/
  backend/
  persistence/
  infra/
```

Each subproject has its own README with focused notes.

## Deployment Shape

The Go backend is intentionally shared between traditional server and
serverless deployments. Endpoint metadata lives in
`backend/internal/endpoints` and is mirrored in `infra/endpoints.json`.

- Traditional server: deploys the same Go HTTP handler as a stateless container.
- Serverless: deploys one Lambda per logical endpoint; each Lambda sets
  `MARKETPLACE_ENDPOINT_NAME` and mounts only that route.
- Marketplace OAuth, sessions, account mapping, upload limits, premium checks,
  and creator permissions stay in Go so cloud transport config cannot drift from
  marketplace rules.
