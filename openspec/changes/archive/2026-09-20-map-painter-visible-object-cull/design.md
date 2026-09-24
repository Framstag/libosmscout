## Context

See `proposal.md` - Why for the measurements and the motivation.

Relevant current state, all on `master` (none of the branch work below is merged):

- The painter prepares ways in `MapPainter::CalculatePaths` / `CalculateWayPaths`
  (`libosmscout-map/src/osmscoutmap/MapPainter.cpp:1596`, `:1766`). The per-way style
  resolution (`GetWayLineStyles`, `:1609`) and the line metric computation run for every
  loaded way; the per-line-style visibility decision (`IsVisibleWay`, `:1662`) and the
  geometry transformation (`TransformPathData`, `:1690`) follow it.
- Way shields are prepared by `CalculateWayShields` (`:1813`), which walks all loaded ways
  (not the prepared ones), resolves a shield style per way and, through
  `RegisterPointWayLabel` (`:321`), builds grid positions as a `std::set<GeoCoord>` and
  registers one label per grid position, without any visibility decision.
- Point objects are prepared by `PrepareNodes` (`:524`) / `PrepareNode` (`:706`), which
  resolve icon and text styles, project the position and call `LayoutPointLabels` (`:380`)
  for every loaded node, without any visibility decision. `LayoutPointLabels` builds a
  per-object `std::vector<LabelData>` and registers it.
- The label stage is the template `LabelLayouter` in
  `libosmscout-map/include/osmscoutmap/LabelLayouter.h`. Registration measures immediately
  (`ProcessLabel`, `:712` for text, `:790` for contour labels); the viewport decision exists
  only in the drawing path (`DrawLabels`, `:628`). The layouter holds the frame's layout
  viewport, so the decision the drawing path uses is available at registration time.
- Every backend forwards its registration hooks into that shared layouter
  (`MapPainterCairo.cpp:1155`, `MapPainterQt.cpp:905`, `MapPainterSkia.cpp:1064`), so a
  decision taken in the layouter covers all backends.
- `StyleConfig` already derives per-level style bounds during its per-level postprocessing of
  the style lookup tables; the area work on branch `map-painter-area-visibility-cull` adds
  the first such bound (widest area border style of a level) plus an accessor and a debug
  assertion that the per-object tolerance never exceeds it.
- `MapParameter` offers one observation hook for tests, `RegisterFillStyleProcessor`
  (fill styles only, `MapParameter.h:191`); there is no equivalent for line, icon or text
  styles. Tests that need to observe work use allocation counters, coordinate buffer contents
  and label sets, or a temporary probe that is reverted before the change is finished (see
  `openspec/changes/map-painter-area-preparation/verification.md`).
- Measurement harness for before/after numbers: `Tests/src/PerformanceTest.cpp`, with the
  two-library `LD_LIBRARY_PATH` A/B harness described in
  `openspec/changes/map-painter-area-visibility-cull/verification.md`, because wall time on
  the development machine is not stable.
- Unmerged branches that touch the same code: `map-painter-area-visibility-cull` (the level
  bound, the cull pattern, `StyleConfig`, `MapPainter.cpp`) and `map-painter-label-reuse`
  (measurement reuse and scratch storage in `CalculateWayShields`, `RegisterPointWayLabel`,
  the layouter). Both are noted in the proposal as dependencies.

## Goals / Non-Goals

**Goals:**

- Give ways, point objects and labels the same "work follows the view" property that areas
  received, with one shared, conservative decision per object class.
- Keep the decision cheap: no measurement, no allocation and no style resolution before it.
- Keep the invariants provable and observable: the accepted set is a superset of what the
  existing per-object decision keeps, so prepared data, label placement and rendered output
  cannot change.
- Land the change without depending on the unmerged branches, while keeping it mergeable with
  them.

**Non-Goals:**

- Reducing the number of objects the data layer loads into a view (the over-fetch recorded in
  `TODO.md`): this change culls downstream of the load, it does not change loading.
