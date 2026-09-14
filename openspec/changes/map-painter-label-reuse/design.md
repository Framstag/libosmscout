## Context

See `proposal.md` for the motivation and measured numbers, and
`specs/map-painter-label-reuse/spec.md` for the requirements this design has to satisfy.

The label stage has three parts, in this order inside a frame:

1. **Registration** - backends forward labels into the layouter: `MapPainter::LayoutPointLabels`
   (`libosmscout-map/src/osmscoutmap/MapPainter.cpp:380`), `RegisterPointWayLabel` (`:321`,
   shield labels), `DrawWayContourLabel` (`:945`, path labels) and
   `LabelLayouter::RegisterLabel` / `RegisterContourLabel`
   (`libosmscout-map/include/osmscoutmap/LabelLayouter.h:731`, `:782`). This is where text is
   measured through the backend's `Layout()` hook.
2. **Resolution** - `LabelLayouter::Layout` (`LabelLayouter.h:563`) builds a per-frame
   `LayoutJob`, sorts the registered labels and resolves overlaps
   (`ProcessLabels` `:521`, `ProcessLabelInstance` `:394`, `ProcessLabelContourLabel` `:478`).
3. **Drawing** - `MapPainter::DrawLabels` (`MapPainter.cpp:2728` region) and
   `LabelLayouter::DrawLabels` (`LabelLayouter.h:610`) hand the resolved elements to the
   backend's `DrawLabel`, `DrawIcon`, `DrawSymbol` and `DrawGlyphs`.

Constraints and existing state that shape the approach:

- A label's measurement is a pure function of the text, the effective font and the wrapping
  width. Nothing measures the same label twice in one frame, but every frame measures its
  labels again, and a frame's label set is nearly the same as the previous frame's for a pan
  step or a redraw.
- The `LayoutJob` (`LabelLayouter.h:310-344`) owns the three viewport-sized `ScreenMask`
  canvases and the two ordered stores. It is stack-allocated per `Layout()` call (`:567`) and
  its copy and move constructors are deleted, so its bitmaps are allocated and zeroed on every
  frame.
- `ScreenRectMask` (`libosmscout-map/include/osmscoutmap/LabelLayouterHelper.h:117`) allocates
  its bitmask in the constructor (`libosmscout-map/src/osmscoutmap/LabelLayouterHelper.cpp:29`),
  so a reused `masks` vector still re-allocates one bitmask per element.
  `ScreenMask::AddMask` and `HasCollision` take the mask by const reference and copy bits, so
  no mask is retained across a call - which is what makes reuse possible. `ScreenMask` and
  `ScreenRectMask` already have a dedicated test file (`Tests/src/ScreenMaskTest.cpp`).
- `Label::ToGlyphs()` is a backend hook (`LabelLayouter.h:142`). Only two call sites exist:
  `LabelLayouter` when it places contour labels on a path (`:802`) and
  `MapPainter::MeasureLabel` for the text measurement API (`MapPainter.h:624-641`). Backends
  draw regular labels through their own native label object, so glyph derivation is a path
  label and tooling cost, not a regular label draw cost.
- Backends that implement `Layout()` and `ToGlyphs()`: Cairo, Qt, Skia, SVG, AGG, GDI,
  DirectX.
- The Cairo backend's measurement depends on state that is not an argument of `Layout()`:
  `MapPainterCairo::GetFont` (`libosmscout-map-cairo/src/osmscoutmapcairo/MapPainterCairo.cpp:298`)
  scales the requested size by `projection.ConvertWidthToPixel(parameter.GetFontSize())` (so a
  measurement depends on the DPI and the magnification), keys its own `fonts` map by that
  effective size only, and `Layout()` (`:779`) creates a `PangoLayout` with
  `pango_cairo_create_layout(draw)`, so the measurement also depends on the drawing target's
  font options. `MapPainterCairo::DrawMap` re-installs the target every frame (`:1389`).
- The codebase already has a bounded cache type, `osmscout::Cache`
  (`libosmscout/include/osmscout/util/Cache.h:57`, LRU with `SetMaxSize`), used for the
  database object caches (`libosmscout/include/osmscout/db/Database.h:85-90`).
