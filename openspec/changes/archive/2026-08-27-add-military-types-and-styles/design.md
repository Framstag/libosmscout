# Design: add-military-types-and-styles

## Context

See proposal.md — Why. `stylesheets/map.ost` defines 7 `military=*` types; 15 significant values are missing. The type system is data-driven: `map.ost` (OST format) defines import-time types, and `.oss` stylesheets (loaded via `MODULE "include/..."`) define rendering. All three consumer stylesheets (`standard.oss`, `cycle.oss`, `winter-sports.oss`) include `include/military.oss`. No C++ changes are required — the OST/OSS parser already handles the constructs used.

## Goals / Non-Goals

**Goals**
- Add import-time types for all `military=*` values with taginfo usage >= 0.10% that are documented or not discouraged.
- Add obvious rendering: area fills/labels for installations, dashed lines for linear features, node icons for POI-like features.
- Keep the change purely data-driven (OST/OSS), zero C++/build changes.

**Non-Goals**
- No new C++ code, no schema/format changes, no import pipeline changes.
- No styles for discouraged values (`military=yes`, `military=abandoned`).
- No changes to the deprecated `military_naval_base` type (kept for backward compatibility).
- No changes to `landuse=military` handling.

## Decisions

### D1: Which values get types — wiki-documented set + taginfo threshold

**Decision:** Add types for (a) all values documented on the OSM wiki (Key:military template + individual tag pages) with usage >= 0.10%, and (b) undocumented values with taginfo usage >= 0.10% that are not discouraged.

**Alternatives considered:**
1. *Wiki-documented values only* — cleaner provenance, but drops `embrasure` (0.5%), `radar` (0.4%), `police` (0.3%), `road` (0.3%), `shelter` (0.25%), `cannon` (0.2%), `cordon` (0.1%) which are all significantly used in the wild. Rejected: user explicitly asked to check taginfo for additional types.
2. *All taginfo values >= 0.10% including discouraged* — would add `military=yes` (1.6%) and `military=abandoned` (0.1%), both flagged as tagging mistakes on the wiki. Rejected: user asked to skip discouraged values.
3. *Chosen approach* — wiki-documented values (8 new: trench, office, checkpoint, base, training_area, nuclear_explosion_site, obstacle_course, ammunition) plus undocumented-but-significant values (7 new: embrasure, radar, police, road, shelter, cannon, cordon).

**Risk:** Undocumented values have no wiki element table; element types are inferred from taginfo node/way/relation counts. → Mitigation: element types chosen conservatively from the dominant element type in taginfo stats (e.g., `military=road` is 100% ways → WAY only).

### D2: Element types for undocumented values

**Decision:** Derive element types from taginfo per-element statistics rather than guessing:

| Value | taginfo nodes/ways | Type elements |
|-------|--------------------|---------------|
| `embrasure` | 1034 / 0 | NODE |
| `radar` | 755 / 100 | NODE AREA |
| `cannon` | 430 / 3 | NODE |
| `cordon` | 6 / 207 | WAY |
| `shelter` | 83 / 441 | WAY AREA |
| `road` | 0 / 597 | WAY |
| `police` | 424 / 226 | NODE AREA |

**Alternatives considered:**
1. *Assume NODE AREA for everything* — simple but wrong for linear features (cordon, road) and would import closed ways as areas incorrectly.
2. *Chosen approach* — per-value element types from taginfo stats, matching how the wiki element table drives documented values.

**Risk:** Taginfo stats mix ways-as-lines and ways-as-areas; a value could be imported with the wrong element capability. → Mitigation: for mixed values (radar, shelter, police) both NODE and AREA are enabled, which covers the dominant usage; the import pipeline only assigns the type when the element matches.

### D3: `military=office` reuses the `office` group

**Decision:** `military_office` is declared `GROUP office, routingPOI` so it inherits the established office styling (blue fill, office icon, office label) from `include/office.oss` instead of duplicating office rules in `include/military.oss`.

**Alternatives considered:**
1. *Style military offices in military.oss* — duplicates office rendering logic and risks divergence. Rejected.
2. *Chosen approach* — group reuse. Objects tagged `military=office` + `office=*` already match the generic `office` type (defined earlier in map.ost); the new type only catches `military=office` without `office=*`, and both render identically.

**Risk:** Military offices lose any military-specific visual identity. → Accepted: consistency with the "make use of existing definitions for similar types" requirement; a military-specific tint can be layered later.

### D4: Linear features get dashed lines, POI features get dedicated symbols

**Decision:** Linear types (`trench`, `cordon`, `road`, `shelter`) render as dashed lines at close zoom, mirroring the existing `historic_*` way rendering (`WAY { color: ...; displayWidth: 0.1mm; dash: 2,2; }`). POI-like node types get dedicated vector symbols at very close zoom; installation areas (`training_area`, `nuclear_explosion_site`, `ammunition`) reuse the existing generic `military` triangle symbol.

**Alternatives considered:**
1. *Solid lines for linear features* — visually heavier; dashed matches the established convention for non-road linear features (historic roads, ruins). Rejected.
2. *Dedicated symbol for every node-capable type* — 15 new symbols; several (e.g., training_area) have no obvious pictogram. Rejected: only 7 obvious pictograms drawn, rest fall back to the generic `military` symbol.

**Risk:** Hand-drawn symbols may look inconsistent with the existing `military` triangle. → Mitigation: symbols reuse the existing `@militaryBorderColor` (#fe9898) palette and the geometric style of existing symbols in `historic.oss`/`leisure.oss`.

## Risks / Trade-offs

- [Undocumented values may be re-tagged or deprecated upstream] → Types are additive; if a value becomes discouraged, the type can be removed without breaking existing data.
- [Element-type inference from taginfo may mismatch future tagging practice] → Element sets are permissive (NODE AREA for mixed values); narrowing later is non-breaking.
- [New symbols increase stylesheet surface area] → All symbols validated by `OSTAndOSSTest` (unknown symbol references produce warnings); no new warnings introduced.
- [`military_road` may shadow highway rendering] → Type matching is order-based; highway types are defined before the military section in map.ost, so ways with `highway=*` keep their highway type. `military_road` only matches ways without highway tags.

## Validation

- `Tests/src/OSTAndOSSTest.cpp` (built as `build/Tests/OSTAndOSSTest`): loads `map.ost` + each `.oss`, reports unknown types/symbols as warnings, and with `--analyze` lists types without styles. Run for `standard.oss`, `cycle.oss`, `winter-sports.oss`.
- Acceptance: OST/OSS load OK, zero new warnings, no military type in the "without style" lists.
