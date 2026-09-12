# Design: man-made-types-and-styles

## Context

`stylesheets/map.ost` defines import-time feature types; `stylesheets/include/man_made.oss` defines their rendering. The man made section currently covers 3 `man_made=*` values (bridge, pier, wastewater_plant). The OSM wiki documents 195 values; taginfo shows 73 additional documented values with usage >= 0.05% that are not discouraged. See proposal.md - Why for motivation and the spec for the full requirement list.

The `TypeConfig` is generated from the stylesheet at import time — no C++ code changes are needed. This is a pure stylesheet change, same pattern as the earlier `barrier-type-definitions`, `power-type-definitions`, `waterway-type-definitions`, and `amenity-type-definitions` changes.

## Goals / Non-Goals

**Goals:**
- Add 73 new `TYPE` definitions to the man made section of `stylesheets/map.ost`.
- Add rendering rules in `stylesheets/include/man_made.oss` for the new types where visualisation is obvious.
- Follow existing type/style conventions so the new definitions are indistinguishable in style from the current ones.

**Non-Goals:**
- No changes to existing type or style definitions.
- No C++/import-pipeline changes.
- No changes to other stylesheets (`standard.oss`, `cycle.oss`, `winter-sports.oss`).

## Decisions

### D1: Add types to the existing man made section of map.ost

New `TYPE` definitions go into the `// Man made` section of `stylesheets/map.ost`, after `man_made_wastewater_plant` and before the `// Leisure` section.

- **Alternative considered**: a separate `man_made.ost` include file. Rejected — `map.ost` is the single source of type definitions; the prior type-definition changes all extended `map.ost` in place.
- **Rationale**: keeps type definitions discoverable in one place and matches established project convention.

### D2: Element types follow the OSM wiki element table

