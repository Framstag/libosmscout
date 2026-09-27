# Tasks: Add missing railway type and style definitions

## 1. Type definitions

- [x] 1.1 Add track types `railway_construction`, `railway_proposed`, `railway_miniature` (WAY) to `stylesheets/map.ost` and verify `ctest -R CheckStyleSheet-standard.oss` passes
- [x] 1.2 Add stop types `railway_stop`, `railway_tram_crossing`, `railway_tram_level_crossing`, `railway_train_station_entrance` (NODE) to `stylesheets/map.ost` and verify `ctest -R CheckStyleSheet-standard.oss` passes
- [x] 1.3 Add node-area types `railway_service_station`, `railway_signal_box`, `railway_yard` (NODE AREA) to `stylesheets/map.ost` and verify `ctest -R CheckStyleSheet-standard.oss` passes
- [x] 1.4 Add infrastructure node types `railway_switch`, `railway_signal`, `railway_buffer_stop`, `railway_milestone`, `railway_railway_crossing`, `railway_derail`, `railway_junction`, `railway_radio`, `railway_vacancy_detection`, `railway_phone`, `railway_spur_junction`, `railway_rail_brake`, `railway_defect_detector`, `railway_power_supply`, `railway_owner_change`, `railway_crossover`, `railway_platform_marker` (NODE) to `stylesheets/map.ost` and verify `ctest -R CheckStyleSheet-standard.oss` passes
- [x] 1.5 Add multi-element types `railway_loading_ramp` (NODE WAY AREA), `railway_ventilation_shaft` (NODE WAY), `railway_workshop` (AREA), `railway_platform_edge` (WAY) to `stylesheets/map.ost` and verify `ctest -R CheckStyleSheet-standard.oss` passes

## 2. Style definitions

- [x] 2.1 Add symbols `railway_switch`, `railway_signal`, `railway_buffer_stop`, `railway_milestone`, `railway_stop` to `stylesheets/include/railway.oss` and verify they render via `SymbolsAll` demo
- [x] 2.2 Add NODE.ICON rules for new infrastructure types at veryClose zoom, reusing existing symbols for `tram_crossing`, `tram_level_crossing`, `railway_crossing`, `train_station_entrance`, and verify `ctest -R CheckStyleSheet-railways.oss` passes
- [x] 2.3 Add WAY styles for `railway_construction`, `railway_proposed`, `railway_platform_edge` and AREA style for `railway_yard` at close zoom; add `railway_miniature` to the narrow-gauge line group and verify `ctest -R CheckStyleSheet-railways.oss` passes
- [x] 2.4 Add building-area styles for `railway_workshop` and `railway_signal_box` in the `_building` section and verify `ctest -R CheckStyleSheet-railways.oss` passes

## 3. Symbol improvements

- [x] 3.1 Redesign `railway_station` (square with white center dot), `railway_halt` (smaller solid square), `railway_tram_stop` (solid circle) and verify distinct rendering via `SymbolsAll` demo
- [x] 3.2 Fix `railway_signal` orientation (pole at bottom, light at top) and verify rendered SVG shows light above pole

## 4. Validation

- [x] 4.1 Run full `ctest -R CheckStyleSheet` suite (7 stylesheets) and verify all pass
- [x] 4.2 Run `SymbolsAll` demo on `standard.oss` and verify all 526 symbols render without failure, including the 5 new railway symbols
