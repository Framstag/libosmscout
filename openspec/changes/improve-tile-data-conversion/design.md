# Design

## Context

See proposal.md - Why. This section only holds the current state and the constraints the approach has
to respect.

**Where the conversion sits.** `MapService::AddTileDataToMapData` has two entry points,
`MapService.h:380` (`tiles` + `MapData`) and `MapService.h:383` (`tiles` + `TypeDefinition` +
`MapData`). All 16 call sites in the repository use the two-argument form; the type-filtered form has
no caller. The hot path is `libosmscout-client-qt/src/osmscoutclientqt/MapRenderer.cpp:353`, once per
render job per database (`MapRenderer.cpp:333-353` loops the loaded databases), reached from
`DBLoadJob.cpp:200` and `IconLookup.cpp:204`; `libosmscout-client-java/src/OSMSCoutClient.cpp:1521`
and `:1585` call it from the JNI bridge.

**What the implementation this change replaces did** (`MapService.cpp:1194-1278` regular, `:1283-1367`
filtered, revision `origin/master`). Six hash maps (`unordered_map<FileOffset, ...>`, four of them
constructed with 10000 buckets, two with 1000), one insertion per object of every tile, then a second
pass that copies the maps into the four `MapData` vectors. The deduplication phase was timed into a
commented-out stream (`MapService.cpp:1240`); only the copy phase warned when it exceeded 20 ms
(`:1275`).

**Constraint: an offset does not identify an object across data files.** `FileOffset` is an offset
within one data file, and the regular and the optimized data of a kind are read from different files -
`ways.dat` versus `waysopt.dat` (`OptimizeWaysLowZoom.cpp:36`), `areas.dat` versus `areasopt.dat`
(`OptimizeAreasLowZoom.cpp:36`). Two objects of the same kind, one from each file, can carry the same
offset value. The previous code kept `wayMap` and `optimizedWayMap` (and the area pair) in separate
maps, so the two offset spaces never met. Any deduplication has to keep that separation.

**Constraint: the order of `MapData` is observable in the rendered image.** A backend stable-sorts its
prepared ways and areas (`libosmscout-map/src/osmscoutmap/MapPainter.cpp:2375-2379`), so the sequence
of `MapData` is the tiebreaker for objects with equal sort keys. In the previous implementation that
tiebreaker was the iteration order of a hash map, which is implementation- and library-dependent.

**Constraint: a test cannot fabricate an object with a chosen offset.** A file offset is assigned
inside `Node::Read`, `Way::Read` and `Area::Read` from the scanner position (`Node.cpp:51`,
`Way.cpp:82`, `Area.cpp:187`), and there is no setter anywhere in the API. The tests of this change
therefore load a real view of a real database and read the objects of that view back.

**Constraint: the tile sets of the tests are small.** The repository carries one database,
`Tests/data/testregion` (a rural extract of about 6 km x 5 km, imported 2026-07-05). A view of it at
zoom 15 holds about 1600 distinct objects in 56 tiles, while a production view (measured on a local,
untracked Dortmund database) holds about 13800 distinct objects in 16 tiles. The mechanism was
compared on the test database and verified end to end on the production-shaped one; both numbers are
recorded in `verification.md`.

## Goals / Non-Goals

**Goals:**

- The per-conversion cost follows the objects of the tiles and does not grow with the tiles that repeat
  them.
- The result holds every object once, with a defined and reproducible order, including the case of the
  two data files of a kind that share an offset value.
- Each phase of the conversion is separately observable and the threshold of the report is settable.
- A conversion that became slower fails a test, and the structural cost is pinned without timing.

**Non-Goals:**

- The painter's prepared per-frame data (`map-painter-frame-buffers`, `map-painter-way-culling`,
  `map-painter-area-preparation` define that step; its input changes its order, not its shape).
- The tile cache, its eviction and its prefill policy; the conversion consumes what it is given.
- The per-database loop of the Qt client (`MapRenderer.cpp:333-353`), which multiplies the conversion
  by the number of loaded databases - tracked separately in TODO.md.
- Removing the type-filtered entry point (`MapService.h:383`); see decision D3.
- The import pipeline, the map database format and the stylesheets: unchanged.
- Any change to a public header. The design first added group bookkeeping and a read guard to
  `DataTileCache.h` so a conversion could merge sorted runs; the measurement in decision D1 removed the
  need for both, and they were reverted.

## Decisions

### D1 - Deduplication: a flat table of the offsets already seen

Chosen: every source data file keeps the offsets it has seen in a hash set with linear probing
(`FileOffsetSet` in `MapService.cpp`), the object appended to the result on its first sight. The table
holds the offset plus one, so zero marks a free slot, starts at 1024 slots and doubles when it is
three quarters full: it allocates its table when it grows, not a node per object.

Alternatives, all measured in one run on the test database (`verification.md` holds the table):