- Changing any backend's drawing, measurement or glyph code, or the dash/pattern/geometry
  semantics of a backend.
- Changing area preparation or area culling; the area work on its branch stays as it is.
- Changing the label layout algorithm, priorities, overlap rules or draw order.

## Decisions

### D1: Where the way-level decision lives

The early way decision is taken in `MapPainter::CalculateWayPaths` (before `GetWayLineStyles`)
and in `MapPainter::CalculateWayShields` (before the shield style lookup), through one shared
private predicate on `MapPainter` that takes the way and the width bound of the current level.

- **Alternative A (chosen)**: one predicate, both call sites, using a per-level bound derived
  from the loaded stylesheet. Keeps the decision in the painter, where the projection and the
  level are known, and reuses the pattern the area work established.
- **Alternative B**: reject ways while filling `MapData` (loader side). Rejected: the loader
  does not know the level-dependent, stylesheet-dependent reach, and a rejection there is not
  provable against the painter's own decision; it would also silently change what
  `MapData::ways` contains for consumers beyond the painter.
- **Alternative C**: keep only the existing per-line-style decision and reduce its cost.
  Rejected: the style resolution and the shield registration in front of it are the measured
  cost (22 % of the painter-side frame for the way step at zoom 15, up to 44 % in the zoom
  band), and they cannot be removed without a decision that precedes them.

### D2: The per-level way width bound

`StyleConfig` derives, per level, a bound over the widest reach a drawn way line can have:
the maximum of the level's line style widths and display widths, plus the widest width a
data-carried width value can contribute, expressed in the same units the per-line-style width
computation uses and converted with the same conversion, halved in the same way the
per-line-style tolerance is (`lineWidth/2`).

- **Alternative A (chosen)**: bound derived from the loaded stylesheet during the existing
  per-level postprocessing, exposed through one documented accessor, with a debug assertion
  that no per-way tolerance exceeds the bound (mirroring the area work).
- **Alternative B**: bound the reach by the widest style width only, ignoring a data-carried
  width. Rejected: `CalculateLineWith` (`:1490`) adds the object's width value when the style
  declares a width, and a way with a large width value would then be rejected although the
  per-line-style decision keeps it, i.e. the rejection would not be conservative.
- **Alternative C**: fixed pixel margin (for example a constant of 100 px). Rejected: a
  constant is either unsafe for wide styles or wasteful for narrow ones, it cannot be derived
  from the stylesheet, and the project treats magic numbers as a defect.

### D3: The conservative extent bound for labels and point objects

