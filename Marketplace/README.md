# WaviateScript Marketplace

This directory contains an isolated scaffold for the future WaviateScript marketplace. It is split into three parts:

- `frontend`: Vue/Vite marketplace UI scaffold.
- `backend`: Go service scaffold with server and serverless entry points.
- `persistence`: local Postgres simulation scripts, schema scripts, and manual data scripts.

The marketplace entries are expected to be Waviate scripts created by users. The upload payload may eventually be raw text, compressed, encrypted, or another packaged format. The current scaffold keeps that decision abstract.

## Local Shape

```text
Marketplace/
  frontend/
  backend/
  persistence/
```

Each subproject has its own README with focused notes.