- The two preceding painter changes established the pattern this design follows: reusable
  painter members cleared per use, no observable output change, allocation-counting unit tests
  (`Tests/src/MapPainterAreaPreparationTest.cpp`, `Tests/src/MapPainterFrameBuffersTest.cpp`).

## Goals / Non-Goals

**Goals:**

- Measure a label once while its measurement inputs and the painter's measurement environment
  are unchanged, and reuse the result for later frames, including frames in which the label is
  not drawn.
- Derive a label's per-glyph data once per measured label.
- Reuse the frame-wide label state and the per-object/per-label scratch storage instead of
  allocating them per frame.
- Keep memory bounded, so a long pan session does not accumulate measurements of every label
  ever seen.
- Keep the label set, placement, draw order, rendered output and text measurement results
  identical.
- Make reuse observable in tests without adding public API to the painter.

**Non-Goals:**

- Caching rendered label pixels, symbol primitives or icons. The draw path (item "no backend
  caches rendered symbols" in `TODO.md`) stays as it is. Measurement corrected the assumption
  this section started from: the `DrawLabels` step's allocation share is dominated by the
  layouter's overlap resolution, not by drawing - at zoom 15 the drawing itself allocates about
  24 of the step's 2012 allocations per frame (see `verification.md`, section 1.2 and 6), so the
  symbol raster cache is a frame-time item, not an allocation item.
- Changing the text measurement contract, the tolerance rules of `text-metrics-api`, or the
  shield geometry of `shield-label-rendering`.
- Changing the backends' glyph derivation or measurement implementations.
- Reusing the label stores across a *style* change or a database switch; a label of another
  stylesheet is a different label.
- Persisting, sharing or serialising measurements.

## Decisions

### D1 - Label measurement reuse lives in the layouter, not in each backend

The measurement cache is a member of `LabelLayouter` (`LabelLayouter.h`), keyed by the
`Layout()` arguments, and consulted by the single place that measures a label
(`ProcessLabel` `:680`, `RegisterContourLabel` `:782`). Backends keep implementing `Layout()`
unchanged.

*Alternatives:*

- **Memoise inside each backend's `Layout()`** (each backend owns a `map<Key, LabelPtr>`).
  Pro: the backend knows its own inputs, no new layouter state. Con: seven copies of the same
  cache logic, seven places to keep in sync with the contract, and the tests would have to
  verify reuse per backend. The layouter already owns the frame's label state, so the cache
  belongs there.
- **No cache, scratch reuse only** (the "A1" scope). Pro: lowest risk. Con: does not satisfy
  the "Label measurement is reused while its inputs are unchanged" requirement, which is where
  the measured cost sits.

*Risk:* the layouter must not miss an input that a backend's measurement silently depends on -
covered by D2.

### D2 - The key carries the layout inputs; the backend contributes a measurement environment

The cache key is the layout arguments - text, font size, proposed width (wrapping width),
wrapping flag and contour flag - combined with a **measurement environment** value that the
backend owns and that the layouter compares before serving a cached entry. The environment
covers what `Layout()` reads from the projection and parameters but does not receive as an
argument: the font name and size, the DPI, the magnification, and the drawing target's font
settings.

For Cairo the environment is derived from `parameter.GetFontName()`,
`projection.GetDPI()`, the magnification and the target's
`cairo_get_font_options` hash; `MapPainterCairo::DrawMap` (`:1389`) refreshes it together with
the target, and a change discards the cached measurements. Qt, Skia and the others derive an
environment from the equivalent state (font name, DPI, device pixel ratio).

*Alternatives:*

- **Discard the cache on every frame.** Trivially correct, but it removes the reuse the
  requirement asks for. Rejected.
- **Key on a hash of the whole `MapParameter` and `Projection`.** No backend hook needed, but
  too coarse: any unrelated parameter change (or a rotation) would discard all measurements,
  and it would still need the drawing target's font options, which are not part of either
  object. Rejected.
- **Key on the effective font object instead of an environment value.** Equivalent for Cairo
  (which already resolves an effective size in `GetFont`) but not portable: Qt, Skia and SVG
  resolve fonts differently, and `Layout()` would have to expose the resolved font to the
  layouter. Rejected as a new public hook for something a generation counter expresses.

*Risk:* a backend that changes its measurement inputs without updating its environment would
serve stale measurements. Mitigations: the environment is refreshed at frame start in one
place per backend, `text-metrics-api` cross-backend tests compare reused measurements against
fresh ones, and a unit test measures the same text at two DPI values and requires two
measurements.

### D3 - The measurement cache is bounded and cleared on environment change

The cache is bounded by a maximum entry count and evicts on overflow. It is implemented with
the existing `osmscout::Cache` (`libosmscout/include/osmscout/util/Cache.h:57`) rather than a
bare `unordered_map`, so eviction and statistics come from code the project already tests. The
bound is a constant in the layouter (the label stage is internal state, so no new
`MapParameter` setter is introduced) and the whole cache is dropped when the environment of
D2 changes.

*Alternatives:*

- **Unbounded `unordered_map`.** Pro: simplest, and the entries are also the glyph cache keys.
  Con: a long session across a whole country accumulates a `PangoLayout` per distinct label;
  the codebase's own caches are all bounded. Rejected.
- **Bounded per-frame cache (cleared after every frame).** Removes cross-frame reuse except
  within a frame, where nothing needs it. Rejected.
- **New `MapParameter` option for the bound.** Pro: configurable. Con: a new public setter for
  an internal cache; the database caches use constructor defaults instead. Rejected for now;
  recorded as an open question.

*Risk:* eviction can drop a measurement that the next frame needs again, turning a hit into a
miss. Mitigation: the bound is chosen well above the label count of a typical view (measured
label counts are in the hundreds per frame at z16), and eviction only costs what the current
code always pays.

### D4 - Per-glyph reuse is a layouter-side table keyed by the measured label

`LabelLayouter` keeps a table from the cached label (the `shared_ptr` it hands out of the
measurement cache) to its `std::vector<Glyph<NativeGlyph>>`, and the contour label path at
`LabelLayouter.h:802` reads through it instead of calling `ToGlyphs()` directly. Because the
label pointer is stable while the measurement is cached, the glyph entry is stable too, and it
is dropped with the measurement.

*Alternatives:*

- **Memoise inside each backend's label object** (`mutable` glyph vector in `CairoLabel` and
  its six peers), which would also speed up `MapPainter::MeasureLabel`. Pro: one change per
  backend covers every `ToGlyphs()` caller. Con: seven backend edits, a `mutable` cache inside
  a shared object (mutation visible to whoever else holds the `shared_ptr`), and thread-safety
  questions the layouter-side table does not have. Rejected for this change; the measurement
  API keeps working uncached (it is not a per-frame path).