- *Merge the ascending groups of the tiles through a heap over their heads* (the design's original
  choice, implemented and measured). It allocates nothing per reference and yields an ascending order
  as a by-product, but it performs a heap operation - `O(log runs)` with a 24-byte record moved per
  swap - per **object reference**, while a hash lookup costs a few nanoseconds. It therefore lost to
  the previous implementation on exactly the tile sets this change is about: with 90 tiles and a
  duplication factor of 13 it needed 2.3x the time of the hash maps, and only the smallest tile sets
  were faster. Rejected on that measurement.
- *Keep the six hash maps and fix only the cheap items* (reserve from the tiles, a threshold on the
  deduplication phase). This is the "at minimum" variant of the TODO entry; it leaves the per-object
  hashing, the per-object allocation and the second pass in place.
- *A node-based `unordered_set` per source, the object appended on first sight.* The same per-reference
  cost as the chosen table, but it allocates a node per distinct object and rehashes from an empty
  table while the maps it replaces pre-allocate 10000 buckets, so it was *slower* than the previous
  implementation on the large tile sets (up to 1.18x) and only won on the small ones. Measured and
  rejected; it was the step between the merge and the flat table.

Rationale: the flat table has the per-reference cost of a hash lookup (what the previous
implementation's maps did well), removes the second pass and the per-object allocation (what they did
badly), and needs no public header change.

### D2 - Order of the result: grouped by source data file, then the order the tiles present

Chosen: the result of a kind holds the objects of its regular data file first and the objects of its
optimized data file second, and within a source data file the objects follow the order in which the
tiles of the given list and their stored data present them. `MapData` is handed over by the caller, so
a second conversion into the same instance appends behind the first.

Alternatives:

