# Proposal

## Why

The per-source free-text candidate budget was spent on the hits the walk *collected* rather than on the
hits the search *kept*: the search scope was applied only after the walk. A scoped search that dropped
many collected hits therefore answered with fewer than `limit` candidates even though in-scope hits lay
behind the walk's stopping point (NaviVeylin issue #19, TODO §110, found while implementing
`fix-cross-database-search-scope`).

## What Changes

- A `FreeTextBudget` counts a hit against the budget only when the active search scope keeps it, so the
  walk continues until `limit` in-scope candidates are collected instead of stopping at `limit` scanned
  hits.
- The scope's extent is derived once, before the database walk, because the budget has to measure every
  database's hits against the same extent while it counts.
- The post-walk scope filter is removed: every collected entry is in scope by construction, so a second
  pass could only shorten a candidate list the budget had already spent.
- The debug log that reports how many hits the scope refused moves to where collection happens, keeping
  its counts and its coordinate-free wording.
- A host test covers the budget rule: a scoped walk collects the requested candidates, an out-of-scope hit
  does not spend the budget, and an unset scope spends it on the hits themselves.

The rule bounds the object loads behind the text index, not the index traversal: the index materializes a
query's complete hit set in one call, so a refused hit costs its own load and nothing more.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `search-free-text`: adds the requirement that the per-source free-text candidate budget counts the
  candidates the search keeps, so a scoped search still collects the requested number of candidates when
  the index holds them.

## Impact

Affected files and modules:

- `libosmscout-client-java/src/search_scope.h` — adds the `FreeTextBudget` class and its rule beside the
  existing scope helpers.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — `DoSearchLocations` derives the scope extent before
  the database walk, spends the budget through `FreeTextBudget`, and drops the post-walk scope filter.
- `Tests/src/SearchScopeTest.cpp` — three host cases over the budget rule.
- `openspec/changes/client-java-search-free-text-budget` — this change.

No build-file change: the test file is already registered with the `libosmscout-client-java/src` include
path. No database, map or style file format change, so no `FileFormatVersion.md` version bump applies. No
new dependency. No JNI signature change.

Consumers: the JavaScout search path and the host test suite.