- **Change `ToGlyphs()` to return a `const` reference** so no copy is made. Pro: eliminates the
  vector copy as well. Con: changes a hook implemented by seven backends plus the templated
  callers, and it makes the hook contract weaker (the returned vector must outlive the call).
  Rejected as a larger API change than the requirement needs.

*Risk:* the table is keyed by pointer, so a stale pointer would serve the wrong glyphs.
Mitigation: the table is only filled from labels taken out of the measurement cache, and it is
cleared exactly when the measurement cache is cleared.

### D5 - The frame-wide label state becomes reusable state of the layouter

`LayoutJob` stops being a per-call stack object. Its contents (paddings, the two ordered
stores, the three canvases) become state the layouter owns and resets per frame:
`ScreenMask` gains a method that zeroes the bitmask in place and resizes only when the layout
viewport changed, the ordered stores are cleared and swapped as today, and the paddings are
recomputed per frame. Copy/move stay deleted; the job is no longer copied.

*Alternatives:*

- **Keep constructing the job per frame and only shrink the canvases** (for example one canvas
  with three bit planes). Pro: less restructuring. Con: still allocates and zeroes a
  viewport-sized bitmap per frame, which is the part the requirement targets. Partially adopted:
  the three canvases keep their shape, the allocation is removed.
- **Recreate the job only when the layout viewport changes.** Pro: keeps the current structure
  and skips the reset when nothing changed. Con: the bitmask must be zeroed per frame anyway
  (masks of the previous frame must not leak into the new one), so the saving is the
  allocation only, which the chosen variant already keeps; and the canvas size changes with
  every window resize and zoom-independent viewport change. Rejected as no better than the
  chosen variant with more branching.
