# Design: type-historic

## Context

`stylesheets/map.ost` defines import-time feature types; `stylesheets/include/historic.oss` defines their rendering. The historic section previously covered 8 `historic=*` values (castle, manor, monument, memorial, ruins, archaeological_site, battlefield, wreck). The OSM wiki documents ~62 values; taginfo shows 65 additional documented values with usage >= 150 that are not discouraged. See proposal.md - Why for motivation and the spec for the full requirement list.

The `TypeConfig` is generated from the stylesheet at import time — no C++ code changes are needed. This is a pure stylesheet change, same pattern as the earlier `leisure-type-definitions`, `tourism-type-definitions`, `natural-type-definitions`, `shop-type-definitions`, and `amenity-type-definitions` changes.

## Goals / Non-Goals

**Goals:**
- Add 86 new `TYPE` definitions to the historic section of `stylesheets/map.ost` (65 base types + 21 `_building` variants).
- Add rendering rules in `stylesheets/include/historic.oss` for the new types: area fills, labels, way rendering, and a dedicated vector symbol per node-capable type where an obvious pictogram exists.
- Follow existing type/style conventions so the new definitions are indistinguishable in style from the current ones.

**Non-Goals:**
- No changes to existing type or style definitions.
- No C++/import-pipeline changes.
- No changes to other stylesheets (`standard.oss`, `cycle.oss`, `winter-sports.oss`).
- No external icon/image assets — all symbols are vector primitives defined in the stylesheet.

## Decisions

### D1: Add types to the existing historic section of map.ost

New `TYPE` definitions go into the `// Historic` section of `stylesheets/map.ost`, between `historic_wreck` and `historic_building` (the generic catch-all).

- **Alternative considered**: a separate `historic.ost` include file. Rejected — `map.ost` is the single source of type definitions; the five prior type-definition changes all extended `map.ost` in place.
- **Rationale**: keeps type definitions discoverable in one place and matches established project convention.

### D2: Element types follow the OSM wiki element table

