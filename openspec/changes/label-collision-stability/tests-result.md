# Result Summary

## Round 3b: contour label stickiness + diagnostics v2

Second log round (16:44): hiddenByCollision still 10..47 per round, plus one
243-spike frame. Street names had no stickiness at all (contour labels were
resolved in the flat order, glyph jitter toggles winners). Diagnostics v2
added: per-round contour counters (`contoursRegistered`,
`prevVisibleContours`, `lostContours`) to attribute the remaining losses.

Changes:

1. Previously visible contour labels (way refs) claim their space in phase 1
   before the merged newcomer phase, with the same glyph-level 2px jitter
   tolerance. State kept across Reset(), refreshed by Layout().
2. Layout diagnostics extended with contour registration/loss counters.

Build/test: label suite green (12 cases / 145 assertions), OSMScout2 rebuilt
(16:51, both libs fresh).

## Round 3: renderer-side tile churn fix (newest)

Log re-analysis: labels oscillate over rounds (`Node 27611461` hidden in
rounds 10,13,15,20,29,31,34,36,41) while `registered` jumps by hundreds
between rounds. The plane renderer re-renders with whatever partial tile
set has arrived (INITIAL_DATA_RENDERING_TIMEOUT=10ms, per tile change 200ms).
The candidate set churns -> previously stable group0 membership churns ->
blockers appear/vanish -> downstream labels flip. Layouter-level hysteresis
cannot compensate for candidates that are absent from a round.

Fix (design D4 Alternative C, chosen and implemented):
`PlaneMapRenderer::DrawMap` skips rendering while the load job has not
finished ALL requested tiles, unless no finished image exists (first render
still uses partial data). Retry via tile state changes or the update timer.
Effect: the finished image and its labels stay stable while tiles load; the
label set is recomputed once per complete tile set.

Spec: new requirement "Rendering uses complete tile data". Task 3.5.
Build/test: 12 cases / 145 assertions green both systems, warnings baseline,
OSMScout2 target built.

## Round 2: real-world log analysis (label.txt) and jitter tolerance

Log evidence collected with OSMSCOUT_DEBUG_LABEL_HYSTERESIS=1 (first
label.txt session):

- `hiddenByCollision` on EVERY round: 29..69 labels (479 total), although
  collision partners were previously visible and the candidate data was
  identical between rounds. Cause: mask rectangles are integer truncations
  of fractional pixel positions; while the map slides the effective
  distance between adjacent labels jitters by +/-1px and mask overlaps
  toggle between rounds. Both partners previously visible -> priority
  decides inside the visible group -> the loser alternates -> flicker.
- `notRegistered` spikes (294 total): tile data churn (see Round 3).

Fixes added:

1. Hysteresis tolerance (2px): collision check of previously visible
   labels ignores collisions that vanish when the rectangle is shrunk by
   2px; real overlaps keep priority resolution.
2. Phase 0 ordering: previously visible regular labels claim their space
   before contour labels are processed.

Test suite additions: regression case "previously visible labels survive
single-pixel mask jitter" (adjacent 100px labels, fractional shifts
0.8/2.0px, previously flipped, now stable).

## Round 1: synthetic diagnostics (layouter core, first implementation)

Date: 2026-08-29. GCC 13 / Linux, CMake (`build/`) and Meson (`debug/`).

`libosmscout-map/include/osmscoutmap/LabelLayouter.h`:

- `LabelLayouter` keeps `lastVisibleRefs` (object refs visible in the
  previous layout round). Preserved across `Reset()`, refreshed by every
  `Layout()`.
- `LayoutJob` sorting: two-group resolution. Previously visible labels
  claim their space first (`LabelInstanceSorter`), then all other labels;
  the `(priority, basemap, ref)` order rules inside each group. Prior
  behavior: one flat priority order over the whole candidate set.

## Test outcome

`Tests/src/LabelLayouterTest.cpp`: **11 test cases, 134 assertions — all
pass** on both build systems (ctest + meson test).

| Test case                                              | Result |
|---------------------------------------------------------|--------|
| backend-independent execution/result query              | PASS   |
| determinism (fresh instances, competing labels)         | PASS   |
| horizontal pan, fully visible labels                    | PASS   |
| vertical pan, fully visible labels                      | PASS   |
| sub-pixel pan                                           | PASS   |
| enlarged viewport, no content outside                   | PASS   |
| collision winner independent of pan                     | PASS   |
| **dense scene, candidate growth between frames (was red: 17/63 vanished)** | **PASS** |
| previously visible label precedes new candidate         | PASS   |
| session state survives reset, follows last layout       | PASS   |
| session rounds identical                                | PASS   |

The dense-scene diagnostic now passes: adding 60 border-ring candidates
(14 overlapping the outermost grid column) no longer removes any of the
labels visible in the first frame of the session.

## Behavior change (intended trade-offs, see design.md)

- A label visible in the previous round keeps its space against *new*
  candidates even when the new candidate has a better style priority.
  It disappears when it leaves the viewport/render set or collides with
  another previously visible label.
- Cross-round state lives only in the painter-owned layouter instance;
  a fresh painter (style reload) starts without hysteresis.
- Contour (path) labels keep the previous flat resolution (no stickiness).

## Build warnings

Pre-existing baseline unchanged (CMake: 51, Meson: 22 - all in
`libosmscout-map-opengl` and `libosmscout-client-qt`). Zero warnings in
changed files.