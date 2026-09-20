## Context

See `proposal.md` - Why for the motivation. This section records the current state, the
measured baseline and the constraints that shape the approach.

### Where the step sits

`RenderSteps::ProcessAreas` (step 4 of 24, `libosmscout-map/include/osmscoutmap/MapPainter.h:92`)
runs once per frame, before any drawing step, and is backend-independent. Call chain:

```
MapPainter::ProcessAreas                 libosmscout-map/src/osmscoutmap/MapPainter.cpp:1281
  for each MapData, for each area (areas then poiAreas)
    MapPainter::PrepareArea              MapPainter.cpp:1218
      std::vector<CoordBufferRange> td(area->rings.size())          :1224   one vector per LOADED area
      per ring: TransformArea(...)                                  :1236   transforms every ring
      area->VisitRings(<lambda with 7 captures>)                    :1266   std::function temp per area
        MapPainter::PrepareAreaRing      MapPainter.cpp:1089
          std::vector<BorderStyleRef> borderStyles                  :1110   one vector per ring visited
          GetAreaFillStyle / GetAreaBorderStyles                    :1113/:1124
          IsVisibleArea(projection, ring bbox, borderWidth/2)       :1150   AFTER the transform
          areaData.push_back(a)                                     :1177   only for accepted rings
```

### Measured baseline (this machine, master, Dortmund database)

Cairo backend, `Tests/src/PerformanceTest.cpp`, draw-repeat 3:

```
step                     z14      z15      z16      z17
objects/tile  nodes       96      101      680      165
              ways      3018     3030     2035     1153
              areas     1802     8812     2747      917
ProcessAreas (ms/tile)  0.57     2.49     0.95     0.38
share of frame CPU       8%      20%       9%       7%
```

Wide view (304 tiles at z15, 17629 areas and 6308 ways loaded per tile): `ProcessAreas`
4.43 ms/tile = 27% of frame CPU (16.69 ms/tile), and 26720784 of 28700000 draw allocations
(93%). Backend comparison on the same view: noop 4.48, Qt 4.22, cairo 4.43 ms/tile - the step
is backend-independent. Data load is 4.33-5.57 ms/tile for the same views (it stays out of
scope, see Non-Goals).

Attribution inside the step (from `TODO.md` and the allocation counters): about one third of
the per-frame allocations of the step come from the per-area `td` vector, about two thirds
from the per-ring work in the visit, while only ~4% of the loaded areas produce a prepared
entry (725 of 17629 in the wide view).

### Constraints

- **Output identity.** Draw order of prepared areas comes from `areaData` order plus
  `std::stable_sort`, so the *preparation order* of equal-comparing areas is observable
  (established by `map-painter-frame-containers`, design D3). The new code must emit entries
  in the same order and must not change which rings are accepted.
- **Clipping rings.** Ignore-typed inner rings are not drawn but their coordinate ranges are
  copied into the parent's prepared entry (`MapPainter.cpp:1160-1166`,
  `AreaData::clippings`, `MapPainter.h:245`). They therefore need valid ranges even though
  no style resolves for them.
- **Visit semantics.** `Area::VisitRings` (`libosmscout/src/osmscout/Area.cpp:608`) walks
  rings breadth-first by ring depth and uses the visitor's return value as the "descend
  deeper" signal (`foundRing |= visitor(...)`, `Area.cpp:627`). `PrepareAreaRing` returns
  `true` for ignore-typed rings and for accepted rings.
- **`Area::VisitRings` takes `const std::function<...>&`** and the visitor type is public
  (`Area.h:234`). The closure built at `MapPainter.cpp:1266` captures seven values (~56
  bytes), which exceeds the `std::function` small-buffer size on both libstdc++ and libc++,
  so it allocates per area.
- **`CoordBufferRange` is a copyable handle** (`coordBuffer*`, `start`, `end`,
  `libosmscout/include/osmscout/util/Transformation.h:383`), so storing ranges in `AreaData`
  and reusing the range vector is safe; `CoordBuffer` itself keeps growing within a frame.
