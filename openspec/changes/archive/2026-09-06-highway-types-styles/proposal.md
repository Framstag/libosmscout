## Why

The `map.ost` type definition file is missing many documented and widely used `highway=*` values from the OSM wiki (Key:highway). Objects with these tags are currently not imported into the database and thus not rendered or routable. Adding them improves map coverage for common road features (crossings, stop/give-way signs, turning circles, busways, raceways, corridors, etc.).

## What Changes

- Add 20 new NODE types to `map.ost` for highway features mapped on nodes: crossing, turning_circle, stop, give_way, turning_loop, passing_place, emergency_access_point, elevator, rest_area, traffic_mirror, emergency_bay, trailhead, traffic_sign, toll_gantry, priority, speed_display, ladder, hitchhiking, cyclist_waiting_aid, platform
- Add 5 new WAY types to `motorways.ost` (module included by `map.ost`): busway, raceway, escape, proposed, corridor
- Add corresponding style definitions in `stylesheets/include/roads.oss`: line styles for the new way types, geometric SYMBOLs + NODE.ICON rules for the new node types, and label rules
- Add new way types to `ORDER WAYS` in `standard.oss` for correct render order
- Object types (NODE/WAY/AREA) follow the OSM wiki element tables for each value
- Values with usage < 1000 or discouraged status (no, yes, planned, disused, abandoned, razed, piste, ford, turntable, residential_link, traffic_calming, sidewalk, traffic_island) are intentionally not added

## Capabilities

### New Capabilities
- `highway-types`: Complete coverage of documented `highway=*` values in the type definition file, with object types per the OSM wiki and styles for obvious visualizations

### Modified Capabilities
<!-- None: no existing spec-level behavior changes -->

## Impact

- `stylesheets/map.ost` — 20 new NODE type definitions
- `stylesheets/motorways.ost` — 5 new WAY type definitions
- `stylesheets/include/roads.oss` — new SYMBOLs, WAY styles, NODE.ICON rules, label rules
- `stylesheets/standard.oss` — ORDER WAYS groups extended
- No C++ code changes; type/style files only
- Validated with `OSTAndOSSTest` (map.ost + all stylesheets load without new warnings) and full ctest suite (70/70 pass)
