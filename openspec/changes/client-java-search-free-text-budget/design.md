# Design

## Context

See `proposal.md` — Why. The current state that shapes the approach:

- `DoSearchLocations` in `libosmscout-client-java/src/OSMScoutClient.cpp` collects free-text candidates in
  a `freeTextEntries` vector while it walks the databases, and bounds the collection with a per-source
  budget expressed as `freeTextEntries.size() >= limit`.
- The search scope's geographic extent enters the same function only on the way through the walk (derived
  from the database that resolved the region handle), and the scope itself is applied as a
  `std::remove_if` over `freeTextEntries` after the walk. The budget therefore measures the wrong set: it
  counts the hits before the filter, so a scoped search that drops many of them answers with fewer than
  `limit` candidates.
- `naviveylin::IsInsideGeoBox` and `naviveylin::DeriveScopeExtent` already exist; `search_scope.h` is a
  dependency-free header the host suite compiles (`Tests/src/SearchScopeTest.cpp`).

## Goals / Non-Goals

**Goals:**

- Spend the free-text budget on the candidates the search keeps, so a scoped walk delivers `limit`
  in-scope candidates while the index holds them.
- Keep the rule a pure function of the extent and the limit, so the host suite covers it.
- Report the same "how much the scope refused" debug number as before.

**Non-Goals:**

- Changing the index traversal or the number of object loads; the rule bounds loads, not traversal.
- Changing the structured search, the scope expansion, the region resolution or the extent derivation.
- Any JNI signature, database, map or style file format change.

## Decisions

### D1: A `FreeTextBudget` in `search_scope.h` owns the accounting

Alternatives:

- **Keep a lambda in `OSMScoutClient.cpp` and count only kept hits there** — rejected: the rule would stay
  inside the JNI translation unit the host suite cannot compile, so the regression that motivated the
  change (a budget spent on dropped hits) would be caught by no host test.
- **Put the class in a new header beside `search_scope.h`** — rejected: it uses `IsInsideGeoBox` and
  `GeoBox` from that header, and the file is the established home of the pure scope rules.

Chosen: `naviveylin::FreeTextBudget` in `search_scope.h`, constructed from `(limit, extent)`, with
`Admit(lat, lon)` returning whether the hit is kept and `Reached()` reading only the kept count.

### D2: The scope extent is derived before the database walk

Alternatives:

- **Derive the extent lazily on first use inside the walk, as before** — rejected: the budget needs the
  extent in hand when it starts collecting, and the resolving database may be walked after databases whose
  hits the budget already spends itself on.

Chosen: the scope of the database that resolved the region handle is resolved once, before the loop, and
the extent derived from it; the walk reuses the resolved vector for the structured search of that
database. Behaviour of the extent is unchanged: the name-based path (no handle) still leaves it unset.

### D3: The post-walk filter is removed, not kept as a safety net

Alternatives:

- **Keep the `remove_if` after the walk as a belt-and-braces filter** — rejected: every collected entry was
  admitted against the same extent, so the filter could only drop entries the budget already refused to
  spend on; keeping it reintroduces the shortening the change removes and hides a real regression.

Chosen: the collection loop admits each built entry through `freeTextBudget.Admit(entry.lat, entry.lon)`
and the filter is gone. The debug log of dropped/scanned counts moves into the collection block.

## Risks / Trade-offs

- [The budget's `Dropped`/`Scanned` counts are reported before the per-database dedup that drops hits the
  structured search already published.] → The log keeps its wording ("hit(s) outside the scope"); the
  denominator is every hit the walk reached, which is the number the device check wants.
- [The pre-walk derivation runs even if the resolving database is not among the walked ones.] → The
  resolution and derivation only read the region's own object from the database that resolved it, which is
  what the previous in-walk derivation did for that same database; no walk order changes the result.

## Migration Plan

No data migration. The JNI library's public surface and the Java API are unchanged. Rollback is a revert of
the change's commit.

## Open Questions

None.
