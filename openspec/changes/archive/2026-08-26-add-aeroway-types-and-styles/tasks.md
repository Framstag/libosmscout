# Tasks: Add missing aeroway type and style definitions

## 1. Type definitions

- [x] 1.1 Add node types `aeroway_navigationaid`, `aeroway_windsock`, `aeroway_threshold`, `aeroway_aircraft_crossing` (NODE) to `stylesheets/map.ost` and verify `ctest -R CheckStyleSheet-standard.oss` passes
- [x] 1.2 Add node-way types `aeroway_parking_position`, `aeroway_holding_position` (NODE WAY) to `stylesheets/map.ost` and verify `ctest -R CheckStyleSheet-standard.oss` passes
- [x] 1.3 Add node-area types `aeroway_hangar` (AREA), `aeroway_airstrip`, `aeroway_tower`, `aeroway_heliport`, `aeroway_fuel` (NODE AREA) to `stylesheets/map.ost` and verify `ctest -R CheckStyleSheet-standard.oss` passes
- [x] 1.4 Add way types `aeroway_taxilane`, `aeroway_jet_bridge`, `aeroway_stopway`, `aeroway_model_runway` (WAY) to `stylesheets/map.ost` and verify `ctest -R CheckStyleSheet-standard.oss` passes

## 2. Style definitions

- [x] 2.1 Add symbols `aeroway_windsock`, `aeroway_heliport`, `aeroway_fuel`, `aeroway_tower`, `aeroway_navigationaid`, `aeroway_aircraft_crossing`, `aeroway_threshold`, `aeroway_holding_position`, `aeroway_parking_position` to `stylesheets/include/aeroway.oss` and verify they render via `SymbolsAll` demo
- [x] 2.2 Add NODE.ICON rules for the new node types at veryClose zoom and verify `ctest -R CheckStyleSheet-standard.oss` passes
- [x] 2.3 Add WAY styles for `aeroway_taxilane`, `aeroway_jet_bridge`, `aeroway_stopway`, `aeroway_model_runway` at suburb/close zoom and verify `ctest -R CheckStyleSheet-standard.oss` passes
- [x] 2.4 Add AREA fill styles for `aeroway_hangar`, `aeroway_airstrip`, `aeroway_heliport`, `aeroway_tower`, `aeroway_fuel` at city zoom and verify `ctest -R CheckStyleSheet-standard.oss` passes
- [x] 2.5 Add `_building` section styles for `aeroway_hangar`, `aeroway_tower`, `aeroway_fuel` (building outline for areas also tagged `building=yes`) and verify `ctest -R CheckStyleSheet-standard.oss` passes
- [x] 2.6 Add `aeroway_taxilane`, `aeroway_jet_bridge`, `aeroway_stopway`, `aeroway_model_runway` to the `ORDER WAYS` aeroway group in `stylesheets/standard.oss` and verify `ctest -R CheckStyleSheet-standard.oss` passes

## 3. Validation

- [x] 3.1 Run full `ctest -R CheckStyleSheet` suite (all stylesheets) and verify all pass
- [x] 3.2 Run `SymbolsAll` demo on `standard.oss` and verify all symbols render without failure, including the 9 new aeroway symbols
