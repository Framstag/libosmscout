# Design

## Context

See proposal.md - Why. This section only holds the current state and the constraints the approach has
to respect.

**Where the conversion sits.** `MapService::AddTileDataToMapData` has two entry points,
`MapService.h:380` (`tiles` + `MapData`) and `MapService.h:383` (`tiles` + `TypeDefinition` +
`MapData`). All 16 call sites in the repository use the two-argument form; the type-filtered form has
no caller. The hot path is `libosmscout-client-qt/src/osmscoutclientqt/MapRenderer.cpp:353`, once per
render job per database (`MapRenderer.cpp:333-353` loops the loaded databases), reached from
`DBLoadJob.cpp:200` and `IconLookup.cpp:204`; `libosmscout-client-java/src/OSMScoutClient.cpp:1521`
and `:1585` call it from the JNI bridge.

**What the current implementation does** (`MapService.cpp:1194-1278` regular, `:1283-1367` filtered).
Six hash maps (`unordered_map<FileOffset, ...>`), one insertion per object of every tile, then a
second pass that copies the maps into the four `MapData` vectors, reserving from the map sizes. The
deduplication phase is timed but its result is only written to a commented-out stream
(`MapService.cpp:1240`); only the copy phase warns when it exceeds 20 ms (`:1275`).

**What a tile holds.** `TileData<O>` keeps `prefillData` (objects carried over from parent tiles) and
`data` (objects the tile loaded itself), and `CopyData` visits `prefillData` first and `data` second
(`DataTileCache.h:226-232`). `GetDataSize()` returns the sum of both sizes (`:219`). `AddPrefillData`
and `AddData` each append, so a tile can hold several concatenated groups, not just two.

**Group ordering.** Before a tile reads its objects, the loader sorts the offsets ascending "to
optimize disk access" (`MapService.cpp:288` for nodes, `:460` for areas and ways). Each group a tile
receives is therefore ascending by offset.

**Constraint: an offset does not identify an object across data files.** `FileOffset` is an offset
within one data file, and the regular and the optimized data of a kind are read from different files -
`ways.dat` versus `waysopt.dat` (`OptimizeWaysLowZoom.cpp:36`), `areas.dat` versus `areasopt.dat`
(`OptimizeAreasLowZoom.cpp:36`). Two objects of the same kind, one from each file, can carry the same
offset value. The current code keeps `wayMap` and `optimizedWayMap` (and the area pair) in separate
maps, so the two offset spaces never meet.

**Constraint: the order of `MapData` is observable in the rendered image.** A backend stable-sorts its
prepared ways and areas (`libosmscout-map/src/osmscoutmap/MapPainter.cpp:2375-2379`), so the sequence
of `MapData` is the tiebreaker for objects with equal sort keys. Today that tiebreaker is the iteration
order of a hash map, which is already implementation- and platform-dependent.

**Constraint: `TileData` is a header-only template** in the public header
`libosmscout-map/include/osmscoutmap/DataTileCache.h`.

## Goals / Non-Goals

**Goals:**

- The per-conversion cost follows the distinct objects of the result, not the objects of all tiles.
- The result sequence is defined, reproducible and specifiable, including the case of two data files
  of one kind that share an offset value.
- Each phase of the conversion is separately observable.
- A conversion that became slower fails a test, and the structural cost is pinned without timing.

**Non-Goals:**

- The painter's prepared per-frame data (`map-painter-frame-buffers`, `map-painter-way-culling`,
  `map-painter-area-preparation` define that step; its input only changes its order).
- The tile cache, its eviction and its prefill policy; the conversion consumes what it is given.
- The per-database loop of the Qt client (`MapRenderer.cpp:333-353`), which multiplies the conversion
  by the number of loaded databases - tracked separately in TODO.md.
- Removing the type-filtered entry point (`MapService.h:383`); see decision D3.
- The import pipeline, the map database format and the stylesheets: unchanged.

## Decisions

### D1 - Deduplication: merge the existing groups instead of hashing

Chosen: the conversion merges the ascending groups of a data file directly, comparing the head of each
group, emitting the smaller head, and emitting an offset equal to the already emitted one only once. No
per-object container, no second pass.

Alternatives:

- *A seen-offset set per data file, appended on first sight.* One pass, one allocation per distinct
  object, but every (tile, object) pair is still hashed and the result has no defined order, which
  forces option D2-C.
- *Keep the six hash maps and only fix the cheap items* (reserve from the tiles, threshold on the
  deduplication phase). This is the "at minimum" variant of the TODO entry; it leaves the per-object
  hashing, the per-object allocation and the second pass in place, which is exactly the cost this
  change exists to remove.

