# Tasks: types-and-styles-for-leisure

## 1. Type definitions in map.ost

- [x] 1.1 Add `leisure_picnic_table`, `leisure_outdoor_seating`, `leisure_firepit`, `leisure_dog_park`, `leisure_beach_resort`, `leisure_recreation_ground`, `leisure_bathing_place`, `leisure_swimming_area` (AREA only), `leisure_resort`, `leisure_summer_camp`, `leisure_schoolyard` TYPE definitions to the leisure section of `stylesheets/map.ost` (spec: Outdoor leisure area types) and verify each type name is unique in the file
- [x] 1.2 Add `leisure_fitness_centre`, `leisure_sports_hall`, `leisure_bleachers` (AREA only), `leisure_miniature_golf`, `leisure_bowling_alley`, `leisure_disc_golf_course`, `leisure_climbing`, `leisure_horse_riding`, `leisure_trampoline_park`, `leisure_dance` TYPE definitions to the leisure section of `stylesheets/map.ost` (spec: Sports facility types) and verify each type name is unique in the file
- [x] 1.3 Add `leisure_amusement_arcade`, `leisure_adult_gaming_centre`, `leisure_bandstand`, `leisure_escape_game`, `leisure_indoor_play`, `leisure_hackerspace`, `leisure_sauna`, `leisure_hot_tub`, `leisure_tanning_salon` TYPE definitions to the leisure section of `stylesheets/map.ost` (spec: Entertainment and wellness types) and verify each type name is unique in the file
- [x] 1.4 Extend `leisure_slipway` from `NODE` to `NODE WAY` in `stylesheets/map.ost` (spec: Slipway element extension) and verify the type line reads `= NODE WAY ("leisure"=="slipway")`
- [x] 1.5 Verify no TYPE definitions were added for discouraged/undocumented values (`leisure=yes`, `hot_spring`, `turkish_bath`, `arena`, `sailing_club`, `wildlife_hide`, `high_ropes_course`, `sunbathing`, `barefoot`, `hammock`) by grepping `stylesheets/map.ost` for the corresponding type names

## 2. Style definitions in leisure.oss

- [x] 2.1 Add green park-like area fills for `leisure_dog_park`, `leisure_recreation_ground`, `leisure_disc_golf_course`, `leisure_miniature_golf` in `stylesheets/include/leisure.oss` (spec: Area rendering for new leisure types) and verify the rules reference existing green colors
- [x] 2.2 Add water-like area fills for `leisure_swimming_area`, `leisure_hot_tub`, `leisure_bathing_place` and a sand fill for `leisure_beach_resort` in `stylesheets/include/leisure.oss` (spec: Area rendering for new leisure types) and verify the rules reference existing water colors and the existing `@sandColor` CONST from `natural.oss`
- [x] 2.3 Add building-like area fills for the venue types (`leisure_fitness_centre`, `leisure_sports_hall`, `leisure_amusement_arcade`, `leisure_adult_gaming_centre`, `leisure_escape_game`, `leisure_indoor_play`, `leisure_hackerspace`, `leisure_sauna`, `leisure_tanning_salon`, `leisure_bowling_alley`, `leisure_dance`, `leisure_bandstand`, `leisure_resort`, `leisure_summer_camp`, `leisure_schoolyard`, `leisure_horse_riding`, `leisure_climbing`, `leisure_trampoline_park`, `leisure_picnic_table`, `leisure_outdoor_seating`, `leisure_firepit`, `leisure_bleachers`) in `stylesheets/include/leisure.oss` and verify the rules reference the light leisure venue fill `#f1eee8`
- [x] 2.4 Add the new types to the `[MAG close-]` AREA.TEXT label block and the `[MAG veryClose-]` NODE.TEXT block in `stylesheets/include/leisure.oss` (spec: Area rendering for new leisure types) and verify each new type appears in at least one label rule
- [x] 2.5 Add `leisureSymbolColor` and `leisureWaterSymbolColor` CONSTs and define a `SYMBOL` block for each new leisure type that accepts NODE elements (28 symbols) in `stylesheets/include/leisure.oss` (spec: Node icon rendering for new leisure types) and verify each symbol is referenced by a `NODE.ICON` rule
- [x] 2.6 Add `NODE.ICON` rules for the 28 node-capable new types in the `[MAG veryClose-]` block of `stylesheets/include/leisure.oss` (spec: Node icon rendering for new leisure types) and verify `leisure_bleachers` and `leisure_swimming_area` have no node icon rule

## 3. Verification

- [x] 3.1 Verify the stylesheets parse by running the style validation on `map.ost` + `standard.oss` and confirming no type/style parse errors
- [x] 3.2 Verify the build compiles without errors (`cmake --build build` for the affected targets)
- [x] 3.3 Verify existing tests still pass (`cd build && ctest -j 2 --output-on-failure`)
- [x] 3.4 Re-import a test database and verify the new types exist in the `TypeConfig` (e.g. via `DumpData` or a type listing) and that areas render with the expected fills
