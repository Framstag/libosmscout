# STATUS SNAPSHOT — 2026-08-30

State of the label flicker investigation at the switch to another topic.
Everything below is verified with logged evidence, either from the user's
OSMScout2 sessions (`label.txt` variants) or from the local headless pan
harness.

## Current build state

- All code changes compile clean on CMake and Meson; **test suite green:
  12 cases / 145 assertions** (`ctest -R LabelLayouterTest`).
- Working tree contains (in addition to committed state):
  - `MapPainter.h/.cpp` — area anchor cache + [AnchorJump] probe (gated by
    `OSMSCOUT_DEBUG_LABEL_HYSTERESIS`)
  - `MapWidget.cpp/.h`, `main.qml` — temporary `debugPan` helper + QML
    auto-pan timer (TEMPORARY, must be reverted before archive)
  - `PlaneMapRenderer.cpp` — tile gate + swap/skip/flush logs
  - `MapRenderer.cpp` — render tile-count log (gated)
  - `LabelLayouter.h` — hysteresis (2 groups), jitter tolerance (2px),
    contour stickiness (3 phases), gated diagnostics
  - `Tests/`: label layout session tests + build registration
  - `openspec/changes/label-collision-stability/` — full change docs
  - `harness-run.log` — last headless pan session

## Verified conclusions (evidence-backed)

1. Label layouter is deterministic and translation invariant for fixed
   candidate sets (synthetic sessions).
2. Renderer gate for partial tile sets works (harness: `render: tiles=6`
   stable, skip logic verified locally; user's 155/243-spike rounds came
   from renders on incomplete tile data).
3. Area label anchors are now stable per zoom level (AnchorJump probe:
   **0 jumps**) — yet flicker persists.
4. Strict alternation: `Area 127329304` loses even rounds, `Area 127329670`
   loses ODD rounds (and symmetrical partner chains for node labels).
   Pure group-based hysteresis + tolerance cannot break a parity
   oscillation of two previously visible labels.

## Remaining root cause (to fix after switch back)

The alternation is a **collision decision toggle between two claims whose
overlap hovers around the 2px tolerance boundary** (mask overlap oscillates
between tolerated and contention across rounds). The clean fix is pairwise
decision persistence ("who won last time keeps winning") as designed in
design.md as the rejected Alternative B — it now needs to be revisited:

- Option A: pairwise decision cache `map<pair<refA,refB>, winner>` in
  `LabelLayouter`, checked before greedy priority resolution (bigger
  change, fully deterministic), OR
- Option B (quick experiment, may visually degrade): raise
  `hysteresisTolerancePx` from 2 to a value above the measured
  overlap band (~4-6px). One-line change, verify in harness.

NOT the cause (all measured smooth): tile gate, area anchor jumps
(0 jumps), contour stickiness, style/data presence for stable rounds.

## Remaining known issues

- `notRegistered` spikes up to ~243 rounds still possible when a new
  render request replaces the load job (needs investigation of the
  request/job lifecycle; the gate holds during a single request)
- temp artifacts to revert before archive:
  - `OSMScout2/qml/main.qml` (debugPan Timer)
  - `libosmscout-client-qt/.../MapWidget.{h,cpp}` (debugPan)
  - `[AnchorJump]` probe block in `MapPainter.cpp`
  - `[LabelHysteresis]` diagnostics + `IsHysteresisDebug()` in
    LabelLayouter.h (gate via env var, keep until fix is proven)

## Reproduction tooling

- `/tmp/panharness/main.cpp` + `/tmp/panharness/build.sh` — headless pan
  harness (throwaway)
- Run:
  `OSMSCOUT_DEBUG_LABEL_HYSTERESIS=1 QT_QPA_PLATFORM=offscreen ./PanHarness maps/nordrhein-westfalen stylesheets/standard.oss`
- User reproduces with: `OSMSCOUT_DEBUG_LABEL_HYSTERESIS=1 ./OSMScout2/OSMScout2 ...`