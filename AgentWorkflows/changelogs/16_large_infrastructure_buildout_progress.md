# 16 Large Infrastructure Buildout Progress

Updated: 2026-06-25

## Completed in this pass

- Added backend search modes for script name and author:
  - `contains`
  - `starts_with`
  - `exact`
- Added backend include/exclude tag filtering with repeated `includeTag` and `excludeTag` query params.
- Kept backward compatibility for the old single `tag` query param by treating it as an included tag.
- Added `GET /api/tags` so the tag widget is populated from the Postgres `tags` table.
- Updated the Vue search UI:
  - Name/author match-mode selectors.
  - Tag-list search inside the include/exclude tag dropdown.
  - Selected tag chips remain independent from the filtered visible tag list.
  - Download Source uses the backend source decompression/formatting endpoint.
- Added focused Postgres indexes:
  - Trigram indexes for contains searches on script and author names.
  - Lowercase prefix indexes for exact/prefix name matching.
  - Sort indexes for downloads and updated-time ordering.
  - Auth/session and role-related indexes.
- Added Google-only OAuth flow in the Go backend:
  - `/api/auth/google/start`
  - `/api/auth/google/callback`
  - `/api/auth/providers`
  - `/api/auth/me`
- Added opaque marketplace session tokens stored by hash in `auth_sessions`.
- Added marketplace account role/plan fields:
  - `plan` as `standard` or `premium`
  - `subscription_expires_at`
  - `creator` as a separate unlimited-upload flag
- Added upload limit enforcement:
  - Standard: 5 uploads.
  - Active premium: 200 uploads.
  - Creator: unlimited, separate from premium.
- Added service-level tests for the standard, active-premium, expired-premium, and creator upload limit rules.
- Changed upload handling so the authenticated session supplies the author/account; request `authorId` is no longer trusted.
- Preserved and wired existing server-side upload validation, source compression, source formatting, and native compile-check path.
- Tightened metadata validation for uploads.
- Added a shared JUCE `MarketplaceClient` with:
  - Browser login launch.
  - Pasted backend session token storage with expiry.
  - Local token freshness checks.
  - Authenticated upload POST.
- Added shared desktop editor controls for both Standalone and VST builds:
  - Login/Account button.
  - Upload button disabled unless the stored session is fresh.
- Seed data now marks Phase Fold as creator and Mod Matrix as active premium without merging those concepts.

## Verification

- `go test ./...` from `Marketplace/backend` passed with escalation for Go build-cache access.
- `npm.cmd run build` from `Marketplace/frontend` passed with escalation for Vite/esbuild config resolution.
- `cmake --build build-cmake-vs --config Release --target WaviateScript_Standalone` passed.
- `cmake --build build-cmake-vs --config Release --target WaviateScriptTests` passed.
- `./build-cmake-vs/tests/bin/Release/WaviateScriptTests.exe` passed.

## Intentional limits / follow-up context

- OAuth is implemented only for Google. Provider shape is modular enough to add more later, but no other providers are exposed.
- Desktop login uses a browser flow plus paste-back marketplace session value. A future installer/protocol-handler pass can replace this with automatic deep-link handoff.
- Desktop upload currently sends current editor text with basic metadata and a default `sample-shader` tag; richer upload metadata UI can be added later.
- Native compiler validation remains optional unless `MARKETPLACE_REQUIRE_NATIVE_COMPILER` is enabled and the native compiler bridge is built/available.