Rationale: the ascending groups already exist because the loader sorts before reading; a merge adds no
sort pass, allocates only the group heads, and yields the defined order as a by-product.

### D2 - Order of the result: grouped by data file, ascending by offset within a group

Chosen: the result of a kind is grouped by its source data file in a fixed order (regular data first,
optimized data second, as today) and ascending by offset inside each group.

Alternatives:

- *Leave the order unspecified.* Then the contract cannot be asserted, and the tiebreak stays a
  standard-library detail; the spec's reproducibility scenario would have nothing to check.
- *Assemble unsorted and `std::sort` the vectors at the end.* Deterministic, but adds an O(n log n)
  sort per pan step - the cost this change removes - and a sort by offset alone would still be wrong
  across two data files of one kind.

Rationale: the order is free from the merge, it is portable, and it makes the painter's tiebreak
reproducible instead of platform-dependent.

### D3 - The type-filtered entry point: one core, no removal here

Chosen: both entry points run the same core conversion; the filtered variant skips an object whose
type is not in the requested set while merging, so a filtered conversion keeps the same uniqueness and
order guarantees.

Alternatives:

- *Delete `MapService.h:383`.* It is exported (`OSMSCOUT_API`) and could have out-of-tree consumers;
  deleting it is a **BREAKING** API change that widens this change for no conversion-cost gain, and it
  needs its own release note. Worth a separate decision, not a silent inclusion.
- *Keep it and mark it deprecated.* Keeps a second contract alive longer and defers the same removal to
  a later change with no benefit inside this one.

Rationale: the spec requires every entry point to hold the same contract, which is satisfied either
way; the removal question stays open (see Open Questions) without blocking this change.

### D4 - How the speed gain is asserted: an in-test baseline plus a structural assertion

Chosen: `Tests/src/TileDataConversionPerformanceTest.cpp` carries the current algorithm as its own
baseline, converts the same generated tile set through the baseline and through the conversion in one
run, and fails when the conversion is slower than `baseline * margin`. The margin is a named constant
in the test. A second, deterministic assertion in `Tests/src/TileDataConversionTest.cpp` counts the
allocated memory blocks and requires them to stay within the distinct object count plus a constant.

Alternatives:

- *Wall clock only.* Flaky on a shared CI runner, and it produces no number that can be compared
  between platforms.
- *Counters only.* Proves the structure changed but not that the step got faster; the user asked for a
  gain that is asserted, so a duration comparison is required.
- *An absolute time budget.* Machine-dependent; the same conversion is fast on a desktop and slow on a
  phone.

Rationale: the baseline stays in the test, so the assertion survives the old code being deleted from
the library; the pair covers both the "is it faster" and the "did the structure change" question. The
test is registered the way `PerformanceTest` is registered, so the sanitizer job can exclude it the
same way it already excludes the leak-prone performance tests.

### D5 - Phase observability: one threshold per phase

Chosen: each phase of the conversion (group collection, merge, and the existing copy where it still
applies) is timed with its own threshold and logs one warning naming the phase, replacing the current
commented-out timing output.

Alternatives:

- *One threshold for the whole conversion.* Cannot name the slow phase, so the spec's attribution
  scenario has nothing to match.
- *Log every phase of every conversion.* Noise on every pan step in a log that is already busy; the
  spec explicitly requires no warning below the threshold.

Rationale: the copy phase already warns at 20 ms; per-phase thresholds keep that behaviour and make the
merge attributable. Thresholds are sized from the baseline measurement recorded in `verification.md`.

### D6 - How the conversion obtains the ascending groups of a tile

Chosen: `TileData<O>` records the boundaries of the groups it holds (it already appends them one by
one) and offers additive, const access to them, so the conversion can walk the groups of all tiles.

Alternatives:

- *Infer the boundaries while iterating through `CopyData`* - a group ends where the offset stops
  ascending. No header change at all, but it turns an undocumented ordering invariant into the
  contract, it cannot be asserted by the cache itself, and a future loader that stops sorting would
  silently corrupt the merge.
- *Copy each tile's objects into per-tile sorted vectors first.* Allocation per object, which violates
  the spec's allocation bound, and it is the same hashing/sorting cost in a different place.

Rationale: the boundaries are data the cache already produces; exposing them makes the ordering
invariant explicit. `TileData` is a header-only template, so additive const accessors are
source-compatible; the cost is a recompile of the consumers of `libosmscout-map`.

Locking note: a merge needs the heads of the groups of all tiles at once, so the conversion holds the
tiles' mutexes for the duration of the merge, acquired in tile-list order to exclude a deadlock.
Nothing else holds more than one tile mutex at a time, and the merge only uses const access.

