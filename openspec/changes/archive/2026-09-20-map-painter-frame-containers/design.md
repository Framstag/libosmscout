## Context

See `proposal.md` - Why for motivation and the requirements in
`specs/map-painter-frame-buffers/spec.md` for the contract this design must satisfy.

Current state (verified in the tree):

- `libosmscout-map/include/osmscoutmap/MapPainter.h:292-295` declares the four
  prepared per-frame stores as `std::list`: `areaData`, `wayData`, `wayPathData`,
  `routeLabelData`.
- Route labels keep a reference into the prepared way paths through
  `WayPathDataIt = std::list<WayPathData>::iterator`
  (`MapPainter.h:248`) stored in the public `RouteLabelData::wayData`
  (`MapPainter.h:253-257`).
- Frame lifecycle in `libosmscout-map/src/osmscoutmap/MapPainter.cpp`: every frame
  `clear()`s the stores (`:1285` `areaData`, `:1669-1671` `wayData`/`wayPathData`/
  `routeLabelData`, `:1732` `routeLabelData` again) and re-`push_back`s one node per
  object (`:1053`, `:1085`, `:1177`, `:1212`, `:1591`, `:1632`, `:1654`, `:1660`,
  `:1841`, `:1994`, `:2364`).
- `AfterPreprocessing` (`:2092-2103`) orders the stores with the node-based
  `std::list::sort`: `wayData.sort()` and `areaData.sort(AreaSorter)`. Both are
  stable. `AreaSorter` (`:104-144`) compares fill colours and bounding boxes and
  produces equivalent elements for identical bounding boxes with the same
  outer/inner ring role - the case the comment at `:126-131` describes.
- Prepared areas and ways are readable by backends through
  `GetWayData()`/`GetAreaData()` returning `const std::list<...>&`
  (`MapPainter.h:627-635`). The only in-repo reader is
  `libosmscout-map-svg/src/osmscoutmapsvg/MapPainterSVG.cpp:878,885` in
  `AfterPreprocessingCallback`.
- `AreaData::clippings` is a further `std::list` (`MapPainter.h:245`) but is read
  directly by many backends (GDI `MapPainterGDI.cpp:812-856`, AGG
  `MapPainterAgg.cpp:667-685`, Cairo `MapPainterCairo.cpp:1322-1339`, Skia
  `MapPainterSkia.cpp:1308-1309`, DirectX `MapPainterDirectX.cpp:1008`,
  `Android/OsmScoutLib/jni/src/jniMapPainterCanvas.cpp:700-704`).

Constraints: C++20, no GNU extensions, both CMake and Meson builds, Uncrustify and
clang-tidy clean, Catch2 tests, `PerformanceTest` excluded from the sanitizer CI job.

## Goals / Non-Goals

**Goals:**

- Prepared frame data is reused between frames and traversed contiguously, so per-frame
  allocation count and draw traversal cost stop growing with the visible object count.
- Draw order, geometry, styling and label placement are bit-for-bit unchanged, which
  keeps the existing rendering tests meaningful as regressions.
- The change is measurable before and after with the harness that already exists.

**Non-Goals:**

- No change to what is prepared, to the ordering criteria, or to any backend's drawing
  code.
- No change to tile loading, label layout or symbol drawing (separate levers tracked in
  `TODO.md`).
- No change to `AreaData::clippings`, whose type is read by six backends (see D5).
- No new public API beyond the necessary accessor adjustment.

## Decisions

### D1: Prepared stores become `std::vector` with retained capacity

Chosen: `std::vector<AreaData>`, `std::vector<WayData>`, `std::vector<WayPathData>`,
`std::vector<RouteLabelData>` in `MapPainter.h:292-295`. Per frame the stores are
emptied with `clear()` (which keeps capacity) and sized up front with `reserve()` from
a per-frame estimate, then filled.

Alternatives:

1. Keep `std::list` - rejected: node-per-object allocation and pointer-chasing
   traversal are exactly the cost this change removes; `clear()` frees every node.
2. `std::deque` - rejected for the main stores: chunked allocation removes the
   per-element malloc, but traversal still goes through a block map and sorting is not
   cheaper. It was only attractive for `wayPathData`, where push_back iterator
   stability matters (see D2); with D2 the reason disappears.
3. Flat arena / custom allocator - rejected: more machinery than the problem needs,
   and the element types contain non-trivial members (`std::optional`, `shared_ptr`,
   nested containers) that would need careful lifetime handling.

