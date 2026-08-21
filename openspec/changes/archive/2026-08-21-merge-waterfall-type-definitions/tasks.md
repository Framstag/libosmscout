# Tasks: Merge waterfall type definitions

## 1. Type definition merge in `stylesheets/map.ost`

Ref: specs/natural-type-definitions — "Water natural types" requirement (scenarios: Waterfall type does not exist; Natural waterfall objects import as merged waterway type).

- [x] 1.1 Extend `TYPE waterway_waterfall` (line ~347): add `OR NODE WAY AREA ("natural"=="waterfall")` condition and a comment noting that `natural=waterfall` (deprecated tag) is merged here so both variants share one definition.
- [x] 1.2 Remove the `TYPE natural_waterfall` block (line ~878) and replace it with a comment explaining that `natural=waterfall` is deliberately NOT a separate import type — it is imported as `waterway_waterfall` (same pattern as the `natural_coastline` comment).

## 2. Rendering style cleanup

Spec: rendering stays covered for merged type per proposal Impact.

- [x] 2.1 `stylesheets/include/natural.oss`: remove the four `natural_waterfall` rules (AREA fill at `MAG state-`, `AREA.TEXT` at `MAG detail-`, `NODE.TEXT` at `MAG detail-`, `WAY` at `MAG detail-`).
- [x] 2.2 `stylesheets/cycle.oss`: remove the two duplicated `natural_waterfall` rules (AREA fill, `AREA.TEXT`).

## 3. Waterway waterfall rendering coverage

Spec: waterfall stays visible at zoom levels previously covered by natural rules (design D4).

- [x] 3.1 `stylesheets/include/waterway.oss`: add `[TYPE waterway_waterfall] AREA { color: @waterColor; }` inside the `IF waterway` block at `MAG state-` (low-zoom area fill parity with removed natural rule).
- [x] 3.2 `stylesheets/include/waterway.oss`: add `AREA.TEXT` and `NODE.TEXT` label rules for `waterway_waterfall` at `MAG detail-` (parity with removed natural label rules; keep `@waterLabelColor` / `@labelPrioNatural`).

## 4. Verification

- [x] 4.1 `grep -ri natural_waterfall stylesheets openspec` returns no matches (except archived changes and the comment in map.ost).
- [x] 4.2 Run `openspec validate merge-waterfall-type-definitions` — passes.
- [x] 4.3 Configure and build the project (`cmake -B build && cmake --build build`) — compiles without errors.
- [x] 4.4 Run existing test suite (`ctest -j 2 --output-on-failure`) — no regressions. Stylesheet-only change; no new unit tests required (no new C++ code). 100/100 non-OpenGL tests pass; 7 `PerformanceTest-opengl-*.oss` failures are pre-existing (missing installed OpenGL shaders at `/usr/local/share/osmscout/shaders` — verified identical failure with stylesheets stashed).