Point objects and labels are rejected with the same shape of bound: a per-level extent from the
stylesheet (widest icon and symbol the level's styles can resolve) plus a label extent derived
from the map parameters and the label's character count and font size, both documented as
conservative bounds. One per-level bound record on `StyleConfig` serves way width, icon/symbol
extent and label extent, so the change adds one accessor rather than three.

- **Alternative A (chosen)**: derive the bounds, document them, assert the invariant in debug
  builds - the accepted set must contain every element the drawing path's viewport test keeps.
- **Alternative B**: measure first and discard afterwards (today's behaviour for labels).
  Rejected: that is the cost being removed.
- **Alternative C**: a per-element exact rectangle without measurement, from the backend's font
  metrics (worst-case advance for the current font and size). Rejected *for this change*: it
  needs a new backend hook through the text layouter contract in every backend, which a
  culling change should not require; it stays available as a later refinement if the coarse
  bound turns out to reject too little.

### D4: Where the label decision lives

The label decision is taken once, in the shared `LabelLayouter` registration path (both
`RegisterLabel` overloads and `RegisterContourLabel`), in front of the measurement calls.

- **Alternative A (chosen)**: in the layouter. It holds the frame's layout viewport, it is
  shared by all backends through their unchanged hooks, and one decision covers every label
  source.
- **Alternative B**: in the painter's four registration call sites (`LayoutPointLabels`,
  `RegisterPointWayLabel`, `DrawWayContourLabel`, `PrepareAreaLabel`). Rejected: four copies of
  the same rule, and the painter-side sites do not all know the final element geometry.
- **Alternative C**: in the backends' `RegisterRegularLabel` / `RegisterContourLabel`
  implementations. Rejected: eight copies of the same rule, and it would make the
  backend hooks carry a decision that is not backend-specific.

### D5: Shields are rejected by the way decision, not by the prepared-way set

`CalculateWayShields` keeps walking the loaded ways (not the prepared ones) and rejects a way
before its grid positions are built and before its labels are registered. The shield label's
extent depends on its text, so the rejection is taken after the shield style and its label text
are resolved, and it adds the label extent bound of that text plus the shield geometry
(`ShieldBorderInset`, `ShieldBackgroundClearance`) to the offset of the visibility decision.

- **Alternative A (chosen)**: reject inside the shield label preparation, in front of
  `RegisterPointWayLabel`. Everything the shield registration does per grid position (the grid
  positions themselves, one label registration per position, its measurement and its allocation)
  is removed for an off-view way; only the style lookup and the label text of the way remain.
- **Alternative B**: reject in `CalculateWayShields` in front of the shield style lookup, using a
  level bound of the widest shield style. Rejected: the reach of a shield label is set by its
  *text*, which is only known after the label provider resolved it; a bound over the stylesheet
  alone would either be unsound or so generous that it removes nothing. The spec only requires
  that a rejected way registers no shield label and contributes no grid position, which
  alternative A satisfies.
- **Alternative C**: iterate the prepared ways (`wayData` / `wayPathData`) for shields.
  Rejected: behaviour change - a shield-styled way without a drawn line style would lose its
  shield.
- **Alternative D**: leave shields alone and only cull the way path. Rejected: the shield path
  is one of the measured label-stage steps (16 % of the Cairo frame, 19 % of its allocations at
  zoom 15) and it is the same loaded-way set.

### D6: Observation of "the work was not done"

- **Alternative A (chosen)**: observe the consequences that are already observable - the
  prepared ways, the frame's coordinate data, the registered label set and its placement, the
  allocation counters, and the number of measurement calls through a counting test painter.
  Use the debug performance timers for before/after numbers, and a temporary probe (reverted
  before the change is finished) for the counts that no shipped tool prints, as the area work
  did.
- **Alternative B**: add a `LineStyleProcessor` / point-style processor to `MapParameter`
  mirroring `RegisterFillStyleProcessor`. Rejected: public API for a test-only need, and it
  would not observe anything the decisions above do not already skip.
- **Alternative C**: make the removed sites virtual so a test painter can count them.
  Rejected: changes the painter subclass contract for a test-only need.

### D7: Testability of the invariant in debug builds

Alongside the derived bounds, a debug assertion checks that the conservative bound of the label
decision holds for every element a label actually built: the rectangle of an element has to stay
inside the anchor plus the reach of the label (`AssertElementsInsideReach` in the shared label
layouter). It is checked where the element geometry becomes known, which is the only place where
the measured rectangle and the anchor of a label are both available.

- **Alternative A (chosen)**: assertion on the built element rectangles against the reach box of
  the label, in the shared label layouter.
- **Alternative B**: assertion at the drawing path (`DrawLabels`), where the drawing path decides
  which elements to draw. Rejected: the drawing path sees the element rectangle but not the anchor
  position of its label, so it cannot recompute the reach box without storing the anchor in every
  label instance, which the unmerged `map-painter-label-reuse` branch restructures anyway.
- **Alternative C**: assert only in unit tests. Rejected: it would not cover real views and
  stylesheets, which is where an over-aggressive bound shows up as missing labels.
- **Alternative D**: a runtime warning when a rejected element would have been drawn. Rejected:
  the condition must be impossible, not merely reported.

### D8: The decision point for point objects

A point object is rejected in two places, depending on what the stylesheet can make of its type at
the level of the frame:

- Where the type resolves no label style at that level, only the object's icon and symbol can
  reach the view, and their extent is bounded by the stylesheet, so the object is rejected in
  front of every style lookup (`PrepareNode`).
- Where the type has label styles at that level, the object is rejected after its styles are
  resolved and its label elements are built, but in front of the label registration
  (`LayoutPointLabels`), because the extent of a label is set by its text and only the resolved
  styles produce that text.

- **Alternative A (chosen)**: the two decisions above. The first removes the style lookup and all
  allocation of an object that only draws an icon; the second removes the storage, the measurement
  and the overlap processing of the elements of an object whose labels cannot reach the view, which
  is the allocation-heavy part of the step.
- **Alternative B**: reject every point object in front of its style lookups, using a bound over
  the stylesheet's label font sizes and a bound on the number of characters of a label text.
  Rejected: the character count of a label text is data-driven and unbounded (a label provider
  reads features of the object), so the bound would be unsound and would either drop labels of real
  data or have to be so generous that it removes nothing. The reach of a label remains unbounded
  until its text is known.
- **Alternative C**: keep the decision only at registration in the shared label stage (the label
  task of this change) and leave the point object preparation untouched. Rejected: the object
  would still build its label elements and look up its styles for every loaded object.
- **Alternative D**: reuse the per-object label element store of the painter (the vector
  `LayoutPointLabels` fills per object) instead of building it per object, so that an object with
  label styles adds no vector allocation either. Not part of this change: it is a scratch-storage
  change in the same place as the unmerged `map-painter-label-reuse` branch, and it is recorded in
  `TODO.md` as a finding of this work instead.

### D9: The frame a label decision may use

A backend reports the viewport of the frame from the drawing target (`cairo_clip_extents` in the
Cairo backend, `MapPainterCairo.cpp:700`), and it does so in `BeforeDrawingCallback`, i.e. in the
`Prerender` step (7). The shield labels, however, are registered in step 3, in front of it, and
node and area labels in the steps 14 - 18 behind it. A label decision therefore must not use the
viewport of the previous frame, and it must leave room for the region in which a label still
suppresses other labels.

- **The layouter decision** applies only when the viewport has been reported for the frame being
  prepared: `SetViewport` marks the layout viewport as valid and `Reset` (the end of a frame)
  clears the mark again, so a label of an early step is kept instead of being decided against the
  viewport of the previous frame (`CannotReachViewport`).
- **The painter's decisions** (point objects, way shields) use the projection, which is valid for
  every step, plus the margin the label layout leaves around an element: the layout viewport is the
  visible view enlarged by `MapParameter::GetLabelLayouterOverlap()` and the overlap canvases mark an
  element with the widest padding of the frame around its rectangle
  (`GetLabelLayoutMarginPixel`). Without that margin, rejecting an element outside the visible view
  lets a label appear that the unculled pipeline suppresses.

Measured before the two corrections: the demo view rendered 810 differing pixels against the
unculled library, of which 707 came from shields registered in step 3 and the rest from the
suppression margin; with both in place the render is byte-identical (0 differing pixels).

- **Alternative A (chosen)**: both rules above.
- **Alternative B**: set the layouter viewport in the painter's `InitializeRender` instead of the
  backend callback, so all steps of a frame would know it. Rejected: the viewport of a tiled
  backend is the rectangle of the drawing target within the surface (a translated, possibly
  larger region than the projection), and only the backend knows it.
- **Alternative C**: take the decisions only in the label steps behind `Prerender` (node and area
  labels) and leave shield labels completely to the painter's own rejection. Rejected: it would
  lose the decision for a backend that does not call `SetViewport` at all, and the shared label
  stage should behave the same for every source.
- **Alternative D**: ignore the suppression effect and accept small output changes. Rejected: the
  capability has "rendered output unchanged" as a requirement.

### Flow: way preparation after the change

```
CalculatePaths
   |
   +-- for every loaded way
          |
          +-- CalculateWayPaths
                 |
                 +-- early decision: can the way reach the viewport?
                 |   (bounding box + per-level width bound)
                 |        |
                 |        +-- no  -> return (no style lookup, no shield, no transform)
                 |        |
                 |        +-- yes -> GetWayLineStyles
                 |                     |
                 |                     +-- per line style: IsVisibleWay
                 |                     |      |
                 |                     |      +-- accept -> TransformPathData -> WayData
                 |
                 +-- (unchanged) lane divider and lane offset preparation

CalculateWayShields
   |
   +-- for every loaded way
          |
          +-- early decision: can the way reach the viewport?
          |   (bounding box + width bound + label extent bound)
          |        |
          |        +-- no  -> next way (no shield style lookup, no grid points)
          |        |
          |        +-- yes -> shield style lookup -> grid points -> RegisterRegularLabel
          |                                                                   |
          |                                                       (D4 decision)
```

### Flow: label registration after the change

```
registration (any source: node, area, way shield, contour)
   |
   +-- LabelLayouter::RegisterLabel / RegisterContourLabel
          |
          +-- early decision: can the label rectangle reach the layout viewport?
          |   (anchor + per-level icon/symbol extent + label extent bound)
          |        |
          |        +-- no  -> return (not stored, not measured, not laid out)
          |        |
          |        +-- yes -> measure -> store element
          |
   ... layout ...
   |
   drawing path: viewport test per element (unchanged)
          |
          +-- debug assertion: every element the viewport test keeps
              satisfies the early decision of its anchor
```

## Risks / Trade-offs

| Risk | Mitigation |
|---|---|
| An over-aggressive bound rejects a way, an icon or a label that should be drawn, changing the rendered output | Bounds are derived from the loaded stylesheet and the map parameters, never constants; a debug assertion checks that every accepted element of the drawing path satisfies the early decision; dedicated unit tests place objects just outside the viewport within the bound, and a rendered-image comparison (same view before/after) is part of the verification |
| The conservative bound is too coarse and the step gains less than the loaded-versus-visible ratio suggests (the area work returned 29 % - 45 % of its step, not the 96 % its visible ratio implied) | Measure before/after with the two-library A/B harness, report the achieved share, and keep the measurement in `verification.md`; if the gain is small, the coarse bound is revisited (D3 alternative C) instead of widening the cull |
| The change is large (three capabilities, three measured steps) and hard to review in one piece | Tasks are grouped per capability and ordered so that each group is independently verifiable; if the review shows it is too large, the label capability can be split off, because it does not depend on the way or point-object decisions |
| Merge conflicts with the two unmerged branches (`map-painter-area-visibility-cull`, `map-painter-label-reuse`), which touch the same files and even the same functions | Build the bound as one per-level record that the area bound can join later; keep the way and node decisions in predicates of their own; resolve the sequencing before implementation starts (see Open Questions) |
| The label decision moves registration out of the label stage's own set, so a label that the drawing path would keep could be lost through a unit mismatch between the extent bound and the measured rectangle | The debug assertion of D7 is defined on exactly that relation (accepted by the viewport test implies inside the early bound); the label tests compare against a view with known labels at the viewport edge |
| Additional public API on `StyleConfig` (one per-level bound accessor) | Documented next to the existing per-level queries, with the bound's meaning and its conservative direction stated; no other public API change |

## Migration Plan

Internal change to the map painter and its label layouter; no database, file format, style sheet
or build system change, so there is nothing to migrate for users of the library.

- Rollout: merge as a single change after the sequencing question below is resolved; no feature
  flag, because the rendered output is required to be unchanged and is verified as such.
- Rollback: revert the commits of the change. The only public API addition is the per-level
  bound accessor; reverting removes it again, and no persisted data depends on it.
- Documentation: the new accessor gets a doc comment in `StyleConfig.h`; `TODO.md` records the
  loader-side over-fetch found during the analysis as a pre-existing finding.

## Open Questions

None. The sequencing question with `map-painter-label-reuse` (branch) was resolved before the task
list was written: the label tasks are implemented against the current `master`, i.e. against the
`std::set<GeoCoord>` grid positions and the per-frame measurement of the registration path. The
label tasks are written so that they stay valid if `map-painter-label-reuse` lands first; a
follow-up rebase is then expected, because both changes touch the same registration path.
