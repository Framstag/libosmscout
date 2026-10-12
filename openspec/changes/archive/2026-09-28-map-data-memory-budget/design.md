# Design

## Context

See `proposal.md` — Why.

Constraints that shape the approach, all verified in the current tree:

- A map data cache belongs to a `MapService`, and a `MapService` belongs to one open database:
  `MapService.h:122` (`DatabaseRef database`), `MapService.h:123` (`mutable DataTileCache cache`),
  `MapService.cpp:316` (`cache(25)`). `DBInstance` owns the service (`DBInstance.h:62`), `DBThread`
  creates one per database and one for the basemap (`DBThread.cpp:319-324`, `DBThread.cpp:622-627`).
- The second tier is per database as well and is never configured by a client: `Database` holds a
  private `DatabaseParameter` (`Database.h:299`) whose defaults are nodes 5,000, ways 40,000, areas
  5,000, routes 1,500 and 5,000 area-index cells (`Database.h:85-90`). `DBThread` reuses one parameter
  value for every database it opens, and `Database::GetNodeDataFile()` creates each data file with the
  parameter's size lazily (`Database.cpp:341`).
- `MapService` already knows the database it serves, and `Database` can answer the two questions the
  distribution needs: `GetBoundingBox()` (`Database.h:427`) and `IsBasemap()` (`Database.h:396`).
  The Qt client computes relevance client-side today by the same intersection test
  (`DBLoadJob.cpp:62-75`); the Java client does not compute it at all and loads every open database
  (`OSMScoutClient.cpp:1560-1563`).
- Cache access is serialized: `MapService::stateMutex` guards the tile cache and is held across a
  whole tile load (`MapService.cpp:1090`, `:1112`, `:1132`, `:1260`); `DataFile::accessMutex`
  (`DataFile.h:91`, `:275`, `:313`) guards a data file's object cache.
- The generic cache cannot evict by bytes: `Cache::GetMemory()` and its `ValueSizer` exist
  (`Cache.h:280`, `Cache.h:89`) but eviction counts entries (`Cache.h:119-134`, `size++`/`size--`).
  `Cache::SetMaxSize()` does exist and strips the oldest entries (`Cache.h:244`).
- `Database::FlushCache()` already releases the object caches of a database (`Database.cpp:944`).
- The idle release path in the client is inert: `DBThread::FlushCaches` is defined
  (`DBThread.cpp:525`) and emits `flushCachesSignal`, but nothing in the tree ever calls it, so the
  tile caches and object caches of an idle database are never released today.
- Object layouts give the relative cost: `Point` is serial plus `GeoCoord` (`Point.h:34-38`, 24 B),
  a way is features plus `n` points (`Way.h:56`), an area ring is features, `n` points and `n`
  precomputed segment boxes (`Area.h:106-107`), and `SegmentGeoBox` is two `size_t` plus a `GeoBox`
  (`Geometry.h:1189-1194`, 48 B). One cached polygon is therefore tens of times the size of one cached
  point, so entry counts are not comparable between kinds.

## Goals / Non-Goals

**Goals:**

- One configurable memory bound over the map data caches of every open database, expressed and
  reported in memory units.
- Distribution that follows what the view needs, with hysteresis so that a stable user view does not
  cause per-frame rebalancing.
- Release of the caches of a database that stays out of view, so idleness returns memory.
- Behaviour-preserving for tools, demos and tests: off by default at library level, on by default in
  the shipped clients.
- Resizing must be enforceable at runtime for both tiers.

**Non-Goals:**

- No accounting of the actual heap (no allocator hooks, no per-object heap measurement).
  The budget is an accounting model, not a measurement of the process.
- No change to which objects a view contains, and no change to the tile data format or the database
  file format.
- No unification of the two cache tiers into one object pool (see Decision B below).
- No change to the raster tile caches of the Qt renderers (`TiledMapRenderer.h:76-77`), which cache
  pixels, not map data.
- The two pre-existing defects (inert idle release, `lastUsage` refreshed by every render) are
  recorded in `TODO.md`, not fixed here, except where the budget needs the same signal anyway.

## Decisions

### Decision A: Accounting unit — weighted entries

Chosen: each cached object contributes a fixed weight per kind (point 1, line 3, polygon 30, route 3,
index cell 1); a tile contributes the sum of the weights of the objects it references; a budget is
expressed in memory units at the client edge and converted with a bytes-per-weight factor.

Alternatives:

1. **Raw entry counts** — matches today's per-database parameters, no new accounting. Rejected:
   incomparable between kinds (one polygon entry ~ 30-50 point entries), so a single total over
   mixed kinds is not a memory bound at all.
