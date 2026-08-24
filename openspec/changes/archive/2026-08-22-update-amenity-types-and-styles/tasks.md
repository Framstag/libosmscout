# Tasks: update-amenity-types-and-styles

## 1. Extend type definitions in map.ost

- [x] 1.1 Extend existing types per wiki: `amenity_bench` → NODE WAY, `amenity_toilets` → NODE AREA, `amenity_shelter` → NODE AREA
- [x] 1.2 Add sustenance types: `amenity_biergarten`, `amenity_food_court`, `amenity_ice_cream` (NODE AREA)
- [x] 1.3 Add education types: `amenity_college` (+`_building`), `amenity_university` (+`_building`), `amenity_childcare` (+`_building`), `amenity_music_school` (+`_building`), `amenity_language_school` (+`_building`), `amenity_prep_school` (+`_building`), `amenity_dancing_school`, `amenity_driving_school` (+`_building`), `amenity_training`, `amenity_research_institute` (+`_building`), `amenity_dojo` (+`_building`)
- [x] 1.4 Add transportation types: `amenity_bicycle_rental`, `amenity_bicycle_repair_station`, `amenity_motorcycle_parking`, `amenity_car_rental` (+`_building`), `amenity_car_sharing`, `amenity_car_wash`, `amenity_vehicle_inspection`, `amenity_weighbridge`, `amenity_parcel_locker`, `amenity_boat_rental`, `amenity_boat_storage`, `amenity_public_bookcase`, `amenity_bbq` (NODE), `amenity_loading_dock` (NODE, IGNORE), `amenity_trolley_bay` (IGNORE), `amenity_ticket_validator` (NODE WAY, IGNORE), `amenity_compressed_air` (IGNORE), `amenity_vacuum_cleaner` (IGNORE)
- [x] 1.5 Add financial types: `amenity_bureau_de_change` (+`_building`), `amenity_money_transfer`, `amenity_mobile_money_agent`, `amenity_payment_terminal` (NODE, IGNORE)
- [x] 1.6 Add healthcare types: `amenity_clinic` (+`_building`), `amenity_dentist` (+`_building`), `amenity_veterinary` (+`_building`), `amenity_health_post`, `amenity_social_facility` (+`_building`)
- [x] 1.7 Add entertainment/culture types: `amenity_arts_centre` (+`_building`), `amenity_cinema` (+`_building`), `amenity_theatre` (+`_building`), `amenity_community_centre` (+`_building`), `amenity_conference_centre` (+`_building`), `amenity_events_venue` (+`_building`), `amenity_nightclub` (+`_building`), `amenity_casino` (+`_building`), `amenity_gambling`, `amenity_social_centre` (+`_building`), `amenity_studio`, `amenity_fountain`, `amenity_marketplace`
- [x] 1.8 Add public service types: `amenity_townhall` (+`_building`), `amenity_courthouse` (+`_building`), `amenity_fire_station` (+`_building`), `amenity_prison` (+`_building`), `amenity_ranger_station` (+`_building`), `amenity_post_depot` (+`_building`), `amenity_crematorium` (+`_building`), `amenity_funeral_hall` (+`_building`), `amenity_monastery` (+`_building`), `amenity_public_bath` (+`_building`)
- [x] 1.9 Add facility types: `amenity_telephone` (NODE), `amenity_shower` (NODE AREA), `amenity_letter_box` (NODE, IGNORE), `amenity_lounger` (NODE, IGNORE), `amenity_chair` (NODE, IGNORE), `amenity_table` (NODE, IGNORE), `amenity_dressing_room` (IGNORE), `amenity_smoking_area` (IGNORE), `amenity_reception_desk` (IGNORE), `amenity_sanitary_dump_station` (IGNORE)
- [x] 1.10 Add waste types: `amenity_waste_transfer_station`, `amenity_waste_dump_site` (IGNORE)
- [x] 1.11 Add animal/outdoor types: `amenity_animal_shelter`, `amenity_animal_boarding`, `amenity_animal_breeding`, `amenity_feeding_place` (IGNORE), `amenity_game_feeding` (IGNORE), `amenity_hunting_stand`
- [x] 1.12 Add remaining types: `amenity_internet_cafe` (+`_building`), `amenity_driver_training`, `amenity_lavoir`, `amenity_love_hotel` (IGNORE)
- [x] 1.13 Verify no duplicate type names; verify all taginfo values >= 0.01% covered (cross-check: only `fixme`, `nursing_home`, `public_building` excluded — all non-viable/discouraged)