- **Move the canvases into the painter and pass them into `Layout()`.** Pro: explicit
  ownership. Con: widens the layouter's interface for state nobody outside the layouter reads.
  Rejected.

*Risk:* a canvas that is not fully zeroed leaks a mask of the previous frame and silently
suppresses labels. Mitigation: the reset zeroes the whole bitmask and resizes on viewport
change; a unit test renders two frames with different label sets and asserts both frames place
their labels as a fresh painter would.

### D6 - Per-object and per-label scratch becomes reusable painter and layouter members

Following the two preceding painter changes:

| scratch | site | reuse |
|---------|------|-------|
| per-object label list | `MapPainter::LayoutPointLabels` (`MapPainter.cpp:391`) | cleared painter member |
| per-label mask, canvas and visible-element vectors | `ProcessLabelInstance` (`LabelLayouter.h:399`, `:400`, `:403`) | cleared layouter members |
| per-label mask vector of a contour label | `ProcessLabelContourLabel` (`:487`) | cleared layouter member |
| per-element mask bitmask | `ScreenRectMask` (`LabelLayouterHelper.cpp:29`) | `ScreenRectMask` gains a reset that reuses its bitmask |
| path geometry of a path label | `DrawWayContourLabel` (`MapPainter.cpp:982`) | `LabelPath` gains a reset; reused painter member |
| shield grid points | `GetGridPoints` (`MapPainter.cpp:44`) | reused painter member holding a sorted unique vector instead of a `std::set` per way |

*Alternatives:*

- **`thread_local` buffers instead of members.** Pro: no ownership question and safe if several
  painters run in one thread. Con: hidden lifetime, and the codebase passes state through
  objects everywhere else. Rejected.
- **Fixed-size stack buffers for the small vectors** (the element, canvas and visible-element
  lists are single-digit). Pro: removes the allocation entirely. Con: needs a new
  small-vector type the codebase does not have, for buffers that a cleared member already
  removes from the per-frame allocation path. Rejected; the members are enough to make the
  count independent of the label count.
- **Leave the shield grid set as a `std::set` and only reuse it.** Pro: no container change.
  Con: a reused `std::set` still allocates a tree node per grid point; a sorted unique vector
  keeps the same iteration order (ascending coordinates) with one buffer. Rejected.

*Risk:* reuse changes the lifetime of objects other code may hold. Mitigations: the mask
vectors are never retained (`AddMask`/`HasCollision` take references), the reset methods are
tested (`Tests/src/ScreenMaskTest.cpp`, `Tests/src/LabelPathTest.cpp`), and the ordered label
stores keep their current swap-based ownership.

### D7 - Tests observe reuse without new painter API

The new unit test instantiates `LabelLayouter` directly with a counting fake text layouter, the
way the layouter's template parameter invites (`LabelLayouter.h:244-257` documents the required
interface), and counts measurements and glyph derivations. Allocation behaviour is asserted
with the counting `operator new` used by the existing painter tests. No counter is added to the
painter's public interface.

*Alternatives:*

- **Add measurement statistics to the painter** (counters with getters, or extra
  `IsDebugPerformance` logging). Pro: observable in production too. Con: new public API for a
  test concern. Rejected.
- **Make `Layout()` virtual so a test painter can count calls.** Pro: tests the real backend
  code path. Con: changes a public backend hook signature and the layouter's template
  parameter contract for observability alone. Rejected.

*Risk:* a test that drives the layouter directly does not prove the backend refreshes its
measurement environment. Mitigation: a separate backend-level test measures the same text at
two DPI values and requires a fresh measurement; the `text-metrics-*` tests keep checking
metric equality.

### D8 - Caches are painter-private and frame-serialised

The measurement and glyph caches live in the label layouter the painter owns, are mutated only
during a frame, and are never shared between painter instances. No new locking is introduced;
backends that serialise frames already do so (`MapPainterCairo::DrawMap` locks its mutex,
`:1387`).

*Alternatives:*