2. **Byte accounting through the existing `ValueSizer`** — honest and directly reportable, and the
   mechanism exists (`Cache.h:89`, `Cache.h:280`). Rejected for this change: eviction is entry-based
   (`Cache.h:119-134`), so byte-based eviction requires changing a class every data file and every
   index cache uses, and the sizer would run on every cached object in the load path. The weight table
   gives the same ordering at a fraction of the risk, and can be replaced by real bytes later behind
   the same accounting interface.
3. **One global object pool keyed by (source data file, offset)** — would fix the double accounting of
   objects that both a tile and an object cache hold, and is the honest end state. Rejected here: it
   merges two cache designs and needs reference sets and byte-based eviction.

Risk of the chosen unit: weights drift with the data (a 500-point polygon weighs the same as a
100-point one). Mitigation: the weights are constants with a documented rationale derived from the
object layouts, and the accounting is reported so the drift is observable; the interface does not
change if weights are later replaced by measured sizes.

### Decision B: One budget over all open databases, distributed by relevance to the view

Chosen: a database is relevant when its geographic extent intersects the view. Relevant databases
share the budget, a database that is not relevant is reduced to a floor, and the distribution is
applied only after the relevant set has been stable for a settling period. A database that has not
been relevant for a longer idle period releases its content entirely.

Alternatives:

1. **Equal share for every open database** (`budget / number of open databases`) — trivial and needs
   no activity information. Rejected: with 20 open databases and 3 of them covering the view, most of
   the budget pays for databases the user is not looking at, which is the situation this change
   exists for.
2. **A global LRU across all caches** — one eviction order, no relevance logic. Rejected: the tiles of
   a view span several databases by construction, so a global order evicts data that the next frame
   needs; it also requires a cross-cache candidate structure that the current per-cache LRU lists do
   not offer.
3. **Relevance without hysteresis** — simplest correct rule. Rejected: panning along a database border
   would rebalance on every frame.

Risk: a database that is reduced to its floor and then becomes relevant again pays disk reads.
Mitigation: the floor is a tunable constant per tier, hysteresis keeps short excursions cheap, and the
settling period is a constant that can be raised without touching the spec.

### Decision C: The policy lives in the library, not in the clients

Chosen: the map service derives relevance of its own database from the projection it is given, using
its `DatabaseRef` (`MapService.h:122`, `Database.h:427`, `Database.h:396`). Clients only configure a
budget and, optionally, read usage.

Alternatives:

1. **Policy in the client** — the Qt load job already computes the relevant set
   (`DBLoadJob.cpp:62-75`) and would publish it to a budget owner in `DBThread`. Rejected: the Java
   client has no relevant-set notion and loads every database every render
   (`OSMScoutClient.cpp:1560-1563`), so it would stay unbounded, and Demos, the `Tiler` tool,
   `OSMScoutOpenGL` and tests would stay unbounded as well.
2. **A library-level singleton budget** — no plumbing. Rejected: hidden global state; explicit
   passing matches the project's style of manually constructed objects, and the library already
   rejects globals in this area.

Risk: the map service must not mistake "no data returned" for "not relevant" — a relevant database
whose type set matches nothing in the view returns no tiles. Mitigation: relevance is computed from
the geographic extents, never from the number of tiles returned.

### Decision D: Enforcement — caps by resize, release by flush

Chosen: the distribution sets cache caps (`DataTileCache::SetSize()` exists; a data file gains a
size setter that takes `accessMutex` and calls `Cache::SetMaxSize()`), and a database that stays out
of view for the idle period has its caches released through `Database::FlushCache()`.

Alternatives:

1. **Caps only** — smooth resumption, floor content kept. Rejected as the only mechanism: a cap bounds
   the worst case but does not return the memory an idle database already holds.
2. **Flush only** — no new resize API. Rejected as the only mechanism: no floor, so every database
   that leaves the view reloads from disk, and in-view databases cannot be given a larger share.

Risk: two thresholds (settling and idle) that can fight each other. Mitigation: they are ordered
constants with the idle period strictly longer than the settling period, and both are documented in
one place. Risk: a resize while a load is in flight. Mitigation: resizing happens under the locks the
caches already hold (`MapService::stateMutex`, `DataFile::accessMutex`), and `Cache::SetMaxSize()`
strips the oldest entries only.

### Decision E: The budget is opt-in at library level, on by default in the shipped clients

Chosen: no budget configured means today's behaviour. The Qt builder and the Java client configure a
default budget; tools, demos and tests are unchanged.

Alternatives:

1. **A library default budget** — everything bounded, no client work. Rejected: it would silently
   change `PerformanceTest`, which compares cache configurations deliberately
   (`PerformanceTest.cpp:865-882`, `:1055`, `:1080`), `TileDataConversionPerformanceTest`, the
   `Tiler` tool and the demos, and would invalidate existing measurements.
