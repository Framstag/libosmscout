## Context

See proposal.md — Why. Current state:

- `stylesheets/map.ost` (power section, ~line 6647): 6 types — `power_tower` (NODE), `power_pole` (NODE), `power_line` (WAY, IGNORESEALAND), `power_minor_line` (WAY), `power_sub_station` (NODE AREA, matches `"station"` OR `"sub_station"`), `power_generator` (NODE AREA). A TODO comment marks the missing `power_plant`.
- `stylesheets/include/man_made.oss`: `SYMBOL power_tower`, `SYMBOL power_pole`; styles for `power_line`/`power_minor_line` (WAY), `power_tower`/`power_pole` (NODE.ICON), `power_generator` (AREA) at `[MAG close-]`/`[MAG detail-]`.
- `stylesheets/include/power.oss`: building-magnification styles for `power_sub_station`/`power_generator` (AREA fill + labels at `@buildingMag-`/`@labelBuildingMag-`).
- `stylesheets/basemap.ost`: no power types — out of scope.

## Goals / Non-Goals

**Goals:**
- Add 16 new type definitions in `map.ost` with wiki-correct object types
- Add styles for the new types reusing existing symbols/styles
- Resolve the `power_plant` TODO

**Non-Goals:**
- No changes to existing 6 power types (names, object types, styles)
- No basemap.ost changes (no power types there today)
- No C++/API changes; no new symbols — reuse `power_tower`/`power_pole` symbols
- No lifecycle-prefix handling (`construction:power`, `disused:power`)

## Decisions

### D1: Type definitions go in `map.ost` power section
Add new `TYPE` blocks after `power_generator`, replacing the TODO comment. `map.ost` is the single home for type definitions; `include/*.oss` files hold only `STYLE` blocks.
- Alternative: separate `include/power_types.ost` — rejected: no precedent, would split the power section.

### D2: Object types per OSM wiki
- NODE: `catenary_mast`, `portal`, `transformer`, `switch`, `terminal`, `insulator`, `connection`, `catenary_portal`
- WAY: `cable`, `circuit`
- NODE AREA: `plant`, `heliostat`, `compensator`, `inverter`, `switchgear`, `converter`
- `power_plant` resolves the TODO: NODE AREA with `{Name, NameAlt}` + ADDRESS (mirrors `power_sub_station`).
- Alternative: NODE-only for `plant` — rejected: wiki documents areas for facilities.

### D3: New styles reuse existing definitions, placed where related styles live
- Node support structures: distinct `SYMBOL` definitions for the visually distinct types — `power_catenary_mast` (pole + crossarm), `power_portal` (Π frame), `power_transformer` (drum), `power_switch` (box + lever), `power_plant` (building + chimney), `power_heliostat` (tilted mirror) — wired as `NODE.ICON` in `man_made.oss` at `[MAG close-]`. Remaining node types (`catenary_portal`, `terminal`, `connection`, `insulator`, `compensator`, `inverter`, `switchgear`, `converter`) reuse the existing `power_pole` symbol.
- Ways (`cable`, `circuit`): `WAY` style cloned from `power_minor_line` (grey, thin) in `man_made.oss`.
- Areas (`plant`, `heliostat`, `compensator`, `inverter`, `switchgear`, `converter`): AREA fill + labels cloned from `power_sub_station`/`power_generator` in `power.oss` at `@buildingMag-`/`@labelBuildingMag-`.
- Alternative: all new styles in `power.oss` — rejected: `power_tower`/`power_pole` symbols live in `man_made.oss`; styles referencing them belong beside them.

### D4: Exclusions
`cable_distribution_cabinet` (discouraged on wiki) and `cable_distribution` (not on wiki) get no types. `converter` at exactly 0.01% usage is included (wiki-documented, not discouraged).

## Risks / Trade-offs

- [Node icon reuse may look identical for many types] → mitigated: 6 distinct symbols added for visually distinct types; remaining types share `power_pole` (matches existing `power_tower`/`power_pole` treatment)
- [`power_sub_station` legacy `"station"` match untouched] → intentional; changing it is out of scope
- [Style file organization split across two includes] → follows existing layout; power.oss keeps building-magnification styles, man_made.oss keeps symbol/way styles
- [Type name collisions with other sections] → new names are unique `power_*` prefixed; verify with grep before commit

## Migration Plan

Stylesheet-only change. No data migration. Rollback = revert the two files. Verify by rendering a map with the standard style after import.

## Open Questions

None.
