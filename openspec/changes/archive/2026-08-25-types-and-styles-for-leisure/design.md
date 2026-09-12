# Design: types-and-styles-for-leisure

## Context

`stylesheets/map.ost` defines import-time feature types; `stylesheets/include/leisure.oss` defines their rendering. The leisure section currently covers 19 `leisure=*` values. The OSM wiki documents ~45 values; taginfo shows 31 additional documented values with usage >= 0.01% that are not discouraged. See proposal.md - Why for motivation and the spec for the full requirement list.

The `TypeConfig` is generated from the stylesheet at import time — no C++ code changes are needed. This is a pure stylesheet change, same pattern as the earlier `tourism-type-definitions`, `natural-type-definitions`, `shop-type-definitions`, and `amenity-type-definitions` changes.

## Goals / Non-Goals

**Goals:**
- Add 31 new `TYPE` definitions to the leisure section of `stylesheets/map.ost`.
- Extend `leisure_slipway` to accept WAY elements.
- Add rendering rules in `stylesheets/include/leisure.oss` for the new types where visualisation is obvious.
- Follow existing type/style conventions so the new definitions are indistinguishable in style from the current ones.

**Non-Goals:**
- No changes to existing type or style definitions (except `leisure_slipway` element extension).
- No new symbols/icons — only fills, borders, and labels, reusing existing colors and patterns.
- No C++/import-pipeline changes.
- No changes to other stylesheets (`standard.oss`, `public-transport.oss`, `roads.oss`, `winter-sports.oss`).

## Decisions

### D1: Add types to the existing leisure section of map.ost

New `TYPE` definitions go into the `// Leisure` section of `stylesheets/map.ost`, between `leisure_ice_rink` and the `// Amenity` section.

- **Alternative considered**: a separate `leisure.ost` include file. Rejected — `map.ost` is the single source of type definitions; the four prior type-definition changes all extended `map.ost` in place.
- **Rationale**: keeps type definitions discoverable in one place and matches established project convention.

### D2: Element types follow the OSM wiki element table

