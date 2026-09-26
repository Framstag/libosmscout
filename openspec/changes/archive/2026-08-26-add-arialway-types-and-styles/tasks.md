## 1. Type definitions (spec: aerialway-type-definitions)

- [x] 1.1 Add `aerialway_pylon` (NODE), `aerialway_station` (NODE AREA, POI, GROUP routingPOI), `aerialway_zip_line` (WAY), `aerialway_goods` (WAY) types to `stylesheets/map.ost` Aerialway section and verify the file parses with the import tool / style parser
- [x] 1.2 Verify each new type exists in the database `TypeConfig` after import and that element kinds match the wiki (pylon/zip_line/goods reject wrong geometries, station accepts node+area)

## 2. Style definitions (spec: aerialway-type-definitions)

- [x] 2.1 Add SYMBOL definitions for station and pylon to `stylesheets/include/aerialway.oss` and verify symbols render in a test render
- [x] 2.2 Add WAY styles for `aerialway_zip_line` and `aerialway_goods` (cable line + dashed carrier overlay) to `stylesheets/include/aerialway.oss` and verify dashed lines render at close zoom
- [x] 2.3 Add NODE.ICON/NODE.TEXT rules for station and pylon and add new way types to the label rule in `stylesheets/include/aerialway.oss`; verify labels render

## 3. Stylesheet registration (spec: aerialway-type-definitions, requirement "Aerialway way types render in consistent order")

- [x] 3.1 Add `aerialway_zip_line, aerialway_goods` to the aerialway ORDER WAYS group in `standard.oss`, `winter-sports.oss`, `cycle.oss`, and `public-transport.oss` and verify all five aerialway way types share one group in each file

## 4. Verification

- [x] 4.1 Run the style/type validation (import a small OSM extract or run the stylesheet parser test) and verify no parse errors
- [x] 4.2 Render a test map containing pylon/station/zip_line/goods objects and verify symbols, lines, and labels appear