- **One process-wide cache shared by all painters**, which would let a second painter reuse
  measurements of the first. Con: measurements depend on the painter's environment (font
  settings, DPI, target), so sharing needs environment-aware keys everywhere plus locking.
  Rejected.
- **Lock the caches individually** so a shared painter could be used concurrently. Con: the
  painter's frame state is not thread-safe today and the change does not add concurrency.
  Rejected.

*Risk:* an application that renders several views in parallel pays one cache per painter.
Mitigation: the bound of D3 keeps the per-painter footprint small, and the caches hold only
what that painter measured.

## Sequence

Two consecutive frames of the same view. `E` is the backend's measurement environment.

```
frame 1                                     frame 2
  |                                           |
  | backend refreshes E (target, dpi, font)    | backend refreshes E'
  |                                           |
  +-- register label "Hauptbahnhof" ---------> +-- register label "Hauptbahnhof"
  |     key = (text, size, width, wrap)        |     key = (text, size, width, wrap)
  |     miss: call Layout()  [pango]           |     look up key, E' == E  ->  hit
  |     store label in cache                   |     reuse label  (no Layout())
  |                                           |
  +-- resolve overlaps (LayoutJob canvases)--> +-- reset reusable canvases in place
  |     new canvases, zeroed                   |     same canvases, zeroed
  |                                           |
  +-- contour label on a way ----------------> +-- contour label on a way
  |     ToGlyphs()  [per glyph allocs]         |     glyph table hit for the label
  |     store glyphs under the label           |     no ToGlyphs(), no per-glyph allocs
  |                                           |
  +-- draw resolved elements -----------------> +-- draw resolved elements
        same label set, same order                   identical output
```

If `E' != E` (a new target, DPI, magnification or font name) both caches are dropped before
the first registration of frame 2, and frame 2 behaves like frame 1.

## Risks / Trade-offs

| risk | mitigation |
|------|------------|
| A backend's measurement input is missing from the environment key, so stale metrics are drawn | environment refreshed at frame start per backend; unit test measuring at two DPI values; `text-metrics-*` cross-backend tests keep comparing dimensions and glyph boxes |
| The environment token is refreshed per frame unconditionally, silently disabling reuse | a unit test asserts a second frame performs no measurement; the reuse must be visible in the test's counter |
| Cache memory grows with the session | bounded cache (D3), cleared on environment change; a test asserts the bound is honoured |
| Frame canvas reset misses pixels and suppresses labels | reset zeroes the whole bitmask, resizes on viewport change; two-frame test comparing placements against a fresh painter |
| Glyph table keyed by pointer serves glyphs of a freed label | the table is filled only from measurement-cache labels and cleared with it; the contour path holds the `shared_ptr` it reads |
| `ScreenRectMask` and `LabelPath` gain reset methods that other code misuses | both classes have dedicated test files that get cases for the new methods |
| The draw path (`DrawLabels`) is untouched, so its symbol and text rendering cost stays | expected: the step's allocations were the layouter's resolution work (measured), so this change removes them; the remaining drawing cost is a frame-time item recorded as the symbol raster follow-up in `TODO.md` |
| The key hashes the label text per registered label per frame | one hash plus one lookup per label replaces a shaping pass; the A/B measurement task checks the net effect |

## Migration Plan

No data, database format, style sheet or build system migration. The change is additive inside
`libosmscout-map` plus per-backend environment code, and lands as a commit series on one
branch with the usual verification (CMake and Meson builds, full `ctest`, uncrustify,
clang-tidy). Rollback is reverting the series; no stored artefact depends on the caches.

## Open Questions

- Whether the measurement cache bound should become a `MapParameter` option, as the database
  object caches have their sizes in `Database` constructors. Deferrable: the bound is an
  internal default and the specs do not mention configuration.
- `MapPainterCairo`'s `fonts` map (`MapPainterCairo.cpp:309-320`, keyed by effective font size
  only, never invalidated when the font name changes) is a pre-existing cache whose key is
  incomplete. Out of scope here and not required by the specs; recorded in `TODO.md` with the
  other pre-existing findings.
- Whether the glyph table should also serve `MapPainter::MeasureLabel`. Deferrable: that path
  is used by tooling and tests, not per frame, and the specs only require equality of results.
