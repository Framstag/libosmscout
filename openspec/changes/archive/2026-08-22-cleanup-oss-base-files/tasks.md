# Tasks — cleanup-oss-base-files

References: design D1-D6. No open questions in design.md.

## 1. map.ost — complete `building` GROUP (D1)

- [x] 1.1 Add `building` to the GROUP of generic types `building` and `building_garage` (currently no GROUP) and verify `grep -A4 "TYPE building$" map.ost` shows the group line
- [x] 1.2 Add `building` to the GROUP of `landuse_farmyard_building` (currently `GROUP landuse`) and verify the line reads `GROUP landuse, building`
- [x] 1.3 Add `building` to the GROUP of the 8 religious building types (`temple_building`, `shrine_building`, `christian_cathedral_building`, `christian_chapel_building`, `christian_church_building`, `jewish_synagogue_building`, `muslim_mosque_building`, `worship_building`; each currently `GROUP religious, routingPOI`) and verify all read `GROUP religious, building, routingPOI`
- [x] 1.4 Add `building` to the GROUP of `leisure_building`, `sport_building`, `military_bunker_building` (currently no GROUP) and verify each has a GROUP line
- [x] 1.5 Verify every renderable type whose name ends `_building` (plus `building`, `building_garage`) now carries `building` in GROUP: run the grep check from the proposal (all `*_building` names → GROUP contains `building`), leaving only `IGNORE` types (`building_any`, `building_entrance`, `related_building_any`) untouched — also found and fixed `amenity_library_building` (was `GROUP amenity, routingPOI`)

## 2. New module include/buildings.oss (D4)

- [x] 2.1 Create `stylesheets/include/buildings.oss` containing the generic building block, parameterized by parent CONSTs: farmyard fill/border (`#bcbcbc`/`#887b7b`), generic `building` fill/border (`@buildingColor`/`@buildingBorderColor`), `building_garage` fill (`#bcbcbc`), address labels + building-name label (`@buildingLabelColor`, `@labelPrioBuilding`), farmyard text label with `color: @buildingLabelColor` and `priority: @labelPrioLanduse`; gates: `IF _building` / `IF _minorBuilding` with `@buildingMag`, `@minorBuildingMag`, `@labelBuildingMag` — exact-type selectors only, no `[GROUP building]`
- [x] 2.2 Keep relative rule order identical to standard.oss (farmyard+building under `@buildingMag`, garage under `@minorBuildingMag`, labels under `@labelBuildingMag`) and verify the file parses standalone via `OSTAndOSSTest map.ost include/buildings.oss` (module needs parent CONSTs — verified via parents in task 3)

## 3. standard.oss — adopt include/buildings.oss

- [x] 3.1 Add `MODULE "include/buildings"` after `MODULE "include/power"` (before `STYLE`) and delete the inline building block from the STYLE section (farmyard, `building`, garage, address, building-name, farmyard-text rules)
- [x] 3.2 Verify `CheckStyleSheet-standard.oss` test passes (`cd build && ctest -R CheckStyleSheet-standard.oss --output-on-failure`) and rendering is unchanged (PerformanceTest compare, standard.oss — render compare covered in 8.4)

## 4. winter-sports.oss — drop module duplicates + adopt buildings.oss (D5)

- [x] 4.1 Delete duplicate rules covered by `include/leisure.oss` (already included): `[TYPE leisure_stadium]` block, `[TYPE leisure_sports_centre, leisure_building]` fill block, `[TYPE leisure_building]` text rule, `[TYPE leisure_pitch, leisure_fitness_station]` text rule
- [x] 4.2 Add `MODULE "include/buildings"` (after `include/power`) and delete the remaining inline generic building block (farmyard, `building`, garage, address, building-name, farmyard-text rules)
- [x] 4.3 Verify `CheckStyleSheet-winter-sports.oss` passes and PerformanceTest render shows only the reviewed delta (farmyard text label color gains `@buildingLabelColor`)

## 5. cycle.oss — adopt natural/waterway/landuse/leisure/amenity/sport/power modules (D3)

