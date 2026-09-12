# Tasks: highway-types-styles

## 1. Type definitions (spec: highway-types — Highway value coverage, Object types match OSM wiki)

- [x] 1.1 Add 20 NODE types to `stylesheets/map.ost` (crossing, turning_circle, stop, give_way, turning_loop, passing_place, emergency_access_point, elevator, rest_area, traffic_mirror, emergency_bay, trailhead, traffic_sign, toll_gantry, priority, speed_display, ladder, hitchhiking, cyclist_waiting_aid, platform) with object types per OSM wiki — verify: `OSTAndOSSTest map.ost` loads without errors
- [x] 1.2 Add 5 WAY types to `stylesheets/motorways.ost` (busway, raceway, escape, proposed, corridor) with PATH/LOCATION/PIN_WAY options — verify: `OSTAndOSSTest map.ost` loads without errors
- [x] 1.3 Confirm discouraged values (no, yes, planned, disused, abandoned, razed, piste, ford, turntable, residential_link, traffic_calming, sidewalk, traffic_island) are not defined — verify: grep map.ost shows no types for these values

## 2. Style definitions (spec: highway-types — Style definitions for obvious visualizations, Street lamp day/night rendering)

- [x] 2.1 Add 20 geometric SYMBOLs and NODE.ICON rules for new node types in `stylesheets/include/roads.oss` — verify: `CheckStyleSheet-standard.oss` passes, SymbolAll renders all symbols
- [x] 2.2 Add WAY line styles and label rules for busway, raceway, escape, proposed, corridor in `stylesheets/include/roads.oss` — verify: `CheckStyleSheet-standard.oss` passes
- [x] 2.3 Add new way types to ORDER WAYS in `stylesheets/standard.oss` — verify: `CheckStyleSheet-standard.oss` passes
- [x] 2.4 Add `highway_street_lamp_off` symbol and wire `IF daylight` toggle (off symbol by day, glow symbol at night) — verify: SymbolAll renders both `highway_street_lamp` and `highway_street_lamp_off` SVGs

## 3. Validation

- [x] 3.1 Run `OSTAndOSSTest` against map.ost + standard.oss, cycle.oss, public-transport.oss, motorways.oss — verify: all load OK with no new warnings
- [x] 3.2 Run full ctest suite — verify: 70/70 tests pass
- [x] 3.3 Run `SymbolsAll` and inspect output — verify: all new symbols render with correct orientation (4 orientation bugs found and fixed)
