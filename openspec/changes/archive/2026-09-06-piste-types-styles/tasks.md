## 1. Type definitions (spec: piste-type-definitions)

- [x] 1.1 Add new `TYPE` definitions to `stylesheets/map.ost` in the winter sports section after `piste_downhill_advanced`, in this order (first match wins): `piste_downhill_novice`, `piste_downhill_expert`, `piste_downhill_freeride`, `piste_downhill_extreme`, `piste_downhill` (generic fallback), `piste_nordic`, `piste_skitour`, `piste_hike`, `piste_sled`, `piste_sleigh`, `piste_ice_skate`, `piste_snow_park`, `piste_playground`, `piste_ski_jump`, `piste_fatbike`, `piste_snowkite`, `piste_snowmobile`, `piste_halfpipe`, `piste_connection`, `piste_snowshoe`, `piste_nordic_hike`, `piste_hike_fatbike`, `piste_hike_snowshoe`, `piste_nordic_hike_snowshoe`, `piste_nordic_fatbike` — verify each declares the object types (NODE/WAY/AREA) from the OSM wiki (see spec "Object types match OSM wiki documentation")
- [x] 1.2 Verify no type exists for excluded values (`piste:type=yes`) — grep `stylesheets/map.ost` for `"piste:type"=="yes"` returns nothing

## 2. Style definitions (spec: piste-type-definitions)

- [x] 2.1 Add 3 new symbols to `stylesheets/include/piste.oss` (`piste_snow_park`, `piste_playground`, `piste_snowkite`) — verify symbols parse (CheckStyleSheet tests)
- [x] 2.2 Extend the `[MAG city-]` block with WAY/AREA/AREA.BORDER rules for the new downhill difficulty variants (`novice` green, `expert` orange, `freeride` yellow, `extreme` near-black, generic `piste_downhill` neutral) — verify rendered with the existing downhill style template
- [x] 2.3 Add `[MAG city-]` WAY rules for the new linear types (`nordic`, `skitour`, `hike`, `sled`, `sleigh`, `ice_skate`, `ski_jump`, `fatbike`, `snowmobile`, `halfpipe`, `connection`, `snowshoe`, and the 5 combination types) with OpenSnowMap colors — verify rendered as colored ways
- [x] 2.4 Add `[MAG city-]` AREA/AREA.BORDER rules for the new area-capable types (`skitour`, `sled`, `sleigh`, `ice_skate`, `ski_jump`, `halfpipe`, `connection`, `snowshoe`, `snow_park`, `playground`, `snowkite`) — verify areas get fill + border
- [x] 2.5 Add `NODE.ICON`/`AREA.ICON` rules in `[MAG close-]` for the 3 new symbols — verify icons render at close zoom
- [x] 2.6 Extend `[MAG close-]` label rules: WAY.TEXT for all new linear types, AREA.TEXT for area types, NODE.TEXT for node types — verify labels render

## 3. Priority groups (spec: piste-type-definitions)

- [x] 3.1 Extend the piste GROUP line in `stylesheets/winter-sports.oss` with all new piste types — verify by reading the GROUP block

## 4. Verification

- [x] 4.1 Run `cd build && ctest -R CheckStyleSheet --output-on-failure` — verify all stylesheet tests pass (validates `map.ost` against every `.oss` with `--warning-as-error`)
- [x] 4.2 Run `cd build && ctest -R StyleConfigSymbolsTest --output-on-failure` — verify symbol test passes
- [x] 4.3 Run `git diff --stat` — verify only the 3 intended stylesheet files changed (`map.ost`, `include/piste.oss`, `winter-sports.oss`)
