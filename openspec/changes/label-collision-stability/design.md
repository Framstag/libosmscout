# Design: Label Collision Stability

## Context

See proposal.md and specs/label-collision-stability/spec.md. Root cause evidence: `openspec/changes/label-layout-stability-tests/tests-result.md`.

Current mechanism (all in `libosmscout-map/include/osmscoutmap/LabelLayouter.h`):

- `Layout()` swaps registered instances into a `LayoutJob`, sorts everything by `LabelPriority` (one flat order), then greedily claims `ScreenMask` canvas space.
- Winner set = f(candidate set). Candidate set churn between oversized-canvas swaps (panning, tile loading) flips winners -> on-screen labels flicker.
- `MapPainterQt`/other backends keep one `labelLayouter` member per painter and call `labelLayouter.Reset()` after every draw (`DrawLabels()` -> `Layout()`, `DrawLabels()`, `Reset()`), so the instance survives frames; only its instance lists are cleared.

## Goals / Non-Goals

**Goals:**

- Visibility of a previously visible label survives candidate-set growth (hysteresis).
- Determinism and translation invariance are preserved.
- No public API change; confined to the shared `LabelLayouter`.

**Non-Goals:**

- Cross-process/cross-frame canvas mask memory (positions move every frame; pixel anchoring is impossible without map-space reasoning).
- Contour (path) labels stickiness — they depend on path geometry; separate follow-up if flicker remains on roads.
- Changing `PlaneMapRenderer` swap cadence or overrun size.

## Decisions

### D4: Rasterization jitter tolerance and contour ordering (chosen)

Log evidence from the real OSMScout2 scene (`tests-result.md`, 787 diagnostics): 479
labels lost per `hidden-by-collision` although the collision partners were
previously visible and the candidate data was identical between rounds.
Root cause: mask rectangles are integer truncations of fractional pixel
positions; as the map slides, the effective distance between two adjacent
labels jitters by +-1px, so mask overlaps toggle frame by frame. In dense
scenes many pairs sit exactly at the collision threshold.

Resolution: (a) the collision check of previously visible labels tolerates
collisions that vanish when the rectangle is shrunk by 2px (jitter is not
contention, real overlaps still resolve by priority); (b) previously visible
regular labels claim their space in a phase 0 before contour labels are
processed (jiggling street path labels must not flip point labels).

- **Alternative A - round mask positions to nearest pixel instead of truncating**: rejected; relative distances between two labels still jitter when their fractional parts cross pixel boundaries at different times.
- **Alternative B - pairwise conflict graph instead of canvas masks**: rejected as an architectural rewrite; the tolerance addresses the measured jitter mechanism directly.
- **Alternative C - renderer-side stabilization (bigger overrun, longer initial rendering timeout)**: complementary for the `not-registered` churn (244 labels lost in a single frame in the log), tracked separately; it cannot address collision flips.

Risks: previously visible labels may render with up to 2px visual overlap
(accepted, matches navigation label behavior); tolerance applies only to
previously visible labels, newcomer disputes stay exact.

### D1: Two-group priority resolution with previous-round visibility (chosen)

Regular labels are sorted with a two-level key:

```
group 0: ref was visible in the previous layout round
group 1: everything else
        within each group: LabelPriority (priority, basemap, ref)
```

Implementation: `LabelLayouter` keeps `std::set<ObjectFileRef> lastVisibleRefs`, refreshed at the end of `Layout()` from the final visible instances. `Reset()` keeps it. `LayoutJob::SortLabels()` uses a comparator that reads the set (passed by reference into the job).

- **Alternative A — pixel-anchored frame-to-frame masks**: rejected; canvas pixel positions shift with the projection every frame, so previous masks are meaningless without keeping map-space geometry in the layouter, which is a much bigger architectural change.
- **Alternative B — pairwise negotiation instead of global greedy marking**: rejected as a rewrite of the resolution algorithm with the same chain-dependence problem (winner sets depend on chains, not only direct overlaps); higher regression risk with no stability gain.
- **Alternative C — renderer-level mitigation (bigger canvasOverrun, swap throttling in PlaneMapRenderer)**: rejected; reduces frequency but leaves the mechanism in place, costs memory and render latency, and leaves backends beside Qt unprotected.

Chosen alternative: hysteresis group ordering. Small, local, no new data structures beyond a ref set; emulates the label stickiness that navigation products (including JavaScout's stable sliding) exhibit.

Staleness bound: a previously visible label only keeps its space against *new* candidates; if two previously visible labels overlap (zoom change), priority decides inside the group. Labels leaving the render set are not re-registered and disappear from `lastVisibleRefs` on refresh, so state never grows beyond the last round's visible set.

### D2: State refresh semantics

`lastVisibleRefs` is set after every `Layout()` to exactly the labels visible in that layout. Consequence: a label hidden once (because a previously visible label must win) stays in `lastVisibleRefs` only if it was visible in the previous round; a label hidden in two consecutive rounds while merely squeezed is a deliberate simplification (documented risk).

### D3: Multi-round test harness in the existing test program

Extend `Tests/src/LabelLayouterTest.cpp` with a `LayoutSession` (one `LabelLayouter` instance, several `Frame()` calls through `RegisterLabel` + `Layout`) mirroring the real per-draw sequence `DrawLabels -> Reset -> next frame`. The candidate-growth diagnostic switches from two fresh layouter runs to two frames of one session.

- **Alternative — keep the diagnostic on fresh instances and add the state only inside the renderer**: rejected; the renderer cannot stabilize what the layouter recomputes from scratch, and a session harness is exactly how the real renderer uses the layouter.

## Flow

```
Frame n-1: Layout() -> visible set  ──> lastVisibleRefs (kept across Reset)
                                             |
Frame n:   RegisterLabel(...)                |
           Reset() cleared instances         v
           SortLabels:  group(prev visible) -> group(new), priority inside
           ProcessLabels: greedy mask filling in that order
           -> visible set -> lastVisibleRefs refresh
```

## Risks / Trade-offs

- [Priority inversion: a weak previously visible label can block a strong newcomer (new label appears only when the old one leaves screen)] → Accepted trade-off for navigation-grade stability; matches visible-label stickiness in navigation products. Bounded automatically: state lives only until the label leaves the viewport or collides with another previously visible label.
- [Cross-round state hides real priority changes after a style reload] → Mitigation: style changes create a fresh painter/layouter instance (existing behavior: painters are recreated with the style config), clearing hysteresis.
- [Zoom changes: label pixel size changes between rounds while refs persist] → Mask claim happens with the new size; only the group precedence persists. Zoom in/out behaves like a pan plus new candidates; worst case a previously visible label keeps space for one extra round — accepted trade-off, tested by the session determinism scenarios.
- [Memory: `std::set<ObjectFileRef>` grows unbounded] → Refreshed to the last round's visible set (bounded by on-screen labels); no accumulation.

## Migration Plan

1. Implement header change (pure in-place, no build system changes).
2. Extend tests: session harness + scenarios of this change; switch the dense diagnostic of `label-layout-stability-tests` to the session harness.
3. Both builds green, no warnings in changed files; all existing tests green.
Rollback: revert header + tests; no data, API, or build structure changes.

## Open Questions

None.