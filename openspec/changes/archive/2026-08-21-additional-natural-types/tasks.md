# Tasks: additional-natural-types

## 1. Extend type definitions in map.ost

- [x] 1.1 Add vegetation types to Natural section: `natural_tree_row` (WAY), `natural_shrub` (NODE), `natural_shrubbery` (AREA), `natural_tree_group` (NODE AREA), `natural_tree_stump` (NODE), `natural_moor` (NODE AREA), `natural_tundra` (NODE AREA) — each with `{Name, NameAlt}`
- [x] 1.2 Add water types: `natural_coastline` (WAY), `natural_reef` (AREA), `natural_shoal` (NODE AREA), `natural_strait` (NODE AREA), `natural_isthmus` (NODE AREA), `natural_peninsula` (NODE AREA), `natural_cape` (NODE), `natural_blowhole` (NODE AREA), `natural_hot_spring` (NODE), `natural_geyser` (NODE), `natural_waterfall` (NODE WAY AREA) — each with `{Name, NameAlt}`
- [x] 1.3 Add geological types: `natural_arch` (NODE WAY AREA), `natural_arete` (WAY), `natural_blockfield` (AREA), `natural_crevasse` (WAY AREA), `natural_dune` (NODE WAY AREA), `natural_earth_bank` (WAY), `natural_fumarole` (NODE), `natural_gorge` (WAY), `natural_gully` (WAY), `natural_hill` (NODE, `{Name, NameAlt, Ele}`), `natural_ridge` (WAY), `natural_rock` (NODE AREA), `natural_saddle` (NODE, `{Name, NameAlt, Ele}`), `natural_sinkhole` (NODE AREA), `natural_stone` (NODE), `natural_valley` (NODE WAY) — each with `{Name, NameAlt}` unless noted
- [x] 1.4 Verify no duplicate type names and no overlap with existing types (`wood`, `natural_land`, `waterway_riverbank`)

## 2. Add rendering rules

- [x] 2.1 `stylesheets/include/natural.oss`: add area fill rules for new area-capable types (rock/blockfield/arch → `@bareRockColor`; reef/shoal → water color; shrubbery → `@scrubColor`; moor/tundra → `@fellColor`; crevasse/dune/sinkhole/strait/isthmus/peninsula/blowhole/tree_group → sensible existing colors)
- [x] 2.2 `stylesheets/include/natural.oss`: add way line rules (coastline → `@waterLabelColor`; ridge/arete/gorge/gully/earth_bank → `@cliffColor`; tree_row → line; crevasse/dune/arch/valley → line)
- [x] 2.3 `stylesheets/include/natural.oss`: add node text rules at `[MAG detail-]` for new node-capable types; add `Ele` label for `natural_hill`/`natural_saddle` like peak/volcano
- [x] 2.4 `stylesheets/cycle.oss`: mirror the new area/way/node rules in its natural section
- [x] 2.5 Verify stylesheets parse: run import tool or `openspec validate`; check no unknown type references

## 3. Validation

- [x] 3.1 `openspec validate additional-natural-types --type change` passes
- [x] 3.2 Import a small OSM extract containing new `natural=*` values; confirm types present in `TypeConfig` (e.g. via `DumpData` or import log)
- [x] 3.3 Render a test map with `standard.oss`; spot-check new types visible
