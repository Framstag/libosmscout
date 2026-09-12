# Design: Add missing railway type and style definitions

## Context

See proposal.md — Why. The stylesheet system: `stylesheets/map.ost` defines import-time `TYPE` definitions (first match wins, so type order matters); `stylesheets/*.oss` define style rules and reference types by name; `stylesheets/include/*.oss` are style modules included by `standard.oss` via `MODULE` directives. `railway.oss` is the railway style module. Symbols are defined in the `CONST`/`SYMBOL` section of `.oss` files in a 2.25-unit coordinate space; the renderer centers each symbol's bounding box and flips the y-axis (symbol y=-1.125 renders at the top of the image).

## Goals / Non-Goals

**Goals:**
- Cover all documented `railway=*` values with usage >= 0.02% that are not discouraged, matching the coverage pattern of `historic=*`, `amenity=*`, etc.
- Keep visual consistency with existing railway symbols and styles.
- Avoid regressions: new types must not shadow generic `building` rendering for building-like features.

**Non-Goals:**
- No C++ code changes, no database format changes, no new dependencies.
- No styles for obscure OpenRailwayMap infrastructure nodes without obvious general-map visualisation (e.g. `derail`, `junction`, `phone`, `radio`).
- No support for discouraged values (`facility`, `site`, `yes`, `razed`, `dismantled`).

## Decisions

### D1: Element types from the OSM wiki element table

Each new type's element set (NODE/WAY/AREA) comes from the wiki [Key:railway](https://wiki.openstreetmap.org/wiki/Key:railway) table, cross-checked against the individual `Tag:railway=*` pages where the value is not in the main table (e.g. `milestone`, `signal_box`, `yard`). Where the main table and the tag page conflict (e.g. `ventilation_shaft`: main table NODE+WAY, tag page NODE+AREA), the main table wins as the primary reference.

*Alternative considered:* using only taginfo usage data without element types — rejected, element types are required for correct import.

### D2: Type placement and ordering

New types are inserted in the railway section of `map.ost` after `railway_turntable`, before the "Other public transport" section. First-match-wins means types defined earlier shadow later ones; the new `railway=*` values are disjoint from existing values, so ordering within the block is not semantically critical. Grouping: tracks, then stops, then infrastructure.

### D3: Building-like types get building styles

`railway_workshop` and `railway_signal_box` accept AREA and would shadow the generic `building` type (defined later in `map.ost`) for areas that also carry `building=yes`. To prevent losing building rendering, both get AREA styles in the `_building` section of `railway.oss`, mirroring the existing `railway_station` pattern.

*Alternative considered:* omitting AREA from these types — rejected, the wiki documents AREA usage.

### D4: Yard area style matches landuse_railway

`railway_yard` areas would shadow `landuse_railway` (defined later in `map.ost`). The yard AREA style uses the same fill color (`#dcdcc8`) as `landuse_railway` so rendering is visually unchanged.

### D5: Symbol coordinate convention

Symbols are defined in a 2.25-unit space; the renderer centers the bounding box and maps symbol y=-1.125 to the top of the image. The `railway_signal` symbol was initially defined with the pole at negative y, rendering upside down; the fix places the pole at positive y (bottom) and the light at negative y (top). The station dot must sit at the square center (1.125, 1.125), not the origin.

### D6: Distinct station/halt/tram_stop symbols

The three symbols were identical solid squares. New design: station = solid square + white center dot (classic station marker), halt = smaller solid square (1.5 units vs 2.25, real-map size hierarchy), tram stop = solid circle. Size differences are normalized away in the SymbolsAll overview sheet but are real in map rendering.

### D7: Reuse existing symbols where possible

`tram_crossing`/`railway_crossing` reuse the `railway_crossing` symbol; `tram_level_crossing` reuses `railway_level_crossing`; `train_station_entrance` reuses `railway_subway_entrance`. New symbols are only added where no existing symbol fits (`switch`, `signal`, `buffer_stop`, `milestone`, `stop`).

## Risks / Trade-offs

- [New types shadow generic types for multi-tagged objects] → Mitigated by D3/D4 (building/landuse styles for AREA-capable types).
- [Obscure infrastructure types (radio, phone, etc.) have no styles and render nothing] → Accepted; types exist for import/search, styles only where visualisation is obvious.
- [Symbol size differences normalized in SymbolsAll overview] → Overview is a debugging aid; real map rendering uses true mm sizes.
- [`railway=platform` remains covered by `public_transport_platform`] → No dedicated type added to avoid double-matching; documented in proposal.

## Migration Plan

No runtime migration. Stylesheet changes take effect on next import/render. Rollback: revert the two stylesheet files. Validation: `ctest -R CheckStyleSheet` (loads `map.ost` + each `.oss` with `--warning-as-error`) and `SymbolsAll` demo for symbol rendering.

## Open Questions

None.