- *Ascending by file offset within a source* (the design's original choice). It needs the merge, which
  loses the speed gain (D1), and it made the result depend on the offsets rather than on the tile list.
- *Leave the order unspecified.* Then the contract cannot be asserted, and the painter's tiebreaker
  stays a library detail. The previous implementation was in this position; the change removes that
  hidden dependency without paying for a sort.

Rationale: the order is now a function of the input the caller hands in - the same tile list produces
the same sequence on every platform - which is what the painter's tiebreaker needs, and it costs
nothing.

### D3 - The type-filtered entry point: one core, no removal here

Chosen: both entry points run the same core conversion; the filtered variant skips an object whose
type is not in the requested set while it walks the tiles, so a filtered conversion keeps the same
uniqueness and order guarantees.

Alternatives:

- *Delete `MapService.h:383`.* It is exported (`OSMSCOUT_API`) and could have out-of-tree consumers;
  deleting it is a **BREAKING** API change that widens this change for no conversion-cost gain, and it
  needs its own release note.
- *Keep it and mark it deprecated.* Keeps a second contract alive longer and defers the same removal to
  a later change with no benefit inside this one.

Rationale: the spec requires every entry point to hold the same contract, which is satisfied either
way; the removal question stays open (see Open Questions) without blocking this change.

### D4 - How the speed gain is asserted: a baseline in the test, over several tile sets

Chosen: `Tests/src/TileDataConversionPerformanceTest.cpp` carries the previous conversion as its own
baseline, converts eight tile sets through the baseline and through the conversion in one run, keeps
the fastest run of each side for every tile set, and fails when the conversion needs more than 0.9 of
the baseline for one tile set or more than 0.8 of the baseline over all of them. The tile sets differ
in their zoom level and their extent, because the two mechanisms behave differently over the number of
tiles and the duplication factor (D1). The structural cost is pinned separately in
`Tests/src/TileDataConversionTest.cpp`: a conversion allocates at most one block per distinct object
plus a budget, and converting a tile set whose objects are all carried twice does not allocate more
than converting it once.

Alternatives:

- *Wall clock only, over one tile set.* The tile sets where the conversion wins most are the production
  shaped ones, which the repository's test database cannot produce; a single small tile set would have
  failed the assertion under the merge and passes it now with little headroom (D1's table).
- *Counters only.* Proves the structure changed but not that the step got faster; the speed gain is what
  the change is made for.
- *An absolute time budget.* Machine-dependent; the same conversion is fast on a desktop and slow on a
  phone.
- *A recorded duration from an earlier run.* The same work varied by a factor of 2.4 between runs on
  the machine that produced the baseline, so a cross-run duration comparison is not a verification
  (see `verification.md`).

Rationale: the baseline stays in the test, so the assertion survives the old code being deleted from
the library; the pair of margins covers both "each tile set is not slower" and "the change is a gain",
and the structural assertions cover the property that timing cannot pin. The test is registered the way
`PerformanceTest` is registered, so the sanitizer job excludes it by the same substring match, and Meson
does not register it in a build instrumented for coverage, where the instrumentation - it slows the
library but not the standard library the baseline uses - would decide the comparison.

### D5 - Phase observability: one phase per source data file, one configurable threshold

Chosen: a conversion measures one phase per source data file it converts - `nodes`, `ways`,
`optimized ways`, `areas`, `optimized areas` and `routes` - and logs one warning naming a phase whose
duration exceeds the threshold. The threshold is a single value on `MapService`
(`SetConversionPhaseWarningThreshold()`, `GetConversionPhaseWarningThreshold()`), defaulting to 20
milliseconds.

Alternatives:

- *Two phases, collection and merge.* The merge is gone (D1); the walk of the tiles and the
  deduplication are one pass, so there is nothing to measure separately.
- *A threshold per phase, hardcoded.* The value the previous implementation warned at was hardcoded
  (`MapService.cpp:1275`, > 20 ms), and a hardcoded threshold cannot be forced by a test without a
  workload large enough to exceed it on every machine.
- *Log every phase of every conversion.* Noise on every pan step in a log that is already busy; the
  spec requires no warning below the threshold.

Rationale: naming the source data file attributes a slow conversion better than naming "collect" and
"merge" did, because the sources differ by an order of magnitude in their object counts. The default
keeps the threshold value of the warning the previous implementation had. This also answers the open
question about configurability: the value is configurable on the service that performs the conversion,
not through `MapParameter`, which the conversion does not receive.

## Sequence diagram

```
MapRenderer (pan/zoom step, MapRenderer.cpp:353)
  |
  |  tiles: list<TileRef>, each tile with node/way/area/route data of
  |         its regular and its optimized source data file
  v
AddTileDataToMapData(tiles, data)
  |
  +-- reserve    data.nodes / ways / areas / routes
  |              from the sum of the tiles' GetDataSize() per kind
  |
  +-- per source data file (nodes, ways, optimized ways, areas,
      optimized areas, routes) - each with its own FileOffsetSet:
  |
        for each tile:
          tile->GetXData().CopyData(object ->)
            filter: not requested type -> skip
            seen.Insert(offset)?
              yes (first sight) -> out.push_back(object)
              no  (repeat)      -> nothing, one lookup
  |
  +-- report     one StopClock per source, warn per phase over the threshold
  |
  v
data.nodes, data.ways, data.areas, data.routes
  |
  v
MapPainter::DrawMap -> AfterPreprocessing stable_sorts prepared ways/areas
                        (MapPainter.cpp:2375-2379: MapData order is the tiebreak)
```

## Risks / Trade-offs

- **The seen table holds an offset per distinct object of a source.** A production-shaped view of
  13800 distinct objects holds about 200 KB of tables (the table doubles to about 16 k slots of 8
  bytes per source), against the six 10000-bucket maps of the previous implementation, which alone
  allocated about 480 KB of bucket arrays per conversion. The conversion therefore uses less memory,
  not more, but the memory is held until the conversion returns rather than per map.
- **The order now depends on the tile list the caller hands in.** A caller that hands in the same
  tiles in another order gets another sequence. The previous implementation's order depended on the
  standard library instead, which no caller could control; D2 makes the dependency explicit and
  documents it on the API.
- **The seen table stores the offset plus one, so an object stored at offset zero would collide with a
  free slot.** The data files start with a header, so no object is stored at offset zero; this is a
  documented assumption, not a checked one.
- **The test database cannot produce production-shaped tile sets.** The comparison therefore asserts
  the gain over eight small tile sets and pins the production-shaped number in `verification.md` from a
  local, untracked database. A change that only regressed at production scale would not be caught by
  CI; the structural assertions (per-reference allocation, no growth with repeats) are the part of the
  guard that does not depend on the size of the data.
- **The margins are measured with headroom on one machine.** The worst tile set measured 0.82 against a
  margin of 0.9; a machine whose hash maps are much cheaper relative to the flat table could flip it.
  The margins are named constants in the test, so the reaction is to re-measure and adjust them with
  the reason recorded, not to delete the assertion.
- **A test that captures the log must be thread-safe.** `MapService` loads tiles on worker threads, so a
  capturing `Logger::Destination` is written to from more than one thread; the destination of the test
  guards its output with a mutex. A capturing destination without that guard corrupted its output
  string and made the test fail in three different ways (a hang, a segmentation fault, a failed
  assertion) at random - a defect of the test, found while implementing this change, not of the
  library.
- **Objects of a kind that a tile holds in several groups are visited in the group order.** The order
  within a source therefore also depends on how the loader filled the tile (carried-over data first,
  then the data the tile loaded itself), which is not part of the contract but is stable per database.

## Migration Plan

No data migration and no database format change: the conversion is internal to `libosmscout-map`, the
entry point signatures are unchanged, `MapData` keeps its shape and no public header changed. Rollback
is a revert of the commit; nothing persists. Consumers need a recompile only because of the new public
method on `MapService` (the threshold setter, additive).

## Open Questions

- Is a type-filtered conversion still wanted at all, given `MapService.h:383` has no caller in the
  repository? The specs hold either way; removing it is a separate **BREAKING** change.
- Should the conversion telemetry be extended to the per-database loop of the Qt client, so a
  multi-database rebuild attributes its cost? Tracked separately in TODO.md.

The question about configurable phase thresholds was answered during implementation: they are
configurable on `MapService` (D5); the value is not exposed through `MapParameter`, because the
conversion does not receive a map parameter.