- **`StyleConfig::GetAreaBorderStyles` clears its output vector**
  (`libosmscout-map/src/osmscoutmap/StyleConfig.cpp:1386`) and then reserves; a reused vector
  with sufficient capacity does not allocate.
- **`IsVisibleArea` needs no transformed geometry** - it projects `ring.GetBoundingBox()` and
  expands by `borderWidth/2` (`MapPainter.cpp:239`, called at `:1150`).
- **Painter ownership.** `MapPainter` already owns per-frame state (`areaData`, `transBuffer`,
  `coordBuffer`); it is used by one render thread at a time (e.g.
  `libosmscout-client-qt/include/osmscoutclientqt/MapRenderer.h:122` holds one painter per
  renderer). Existing members already preclude sharing, so new scratch state changes nothing
  about threading.
- **noop baseline caveat.** `MapPainterNoOp::RegisterRegularLabel` does nothing and
  `GetFontHeight` is a constant (`libosmscout-map/src/osmscoutmap/MapPainterNoOp.cpp:35,50`),
  so noop cannot see label registration, text measurement or font work. The step ranking that
  made area preparation look like the single biggest item came from noop runs; it holds for
  area-heavy views (z15) and not for z16/z17, which is why this change is scoped to the area
  path only.

## Goals / Non-Goals

**Goals:**

- Remove heap work in the area preparation step that is proportional to the *loaded* area
  count, leaving a per-frame allocation count bounded by a constant.
- Skip transforming ring geometry that the frame cannot draw (invisible or unstyled rings).
- Keep the prepared area set, the preparation order, the draw order and the rendered output
  identical.
- Keep the backend contract unchanged: no change to the prepared-area accessors or to what
  backends read.
- Make the win verifiable with existing instruments: the per-step allocation counter and the
  per-step timing of `Tests/src/PerformanceTest.cpp`.

**Non-Goals:**

- The label pipeline (`PrepareNodeLabels`, `PrepareAreaLabels`, `DrawLabels`) and font
  construction/caching - separate concern, dominant at z16/z17.
- Way path calculation (`CalculatePaths`) and way drawing.
- Cairo's area fill rasterization cost (`DrawAreas`, 11-21% of frame CPU) - real drawing work,
  not preparation.
- Rendering quality changes of any kind (no new culling rules, no changed visibility
  thresholds).
- `AreaData::clippings` container type change - already deliberately deferred by
  `map-painter-frame-containers` (design D5).
- Changing `Area::VisitRings`'s public signature or the core `Area` API.
- Data loading (`MapService::AddTileDataToMapData`, tile caches) and import wall time.

## Decisions

### D1: Reuse painter-owned scratch for the per-area and per-ring temporaries

Chosen: keep the per-ring range vector (`td`, `MapPainter.cpp:1224`) and the border style
vector (`borderStyles`, `:1110`) as painter members that are reused across areas and frames,
clearing per use.

Alternatives:

1. `thread_local` scratch buffers - avoids new members and stays correct if a painter were
   shared, but hides state from the object that owns the frame, and the painter already has
   instance state that forbids sharing, so it buys nothing. Risk: two painters on one thread
   would silently share buffers.
2. A per-frame arena that hands out spans - one allocation per frame instead of a reused
   vector, but introduces a new abstraction and makes the "constant allocation" requirement
   depend on arena growth behaviour. Risk: harder to reason about than a cleared vector.
3. Leave the vectors per area/ring - no win, the current state. Risk: none, but the
   requirement is not met.

Rationale: `CoordBufferRange` is a value handle and `GetAreaBorderStyles` clears its output,
so reuse cannot leak state between areas; the painter already owns frame-scoped state, so the
lifetime and threading story is unchanged.

### D2: Make the ring visitor closure non-allocating

Chosen: build the visitor from a single pointer to a local context struct, so the closure fits
the `std::function` small buffer and `Area::VisitRings` stops allocating per area.

