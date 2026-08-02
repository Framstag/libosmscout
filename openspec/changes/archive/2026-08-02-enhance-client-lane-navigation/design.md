## Context

Lane guidance data flows from OSM tags through the import pipeline into `LanesFeature` and `LaneDescription`/`SuggestedLaneDescription` in `RouteDescription`. At runtime, `LaneAgent` (in `libosmscout/navigation/`) extracts this data and emits `LaneMessage` with a `Lane` struct containing: `oneway`, `count`, `turns` (vector of per-lane `LaneTurn`), `suggested`, `suggestedFrom`, `suggestedTo`, and `turn` (the single suggested turn).

The Qt client (`NavigationModel`) already exposes all fields via Q_PROPERTYs including `laneTurns` (QStringList). The Java client JNI bridge (`OSMScoutClient.cpp`) registers `LaneAgent` and dispatches `LaneMessage` to `NavigationListener.onLaneUpdate`, but only passes the summary `turn` string — the per-lane `turns` vector is dropped. JavaScout's `RoutePanel.updateLaneInfo()` appends lane info as text to a status label.

See proposal.md for motivation. See specs/ for detailed requirements.

## Goals / Non-Goals

**Goals:**
- Add `LaneTurn` Java enum matching C++ `osmscout::LaneTurn`
- Extend `NavigationListener.onLaneUpdate` to carry `LaneTurn[] turns`
- Update JNI dispatch to build and pass the `turns` array
- Add visual lane guidance overlay in JavaScout `RoutePanel` next-turn display
- Keep backward compatibility via default method

**Non-Goals:**
- No changes to core libosmscout `LaneAgent` or `RouteDescription`
- No changes to Qt client (already complete)
- No lane rendering on the map itself (overlay only)
- No animation of lane transitions

## Decisions

### Decision 1: Add `turns` parameter vs. new callback

**Chosen:** Extend existing `onLaneUpdate` with `LaneTurn[] turns` parameter.

**Alternatives considered:**
1. *New separate callback `onLaneTurns(LaneTurn[])`* — cleaner separation but forces listeners to track two callbacks. The lane data is always emitted together; splitting adds complexity for no benefit.
2. *Keep `onLaneUpdate` as-is, add `onLaneTurns`* — same issue, plus callers must correlate two async events.
3. *Replace `String turn` with `LaneTurn[]`* — breaks existing callers without a migration path.

**Rationale:** Single callback with all lane data is simpler. Default method with the new parameter ensures existing implementations still compile.

### Decision 2: `LaneTurn` enum values vs. string constants

**Chosen:** Java enum with values mirroring C++ `osmscout::LaneTurn`.

**Alternatives considered:**
1. *String constants in a utility class* — no type safety, harder to use in switch statements.
2. *Integer constants* — opaque, requires documentation to understand.

**Rationale:** Enum provides type safety, IDE autocomplete, and switch exhaustiveness. JNI mapping is straightforward via ordinal or explicit mapping.

### Decision 3: Lane overlay rendering approach

**Chosen:** JavaFX `HBox` of `SVGPath` elements in `RoutePanel`, positioned below the next-turn instruction.

**Alternatives considered:**
1. *Canvas overlay on map* — more flexible rendering but requires map redraw coordination and is harder to position relative to the next-turn box.
2. *Custom `Region` with CSS* — limited control over arrow shapes.
3. *Pre-rendered PNG icons* — doesn't scale well with variable lane counts.

**Rationale:** `SVGPath` gives full control over arrow shapes, scales cleanly, and integrates naturally with the existing JavaFX layout. The `HBox` approach handles variable lane counts without layout gymnastics.

### Decision 4: Suggested lane highlighting

**Chosen:** Fill suggested lane arrows with accent color, non-suggested with outline only.

**Alternatives considered:**
1. *Scale/transform suggested lanes* — visually jarring.
2. *Color-code all lanes* — harder to read at a glance.
3. *Animate suggested lanes* — adds complexity, may distract driver.

**Rationale:** Accent fill vs. outline is the standard approach in automotive navigation HMI. It's immediately readable without animation.

## Risks / Trade-offs

- **[JNI array construction overhead]** Building a `LaneTurn[]` array on every lane update adds JNI allocation. → Mitigation: lane updates are infrequent (only when lane configuration changes, not every GPS tick). The array is small (typically 2-5 elements).
- **[JavaFX layout reflow]** Adding/removing lane arrow `HBox` triggers layout pass. → Mitigation: pre-create the `HBox` and toggle visibility + update children instead of rebuilding the scene graph.
- **[LaneTurn enum drift]** C++ `LaneTurn` values may gain new variants. → Mitigation: JNI bridge maps by ordinal; Java enum must be updated in sync. Add a comment in both files listing the sync dependency.
- **[Backward compat]** Existing `NavigationListener` implementations that override `onLaneUpdate` will break. → Mitigation: make the new method a default no-op. Old overrides with the old signature will fail to compile — this is unavoidable for a parameter addition. Document in release notes.
