# Proposal — cleanup-oss-base-files

## Why

Base style files (`*.oss` in `stylesheets/`) still contain "fat" inline type lists that duplicate what the modular include files (`stylesheets/include/*.oss`) already define, or that could be expressed with GROUP selectors. The consequence: adding a new type to `map.ost` forces manual edits in multiple `.oss` files.

Concrete trigger: after new tourism types were added to `map.ost` (change `update-tourism-types`, e.g. `tourism_apartment_building`, `tourism_gallery_building`, `tourism_wilderness_hut_building`), `public-transport.oss` had to be edited: its building rule enumerates ~40 explicit `*_building` type names (lines 330-371). Every new `*_building` type variant (tourism, shop, amenity, historic, …) requires the same manual edit again.

The archive design of `update-tourism-types` claimed "no other .oss files reference tourism types" — that check missed `public-transport.oss`. This is exactly the maintenance trap the fat lists create.

## What Changes

Replace explicit, per-type lists in base files with GROUP-based selectors and module includes, so that:

- new types added to `map.ost` with the established `GROUP` conventions need **zero** `.oss` edits;
- duplicated rule blocks and CONST definitions are removed (single source of truth in `include/`);
- rendering output stays visually identical for existing types.

### Verified facts (investigation)

- `[GROUP a, b]` in OSS = intersection: first collect all types in group `a`, then drop types not in group `b` (`Parser.cpp` `STYLEFILTER_GROUP`). `[GROUP building]` therefore selects all types carrying the `building` group — 60 types in `map.ost` today.
- Building types **missing** the `building` group in `map.ost` (8): generic `building`, `building_garage`, `landuse_farmyard_building` (GROUP landuse), `temple_building`, `shrine_building`, `christian_*_building`, `jewish_synagogue_building`, `muslim_mosque_building`, `worship_building` (GROUP religious), `leisure_building`, `sport_building`, `military_bunker_building` (no GROUP).
- Existing GROUP+building selectors in includes (`[GROUP historic, building]`, `[GROUP office, building]`, `[GROUP shop, building]`, `[GROUP tourism, building]`) all intersect with a category group — completing the `building` group on the 8 missing types does not change their behavior.
- `cycle.oss` duplicates the CONST color block and the building/label rules of `include/amenity.oss` (CONST lines 320-350 vs. `include/amenity.oss` lines 7-30; rules lines 874-1019 vs. `include/amenity.oss` lines 32-128) but does **not** include that module. The inline copy is stale: it misses `amenity_bus_station`, `amenity_bicycle_parking`, `amenity_toilets`, `amenity_waste_disposal`, `amenity_shelter`, `amenity_atm`, `amenity_charging_station`.
- The generic building block (farmyard fill, generic `building` fill, `building_garage`, address/building-name labels) is duplicated nearly verbatim in `standard.oss` (~lines 215-270), `cycle.oss` (lines 883-973) and `winter-sports.oss` (~lines 225-285), each using the same parent-defined CONSTs (`@buildingColor`, `@buildingBorderColor`, `@buildingLabelColor`, MAGs).
- `boundaries.oss` inlines the land/sea tile rules (lines 12-15) that `include/land_sea.oss` already provides; it defines the required `@waterColor`/`@landColor`/`@unknownColor` CONSTs itself. `railways.oss` and `motorways.oss` already use `MODULE "include/land_sea"` for the same purpose.

## Capabilities

### New Capabilities

None. Pure stylesheet refactor: no API, no data format, no user-visible feature change.

### Modified Capabilities

None. `map.ost` GROUP metadata is adjusted for 8 types, but no rule output changes for existing types.

## Impact

- `stylesheets/map.ost` — add `building` to GROUP of the 8 missing building types (keeps type definition order and all other groups; `IGNORE` types untouched).
- `stylesheets/public-transport.oss` — replace the ~40-type building list with `[GROUP building]` + explicit tail list for the 8 non-group types; keep its own colors (`#f0f0f0` fill, `#ffe0e0` amenity override via nested `[GROUP amenity]`). Remaining duplicate rules (places, waterways, tiles) are intentionally custom to this style — documented, not changed.
- `stylesheets/cycle.oss` — add `MODULE "include/amenity"` (and `include/sport`, `include/leisure` where its inline copies duplicate those modules), delete duplicated CONST and rule blocks.
- `stylesheets/standard.oss`, `stylesheets/winter-sports.oss` — replace inline generic building block with new `MODULE "include/buildings"`.
- `stylesheets/boundaries.oss` — replace inline tile rules with `MODULE "include/land_sea"`.
- New file `stylesheets/include/buildings.oss` — generic building styling (farmyard, `building`, `building_garage`, address/building-name labels), parameterized by parent-file CONSTs; usable by standard, cycle, winter-sports, public-transport.
- `railways.oss`, `motorways.oss`, `coastlines.oss`, `basemap-render.oss` — no change (already minimal or intentionally custom).

### Out of scope / deferred

- New include files for the minimal debug styles (`railways.oss`, `motorways.oss`, `boundaries.oss`) share place-label rules that `include/place.oss` styles differently; a shared `include/place_basic.oss` is a possible follow-up.
- `include/religious.oss` and `include/landuse.oss` still use explicit building lists; they become redundant once `[GROUP building]` is complete and can be simplified to `[GROUP religious, building]` / `[GROUP landuse, building]` in the same follow-up.

### Verification

- `openspec validate cleanup-oss-base-files --type change` green.
- Style sheet load tests (StyleConfig parsing) pass for all touched files.
- Render comparison: `standard.oss`, `cycle.oss`, `winter-sports.oss`, `public-transport.oss` on a fixed extract produce identical output for existing types (spot-check via existing render tooling); any intentional additions (e.g. previously unstyled amenity subtypes in cycle.oss) are reviewed.
