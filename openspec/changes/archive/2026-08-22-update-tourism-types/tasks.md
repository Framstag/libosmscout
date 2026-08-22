# Tasks — update-tourism-types

## 1. Type definitions in map.ost (spec: accommodation/attraction tourism types)

- [x] Add `tourism_camp_pitch` (NODE AREA, `{Name, NameAlt}`, ADDRESS POI, GROUP tourism, routingPOI) to `stylesheets/map.ost` camping group (near `tourism_caravan_site`).
- [x] Add `tourism_trail_riding_station` (NODE AREA, `{Name, NameAlt, OpeningHours, Phone, Website}`, ADDRESS POI, GROUP tourism, routingPOI) after `tourism_camp_pitch`.
- [x] Add `tourism_apartment_building` (AREA, building condition) + `tourism_apartment` (NODE AREA) in accommodation group; `_building` block directly above generic.
- [x] Add `tourism_wilderness_hut_building` (AREA, building condition, ADDRESS, GROUP tourism, building) + `tourism_wilderness_hut` (NODE AREA, ADDRESS, GROUP tourism) — mirror `tourism_alpine_hut` pattern.
- [x] Add `tourism_gallery_building` (AREA, building condition) + `tourism_gallery` (NODE AREA, ADDRESS POI, GROUP tourism, routingPOI) — mirror `tourism_museum` pattern; place after `tourism_motel`.
- [x] Verify no `tourism_resort`/`tourism_winery`/undocumented types added (spec: discouraged values).

## 2. Style rules (spec: area rendering)

- [x] Extend `[MAG detail-]` TYPE list in `stylesheets/include/tourism.oss` with `tourism_apartment`, `tourism_camp_pitch`, `tourism_gallery`, `tourism_trail_riding_station`, `tourism_wilderness_hut`.
- [x] Confirm `_building` variants covered by existing `GROUP tourism, building` rules (no new rules needed).

## 3. Build & validation

- [x] Build the import tool: `cmake --build build --target Import` (or equivalent configured target).
- [x] Verify new types in `TypeConfig`: `OSTAndOSSTest --analyze stylesheets/map.ost stylesheets/standard.oss` shows all 8 new types with correct element coverage and style coverage (no full OSM import run).
- [x] Run existing test suite: `cd build && ctest -j 2 --output-on-failure` — all tests still pass.
- [x] `openspec validate update-tourism-types --type change` passes.
- [ ] Optionally render the imported region with `standard.oss` and verify area fill for new types at detail zoom.