- [x] 5.1 Add the `MODULE` directives for natural, waterway, amenity, sport, power, buildings (standard.oss relative order; landuse/leisure/aerialway modules NOT adopted — see 5.5) after `MODULE "include/tourism"`
- [x] 5.2 Delete colliding CONST definitions from cycle's CONST block: 19 natural names (bareRockColor, beachColor, caveColor, cliffColor, fellColor, glacierColor, heathColor, marshColor, mudColor, peakLabelColor, peakSymbolColor, sandColor, screeColor, scrubColor, volcanoLabelColor, volcanoSymbolColor, wetlandColor, woodColor, woodLabelColor) and 10 amenity names (amenityBuildingColor, amenityLabelColor, amenityBuildingBorderColor, hospitalBuildingColor, hospitalBuildingBorderColor, postBuildingColor, postBuildingBorderColor, postLabelColor, parkingBuildingColor, parkingBuildingBorderColor) — landuse/leisure CONSTs kept (sections kept inline); verify no `Constant already defined` on load
- [x] 5.3 Delete duplicate SYMBOLs `natural_peak`, `natural_volcano`, `stream_arrow` from cycle (module versions identical) and verify no `Map symbol ... already defined` on load
- [x] 5.4 Delete inline sections covered by adopted modules: natural, waterway, and the buildings/amenity/sport/power block (kept leisure building bits: leisure_stadium, leisure_sports_centre/leisure_building fill, leisure_building + leisure_pitch/fitness_station text); keep synthetic/contours, highway overrides, landuse, leisure, aerialway, routes
- [x] 5.5 Diff inline copy vs module per section. Findings: waterway/natural/amenity/sport/power are subsets (module superset — adopt, reviewed deltas: dock/stream fill MAG cityOver→suburb, pattern/emphasizeColor additions); landuse (farmColor, village_green/orchard #fafdf7 vs module #cfeca8, different MAG thresholds) and leisure (park #dbf5e0 vs #c6f0cf, golf #e9fadc, garden #eff9e2, playground #ccffff vs #affdbb) are intentionally different cycle palette → kept inline, NOT ported (porting would change standard.oss/winter-sports.oss); aerialway kept inline (black vs module purple)
- [x] 5.6 PerformanceTest render compare (cycle.oss) — deltas expected: gained module rules for previously unstyled types, amenity label emphasizeColor halo, waterway dock/stream fill at suburb instead of cityOver; landuse/leisure/aerialway unchanged

## 6. boundaries.oss — reuse include/land_sea (D6)

- [x] 6.1 Replace inline tile rules with `MODULE "include/land_sea"` (keep `@waterColor`/`@landColor`/`@unknownColor` CONSTs) and verify `CheckStyleSheet-boundaries.oss` passes with unchanged render

## 7. public-transport.oss — GROUP-based building rule (D2)

- [x] 7.1 Replace the ~40-name explicit building list with `[GROUP building]`, keeping the block's `[MAG veryClose-]` fill (`#f0f0f0`)/border (`#d0d0d0`), the nested `[GROUP amenity]` pink override (`#ffe0e0`), and the separate `[TYPE shop]` block unchanged
- [x] 7.2 Verify `CheckStyleSheet-public-transport.oss` passes; PerformanceTest render compare: only reviewed delta (shop `*_building` areas gain gray fill; all listed types render identically)

## 8. Verification

- [x] 8.1 Build: `cmake --build build` succeeds with no new warnings from style files
- [x] 8.2 All stylesheet tests pass: `cd build && ctest -R "CheckStyleSheet" --output-on-failure` (covers standard, winter-sports, boundaries, railways, motorways, public-transport, cycle)
- [x] 8.3 Full test suite: `cd build && ctest -j 2 --output-on-failure` (excluding PerformanceTest) — 69/69 pass, no regressions
- [x] 8.4 Render comparison summary: DrawMapCairo (cairo) renders on freshly imported Dortmund DB (zooms 16000/65000/66000, 800x600), baseline = git worktree at HEAD. Pixel diff (ImageMagick AE): standard 0/0/0 (identical), winter-sports 0/0 (identical), public-transport 0/0/264px@z66000 (shop/other building fills gain gray at building zoom), cycle 236/209/1009px (module-superset rules: amenity label halo, extra types, waterway MAG shift). No contiguous diff regions → landuse/leisure/aerialway unchanged. All deltas match design D3/D5 enumeration; none unexpected
- [x] 8.5 `openspec validate cleanup-oss-base-files --type change` is green
