# Proposal

## Why

A search with a resolved admin region is scoped only in the database that owns the region handle: every
other loaded database is searched unconstrained, and the free-text index takes no region at all. A short
partial token therefore matches objects anywhere in another map region, and the address a user asked for is
answered with foreign noise. The scope is a region identity, which cannot be compared across databases, so
it cannot narrow hits that another database produced.

## What Changes

- A search with a default admin region SHALL admit only hits whose position lies inside the extent of that
  region, whichever database produced the hit.
- The extent SHALL be derived from the object that represents the region, and SHALL be a superset of the
  region: the filter SHALL NOT drop a hit that lies inside the region. For a region that is represented only
  by a position, the extent SHALL be a documented box around it.
- An extent that cannot be established SHALL leave the filter off, so such a scope degrades to the unscoped
  search instead of returning nothing.
- A hit whose position is not usable SHALL NOT be admitted by a set extent.
- Every search result SHALL report whether it lies inside the active extent, so a consumer can rank a
  close out-of-scope hit below an in-scope one without re-deriving a region's area, and a search without a
  scope SHALL report every result as inside it.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `search-context-region`: a scoped search is narrowed by the geographic extent of the resolved region, in
  every loaded database, and every result reports whether it lies inside that extent.

## Impact

Affected files and modules:

- `libosmscout-client-java/src/search_scope.h` — the geographic-extent helpers, kept dependency-free and
  host-testable, beside the existing scope-expansion rule.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — deriving the extent from the resolved region's object
  and applying it as a filter to free-text hits, whichever database produced them.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/LocationEntry.java` — the reported verdict,
  defaulting to "inside" so an older bridge and an older app keep working together.
- `Tests/src/SearchScopeTest.cpp` — extends the existing host test of the helper.
- `Tests/CMakeLists.txt`, `Tests/meson.build` — already register the test; the change needs no new target.

No database, map or style file format change, so no `FileFormatVersion.md` version bump applies. No new
dependency. No JNI signature change. The search request's parameters and the result array's shape are
unchanged; only the set of results a scoped search returns and one field per result change.

Consumers: every caller of the Java location search; a scoped search returns fewer foreign hits, and a
caller that orders results can use the reported verdict.
