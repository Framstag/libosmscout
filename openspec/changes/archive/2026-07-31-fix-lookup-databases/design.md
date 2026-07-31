## Context

`DBThread::Initialize()` previously called `mapManager->LookupDatabases()` to trigger the initial database scan at startup. PR #1744 commented it out, deferring the scan to an explicit call from JavaScout's `OSMScoutClient.cpp`. This broke the Qt client (no initial scan) and was unnecessary — no race condition existed. See proposal.md for motivation.

## Goals / Non-Goals

**Goals:**
- Restore automatic initial database scan in `DBThread::Initialize()` for all clients
- Remove the duplicate `LookupDatabases()` call from JavaScout's build path
- Zero behavioral change for JavaScout (scan still happens, just once)

**Non-Goals:**
- No changes to `LookupDatabases()` implementation or `OnDatabaseListChanged()`
- No changes to `MapManager` API or signal/slot wiring
- No changes to the Qt client's `InstalledMapsModel` or `MapDownloader`

## Decisions

### Decision 1: Revert `Initialize()` instead of adding a new trigger

**Chosen:** Restore the original one-liner `mapManager->LookupDatabases();` in `DBThread::Initialize()`.

**Alternatives considered:**
- Add a new `StartupScan()` method on `MapManager` — unnecessary indirection, same effect
- Have `OSMScoutQt` call `LookupDatabases()` explicitly — would require changes to every client, not just JavaScout
- Keep current code and fix Qt client separately — Qt client has no other trigger for initial scan

**Rationale:** The old code was correct. Reverting is the minimal, safest change.

### Decision 2: Remove duplicate call from JavaScout

**Chosen:** Delete the explicit `LookupDatabases()` + `.wait()` from `OSMScoutClient.cpp` build method.

**Alternatives considered:**
- Keep both calls (harmless double scan) — wasteful, confusing
- Have `Initialize()` return the future for JavaScout to wait on — changes `DBThread` API for all clients, only JavaScout needs it

**Rationale:** `Initialize()` fires the scan async. JavaScout doesn't need to wait for completion before proceeding — subsequent code only stores settings and creates a Java wrapper object, neither of which depends on databases being open. The `databaseLoadFinished` signal will notify the UI when databases are ready.

## Risks / Trade-offs

- **[Risk] JavaScout UI appears before databases are loaded** → Mitigation: `databaseLoadFinished` signal already exists and UI components connect to it. Same pattern as Qt client.
- **[Risk] Regression in non-Qt, non-JavaScout clients** → Mitigation: The change only touches two files. Any client using `DBThread::Initialize()` gets the old behavior back. No client was relying on `Initialize()` being a no-op.
- **[Risk] Double scan if another code path also calls `LookupDatabases()` at startup** → Mitigation: `LookupDatabases()` queues work on `MapManager`'s single worker thread — sequential, not concurrent. Double emission of `databaseListChanged` causes `OnDatabaseListChanged` to close/reopen databases twice, which is wasteful but correct.
