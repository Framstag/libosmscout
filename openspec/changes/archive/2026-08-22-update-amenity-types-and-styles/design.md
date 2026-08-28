# Design: update-amenity-types-and-styles

## Context

`stylesheets/map.ost` defines import-time OSM feature types. The Amenity section covers ~55 `amenity=*` values; the OSM wiki [Key:amenity](https://wiki.openstreetmap.org/wiki/Key:amenity) documents 333 values and taginfo shows ~9500 distinct values in the wild. Many common values (fire station, cinema, university, clinic, bicycle rental, parcel locker, ...) had no dedicated type and fell back to the generic `amenity`/`amenity_building` catch-alls, losing POI indexing and distinctive rendering.

Selection criteria (per request): values documented on the OSM wiki AND taginfo usage >= 0.01% AND not discouraged. Element types taken from the wiki's per-value element table, verified against the individual tag pages for values not listed in the main table (`loading_dock`, `trolley_bay`, `ticket_validator`, `letter_box`, `chair`, `table`, `smoking_area`, `reception_desk`, `mobile_money_agent`, `prep_school`, `dojo`, `vacuum_cleaner`, `health_post`, `waste_dump_site`, `game_feeding`).

## Goals / Non-Goals

**Goals:**
- Add type definitions for all qualifying `amenity=*` values missing from `map.ost` (~125 new types incl. `_building` variants)
- Element types (NODE/WAY/AREA) per the OSM wiki element table
- `_building` AREA variants for building-typical amenities, following the existing pattern (bank/cafe/fuel/...)
- Features per type: `{Name, NameAlt, ...}` and `ADDRESS POI`/`GROUP amenity` following existing conventions
- Rendering rules in `include/amenity.oss` and `include/man_made.oss` so new types are visible
- `IGNORE` (no storage, no rendering) for high-frequency street furniture without obvious visualization, following the existing pattern (vending_machine, clock, grit_bin, ...)

**Non-Goals:**
- No C++ changes — type definitions are data-driven
- No new SVG icon resources — new icons are vector SYMBOLs defined in `.oss`
- No deprecated/discouraged values: `public_building`, `nursing_home`, `office`, `water`
- No values below 0.01% taginfo usage (e.g. `dog_toilet`, `polling_station`, `baby_hatch`, `planetarium`, `music_venue`, ...)

## Decisions

### D1: Type naming follows existing convention

`amenity_<value>` with underscores, e.g. `amenity_fire_station`, `amenity_bureau_de_change`. `_building` suffix for AREA-only variants of dual-tagged (amenity+building) objects, e.g. `amenity_cinema_building`.

### D2: Element types from wiki

Per-value element support from the wiki. Examples:
- `amenity=bench` (existing) → NODE WAY (wiki: node + way)
- `amenity=toilets`, `amenity=shelter` (existing) → NODE AREA (wiki: node + area)
- `amenity=ticket_validator` → NODE WAY (wiki: node + way)
- `amenity=telephone`, `amenity=bbq`, `amenity=letter_box`, `amenity=chair`, `amenity=table`, `amenity=lounger`, `amenity=loading_dock`, `amenity=payment_terminal` → NODE only
- `amenity=parcel_locker`, `amenity=clinic`, `amenity=fire_station`, ... → NODE AREA
- `_building` variants → AREA only, condition `EXISTS "building" AND !("building" IN ["no","false","0"])` (existing pattern)

### D3: IGNORE policy

`IGNORE` (not stored, per Preprocess.cpp `GetIgnore()` checks) for types without visualization that match the existing "too many / no visualisation yet" pattern: `chair`, `table`, `lounger`, `letter_box`, `loading_dock`, `trolley_bay`, `ticket_validator`, `payment_terminal`, `compressed_air`, `vacuum_cleaner`, `smoking_area`, `dressing_room`, `reception_desk`, `sanitary_dump_station`, `waste_dump_site`, `feeding_place`, `game_feeding`, `love_hotel`.

### D4: Style architecture

The OSS loader resolves constants and symbols in module load order, and style rules MERGE per attribute (later rules override same attribute, see `StyleConfig::GetFeatureStyle`). Consequences:

1. **Symbols and styles live in the include that uses them**: all `amenity_*` SYMBOL definitions, icon rules AND amenity style rules (area fills, labels, building colors) are defined in `include/amenity.oss`, together with the base color constants they use (`fireStationColor`, `entertainmentColor`, `civicColor`, `socialColor`, `marketColor`, `iceCreamColor`, `beerGardenColor`, `carWashColor`, `waterAmenityColor`, `huntingStandColor`, `hospitalSymbolColor`, `pharmacySymbolColor`, `postSymbolColor`). `man_made.oss` keeps only its own symbols (`man_made_wastewater_plant`, `power_tower`, `power_pole`) and power/barrier/man_made style rules.
2. **Symbols referenced by icon rules must be defined before the referencing module** — satisfied because symbols, colors and icon rules are in the same module (amenity.oss). Cross-module symbol references (`christian_church_cross` from `religious.oss`) are not possible from an earlier-loaded module — monastery therefore uses the generic square icon.
3. **Icon merge**: types with a dedicated `symbol:` icon must NOT appear in the generic `NODE.ICON { symbol: amenity; }` list, otherwise the generic rule (later, same attribute) overwrites the dedicated icon. Types using `name:` icons (fast_food, restaurant) coexist because `name` and `symbol` are different attributes.
4. **Labels**: generic label rule (amenityLabelColor, priority @labelPrioAmenity) covers all new types; category-specific rules (health pink, fire red, entertainment, civic, social, post, water, market, ice cream, beer, car wash, hunting) override color+priority afterwards.
5. **Area fills**: special fills (`[MAG detail-]` in `amenity.oss`) override the generic `@amenityColor` fill (also in `amenity.oss` since the man_made generic fills moved here) because the special-fill rules come later in the file. Building variants of the health types and the fire station are filled by the `@specialBuildingMag-` rules instead (hospital/fire building colors).

### D5: Building rendering

New `_building` types join the generic amenity building color list in `amenity.oss` (`@amenityBuildingColor`). Exceptions with distinctive building colors: `amenity_fire_station_building` (red), `amenity_post_depot_building` (post yellow), `amenity_clinic/dentist/veterinary/health_post_building` (hospital color). Non-`_building` area types with a `building` tag fall back to the existing `amenity_building` catch-all (GROUP amenity).

### D6: Icons

~21 new vector SYMBOLs in `include/amenity.oss` for iconic amenities (fire station, telephone, fountain, cinema, theatre, townhall, marketplace, prison, bbq, hunting stand, ice cream, car wash, parcel locker, shower, public bath, university, motorcycle parking, nightclub, community centre, veterinary, boat rental, biergarten), together with the pre-existing amenity symbols moved from `man_made.oss`. Reused symbols: clinic/dentist/health_post → `amenity_hospital`, college → `amenity_university`, bicycle rental → `amenity_bicycle_parking`, post depot → `amenity_post_office`, car rental/sharing → `amenity_car_wash`, music school → `amenity_nightclub`. All other new types get the generic `amenity` square.

## Impact

- `stylesheets/map.ost` — ~125 new `TYPE` definitions; 3 existing types extended (bench/toilets/shelter)
- `stylesheets/include/amenity.oss` — base + derived color constants, 33 SYMBOL icon definitions (pre-existing + new), all amenity style rules (building colors, generic + special label rules, generic + special area fills incl. the ones moved from `man_made.oss`, icon rules)
- `stylesheets/include/man_made.oss` — all amenity styles removed; keeps only power/barrier/man_made rules and symbols
- All three root stylesheets (`standard.oss`, `cycle.oss`, `winter-sports.oss`) pick up the changes via module inclusion — no per-stylesheet edits needed
- Existing databases need re-import to gain the new types; `TypeConfig` grows accordingly