2. **Opt-in everywhere including clients** — the smallest possible change. Rejected: the mobile
   ceiling would then exist only where an application sets it, which the shipped applications are
   exactly the ones that must not have to.

Risk: a client forgets to configure a budget and mobile memory stays unbounded. Mitigation: the
default lives in the client builder and in the client's own default, and a scenario in the spec covers
it.

### Decision F: Budget ownership and constants

Chosen: one explicit budget object created by the client and shared with every map service of that
client (an optional argument of the map service constructor, so existing construction sites keep
working); `DBThread` owns it for the Qt and Java clients, since it already owns the open databases.

Proposed constants, all in one place, all tunable without a spec change:

- weights: point 1, line 3, polygon 30, route 3, index cell 1; bytes per weight unit derived from the
  object layouts (see Context).
- floor: a few tiles per database and a small share of each object cache kind; the basemap gets a
  higher floor than a region database because it is the only source at low zoom.
- settling period for the distribution, and a strictly longer idle period for the release.

### Decision G: The Java client's per-database cache size keeps its meaning without a budget

Chosen: the existing per-database setting keeps bounding a single database when no budget is
configured, and the Java bridge stops applying it to every database on every render
(`OSMScoutClient.cpp:1543-1551`); a new budget setting is added next to it.

Alternative: reinterpret the existing setting as the total budget. Rejected: it would silently change
the meaning of a documented Java API parameter and contradict
`JavaScout/src/test/java/com/framstag/libosmscout/client/OSMScoutClientDataCacheSizeTest.java`.

## Flow

Budget reaction to a view change (relevance of each database, settling, idle release):

```
  render/load job            map service (per db)            budget
  ----------------           --------------------            ------
        |                             |                       |
        |-- lookup tiles (proj) ----->|                       |
        |                             |-- intersects(dbBox, view area)?
        |                             |                       |
        |                             |-- report relevance -->|
        |                             |                       |-- relevant set stable
        |                             |                       |   for settling period?
        |                             |<-- shares / floors ---|
        |                             |-- tile cache resize --|     (tier 1)
        |                             |-- object cache resize |     (tier 2)
        |                             |                       |
        |   (view unchanged for idle period)                  |
        |                             |<-- release -----------|
        |                             |-- flush databases caches (tier 1 + 2)
```

Tier 2 resize path (no new file format, no reopen):

```
  budget --> MapService --(DatabaseRef)--> Database::SetDataCacheSizes()
                                              |
                          +-------------------+--------------------+
                          |                                        |
              parameter.Set*DataCacheSize()          for each already created file:
              (Database.h:106-110)                   DataFile::SetCacheSize()
              -> files created later inherit           -> accessMutex (DataFile.h:91)
                 it (Database.cpp:341)                 -> Cache::SetMaxSize (Cache.h:244)
                                                          -> strips oldest entries
```

## Risks / Trade-offs

- [Weight drift with the data] → documented weight table derived from layouts, accounting reported,
  interface unchanged if weights become measured sizes later.
- [Extra disk reads for databases at the floor] → floor constant, settling period, and the existing
  slow-load warnings (`MapService.cpp:1330`) as the observable signal; `PerformanceTest --flush-cache`
  measures the no-cache case for comparison.
- [Two thresholds interfering] → idle period strictly longer than settling period, single constants
  header, scenario "A stable change of the relevant set is applied" and "Long idleness releases the
  content" pin the behaviour.
- [Resize during an in-flight load] → resizing under the existing cache locks; `SetMaxSize()` only
  strips.
- [Reported usage is an accounting figure, not heap] → stated in the spec as "derived from the
  accounting" and in the documentation; the demo `Demos/src/ResourceConsumption.cpp` stays the tool
  for real memory measurements.
- [Java API meaning] → new setting next to the existing one, existing test unchanged (Decision G).
- [Bounding changes rendering] → prevented by the spec requirement that the rendered objects are the
  same; tests compare object sets with a tiny and a huge budget.

## Migration Plan

Behaviour-preserving by default, so no migration is required for existing consumers.

- Library first: accounting, budget object, relevance and distribution, resize and release plumbing;
  no consumer changes behaviour yet because no budget is configured.
- Then the clients: Qt builder and Java client configure a default budget; the Java bridge drops the
  per-render application of the per-database size.
- Rollback: clearing the configured budget returns the previous behaviour for both clients, and no
  file format or database content changes, so an older library reads the same data.

## Open Questions

- None that affect the specs, the approach or the task breakdown. The constants (weights, floors,
  settling and idle periods, client defaults) are chosen in one place and tuned by measurement; if a
  measured value contradicts the chosen default, only the constant changes, not the contract.
