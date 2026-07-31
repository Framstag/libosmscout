## Why

PR #1744 commented out `mapManager->LookupDatabases()` in `DBThread::Initialize()` under the assumption that the initial scan was deferred to an explicit call from JavaScout. This broke the Qt client's startup database discovery — no maps appear until a user-triggered scan (download, delete). The change was unnecessary: there is no race condition, and the old code worked correctly for both Qt and JavaScout clients.

## What Changes

- Revert `DBThread::Initialize()` to call `mapManager->LookupDatabases()`, restoring automatic initial database scan at startup
- Remove the duplicate explicit `LookupDatabases()` call from `OSMScoutClient.cpp` (JavaScout) — `Initialize()` already triggers it
- No breaking changes

## Capabilities

This is a pure bug fix with no spec-level behavior changes. `skip_specs: true` will be set in `.openspec.yaml`.

## Impact

- `libosmscout-client/src/osmscoutclient/DBThread.cpp` — revert `Initialize()` body
- `libosmscout-client-java/src/OSMScoutClient.cpp` — remove duplicate `LookupDatabases()` call after `Initialize()`
- Qt client (`OSMScout2`, `OSMScoutOpenGL`, etc.) — regains automatic map discovery at startup
- JavaScout — no behavioral change, eliminates wasteful double scan
