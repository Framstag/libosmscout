## Context

The libosmscout stylesheets consist of two file types: `.ost` type definitions (`map.ost`, single file, no includes) and `.oss` style modules (`stylesheets/include/*.oss`, included by `standard.oss` and friends). Waterway types live in the "Waterways and assorted" section of `map.ost`; waterway styles live in `include/waterway.oss`, which is included by `standard.oss`, `cycle.oss`, `winter-sports.oss`, and `public-transport.oss`. Rendering priority is controlled by `GROUP` statements in each `.oss` root file. See proposal.md - Why for motivation.

## Goals / Non-Goals

**Goals:**
- Add 22 new `waterway` type definitions to `map.ost` with object types matching the OSM wiki
- Add style definitions and symbols for the new types in `include/waterway.oss`
- Extend `GROUP` priority lines in all four `.oss` root files
- Keep all existing waterway types/styles byte-identical

**Non-Goals:**
- No changes to `basemap.ost` (basemap import has no waterway types today)
- No changes to C++ code, import pipeline, or rendering backends
- No new symbols for linear waterways (reuse `stream_arrow`)
- No support for `waterway=*` values below 500 taginfo uses or marked discouraged/deprecated

## Decisions

### D1: Add types inline in `map.ost` rather than splitting into a separate include file
`map.ost` has no `INCLUDE` mechanism — all type definitions are inline in one file. The waterway section already holds 13 types; the 22 new ones are appended after `waterway_dam`, keeping the section contiguous.
- **Alternative considered**: creating a `waterway.ost` include — rejected because the OST format in this project does not support includes; all other feature groups (railway, natural, etc.) are inline in `map.ost`.

### D2: Reuse existing style groups instead of creating new ones
New linear types (`flowline`, `tidal_channel`, `drystream`) join the `waterway_stream` group (2m width, suburb zoom); `pressurised`, `derelict_canal`, `drainage_channel`, `link`, `fairway` join the `waterway_drain`/`waterway_ditch` group (3m, detail zoom); barriers (`rapids`, `sluice_gate`, `floodgate`, `check_dam`, `floating_barrier`, `flow_control`, `fish_pass`) join the `waterway_weir` group (dashed line); facilities (`fuel`, `sanitary_dump_station`) join the `waterway_boatyard` area style.
- **Alternative considered**: dedicated style blocks per new type — rejected: duplicates rendering logic and diverges from the project's "reuse similar types" convention; the wiki's own rendering treats these as water-colored lines.

### D3: New node symbols for POI-like types, reusing `@waterLabelColor`
Seven new geometric symbols (`waterway_access_point`, `waterway_soakhole`, `waterway_water_point`, `waterway_milestone`, `waterway_stream_end`, `waterway_fuel`, `waterway_sanitary_dump_station`) follow the existing symbol style (simple shapes in `@waterLabelColor`).
- **Alternative considered**: reusing amenity symbols (e.g. `amenity_drinking_water` for `water_point`) — rejected: symbols are module-scoped and waterway module should stay self-contained; also `waterway=fuel` is a boat fuel station, not a road fuel station.

### D4: POI/ADDRESS indexing for facility types
`waterway_fuel`, `waterway_sanitary_dump_station` get `ADDRESS POI` (matching `waterway_dock`/`waterway_boatyard` and `amenity_fuel`); `waterway_water_point`, `waterway_access_point`, `waterway_milestone` get `POI` so they are searchable by name.
- **Alternative considered**: no indexing (like `waterway_lock_gate`) — rejected: these are named, user-relevant places; POI indexing is the established pattern for such facilities.

### D5: GROUP priority updates in all four `.oss` root files
`standard.oss`, `cycle.oss`, `winter-sports.oss`, and `public-transport.oss` each declare waterway GROUPs; all four are updated so the new types render at the same priority as their templates regardless of which style is loaded.
- **Alternative considered**: updating only `standard.oss` — rejected: the other styles would render new types at default priority, causing inconsistent z-ordering.

## Risks / Trade-offs

- [New types render at wrong priority in a style file that was missed] → Mitigation: all four `.oss` files that reference waterway types were located via grep and updated; `CheckStyleSheet-*` tests validate every `.oss` against `map.ost`.
- [Symbol shapes are approximate (simple geometric forms)] → Mitigation: acceptable for a data-driven stylesheet; symbols follow the existing waterway module aesthetic and can be refined later without spec changes.
- [`flow_control` wiki page is a stub without element documentation] → Mitigation: element types (NODE WAY) derived from taginfo node/way counts (596 nodes, 154 ways) and the similar `sluice_gate` page; if the wiki later documents different elements, the type line is a one-line change.
- [`drystream`/`derelict_canal` rendered solid like flowing water though often dry] → Mitigation: accepted trade-off for simplicity; dashed variants can be added later as style-only changes.

## Migration Plan

- No data migration: stylesheets are consumed at import/render time; existing databases re-imported with the new `map.ost` pick up the new types automatically.
- Rollback: revert the stylesheet files; no code or schema changes to undo.

## Open Questions

None.