Each new type uses the element set from the wiki [Key:man_made](https://wiki.openstreetmap.org/wiki/Key:man_made) table and the individual tag pages: `WAY` only for `cutline`, `pipeline`, `embankment`, `goods_conveyor`, `cooling`, `snow_fence`; `NODE` only for the 21 node types (`surveillance`, `manhole`, `mast`, `utility_pole`, `survey_point`, `petroleum_well`, `flagpole`, `chimney`, `beehive`, `charge_point`, `cross`, `adit`, `lighthouse`, `snow_cannon`, `flare`, `beacon`, `telephone_box`, `oil_gas_separator`, `buoy`, `fuel_pump`, `carpet_hanger`); `AREA` only for `clearcut`, `tunnel`, `spoil_heap`, `tailings_pond`; `NODE AREA` for the 32 node/area types; `WAY AREA` for `breakwater`, `groyne`, `dyke`, `quay`; `NODE WAY` for `advertising`, `gantry`; `NODE WAY AREA` for `antenna`, `avalanche_protection`, `ceremonial_gate`, `geoglyph`.

- **Alternative considered**: `NODE WAY AREA` for everything. Rejected — would import node-tagged `cutline`/`pipeline` and way-tagged `manhole`/`mast` objects that the wiki does not document, polluting the type config.
- **Rationale**: the spec requires wiki-conformant element types; import-time filtering is the cheapest place to enforce them.

### D3: Named landmark types are POIs

Types that are typically named and searchable (`tower`, `communications_tower`, `water_tower`, `lighthouse`, `windmill`, `watermill`, `windpump`, `cross`, `cairn`, `column`, `stupa`, `ceremonial_gate`, `storage_tank`, `silo`, `bunker_silo`, `gasometer`, `reservoir_covered`, `pumping_station`, `monitoring_station`, `crane`, `kiln`, `mineshaft`, `water_well`, `charge_point`, `courtyard`, `works`, `water_works`) get `{Name, NameAlt}` and `POI`; the industrial facilities (`works`, `water_works`) additionally get `ADDRESS`. Area types that represent contiguous structures (`storage_tank`, `silo`, `bunker_silo`, `gasometer`, `reservoir_covered`, `clearcut`, `heap`, `spoil_heap`, `tailings_pond`, `breakwater`, `groyne`, `dyke`, `quay`, `tunnel`) get `MERGE_AREAS`, mirroring `man_made_pier` and `man_made_wastewater_plant`.

- **Alternative considered**: `POI` on every type. Rejected — most node types (manhole, utility_pole, surveillance, survey_point, etc.) are unnamed infrastructure; indexing them as POIs would bloat the location index.
- **Rationale**: matches the existing man made convention (`{Name, NameAlt}` + `MERGE_AREAS`, no blanket `POI`) while making genuinely named landmarks searchable.

### D4: Styles grouped by visual nature in man_made.oss

New style rules in `stylesheets/include/man_made.oss` reuse existing colors and patterns:

- Industrial area fill (`@industrialColor`, `[MAG detail-]`): `man_made_works`, `man_made_water_works`, `man_made_pumping_station`, `man_made_monitoring_station` (like `man_made_wastewater_plant`); lighter fill for `storage_tank`, `silo`, `bunker_silo`, `gasometer`, `reservoir_covered`.
- Ground-disturbance area fill (`[MAG detail-]`): `clearcut` (light brown), `heap`/`spoil_heap`/`tailings_pond` (brown), `bioswale` (green), `clarifier` (blue-grey), `tunnel` (grey), `courtyard` (light).
- Water-structure area/way (`#ffffff`/`@damColor`, `[MAG close-]`): `breakwater`, `groyne`, `quay` (like `man_made_pier`), `dyke` (like `waterway_dam`).
- Linear way rendering (`[MAG close-]`): `cutline` (dashed brown), `pipeline` (dashed grey), `goods_conveyor`/`cooling` (solid grey), `snow_fence` (light grey), `embankment` (brown), `advertising` (thin grey).
- Node icons (`[MAG veryClose-]`): dedicated symbols for the visually distinct structures (tower, communications tower, mast, water tower, lighthouse, windmill, watermill, windpump, cross, cairn, column, stupa, ceremonial gate, chimney, flare, crane, gasometer, storage tank, silo, flagpole, beacon, buoy, dolphin, antenna, satellite dish, utility pole, manhole, survey point, surveillance, water tap, water well, petroleum well, mineshaft, charge point, beehive, telescope, works, kiln, street cabinet, snow cannon, telephone box, fuel pump, adit); reused symbols for similar types (`bunker_silo` → `man_made_silo`, `oil_gas_separator` → `man_made_storage_tank`, `gantry` → `man_made_communications_tower`, `planter` → `barrier_planter`, `avalanche_protection` → `barrier_avalanche_protection`, `water_works`/`pumping_station`/`monitoring_station` → `man_made_works`, `advertising` → `man_made_works`).

- **Alternative considered**: one generic icon for all node types. Rejected — towers, lighthouses, windmills, and storage tanks are visually distinct; the spec requires nature-appropriate rendering.
- **Rationale**: reusing existing colors and symbol patterns keeps the palette coherent and avoids new visual noise.

### D5: Symbols are defined in the man_made module

All new symbols are defined in `stylesheets/include/man_made.oss` itself. Symbols from modules loaded later (`amenity_telephone`, `shop_fuel`, `historic_mine_adit`) are not referenced, because `MODULE "include/man_made"` is processed before those modules in `standard.oss` and symbol lookup is order-dependent.

- **Alternative considered**: referencing `amenity_telephone`/`shop_fuel`/`historic_mine_adit` symbols. Rejected — the style parser reports "Map symbol ... is not defined" because the man_made module is loaded before amenity/shop/historic.
- **Rationale**: self-contained module; the telephone, fuel pump, and adit symbols are simple copies of the same designs.

## Risks / Trade-offs

- [New types may collide with existing type names] → Verified against current `map.ost`; all 73 names are new. Import fails loudly on duplicate type names, so a collision would be caught in CI.
- [Over-rendering at low zoom clutters the map] → All new area/way rules are gated behind `[MAG detail-]`/`[MAG close-]` and node icons behind `[MAG veryClose-]`, matching the existing man made rules; nothing renders at low zoom.
- [POI indexing of named structures adds data bloat] → Only genuinely named landmark types are POIs; unnamed infrastructure nodes (manhole, utility_pole, etc.) are not. Acceptable.
- [Import-time behavior change requires database re-import] → Stylesheet-only change; existing databases keep working, new types appear after re-import. Rollback = revert the two stylesheet files.

## Migration Plan

1. Edit `stylesheets/map.ost` (man made section) and `stylesheets/include/man_made.oss`.
2. Validate stylesheets parse (run the existing style validation in CI).
3. Re-import a test database and verify new types appear in `TypeConfig` and render.
4. Rollback: revert the two files; no data migration needed.

## Open Questions

None.