## 2. Add rendering rules

- [x] 2.1 `include/amenity.oss`: define base color constants (fireStation, entertainment, civic, social, market, iceCream, beerGarden, carWash, waterAmenity, huntingStand, hospitalSymbol, pharmacySymbol, postSymbol) before the SYMBOLs that use them (symbols live in the include that uses them)
- [x] 2.2 `include/amenity.oss`: add 21 new SYMBOL icon definitions (fire station, telephone, fountain, cinema, theatre, townhall, marketplace, prison, bbq, hunting stand, ice cream, car wash, parcel locker, shower, public bath, university, motorcycle parking, nightclub, community centre, veterinary, boat rental, biergarten); move the pre-existing amenity symbols from `include/man_made.oss`
- [x] 2.3 `include/amenity.oss`: extend generic area-fill list (`@amenityColor`) with new types; move the generic `NODE.ICON { symbol: amenity; }` list, the grave_yard/post_office/hospital/taxi fills and the minor amenity fill (atm/recycling) from `include/man_made.oss` (excluding types with dedicated `symbol:` icons)
- [x] 2.4 `include/amenity.oss`: add NODE.ICON/AREA.ICON rules for types with dedicated symbols (incl. symbol reuse: hospital cross for clinic/dentist/health_post, university cap for college, bicycle symbol for bicycle_rental, post symbol for post_depot, car symbol for car_rental/car_sharing, note for music_school)
- [x] 2.5 `include/amenity.oss`: add derived color constants (Border/Label variants) for the new base colors
- [x] 2.6 `include/amenity.oss`: add new `_building` types to building color rules (generic amenityBuildingColor; postBuildingColor for post_depot; fireStationColor for fire_station; hospitalBuildingColor for clinic/dentist/veterinary/health_post)
- [x] 2.7 `include/amenity.oss`: extend generic label list with all new non-IGNORE types
- [x] 2.8 `include/amenity.oss`: add category-specific label rules (health pink, fire red, entertainment, civic, social, post, water, market, ice cream, beer garden, car wash, hunting stand) at `@labelSpecialBuildingMag-`
- [x] 2.9 `include/amenity.oss`: add area fill rules at `[MAG detail-]` for category-colored types; university/college/childcare use `@amenityColor`
- [x] 2.10 Verify stylesheets parse: `ctest -R CheckStyleSheet` passes for all 7 stylesheet variants

## 3. Validation

- [x] 3.1 `ctest -R CheckStyleSheet` — all 7 tests pass with `--warning-as-error`
- [x] 3.2 Cross-check new types against taginfo values >= 0.01% — no missing values
- [x] 3.3 `openspec validate` for the change passes (this change)
- [x] 3.4 Import a small OSM extract (60 nodes, 16 areas, 2 ways) with new `amenity=*` values via `Import`; confirmed all new types present in `types.dat` (`strings types.dat`), excluded types (`amenity_public_building`, `amenity_nursing_home`) absent, `_building` variants imported (import log: `amenity_fire_station_building: 1 area`, `amenity_university: 1 node + 1 area`, `amenity_toilets: 1 area`, `amenity_bench: 1 way`)
- [x] 3.5 Render test map with `DrawMapCairo` + `standard.oss` at level 16; pixel-probed PNG confirms all category area fills (`#ff6b6b` fire, `#9acffd` water, `#dc9cdc` entertainment, `#c9c9c9` civic, `#f2cdcd` social, `#f3c3c3` market, `#f2c78f` beer, `#a5cdf0` car wash, `#a08f6f` hunting stand, `#f0f0d8` generic amenity), building colors (hospital color for clinic building), node icons (ice cream cone, fire flame) and name labels render
