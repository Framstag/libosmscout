## 1. Type definitions (spec: waterway-type-definitions)

- [ ] 1.1 Add 22 new `TYPE` definitions to `stylesheets/map.ost` in the waterway section after `waterway_dam`: `waterway_flowline`, `waterway_tidal_channel`, `waterway_drystream`, `waterway_pressurised`, `waterway_derelict_canal`, `waterway_drainage_channel`, `waterway_link`, `waterway_fairway`, `waterway_fish_pass`, `waterway_fuel`, `waterway_water_point`, `waterway_sanitary_dump_station`, `waterway_access_point`, `waterway_milestone`, `waterway_rapids`, `waterway_sluice_gate`, `waterway_floodgate`, `waterway_check_dam`, `waterway_floating_barrier`, `waterway_flow_control`, `waterway_stream_end`, `waterway_soakhole` — verify each declares the object types (NODE/WAY/AREA) from the OSM wiki (see spec "Object types match OSM wiki documentation")
- [ ] 1.2 Verify no type exists for excluded values (`wadi`, `brook`, `artificial`, `riverbank`, `yes`) — grep `stylesheets/map.ost` for `waterway=="wadi"` etc. returns nothing

## 2. Style definitions (spec: waterway-type-definitions)

- [ ] 2.1 Add 7 new symbols to `stylesheets/include/waterway.oss` (`waterway_access_point`, `waterway_soakhole`, `waterway_water_point`, `waterway_milestone`, `waterway_stream_end`, `waterway_fuel`, `waterway_sanitary_dump_station`) in `@waterLabelColor` — verify symbols parse (CheckStyleSheet tests)
- [ ] 2.2 Extend the `[MAG suburb-]` stream group with `waterway_flowline`, `waterway_tidal_channel`, `waterway_drystream` — verify rendered as 2m water-color ways
- [ ] 2.3 Extend the `[MAG detail-]` drain/ditch group with `waterway_link`, `waterway_pressurised`, `waterway_fairway`, `waterway_drainage_channel`, `waterway_derelict_canal` — verify rendered as 3m water-color ways
- [ ] 2.4 Extend the `[MAG detail-]` weir group with `waterway_fish_pass`, `waterway_rapids`, `waterway_sluice_gate`, `waterway_floodgate`, `waterway_check_dam`, `waterway_floating_barrier`, `waterway_flow_control` — verify rendered as dashed ways
- [ ] 2.5 Extend the `[MAG detail-]` boatyard area rule with `waterway_fuel`, `waterway_sanitary_dump_station` — verify areas get `#d3cec4` fill
- [ ] 2.6 Add `NODE.ICON`/`AREA.ICON` rules in `[MAG closer-]` for the 7 new symbols — verify icons render at close zoom
- [ ] 2.7 Extend `[MAG veryClose-]` label rules: WAY.TEXT for all new linear/barrier types, NODE.TEXT for barriers and facilities, AREA.TEXT for `waterway_fuel`/`waterway_sanitary_dump_station`, and `stream_arrow` WAY.SYMBOL for `waterway_flowline`/`waterway_tidal_channel`/`waterway_drystream` — verify labels and flow arrows render

## 3. Priority groups (spec: waterway-type-definitions)

- [ ] 3.1 Extend the waterway GROUP lines in `stylesheets/standard.oss` (barriers join the `waterway_weir` group, linear types join the `waterway_river` group) — verify by reading the GROUP block
- [ ] 3.2 Apply the same GROUP extensions to `stylesheets/cycle.oss`, `stylesheets/winter-sports.oss`, and `stylesheets/public-transport.oss` — verify all four files list the new types

## 4. Verification

- [ ] 4.1 Run `cd build && ctest -R CheckStyleSheet --output-on-failure` — verify all 7 stylesheet tests pass (validates `map.ost` against every `.oss` with `--warning-as-error`)
- [ ] 4.2 Run `cd build && ctest -R StyleConfigSymbolsTest --output-on-failure` — verify symbol test passes
- [ ] 4.3 Run `git diff --stat` — verify only the 6 intended stylesheet files changed (`map.ost`, `include/waterway.oss`, `standard.oss`, `cycle.oss`, `winter-sports.oss`, `public-transport.oss`)
