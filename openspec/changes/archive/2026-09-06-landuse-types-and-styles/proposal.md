## Why

`stylesheets/map.ost` defines import-time feature types for OSM tags. Many common `landuse=*` values are missing: the OSM wiki [Key:landuse](https://wiki.openstreetmap.org/wiki/Key:landuse) documents 84 values, and [taginfo](https://taginfo.openstreetmap.org/keys/landuse#values) shows dozens with usage above 150 objects. Objects tagged with these values are currently not importable as dedicated types and not renderable.

## What Changes

1. **19 new landuse types** in `stylesheets/map.ost`, selected by the criteria: documented on the OSM wiki as a concrete type, taginfo usage >= 150, and not discouraged/deprecated/proposal. Element types (node/area) follow each tag's wiki page. Skipped values (e.g. `school` deprecated in favor of `education`, `churchyard` deprecated in favor of `religious`, `harbour`/`depot`/`port` discouraged, `traffic_island` abandoned proposal) are deliberately not added.

2. **Style definitions** in `stylesheets/include/landuse.oss`: fill colors, area rules at the appropriate magnification levels (region-/city-/suburb-/close-), and label rules (AREA.TEXT/NODE.TEXT), following the existing patterns of similar landuse types.

3. **Pattern fill tiles** for structure/coverage visualization: 18 new `landuse_*` 14x14 PNG tiles (plus SVG sources) in `libosmscout/data/icons/`, wired via `pattern:` + `patternMinMag` in the area rules. Pattern tiles follow the `<type>_<name>` naming convention of all other symbols. The `religious` tile uses a universal building-with-spire symbol (no cross). Also fixes the `gardenpng` typo (renamed `garden.png`, later `leisure_garden.png`) and renames `forest`/`cemetery`/`scrub` tiles to `landuse_forest`/`landuse_cemetery`/`natural_scrub`.

4. **SymbolsAll pattern rendering**: the `SymbolsAll` demo tool now also renders pattern fills, so pattern tiles can be visually scanned like symbols.

## Capabilities

### New Capabilities
- `landuse-type-definitions`: Import-time feature types for `landuse=*` values missing from `stylesheets/map.ost`, with element types per the OSM wiki, plus rendering rules (fills, labels, pattern fills) in the landuse style module.

### Modified Capabilities
- `symbol-scan-tool`: The `SymbolsAll` tool gains pattern-fill rendering — a `StyleConfig` pattern enumeration API, a `--pattern-path` option, per-pattern PNG output, and a pattern contact sheet.

## Impact

- **Modified files**: `stylesheets/map.ost` (19 new types), `stylesheets/include/landuse.oss` (colors, area rules, labels, patterns), `stylesheets/cycle.oss` (pattern name), `stylesheets/include/leisure.oss` (pattern name), `stylesheets/include/natural.oss` (pattern name), `libosmscout-map/include/osmscoutmap/StyleConfig.h` + `libosmscout-map/src/osmscoutmap/StyleConfig.cpp` (new `GetPatternNames()`), `Demos/src/SymbolsAll.cpp` (pattern rendering)
- **New files**: 18 `landuse_*` pattern tiles (PNG + SVG) in `libosmscout/data/icons/14x14/standard/` and `libosmscout/data/icons/svg/standard/`
- **Renamed files**: `gardenpng` → `garden.png` → `leisure_garden.png`, `forest.png` → `landuse_forest.png`, `cemetery.png` → `landuse_cemetery.png`, `scrub.png` → `natural_scrub.png` (PNG + SVG)
- **Dependencies**: No new external dependencies. Pattern rendering in SymbolsAll uses existing Cairo (`cairo_image_surface_create_from_png`).
- **API**: One additive public accessor on `StyleConfig`; no behavioral changes to existing rendering.
