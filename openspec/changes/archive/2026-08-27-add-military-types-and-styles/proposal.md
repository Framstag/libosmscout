## Why

`stylesheets/map.ost` defines only 7 of the 21 documented `military=*` values from the OSM wiki, and `stylesheets/include/military.oss` styles only those. Values with significant real-world usage — `military=trench` (13.8% of all `military=*` objects), `military=office` (4.3%), `military=checkpoint` (4.0%), `military=base` (2.4%), `military=training_area` (1.6%), `military=nuclear_explosion_site` (1.4%) — are currently dropped at import time, so these features are neither importable, searchable, nor renderable.

## What Changes

- Add 15 new feature types to `stylesheets/map.ost` for `military=*` values that are documented on the OSM wiki or have taginfo usage >= 0.10% and are not discouraged.
- Element types (NODE/WAY/AREA) follow the OSM wiki element table for documented values; for undocumented values they follow taginfo node/way/relation usage statistics.
- Add corresponding style definitions to `stylesheets/include/military.oss`: area fills and labels for large installations, dashed line rendering for linear features (trench, cordon, road, shelter), and dedicated node symbols for POI-like features (checkpoint, cannon, radar, embrasure, shelter, police, trench).
- `military=office` reuses the existing `office` type group so military offices render with the established office styling.
- No **BREAKING** changes: existing types and styles are preserved; only new types are added.

## Capabilities

### New Capabilities
- `military-type-definitions`: Import-time feature types and rendering styles for documented and significantly-used `military=*` OSM values, following the pattern of the existing `*-type-definitions` specs.

### Modified Capabilities
- None. No existing spec-level behavior changes.

## Impact

- `stylesheets/map.ost` — 15 new `TYPE military_*` definitions in the Military section (7 existing types unchanged).
- `stylesheets/include/military.oss` — new symbols (`military_checkpoint`, `military_cannon`, `military_radar`, `military_embrasure`, `military_shelter`, `military_police`, `military_trench`), extended area-fill/label groups, new dashed line styles, new node icon rules.
- Consumed by all stylesheets that include the military module: `standard.oss`, `cycle.oss`, `winter-sports.oss`.
- No C++ code, build system, or data format changes.
- Validation: `Tests/src/OSTAndOSSTest.cpp` loads `map.ost` + each `.oss` and reports unknown types/symbols; `--analyze` reports types without styles.