Risk per approach: vector reallocation moves elements, so any pointer or iterator into
an element must not outlive the growth. Verified: nothing stores such a pointer across
steps - labels copy values (`LabelData`, `PathLabelData`, `LabelPath`), and the shield
step registers labels from computed grid points (`MapPainter.cpp:909`,
`RegisterPointWayLabel` at `:321`) without touching the prepared stores. The single
explicit cross-step reference is the route label's way path, handled in D2.

### D2: Route labels reference their way path by index

Chosen: `RouteLabelData::wayData` becomes an index into the prepared way path store
(`MapPainter.h:248,255`), resolved through the store. Capture site
`MapPainter.cpp:1784-1786` and consumption sites `:1987-1994`, `:2665` are updated.

Alternatives:

1. Keep the list iterator - only viable while `wayPathData` stays a list, see D1.
2. `std::deque<WayPathData>` + iterator - smallest diff (alias change only) and
   push_back keeps iterators valid, but it keeps a chunked store for the one container
   with a documented need for stable references, and it hides the coupling instead of
   removing it.
3. Pointer or `shared_ptr` to the way path - rejected: raw pointers into a moving
   vector are exactly the hazard, and ownership semantics would change for a purely
   positional relationship.

Safety argument for the index: `wayPathData` is fully built in `CalculatePaths`
(`MapPainter.cpp:1665`, single push site `:1591` inside `CalculateWayPaths`) before
`ProcessRoutes` (`:1728`, render step 5) captures anything, and before
`AfterPreprocessing` (step 6) sorts. An index therefore cannot dangle on growth or
reordering, unlike an iterator.

### D3: Ordering keeps stability explicitly

Chosen: replace `wayData.sort()` / `areaData.sort(AreaSorter)`
(`MapPainter.cpp:2096-2097`) with `std::stable_sort` over the vectors, keeping the
comparators unchanged.

Alternatives:

1. `std::sort` - rejected: not stable, so equal-comparing areas could swap. The
   comparator produces equivalent elements for identical bounding boxes with the same
   ring role, so the draw order of coincident areas (for example an area that is an
   outer ring in one relation and an inner ring in another) could change and shift
   rendering output.
2. Extend `AreaSorter` with a total-order tie-breaker (for example preparation index) -
   viable and keeps `std::sort` usable, but it changes the comparator's contract and
   would make the ordering property depend on a new field instead of on a standard
   guarantee.
3. Sort on insertion (keep the store ordered while preparing) - rejected: the draw
   step order requires sorting after all preparation steps, and `wayData` is appended
   to again by later steps (see D4).

Cost note: `std::stable_sort` may allocate one temporary buffer; that is a single
allocation per store per frame against the per-object allocations removed.

### D4: Retain capacity, and keep appends from later steps working

Chosen: the stores keep their capacity for the painter's lifetime; no
`shrink_to_fit`. `clear()` on the vector is used where the frame starts fresh, and the
existing appends that happen after sorting keep their current semantics.

Steps that append to `wayData` after `AfterPreprocessing` (step 6): `DrawGroundTiles`
(`MapPainter.cpp:2364`, step 9) and `DrawOSMTileGrid` (`:1053`, `:1085`, step 10), and
`ProcessRoutes` appends route segments at `:1841` before the sort. All of them append
to the end and are drawn afterwards by `DrawWays` (`:2481`, step 12), so the
"appended, therefore unsorted tail" behaviour is identical to today. With a vector
these appends may reallocate; as established in D1 nothing holds an element address
across them.

Alternatives:

1. `shrink_to_fit` after each frame - rejected: reallocates on every viewport size
   change and gives back capacity that the next frame immediately needs.
2. Cap or reset capacity when the viewport shrinks drastically (for example in
   `InitializeRender` at `MapPainter.cpp:2039` or `StyleSheetChanged`) - deferred, not
   needed for the requirements; the memory difference is bounded by prepared object
   count rather than by type count.
3. Reserve exactly and only once - rejected: the estimate differs per view.

### D5: `AreaData::clippings` stays a `std::list` in this change

Chosen: leave `MapPainter.h:245` as is.

Alternatives:

1. Convert to `std::vector` in the same change - rejected for scope and risk: six
   backends read the member directly (listed in Context), including the Android JNI
   painter, so the change would touch GDI, AGG, Cairo, Skia, DirectX, SVG and Android
   for a secondary per-area allocation. Recorded as a follow-up instead.
2. Use a small-buffer type - same reach as (1), plus a new dependency.

### D6: Accessors expose the vector

Chosen: `GetWayData()` / `GetAreaData()` (`MapPainter.h:630-640`) return
`const std::vector<WayData>&` / `const std::vector<AreaData>&`. They are protected, not
public: a backend subclass uses them from `AfterPreprocessingCallback` (the SVG backend is
the only in-repo consumer, and it uses range-for loops, so it needs no source change).
This is a source-breaking change for external backends that subclass `MapPainter` and use
these accessors.