Each new type uses the element set from the wiki [Key:historic](https://wiki.openstreetmap.org/wiki/Key:historic) table: `NODE` for stones/crosses/markers (14 types), `NODE AREA` for most values (42 types), `WAY AREA` for `aqueduct` and `castle_wall`, `WAY` for `hollow_way`/`road`/`roman_road`, `NODE WAY` for `railway`, and `NODE WAY AREA` for `epigraph`/`folly`/`wayside_shrine`.

- **Alternative considered**: `NODE AREA` for everything. Rejected — would import node-tagged `aqueduct` or area-tagged `milestone` objects that the wiki does not document, polluting the type config.
- **Rationale**: the spec requires wiki-conformant element types; import-time filtering is the cheapest place to enforce them.

### D3: Type flags follow existing historic conventions

- `ADDRESS POI` for all new types, mirroring the pre-existing historic types (`historic_castle`, `historic_ruins`, etc.).
- `GROUP historic, routingPOI` for base types, mirroring the pre-existing types; `GROUP historic, building, routingPOI` for `_building` variants, mirroring `historic_castle_building`.
- No `MERGE_AREAS` / `OPTIMIZE_LOW_ZOOM`: the pre-existing historic types do not use them, and historic objects are typically small and sparse.

- **Alternative considered**: minimal flags (element set + name only). Rejected — new types would be invisible to POI search and routing, inconsistent with existing historic types.
- **Rationale**: flag parity with the closest existing type keeps behavior consistent and predictable.

### D4: `_building` variants precede base types; specific types precede generic

Type matching is first-match-wins (`TypeConfig` iterates in definition order). Two ordering rules:

1. Each `_building` variant (e.g. `historic_church_building`) is defined immediately before its base type (`historic_church`), so areas with a positive `building=*` tag match the building variant first.
2. All new specific types are inserted before the generic `historic_building` and `historic` catch-alls, so e.g. `historic=church` matches `historic_church` rather than `historic_building`.

- **Alternative considered**: relying on the generic `historic_building` type for all building-tagged historic areas. Rejected — the base `historic_church` (NODE AREA) would match before `historic_building`, so building-tagged areas would render with the generic historic fill instead of the building fill.
- **Rationale**: preserves building rendering for building-tagged historic areas while keeping specific types first.

### D5: Styles grouped by nature in historic.oss

New style rules in `stylesheets/include/historic.oss` reuse the existing historic palette (`@historicColor`, `@historicBorderColor`, `@historicLabelColor`):

- Area fill: add the 47 area-capable new types to the existing `[MAG detail-]` AREA fill block.
- Labels: add the new types to the existing `[MAG @labelBuildingMag-]` / `[MAG veryClose-]` label blocks.
- Way rendering: add the 9 way-capable types to the existing `[MAG close-]` dashed WAY rule.
- `_building` variants are covered automatically by the existing `[GROUP historic, building]` rules.

- **Alternative considered**: one generic fill for all new types. Rejected — the spec requires the historic area color and label behavior; reusing the existing blocks is the minimal change.
- **Rationale**: reusing existing colors and blocks keeps the palette coherent and avoids new visual noise.

### D6: Dedicated vector symbols per node-capable type

72 new `SYMBOL` definitions (plus the existing generic `historic` rectangle) are added to `stylesheets/include/historic.oss`, one per node-capable type where an obvious pictogram can be drawn. Symbols use the OSS primitive set (`RECTANGLE`, `POLYGON`, `CIRCLE`) on a ~2.0-unit canvas centered at the origin, filled with `@historicSymbolColor` with `#ffffff` accents for openings/details and `AREA.BORDER` for rings/outlines — the same conventions as the leisure symbols.

Per-type `NODE.ICON { symbol: historic_<type>; }` rules are added in the `[MAG veryClose-]` block **before** the generic `[GROUP historic]` fallback rule, so first-match-wins assigns the specific symbol. Types without a dedicated symbol (`historic_district`, `_building` variants) fall back to the generic `historic` symbol.

- **Alternative considered**: one generic symbol for all types. Rejected — the request is to differentiate historic types visually where obvious.
- **Alternative considered**: external image icons. Rejected — the stylesheet symbol system is self-contained, scales with DPI, and needs no asset pipeline.
- **Rationale**: distinct pictograms (castle, church, mosque, anchor, cannon, mine, cross, etc.) make historic features recognizable at a glance; the generic fallback keeps the change safe.

### D7: `historic_stecak` uses ASCII identifier for UTF-8 tag value

The tag value `stećak` contains a non-ASCII character, but the OST grammar requires ASCII identifiers (`ident = letter {letter|digit|'_'}`). The type is named `historic_stecak` and matches the tag value `"stećak"` as a UTF-8 string literal.

- **Alternative considered**: `historic_stećak` as type name. Rejected — would fail the OST parser.
- **Rationale**: the type identifier must be ASCII; the tag value keeps its canonical spelling.

## Risks / Trade-offs

- [New types may collide with existing type names] → Verified against current `map.ost`; all 86 names are new. Import fails loudly on duplicate type names, so a collision would be caught in CI.
- [Symbols may look cluttered at small sizes] → Symbols are only rendered at `veryClose-` zoom; the generic fallback covers types without a dedicated symbol.
- [Type ordering regressions] → `_building` variants precede base types; all new types precede the generic catch-alls. Verified by parsing `map.ost` with the Import tool and by the `OSTAndOSSTest` style validation.
- [Import-time behavior change requires database re-import] → Stylesheet-only change; existing databases keep working, new types appear after re-import. Rollback = revert the two stylesheet files.

## Migration Plan

1. Edit `stylesheets/map.ost` (historic section) and `stylesheets/include/historic.oss`.
2. Validate stylesheets parse (Import tool `--typefile` run, `OSTAndOSSTest --warning-as-error`, `ctest CheckStyleSheet`).
3. Re-import a test database and verify new types appear in `TypeConfig` and render (e.g. via `SymbolsAll` for symbol rendering).
4. Rollback: revert the two files; no data migration needed.

## Open Questions

None.