Each new type uses the element set from the wiki [Key:leisure](https://wiki.openstreetmap.org/wiki/Key:leisure) table: `NODE AREA` for most values, `AREA` only for `bleachers` and `swimming_area` (wiki lists area only), and `NODE WAY` for the extended `leisure_slipway`.

- **Alternative considered**: `NODE AREA` for everything. Rejected — would import node-tagged `bleachers`/`swimming_area` objects that the wiki does not document, polluting the type config.
- **Rationale**: the spec requires wiki-conformant element types; import-time filtering is the cheapest place to enforce them.

### D3: Type flags follow existing leisure conventions

- `ADDRESS POI` for venue-like types (e.g. `leisure_fitness_centre`, `leisure_sports_hall`, `leisure_amusement_arcade`, `leisure_sauna`, `leisure_bowling_alley`, `leisure_dance`, `leisure_escape_game`, `leisure_indoor_play`, `leisure_hackerspace`, `leisure_tanning_salon`, `leisure_adult_gaming_centre`, `leisure_resort`, `leisure_horse_riding`, `leisure_climbing`, `leisure_trampoline_park`, `leisure_miniature_golf`, `leisure_disc_golf_course`, `leisure_bandstand`, `leisure_schoolyard`, `leisure_dog_park`, `leisure_beach_resort`, `leisure_picnic_table`, `leisure_outdoor_seating`, `leisure_firepit`, `leisure_hot_tub`, `leisure_swimming_area`, `leisure_bathing_place`, `leisure_recreation_ground`, `leisure_summer_camp`), mirroring `leisure_sports_centre` / `leisure_stadium`.
- `MERGE_AREAS` for area-heavy types (`leisure_dog_park`, `leisure_recreation_ground`, `leisure_beach_resort`, `leisure_swimming_area`, `leisure_bathing_place`, `leisure_resort`, `leisure_summer_camp`, `leisure_schoolyard`, `leisure_disc_golf_course`, `leisure_miniature_golf`, `leisure_trampoline_park`), mirroring `leisure_park` / `leisure_garden`.
- `OPTIMIZE_LOW_ZOOM` for large-area types, mirroring `leisure_park` / `leisure_common`.
- `GROUP routingPOI` for routing-relevant POIs, mirroring `leisure_sports_centre`.

- **Alternative considered**: minimal flags (`NODE AREA` + name only) for all new types. Rejected — new types would be invisible to POI search and routing, inconsistent with existing leisure types of the same nature.
- **Rationale**: flag parity with the closest existing type keeps behavior consistent and predictable.

### D4: Styles grouped by visual nature in leisure.oss

New style rules in `stylesheets/include/leisure.oss` reuse existing colors and patterns:

- Green park-like fill (`@playgroundColor`-family / `#c6f0cf`-family): `leisure_dog_park`, `leisure_recreation_ground`, `leisure_disc_golf_course`, `leisure_miniature_golf` (golf-like, reuse `leisure_golf_course` fill `#c7f1a3`).
- Water fill (`#b5d6f1`-family / `#74daff`): `leisure_swimming_area`, `leisure_hot_tub`, `leisure_bathing_place`.
- Sand fill: `leisure_beach_resort` (reuse existing `@sandColor` from `natural.oss`).
- Building-like fill (`@buildingColor`): `leisure_fitness_centre`, `leisure_sports_hall`, `leisure_amusement_arcade`, `leisure_adult_gaming_centre`, `leisure_escape_game`, `leisure_indoor_play`, `leisure_hackerspace`, `leisure_sauna`, `leisure_tanning_salon`, `leisure_bowling_alley`, `leisure_dance`, `leisure_bandstand`, `leisure_resort`, `leisure_summer_camp`, `leisure_schoolyard`, `leisure_horse_riding`, `leisure_climbing`, `leisure_trampoline_park`, `leisure_picnic_table`, `leisure_outdoor_seating`, `leisure_firepit`, `leisure_bleachers`.
- Labels: add new types to the existing `[MAG close-]` AREA.TEXT label block and the `[MAG veryClose-]` NODE.TEXT block.

- **Alternative considered**: one generic fill for all new types. Rejected — water-related types would look like parks; the spec requires nature-appropriate fills.
- **Rationale**: reusing existing colors keeps the palette coherent and avoids new visual noise.

### D5: Slipway extension

`leisure_slipway` changes from `NODE` to `NODE WAY`. The existing `leisure_slipway` NODE.ICON style rule stays; WAY elements get no dedicated style (slipway ways are short ramps, typically rendered via the water border rules already present).

- **Alternative considered**: separate `leisure_slipway_way` type. Rejected — the wiki documents a single `leisure=slipway` value; one type with two element sets is simpler and matches `leisure_track` (NODE WAY AREA).
- **Rationale**: minimal, wiki-conformant.

## Risks / Trade-offs

- [New types may collide with existing type names] → Verified against current `map.ost`; all 31 names are new. Import fails loudly on duplicate type names, so a collision would be caught in CI.
- [Over-rendering at low zoom clutters the map] → `OPTIMIZE_LOW_ZOOM` only on large-area types; small POI types render only at close zoom via the existing MAG gates.
- [Style conflicts with overlapping amenity/tourism types (e.g. `leisure=sauna` vs `amenity=public_bath`)] → Fills are subtle and follow existing leisure palette; label priority uses the existing `@labelPrioLeisure`.
- [Import-time behavior change requires database re-import] → Stylesheet-only change; existing databases keep working, new types appear after re-import. Rollback = revert the two stylesheet files.

## Migration Plan

1. Edit `stylesheets/map.ost` (leisure section) and `stylesheets/include/leisure.oss`.
2. Validate stylesheets parse (import a small extract or run the existing style validation in CI).
3. Re-import a test database and verify new types appear in `TypeConfig` and render.
4. Rollback: revert the two files; no data migration needed.

## Open Questions

None.
