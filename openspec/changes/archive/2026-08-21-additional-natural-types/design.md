# Design: additional-natural-types

## Context

`stylesheets/map.ost` defines import-time OSM feature types. The Natural section currently covers ~22 `natural=*` values. The OSM wiki [Key:natural](https://wiki.openstreetmap.org/wiki/Key:natural) documents ~50 values; taginfo shows 1123 distinct values in the wild, of which the top ~50 account for >99% of usage. Missing wiki-documented values with meaningful usage need type definitions so they land in `TypeConfig` and can be rendered.

Rendering lives in `.oss` stylesheets: `stylesheets/include/natural.oss` (included by `standard.oss` and others) and a mirrored section in `stylesheets/cycle.oss`. New types without `.oss` rules would be invisible.

## Goals / Non-Goals

**Goals:**
- Add type definitions for all wiki-documented `natural=*` values missing from `map.ost`, plus `natural=shrub` and `natural=tree_group` (high taginfo usage, wiki-documented)
- Element types (NODE/WAY/AREA) per the wiki element table
- Features per type: `{Name, NameAlt}` for named features, `Ele` for elevation features (hill, saddle)
- Rendering rules in `include/natural.oss` and `cycle.oss` so new types are visible
- Keep existing type definitions untouched

**Non-Goals:**
- No C++ changes — type definitions are data-driven
- No new symbols/icons — reuse existing colors/patterns where sensible
- No deprecated/niche values: `natural=grass`, `natural=landform`, `natural=mountain_range`, `natural=birds_nest`, `natural=islet`, `natural=yes`
- No `natural=wood` (covered by `wood` type), no `natural=land` (exists), no `natural=water`+`water=river` (covered by `waterway_riverbank`)

## Decisions

### D1: Type naming follows existing convention

`natural_<value>` with underscores, e.g. `natural_tree_row`, `natural_hot_spring`. Matches existing `natural_bare_rock`, `natural_cave_entrance`.

### D2: Element types from wiki table

Per-value element support from the wiki (node/way/area). Examples:
- `natural=tree_row` → WAY only (line of trees)
- `natural=coastline` → WAY only
- `natural=shrub` → NODE only
- `natural=shrubbery` → AREA only
- `natural=reef` → AREA only
- `natural=hill`, `natural=saddle`, `natural=cape`, `natural=stone`, `natural=geyser`, `natural=hot_spring`, `natural=fumarole` → NODE only
- `natural=arete`, `natural=earth_bank`, `natural=gorge`, `natural=gully`, `natural=ridge` → WAY only
- `natural=arch`, `natural=dune`, `natural=waterfall` → NODE WAY AREA
- `natural=crevasse` → WAY AREA
- `natural=valley` → NODE WAY
- remaining → NODE AREA

### D3: Features

- Named features (`{Name, NameAlt}`) on all types — consistent with existing natural types
- `Ele` added to `natural_hill` and `natural_saddle` (elevation-relevant, like `natural_peak`/`natural_volcano`)
- `natural_tree_row` gets `{Name, NameAlt}` (no Width — not a path)

### D4: Rendering

- `include/natural.oss`: add new area types to the `[MAG state-]`/`[MAG city-]` fill rules with existing colors (e.g. `natural_rock`/`natural_blockfield`/`natural_arch` → `@bareRockColor`; `natural_reef`/`natural_shoal` → water-ish; `natural_shrubbery` → `@scrubColor`; `natural_moor`/`natural_tundra` → `@fellColor`-like), new way types as lines (e.g. `natural_coastline` → `@waterLabelColor` line, `natural_ridge`/`natural_arete`/`natural_gorge`/`natural_gully`/`natural_earth_bank` → `@cliffColor`-like), node types with text labels at `[MAG detail-]`
- `cycle.oss`: mirror the same rules in its natural section
- No new symbols; reuse existing `natural_spring`-style approach for `natural_hot_spring`/`natural_geyser` if needed, else text-only

### D5: Placement in map.ost

Insert new types in the existing Natural section (lines ~695-798), keeping the current grouping order (vegetation-ish first, then water, then geological) and 2-space indentation style.

## Risks / Trade-offs

- **Re-import required**: existing databases won't gain the new types until re-imported. Acceptable — standard for type definition changes.
- **Color reuse**: new types share colors with similar existing types; distinct hues not guaranteed. Acceptable for v1; style tuning can follow.
- **`natural=coastline` in basemap**: basemap.ost already filters `wr/natural=coastline` via osmium; adding the type to map.ost is complementary, not conflicting.
- **Large diff**: ~35 new types + ~35 rendering rules. Mechanical, low-risk; validated by `openspec validate` and existing import tests.