Alternatives:

1. Templated `VisitRings` overload in `libosmscout` (visitor callable by reference) - removes
   the indirection and the small-buffer dependency, but adds public API to the core library and
   moves the breadth-first walk into a header for a map-layer concern. Risk: wider API surface,
   core header churn, and the existing `std::function` overload must be kept for compatibility.
2. Reimplement the ring walk inside `MapPainter` (rings, `GetRingType` and the visitor type are
   public, `Area.h:234,241,266`) - no core change, but duplicates the depth-ordering and
   `foundRing |= visitor(...)` semantics; a future fix in `Area` would not apply. Risk: silent
   divergence between two traversals.
3. Leave the closure as is - keeps a per-area heap allocation. Risk: requirement not met.

Rationale: option 1 needs core API agreement, option 2 duplicates traversal logic; the chosen
option is local to the map layer and keeps the public contract.

### D3: Decide styling and visibility before transforming, then transform only accepted rings

Chosen: for each ring, first resolve its fill and border styles and test visibility on the
untransformed ring bounding box; only rings that pass both are transformed, and the accepted
rings produce their prepared entries in the same ring visit order as today. The accepted
ring's range vector entry is filled at the point where the entry is built.

Alternatives:

1. Visibility-only prefilter (keep the current order of style lookup and transform, skip only
   invisible rings) - smaller change and lower risk, but the style lookup and the per-ring
   border style vector still run for every ring of every loaded area, and unstyled rings are
   still transformed. Risk: partial win, requirement on unstyled rings unmet.
2. Transform on demand inside the per-ring handler (pass a transform callback into the
   `PrepareAreaRing` equivalent) - keeps decision and emission adjacent, but mixes the decision
   with the "descend deeper" return value and makes the flow harder to follow. Risk: subtle
   ordering bugs in the ring walk.
3. Keep transforming everything and only remove the allocations (D1/D2 only) - simplest, but
   leaves the dominant part of the step (transform + style lookups on rings that are then
   discarded) untouched. Risk: requirement on transformed geometry unmet.

Rationale: `IsVisibleArea` does not need transformed coordinates, so moving the decision in
front of the transform is possible without changing its result, and it removes both the
transform work and the coordinate buffer growth for rings that are thrown away.

The frame's coordinate buffer is renumbered per frame and holds only the rings that take part
in it, so the coordinate range of a prepared entry is a handle onto that buffer whose indices
legitimately differ from the ones the previous implementation produced. The coordinates are the
contractual part; no consumer may interpret the indices of a prepared range across frames. All
range consumers (the draw steps and the backend post-preprocessing callback) read through the
prepared entries, so nothing depends on the indices.

### D4: Ignore-typed rings are always transformed

Chosen: keep transforming rings whose type is ignored, regardless of whether a style resolves,
because their ranges become the clipping ranges of the parent's prepared entry.

Alternatives:

1. Transform ignore-typed rings only once their parent outer ring is prepared - saves the
   remaining case, but couples the decision to ring depth and makes correctness depend on the
   walk order. Risk: a dropped clipping range changes rendered output.
2. Do not transform them and drop the clipping ranges - would fail the clipping-ring
   requirement and change output. Risk: visible holes disappear.

Rationale: clipping rings are a minority of rings, and keeping today's behaviour for them
removes the largest correctness risk from this change.

### D5: Emission order stays the ring visit order

Chosen: accepted rings push their prepared entries in the order the rings are visited today.

Alternatives:

1. Emit entries grouped per outer ring (collect then append) - marginally better locality, but
   changes the input order of the stable sort for nested rings. Risk: draw order changes for
   equal-comparing areas.
2. Sort within `PrepareArea` before appending - same risk, plus redundant work.

Rationale: the prepared order is observable (`map-painter-frame-buffers`, D3), so it must not
change.

### Flow

