# Design: Merge waterfall type definitions

## Context

See proposal.md — motivation. Two import-time types (`natural_waterfall`, `waterway_waterfall`) exist for the same feature. Import type resolution in `TypeConfig::GetNodeType` / `GetWayAreaType` (`libosmscout/src/osmscout/TypeConfig.cpp`) iterates declared types and returns the first match — so keeping both types with overlapping conditions would silently shadow one of them. Only stylesheet/config files change; no C++ code.

## Goals / Non-Goals

Goals:
- Single type definition `waterway_waterfall` matches both `waterway=waterfall` and `natural=waterfall`.
- Waterfall rendering coverage is not reduced at any zoom level previously handled by the `natural_waterfall` rules.
- No `natural_waterfall` references remain in stylesheets or specs.

Non-Goals:
- No change to C++ import code or the DB format.
- No migration of already-imported databases (re-import required; see Risks).
- No style redesign of the waterfall look itself.

## Decisions

### D1: Merge into `waterway_waterfall`, drop `natural_waterfall`

**Chosen:** `waterway_waterfall` is the official type per OSM (taginfo shows `waterway=waterfall` as the accepted tag; `natural=waterfall` is deprecated). Karry's PR #1769 comment and Framstag's agreement point there.

**Alternative:** Merge into `natural_waterfall`. Rejected: keeps a deprecated tag's type name as canonical and contradicts the official definition; `waterway_waterfall` also already carries `IGNORESEALAND` and the `Width` feature.

### D2: Remove `natural_waterfall` type definition, add explanatory comment

**Chosen:** Delete the `TYPE natural_waterfall` block in `stylesheets/map.ost` and replace it with a comment explaining `natural=waterfall` is merged into `waterway_waterfall` and deliberately not a separate import type — same pattern as the existing `natural_coastline` comment (map.ost around line 832).

**Alternative:** keep the type with a deprecation comment and empty/non-matching condition. Rejected: a non-matching type is dead weight; a matching one collides with `waterway_waterfall` in first-match resolution. Removal keeps exactly one definition (the change's stated goal).

### D3: Merged type keeps `waterway_waterfall` options, no `MERGE_AREAS`

**Chosen:** `waterway_waterfall` keeps its current options `{Name, NameAlt, Width}` and `IGNORESEALAND`; only the condition set gains `OR NODE WAY AREA ("natural"=="waterfall")`. `MERGE_AREAS` (which `natural_waterfall` had) is NOT added.

**Alternative:** add `MERGE_AREAS` to preserve `natural_waterfall` area-merge behavior. Rejected: changes behavior for existing `waterway=waterfall` areas too (single type cannot distinguish tag origin), and waterfall areas are rare and rarely adjacent; `MERGE_AREAS` at import is not needed for correctness of rendering.

### D4: Rendering rules — merge into `waterway.oss`, drop natural rules

**Chosen:** Remove the four `natural_waterfall` rules from `include/natural.oss` (area fill at `MAG state-`, `AREA.TEXT` and `NODE.TEXT` at `MAG detail-`, `WAY` at `MAG detail-`) and the two duplicated rules from `cycle.oss`. Move the *zoom coverage* into `include/waterway.oss` so nothing regresses for the merged type:
- `[TYPE waterway_waterfall] AREA { color: @waterColor; }` at `MAG state-` (preserves the low-zoom area fill the natural rule provided).
- `AREA.TEXT` and `NODE.TEXT` labels for `waterway_waterfall` at `MAG detail-` (natural rules provided labels from detail-; current waterway labels only start at `veryClose-`).
- Existing waterway rules (way line at `detail-`, `NODE.ICON` at `closer-`, `WAY.TEXT`/`NODE.TEXT` at `veryClose-`) stay unchanged; the way rendering follows the waterway style (`lighten(@waterColor, 0.3)`, dash) instead of the natural one.

**Alternative:** keep duplicated `natural_waterfall` rules and additionally target them at `waterway_waterfall` in `natural.oss`. Rejected: leaves two modules styling one type, and the natural block is gated on `IF _natural` while the type lives in the waterway group — inconsistent ownership.

## Risks / Trade-offs

- **Databases imported before this change keep `natural_waterfall` objects** → Mitigation: document re-import requirement (proposal.md Impact); the merged type only affects fresh imports.
- **Waterfall style changes subtly** (way line now waterway-style with dash/width instead of plain `waterLabelColor` line; area fill moves from `@waterColor` to `lighten(@waterColor, 0.3)` at `detail-`) → Mitigation: low-zoom fill added at `state-` matches old natural fill color; label colors/priorities preserved (`@waterLabelColor`, `@labelPrioNatural`).
- **`MERGE_AREAS` behavior drop for `natural=waterfall` areas** → Mitigation: negligible in practice (decision D3); revisit if reported.
- **Spec delta must keep the `Water natural types` requirement name exact** for archive-time merge → Mitigation: header copied verbatim from `openspec/specs/natural-type-definitions/spec.md`.

## Migration Plan

1. Edit `stylesheets/map.ost` (type merge + comment) — schema change, no build step needed for stylesheets.
2. Edit stylesheet modules (`natural.oss`, `waterway.oss`, `cycle.oss`).
3. Validate: no `natural_waterfall` references remain (`grep`); `openspec validate` passes; full build + ctest.
4. Rollback: revert stylesheet edits and re-import; no data migration involved.

## Open Questions

None.