## Sequence diagram

```
MapRenderer (pan/zoom step, MapRenderer.cpp:353)
  |
  |  tiles: list<TileRef>, each tile with node/way/area/route data
  |         (prefillData groups ..., data groups ...)   [DataTileCache.h]
  v
AddTileDataToMapData(tiles, data)
  |
  +-- 1 collect   for each kind and source data file:
  |                 gather the ascending group boundaries of every tile
  |                 (prefillData groups first, then data groups)
  |               ways.dat  + waysopt.dat
  |               areas.dat + areasopt.dat
  |               nodes.dat            routes.dat
  |
  +-- 2 reserve   data.nodes / ways / areas / routes
  |               reserve from the sum of the tiles' GetDataSize()
  |
  +-- 3 merge     per data file: k-way merge over the group heads,
  |               head[smallest offset] -> pushed into the vector,
  |               head[equal to the last emitted offset] -> skipped
  |               (an offset is compared only within one data file)
  |                                                    -> ascending, unique
  |
  +-- 4 report    one StopClock per phase, warn per phase over its threshold
  |
  v
data.nodes, data.ways, data.areas, data.routes
  |
  v
MapPainter::DrawMap -> AfterPreprocessing stable_sorts prepared ways/areas
                        (MapPainter.cpp:2375-2379: MapData order is the tiebreak)
```

## Risks / Trade-offs

- **Order changes the painter's tiebreak** (D2). `MapPainter.cpp:2375-2379` stable-sorts prepared ways
  and areas, so a rendered image can differ where two objects have equal sort keys. Today that
  tiebreak is already stdlib-dependent, so this change replaces a hidden dependency with a defined
  one; the difference still has to be seen. Mitigation: before/after render of the same view through
  `Demos/DrawMapCairo`/`DrawMapQt` on the Dortmund database, scanned with
  `.pi/skills/map-render-pixel-scan`; if a tie case shows up, the deterministic order is kept and the
  affected stylesheet is recorded, since the old order was not reproducible either.
- **A single merge across two data files would drop objects** (D1). `ways.dat:4711` and
  `waysopt.dat:4711` are different objects; a merge that treats offsets as globally unique drops one of
  them silently - and the TODO entry that motivated this change states that wrong assumption.
  Mitigation: merge per data file, the added spec scenario for two sources sharing an offset, and a
  unit test that builds exactly that case.
- **The public header changes** (D6). `DataTileCache.h` is included by consumers of `libosmscout-map`;
  the change is additive and const, so it is source-compatible, but every consumer recompiles.
  Mitigation: additive accessors only, no signature change, no behaviour change in the cache.
- **Lock scope of the merge** (D6). Holding the tile mutexes for the merge is a longer hold than
  `CopyData`'s per-tile visit. Mitigation: fixed acquisition order, const access only, and the tiles of
  a render job are not mutated while its conversion runs.
- **The speed test is timing-dependent** (D4). Mitigation: baseline and conversion measured in the same
  run on the same machine, a ratio margin instead of an absolute budget, and registration that mirrors
  `PerformanceTest` so the sanitizer job can exclude it.
- **Over-claiming the gain.** The Qt client calls the conversion once per database per rebuild
  (`MapRenderer.cpp:333-353`), so a whole-rebuild number also contains that multiplier, which this
  change does not touch. Mitigation: `verification.md` reports the per-conversion number and the
  per-rebuild number separately, and the per-database loop stays a separate TODO entry.
- **Log noise** (D5). Mitigation: thresholds sized from the recorded baseline so a normal step logs
  nothing; the "no warning below the threshold" scenario pins it.

## Migration Plan

No data migration and no database format change: the conversion is internal to `libosmscout-map`, the
entry point signatures stay as they are, and `MapData` keeps its shape. Rollback is a revert of the
commit; nothing persists. Consumers need a rebuild only because of the public header the tile cache
lives in. If the pixel scan shows a tie-case difference that is judged undesirable, the merge order of
D2 is the single place to change, without touching the spec's uniqueness or cost requirements.

## Open Questions

- Is a type-filtered conversion still wanted at all, given `MapService.h:383` has no caller in the
  repository? The specs hold either way; removing it is a separate **BREAKING** change.
- Should the conversion telemetry be extended to the per-database loop of the Qt client, so a
  multi-database rebuild attributes its cost? Tracked separately in TODO.md.
- Should the phase thresholds become configurable through `MapParameter`? The spec requires a
  threshold and attribution, not its configurability; deferrable.
