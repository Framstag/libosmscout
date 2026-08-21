# Proposal: additional-natural-types

## What Changes

Extend the import-time type definitions in `stylesheets/map.ost` with missing `natural=*` values, so these features exist in the database `TypeConfig` and can be rendered and searched.

Current `map.ost` already defines: `natural_bay`, `natural_beach`, `natural_bare_rock`, `natural_cave_entrance`, `natural_cliff`, `natural_fell`, `natural_glacier`, `natural_grassland`, `natural_heath`, `natural_land`, `natural_mud`, `natural_peak`, `natural_sand`, `natural_scree`, `natural_scrub`, `natural_spring`, `natural_tree`, `natural_volcano`, `natural_water`, `natural_wetland` (+ `natural_wetland_marsh`, `natural_wetland_tidalflat`), and `wood` (which already covers `natural=wood`).

Missing values documented on the OSM wiki [Key:natural](https://wiki.openstreetmap.org/wiki/Key:natural) and confirmed by [taginfo usage data](https://taginfo.openstreetmap.org/keys/natural#values) will be added as new types, with element types (node/way/area) and features matching the wiki's per-value element table:

- **Vegetation**: `natural=tree_row` (way), `natural=shrub` (node), `natural=shrubbery` (area), `natural=tree_group` (node/area), `natural=tree_stump` (node), `natural=moor` (node/area), `natural=tundra` (node/area)
- **Water**: `natural=coastline` (way), `natural=reef` (area), `natural=shoal` (node/area), `natural=strait` (node/area), `natural=isthmus` (node/area), `natural=peninsula` (node/area), `natural=cape` (node), `natural=blowhole` (node/area), `natural=hot_spring` (node), `natural=geyser` (node), `natural=waterfall` (node/way/area)
- **Geological**: `natural=arch` (node/way/area), `natural=arete` (way), `natural=blockfield` (area), `natural=crevasse` (way/area), `natural=dune` (node/way/area), `natural=earth_bank` (way), `natural=fumarole` (node), `natural=gorge` (way), `natural=gully` (way), `natural=hill` (node), `natural=ridge` (way), `natural=rock` (node/area), `natural=saddle` (node), `natural=sinkhole` (node/area), `natural=stone` (node), `natural=valley` (node/way)

Rendering rules for the new types will be added to `stylesheets/include/natural.oss` (and mirrored in `stylesheets/cycle.oss` where the natural section exists), so the new types are visible on maps.

**Explicitly excluded** (not relevant / deprecated / covered elsewhere):
- `natural=wood` — already covered by existing `wood` type
- `natural=land` — already exists as `natural_land`
- `natural=grass` — deprecated, use `landuse=grass`
- `natural=landform` — deprecated umbrella value
- `natural=mountain_range` — deprecated, relation-based
- `natural=birds_nest`, `natural=islet`, `natural=yes` — negligible usage, not in wiki values table
- `natural=water` + `water=river` — already covered by `waterway_riverbank` type

## Capabilities

### New Capabilities

- `natural-type-definitions`: import-time `natural=*` type definitions in `stylesheets/map.ost` with correct element types and features per the OSM wiki, plus matching rendering rules in the `.oss` stylesheets

### Modified Capabilities

None.

## Impact

- `stylesheets/map.ost` — ~36 new `TYPE` definitions in the Natural section
- `stylesheets/include/natural.oss` — rendering rules (area colors, way lines, node text) for new types
- `stylesheets/cycle.oss` — mirrored natural rendering rules
- Imported databases gain new types in `TypeConfig`; existing databases need re-import to pick up new types
- No C++ code changes; type definitions are data-driven
