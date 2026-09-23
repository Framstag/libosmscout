# Design

## Context

See `proposal.md` — Why for the motivation. This section records the current state the change has to work with.

`searchPOIsByTypes` lives in `libosmscout-client-java/src/OSMScoutClient.cpp` (JNI bridge, single
translation unit; `searchPOIsByTypes` at ~line 7522, `BuildPoiEntry` at ~line 7483). It runs one
synchronous job on `DBThread` and walks the `std::list<DBInstanceRef>` the job is handed:

- The walk order is the database load order (`libosmscout-client/include/osmscoutclient/DBInstance.h:97`,
  `GetDBGeoBox()` gives each database's area).
- Basemap databases are skipped (`IsBasemapDatabase`, `OSMScoutClient.cpp:107`).
- Each database is queried with the full radius through `osmscout::POIService::GetPOIsInRadius`
  (areas, then ways, then nodes), and each hit becomes a `PoiEntry` (`label`, `operatorName`,
  `brand`, `objectType`, `lat`, `lon`, `distance`).
- The walk stops as soon as `entries.size() >= limit` (`OSMScoutClient.cpp:7664`).
- Afterwards the list is sorted by `distance` only (`:7671`) and truncated to `limit` (`:7674`).

`PoiEntry` carries no cross-database identity: file offsets are database-local (the same reason
`search_scope.h` restricts a region handle to its owning database), so "the same POI in two
databases" can only be recognised from its content.

The JavaScout test suite (`JavaScout/src/test/java/com/framstag/libosmscout/client/OSMScoutClientPoiSearchTest.java`)
currently has no multi-database scenario; the multi-database tests only exist on the
`naviveylin-local` branch (introduced with commit `5371eb122`) and run against real overlapping
extracts supplied by a test property.

This work is the migration of that branch's POI-merge behaviour (see `proposal.md`).

## Goals / Non-Goals

Goals (see the delta spec `specs/poi-search-api/spec.md` for the contract):

- One result list over all loaded non-basemap databases, with duplicates collapsed.
- Deterministic order that does not depend on database discovery order.
- The database containing the search center wins for a duplicated POI.

Non-Goals:

- No change to `PoiEntry`'s public fields, to the category mapping, or to the basemap exclusion.
- No change to what a single loaded database returns.
- Not adding an overlap test that would let a database be skipped entirely. A database whose area
  does not intersect the search circle is still queried (see Risks).
- Not unifying this with the JavaScout-side near-duplicate dedup described in
  `openspec/specs/search-dedup/spec.md`. That rule (same object type, < 300 m apart, > 3000 m from
  the center) lives in the result list of the app; this change is about databases, not about
  visually similar entries.

## Decisions

### 1. Fix the merge in the native POI search, not in the app

Alternatives:

- (a) Merge, deduplicate and order inside `searchPOIsByTypes` (JNI bridge).
- (b) Return raw per-database hits and deduplicate in JavaScout, like the existing
  `search-dedup` rule.
- (c) Query only the database whose area contains the search center.

Decision: (a). (b) pushes a database concern into the app and leaves every other
`libosmscout-client-java` consumer with the same duplicates; the existing JavaScout dedup rule also
uses a different criterion (300 m / 3000 m) and would either duplicate or fight this logic. (c)
is the smallest change but silently drops the POIs of neighbouring maps, which is exactly the case
where a user searches near a map border. The dedup information (which database a hit came from) is
only available at the merge point, so (a) is where the contract can be kept.

### 2. Order databases by area containment of the search center, stably

Alternatives:

- (a) Two groups — databases whose area contains the search center, then the rest; discovery order
  preserved inside each group.
- (b) Sort all databases by the distance from their area to the search center.
- (c) Keep the discovery order.

Decision: (a). It gives the intended precedence (the map the user is looking at first) with a
`std::partition`-style pass, needs no geometry beyond the `GeoBox::Includes` test already used
elsewhere in this file, and is stable, so the result of one search does not depend on the scan
order for databases that are equally relevant. (b) would order by a metric that has no meaning for
the user and adds `GeoBox` distance code; (c) is the bug being fixed.

### 3. Duplicate identity: object type plus rounded coordinate

Alternatives:

- (a) Object type name plus coordinate rounded to 1e-5 degrees (~1 m).
- (b) OSM object identity (type plus file offset).
- (c) Label plus object type plus proximity (~300 m), as the JavaScout rule does.

Decision: (a). (b) is impossible: file offsets are database-local, so the two copies of one object
have unrelated offsets. (c) is too coarse for a radius search — in a city centre a 300 m radius
contains several distinct POIs of the same type with the same label (chains), and collapsing them
would hide real results. (a) only collapses hits that are the same object in two databases, and
tolerates the coordinate jitter between extracts of the same area. Including the object type in the
key keeps a node-shaped POI and an area-shaped POI of a different type at the same spot apart, so
the dedup does not remove legitimate distinct results. The 1 m bucket is the same magnitude as the
`search_scope.h` note that region offsets are database-local — it is a content-based approximation
of identity, not exact identity.

### 4. Bound the result list, not the search work

Alternatives:

- (a) Drop the early `entries.size() >= limit` break, collect from every database, then sort and
  truncate once.
- (b) Keep the early break but raise the per-database budget.
- (c) Keep the early break and sort per database, stopping once the nearest `limit` are known.

Decision: (a). With the early break, a database that fills the limit hides the others — the bug in
the proposal — and no budget can be chosen without knowing how many databases contribute nearer
hits. (b) only moves the threshold; (c) is the correct end state but needs a partial-merge across
databases, i.e. more machinery than this change should carry. Collecting everything and truncating
once is bounded by the search radius, which the caller already chose, and keeps the JNI method
simple. The extra work this creates is tracked in Risks.

### 5. Ordering key: distance, then label, then the remaining fields

Alternatives:

- (a) `distance` ascending, then `label`, then `objectType`, then `lat`/`lon`.
- (b) `distance` ascending, then `label` only.
- (c) `distance` ascending, then `objectType`, then `label`.

Decision: (a). The spec requires a deterministic order, so the comparator has to be a total order on
the data `PoiEntry` holds: `label` alone is not total (same-named chain branches at equal distance),
and adding `objectType` and the rounded coordinates makes the comparison total without a tie-break
that depends on the input order. `std::sort` is not stable, so the comparator must be total rather
than relying on the container order.

## Sequence

```
JavaScout / app          JNI searchPOIsByTypes            DBThread job
      |                            |                            |
      | searchPOIs(cat, c, r, n)   |                            |
      |--------------------------->|                            |
      |                            | RunSynchronousJob          |
      |                            |--------------------------->|
      |                            |     list<DBInstanceRef>    |
      |                            |<---------------------------|
      |                            |                            |
      |                  +-----------------------------+         |
      |                  | 1. split databases:         |         |
      |                  |    area contains center?    |         |
      |                  |    (stable partition)       |         |
      |                  +-----------------------------+         |
      |                            |                            |
      |                  +-----------------------------+         |
      |                  | 2. per database, in order:  |         |
      |                  |    skip basemap             |         |
      |                  |    GetPOIsInRadius(cat, r)  |-------->| load
      |                  |    build PoiEntry per hit   |<--------|
      |                  |    no early stop on limit   |         |
      |                  +-----------------------------+         |
      |                            |                            |
      |                  +-----------------------------+         |
      |                  | 3. first-wins dedup over    |         |
      |                  |    (objectType, lat, lon)   |         |
      |                  |    rounded to 1e-5 deg      |         |
      |                  +-----------------------------+         |
      |                            |                            |
      |                  +-----------------------------+         |
      |                  | 4. sort total order         |         |
      |                  |    (distance, label, type,   |         |
      |                  |     lat, lon)               |         |
      |                  | 5. truncate to limit        |         |
      |                  +-----------------------------+         |
      |                            |                            |
      |  PoiEntry[]                |                            |
      |<---------------------------|                            |
```

Steps 3 and 4 happen after the job has returned, on the calling thread, exactly where the current
sort and truncation already are.

## Risks / Trade-offs

- **Search work grows with the number of loaded databases** (the early exit is gone) → the caller's
  radius bounds each database's query, the result list is bounded by `limit`, and the number of
  loaded non-basemap databases is small in practice. Excluded on purpose: skipping databases whose
  area cannot intersect the search circle. A follow-up change can add that without changing this
  contract; if it turns out to be needed, it is added as a behaviour of its own, not smuggled in
  here.
- **1 m coordinate buckets miss duplicates between extracts of different snapshots**, where the same
  object was moved slightly → accepted, and strictly better than today's unconditional duplicates.
  Widening the tolerance would start collapsing genuinely distinct nearby POIs; if this turns out to
  matter, the tolerance becomes a parameter of its own change.
- **Rounding can collapse two distinct objects that share one coordinate** (for example the same
  facility mapped once as a node and once as an area of the same type) → including `objectType` in
  the key keeps objects of different types apart; the same type at the same coordinate is treated as
  one POI, which is what the user sees anyway.
- **Sorting by label changes the visible order of equal-distance entries** → intended, and the point
  of the change: the order stops depending on which map was scanned first.
- **`std::sort` with a non-total comparator is undefined behaviour** → the comparator compares the
  full tuple (decision 5), so equal entries compare equal.
- **The new tests depend on real overlapping extracts** (a directory property, skipped otherwise) →
  the test file keeps the assumption-gated pattern used by the branch; the single-database scenarios
  of the existing suite continue to run without any data property and cover the no-regression side.

## Migration Plan

No data migration: the database file format, `PoiEntry` and the Java API are untouched. The change
is one JNI function plus its tests, so rollback is reverting the merge commit. Nothing needs to be
re-imported, and existing callers keep working unchanged.

Verification for the migration itself: build the client-java module (Meson, plus the CMake path used
by CI), run the existing JavaScout test suite without the multi-database property (single-database
scenarios must behave exactly as before), then run it with the property set to a directory of
overlapping extracts for the new scenarios. The reference implementation to compare against is
`naviveylin-local` at `5371eb122`, which is where these merge rules were first written.
