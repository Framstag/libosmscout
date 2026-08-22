# Proposal: update-amenity-types-and-styles

## Why

The `amenity=*` key is the most widely used POI key in OSM, but `stylesheets/map.ost` only defines a subset of the documented values. Many common amenities (fire stations, cinemas, universities, clinics, bicycle rentals, parcel lockers, fountains, etc.) currently fall back to the generic `amenity`/`amenity_building` types, losing their specific type identity, POI indexing, and any distinctive rendering. Users cannot search or render these features distinctly.

## What Changes

Extend the import-time type definitions in `stylesheets/map.ost` with the missing `amenity=*` values so these features exist in the database `TypeConfig` with correct element types (node/way/area per the OSM wiki) and can be rendered, searched, and routed to.

Sources: OSM wiki [Key:amenity](https://wiki.openstreetmap.org/wiki/Key:amenity) (documented values + per-value element table) and [taginfo usage data](https://taginfo.openstreetmap.org/keys/amenity#values) (only values with usage >= 0.01% added; element types verified against the individual wiki tag pages).

New types, grouped by wiki category (element types from the wiki, `_building` variants follow the existing amenity pattern for dual-tagged areas):

- **Sustenance**: `biergarten`, `food_court`, `ice_cream` (NODE AREA)
- **Education**: `college` (+`_building`), `university` (+`_building`), `childcare` (+`_building`), `music_school` (+`_building`), `language_school` (+`_building`), `prep_school` (+`_building`), `dancing_school`, `driving_school` (+`_building`), `training`, `research_institute` (+`_building`), `dojo` (+`_building`) (NODE AREA)
- **Transportation**: `bicycle_rental`, `bicycle_repair_station`, `motorcycle_parking`, `car_rental` (+`_building`), `car_sharing`, `car_wash`, `vehicle_inspection`, `weighbridge`, `parcel_locker`, `boat_rental`, `boat_storage`, `public_bookcase` (NODE AREA), `bbq`, `loading_dock`, `trolley_bay` (NODE/NODE AREA), `ticket_validator` (NODE WAY)
- **Financial**: `bureau_de_change` (+`_building`), `money_transfer`, `mobile_money_agent` (NODE AREA), `payment_terminal` (NODE)
- **Healthcare**: `clinic` (+`_building`), `dentist` (+`_building`), `veterinary` (+`_building`), `health_post`, `social_facility` (+`_building`) (NODE AREA)
- **Entertainment & Culture**: `arts_centre` (+`_building`), `cinema` (+`_building`), `theatre` (+`_building`), `community_centre` (+`_building`), `conference_centre` (+`_building`), `events_venue` (+`_building`), `nightclub` (+`_building`), `casino` (+`_building`), `gambling`, `social_centre` (+`_building`), `studio`, `fountain`, `marketplace` (NODE AREA)
- **Public service**: `townhall` (+`_building`), `courthouse` (+`_building`), `fire_station` (+`_building`), `prison` (+`_building`), `ranger_station` (+`_building`), `post_depot` (+`_building`), `crematorium` (+`_building`), `funeral_hall` (+`_building`), `monastery` (+`_building`), `public_bath` (+`_building`) (NODE AREA)
- **Facilities**: `telephone`, `shower` (NODE/NODE AREA); `letter_box`, `lounger`, `chair`, `table`, `dressing_room`, `smoking_area`, `reception_desk`, `sanitary_dump_station` (mostly IGNORE, no visualization yet)
- **Waste management**: `waste_transfer_station` (NODE AREA), `waste_dump_site` (IGNORE)
- **Animals/outdoor**: `animal_shelter`, `animal_boarding`, `animal_breeding`, `hunting_stand` (NODE AREA); `feeding_place`, `game_feeding` (IGNORE)
- **Others**: `internet_cafe` (+`_building`), `driver_training`, `lavoir` (NODE AREA), `love_hotel` (IGNORE)

Existing types extended per wiki: `amenity_bench` gains WAY, `amenity_toilets` and `amenity_shelter` gain AREA.

Style definitions added to `stylesheets/include/amenity.oss` (area fills incl. the generic amenity fill and the grave_yard/post_office/hospital/taxi/minor fills moved from `man_made.oss`, label colors/priorities, building colors, ~21 new SYMBOL icons plus the pre-existing amenity symbols, icon rules). Symbols and styles are defined in the include that uses them (amenity.oss); the OSS loader resolves constants and symbols in module load order, so symbols, colors and style rules live in the same module.

**Explicitly excluded** (not relevant / discouraged / below usage threshold):
- `amenity=public_building` — wiki: "Don't use!"
- `amenity=nursing_home` — wiki: discouraged, use `social_facility` + `social_facility=nursing_home`
- `amenity=office` — wiki: "DO NOT USE!", use `office=*`
- `amenity=water` — wiki: discouraged, unclear meaning
- `amenity=fixme` — Every Door artifact, not a real value
- All values with taginfo usage < 0.01% (e.g. `dog_toilet`, `kneipp_water_cure`, `polling_station`, `baby_hatch`, `music_venue`, `planetarium`, ...)

## Capabilities

### New Capabilities

- `amenity-type-definitions`: import-time `amenity=*` type definitions in `stylesheets/map.ost` with correct element types and features per the OSM wiki, plus matching rendering rules in the `.oss` stylesheets

### Modified Capabilities

None.

## Impact

- `stylesheets/map.ost` — ~125 new `TYPE` definitions in the Amenity section (incl. `_building` variants); `bench`/`toilets`/`shelter` element types extended
- `stylesheets/include/amenity.oss` — base + derived color constants, 33 SYMBOL icon definitions (pre-existing + new), building area colors, generic + special label rules, area fills, icon rules
- `stylesheets/include/man_made.oss` — all amenity styles removed; keeps only man_made/power/barrier symbols and style rules
- Imported databases gain new types in `TypeConfig`; existing databases need re-import to pick up new types
- No C++ code changes; type definitions are data-driven