```
TODAY (per loaded area)
  PrepareArea
    td[] = new vector          alloc per area
    for each ring:  TransformArea ---> coordBuffer grows for every ring
    VisitRings(new std::function closure)   alloc per area
      PrepareAreaRing
        borderStyles = new vector            alloc per visited ring
        style lookup -> borders/fill
        IsVisibleArea(ring bbox, border/2)
        accept -> areaData.push_back
        reject -> drop (geometry already transformed, buffers already grown)

AFTER (per loaded area)
  reused scratch: td[], borderStyles
  PrepareArea
    VisitRings(closure from a context pointer)      no per-area allocation
      per ring:  master / <3 nodes       -> skip
                 ignore type             -> transform now (clipping range)
                 style lookup (reused borderStyles)
                 IsVisibleArea(ring bbox, border/2)
                 accept                 -> transform now, fill td[i], push entry
                 reject                 -> nothing transformed
```

## Risks / Trade-offs

- **[Rendered output changes for some areas]** The prefilter must reproduce today's accept
  decision exactly (same stylesheet lookup, same `borderWidth/2` visibility offset, same
  `td[i]` validity check) -> Mitigation: keep the same calls and order, add a test that
  compares the prepared area set and per-entry coordinate ranges against a recording from
  before the change on the test databases, and rely on the existing `MapPainter*Test` suite
  plus the PerformanceTest draw-level statistics as the output-identity guard.
- **[Clipping rings lost]** An ignore-typed ring that would have been transformed is skipped ->
  Mitigation: D4 keeps ignore-typed rings unconditional, plus a scenario-level test on an area
  with holes.
- **[Visibility decision differs because it now runs on untransformed data]** `IsVisibleArea`
  already uses only the ring bounding box and the projection, but the border width must be
  known before it runs, so style resolution must stay in front of it -> Mitigation: do not
  replace it with a coarser bounding-box test; keep the same function and argument.
- **[`std::function` small-buffer assumption in D2]** The closure must fit the implementation's
  buffer; an implementation could still allocate -> Mitigation: the allocation counter in
  `Tests/src/PerformanceTest.cpp` is deterministic and will show it; fall back to D2 option 1
  if it does not fit.
- **[Scratch buffers retain memory for the largest area seen]** A view with one very large area
  keeps its range vector capacity -> Mitigation: bounded by rings of a single area (not by type
  count or view size), consistent with the capacity-retention decision of
  `map-painter-frame-containers` (D4); note it in the code comment, do not add shrinking.
- **[Smaller coordinate buffer changes downstream read paths]** Fewer transformed rings mean
  `CoordBuffer` holds less data per frame; prepared entries hold ranges, and every consumer
  (draw steps, the SVG backend via `AfterPreprocessingCallback`) reads through those ranges ->
  Mitigation: no consumer may assume buffer offsets across frames; verify by running the full
  test suite, which covers the SVG backend read path.
- **[Measurement noise hides or fakes the win]** Total draw time varies ~14% between cairo runs
  on this machine -> Mitigation: make the allocation count of the step the primary acceptance
  signal (deterministic) and report wall time only with repeats, at z15 (worst case for this
  step) and across z14-z17 to show no regression elsewhere.
- **[Scope creep into the label path]** z16/z17 are dominated by labels, not by area
  preparation -> Mitigation: Non-Goals section; the change must show an allocation win at
  every zoom and a wall-time win at z15, and must not regress z16/z17.

## Migration Plan

No data, format or API migration. The change is internal to `libosmscout-map` and lands as one
commit set; rollback is a revert of that commit set (no database, style or client change is
involved). Measurements before and after use the same database and stylesheet, so the numbers
stay comparable.

## Open Questions

- Whether the clipping-ring rule (D4 option 1) is worth tightening later: only relevant if
  measurement shows ignore-typed rings are a noticeable share of the remaining transform work.
- Whether the reused scratch buffers should be bounded (released when a view shrinks) - the
  same question `map-painter-frame-containers` left open for the prepared stores; not needed to
  meet the requirements here.