Alternatives:

1. Return `std::span<const WayData>` - viable and storage-agnostic, but it still
   changes the signature, so it does not avoid the break while adding a second concept.
2. Keep the `std::list` signature and return a copy or a converted temporary -
   rejected: defeats the purpose and would be a dangling-reference trap.
3. Add new vector accessors and keep the list ones - rejected: two accessors for the
   same data, and the list one could not be implemented without a second store.

## Frame flow

Current (per frame, allocation sites marked `*`):

```
Draw()
  |
  +-- CalculatePaths         clear() -> free all nodes
  |     CalculateWayPaths    push_back * per way and way path
  |
  +-- CalculateWayShields    registers shield labels (no store access)
  |
  +-- ProcessAreas           clear() -> free all nodes
  |     PrepareAreaRing      push_back * per ring
  |
  +-- ProcessRoutes          clear route labels
  |     captures way-path iterators, push_back * per segment and label
  |
  +-- AfterPreprocessing     list::sort (stable) -> node relink
  |
  +-- DrawAreas / DrawWays   traverse node chain, pointer chase
  +-- DrawWayDecorations / DrawWayContourLabels / PrepareAreaLabels
  +-- DrawAreaBorderLabels / DrawAreaBorderSymbols / PrepareRouteLabels
  +-- DrawGroundTiles / DrawOSMTileGrid   push_back * (unsorted tail)
```

Target (allocation sites removed; `*` sites become capacity-bounded appends):

```
Draw()
  |
  +-- CalculatePaths         clear() -> keeps capacity
  |     CalculateWayPaths    append (capacity-bounded)
  |
  +-- ProcessAreas           clear() -> keeps capacity
  |     PrepareAreaRing      append (capacity-bounded)
  |
  +-- ProcessRoutes          clear route labels
  |     stores way-path index, append route labels
  |
  +-- AfterPreprocessing     std::stable_sort (one temp buffer)
  |
  +-- DrawAreas / DrawWays   traverse contiguous block, in draw order
  +-- label steps            unchanged
  +-- DrawGroundTiles / DrawOSMTileGrid   append (unsorted tail, as before)
```

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Vector reallocation invalidates an element address held across a growth | Established that no such address is held (D1); the one cross-step reference becomes an index (D2); the new unit test asserts a route label still resolves after further appends |
| Ordering changes for equal-comparing areas, shifting rendering output | `std::stable_sort` keeps the current stable order (D3); drawing tests that compare output must pass unchanged |
| Prepared stores keep peak capacity for the painter's lifetime, which is held by long-lived painters such as the Qt client's (`libosmscout-client-qt/include/osmscoutclientqt/MapRenderer.h:122`) | Capacity is bounded by visible object count, not by type count; `shrink_to_fit` rejected (D4); measure resident memory before/after |
| Source-breaking accessor change for external backends | Only the SVG backend reads them in-repo and is updated (D6); the change is a compile-time break, documented in the release notes |
| Wrong estimate in `reserve()` still causes growth | Correctness does not depend on the estimate; correctness is covered by ordering and reference tests, and the estimate only affects how often the store grows |
| The four stores are used by all backends through callbacks, so a mistake shows only in one backend at runtime | Verification tasks cover Cairo, Qt, AGG, SVG and Skia drawing tests plus the SVG callback test |
| Measurement noise hides the effect, or the win is smaller than assumed | Task 1 records per-step timings and allocation counts on a fixed view before the change; the change is judged against that baseline, not against expectations |

## Migration Plan

Single repository change, no data or file format migration.

1. Record the baseline (task 1): per-step render timings and allocation counts for a
   fixed view with `Tests/src/PerformanceTest.cpp` (draw-repeat loop at `:953-960`,
   gperftools hooks at `:55`).
2. Land the store, ordering, reference and accessor changes together with the new unit
   test; all existing rendering tests must pass without golden-file updates.
3. Re-measure with the same command as (1) and record the difference in the change's
   verification notes.
4. Update `TODO.md` to close the `MapPainter` prepared-data finding, and note the
   `AreaData::clippings` follow-up.

Rollback: revert the commit; nothing outside the renderer depends on the changed
storage, and no persisted state is affected.

## Open Questions

- Should the `AreaData::clippings` conversion (D5, alternative 1) be filed as its own
  change now or only when the per-area allocation cost is measured to matter? This does
  not affect the specs, approach or tasks of this change.
