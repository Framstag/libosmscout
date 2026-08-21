# natural-type-definitions Specification

## Purpose

Defines the import-time OSM feature types for `natural=*` values missing from `stylesheets/map.ost`, so these features exist in the database `TypeConfig` and are renderable. Element types (node/way/area) and features follow the OSM wiki [Key:natural](https://wiki.openstreetmap.org/wiki/Key:natural) element table.

## Requirements

### Requirement: Vegetation natural types

The import-time stylesheet SHALL define feature types for the following `natural=*` vegetation values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `natural=tree_row` | `natural_tree_row` | way |
| `natural=shrub` | `natural_shrub` | node |
| `natural=shrubbery` | `natural_shrubbery` | area |
| `natural=tree_group` | `natural_tree_group` | node, area |
| `natural=tree_stump` | `natural_tree_stump` | node |
| `natural=moor` | `natural_moor` | node, area |
| `natural=tundra` | `natural_tundra` | node, area |

#### Scenario: Tree row type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_tree_row`
- **THEN** the type SHALL exist
- **AND** ways tagged `natural=tree_row` SHALL be importable as that type

#### Scenario: Shrub type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_shrub`
- **THEN** the type SHALL exist
- **AND** nodes tagged `natural=shrub` SHALL be importable as that type

#### Scenario: Shrubbery type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_shrubbery`
- **THEN** the type SHALL exist
- **AND** areas tagged `natural=shrubbery` SHALL be importable as that type

#### Scenario: Tree group type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_tree_group`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `natural=tree_group` SHALL be importable as that type

#### Scenario: Tree stump type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_tree_stump`
- **THEN** the type SHALL exist
- **AND** nodes tagged `natural=tree_stump` SHALL be importable as that type

#### Scenario: Moor type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_moor`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `natural=moor` SHALL be importable as that type

#### Scenario: Tundra type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_tundra`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `natural=tundra` SHALL be importable as that type

### Requirement: Water natural types

The import-time stylesheet SHALL define feature types for the following `natural=*` water values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `natural=reef` | `natural_reef` | area |
| `natural=shoal` | `natural_shoal` | node, area |
| `natural=strait` | `natural_strait` | node, area |
| `natural=isthmus` | `natural_isthmus` | node, area |
| `natural=peninsula` | `natural_peninsula` | node, area |
| `natural=cape` | `natural_cape` | node |
| `natural=blowhole` | `natural_blowhole` | node, area |
| `natural=hot_spring` | `natural_hot_spring` | node |
| `natural=geyser` | `natural_geyser` | node |
| `natural=waterfall` | `natural_waterfall` | node, way, area |

`natural=coastline` SHALL NOT be defined as an import-time type: coastline ways are handled by the separate water/land index and coastline baseline pipeline, not by regular type-based import.

#### Scenario: Reef type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_reef`
- **THEN** the type SHALL exist
- **AND** areas tagged `natural=reef` SHALL be importable as that type

#### Scenario: Shoal type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_shoal`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `natural=shoal` SHALL be importable as that type

#### Scenario: Strait type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_strait`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `natural=strait` SHALL be importable as that type

#### Scenario: Isthmus type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_isthmus`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `natural=isthmus` SHALL be importable as that type

#### Scenario: Peninsula type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_peninsula`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `natural=peninsula` SHALL be importable as that type

#### Scenario: Cape type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_cape`
- **THEN** the type SHALL exist
- **AND** nodes tagged `natural=cape` SHALL be importable as that type

#### Scenario: Blowhole type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_blowhole`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `natural=blowhole` SHALL be importable as that type

#### Scenario: Hot spring type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_hot_spring`
- **THEN** the type SHALL exist
- **AND** nodes tagged `natural=hot_spring` SHALL be importable as that type

#### Scenario: Geyser type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_geyser`
- **THEN** the type SHALL exist
- **AND** nodes tagged `natural=geyser` SHALL be importable as that type

#### Scenario: Waterfall type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_waterfall`
- **THEN** the type SHALL exist
- **AND** nodes, ways, and areas tagged `natural=waterfall` SHALL be importable as that type

#### Scenario: Coastline type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_coastline`
- **THEN** the type SHALL NOT exist
- **AND** ways tagged `natural=coastline` SHALL NOT be importable as a regular natural way type

### Requirement: Geological natural types

The import-time stylesheet SHALL define feature types for the following `natural=*` geological values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `natural=arch` | `natural_arch` | node, way, area |
| `natural=arete` | `natural_arete` | way |
| `natural=blockfield` | `natural_blockfield` | area |
| `natural=crevasse` | `natural_crevasse` | way, area |
| `natural=dune` | `natural_dune` | node, way, area |
| `natural=earth_bank` | `natural_earth_bank` | way |
| `natural=fumarole` | `natural_fumarole` | node |
| `natural=gorge` | `natural_gorge` | way |
| `natural=gully` | `natural_gully` | way |
| `natural=hill` | `natural_hill` | node |
| `natural=ridge` | `natural_ridge` | way |
| `natural=rock` | `natural_rock` | node, area |
| `natural=saddle` | `natural_saddle` | node |
| `natural=sinkhole` | `natural_sinkhole` | node, area |
| `natural=stone` | `natural_stone` | node |
| `natural=valley` | `natural_valley` | node, way |

#### Scenario: Arch type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_arch`
- **THEN** the type SHALL exist
- **AND** nodes, ways, and areas tagged `natural=arch` SHALL be importable as that type

#### Scenario: Arete type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_arete`
- **THEN** the type SHALL exist
- **AND** ways tagged `natural=arete` SHALL be importable as that type

#### Scenario: Blockfield type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_blockfield`
- **THEN** the type SHALL exist
- **AND** areas tagged `natural=blockfield` SHALL be importable as that type

#### Scenario: Crevasse type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_crevasse`
- **THEN** the type SHALL exist
- **AND** ways and areas tagged `natural=crevasse` SHALL be importable as that type

#### Scenario: Dune type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_dune`
- **THEN** the type SHALL exist
- **AND** nodes, ways, and areas tagged `natural=dune` SHALL be importable as that type

#### Scenario: Earth bank type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_earth_bank`
- **THEN** the type SHALL exist
- **AND** ways tagged `natural=earth_bank` SHALL be importable as that type

#### Scenario: Fumarole type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_fumarole`
- **THEN** the type SHALL exist
- **AND** nodes tagged `natural=fumarole` SHALL be importable as that type

#### Scenario: Gorge type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_gorge`
- **THEN** the type SHALL exist
- **AND** ways tagged `natural=gorge` SHALL be importable as that type

#### Scenario: Gully type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_gully`
- **THEN** the type SHALL exist
- **AND** ways tagged `natural=gully` SHALL be importable as that type

#### Scenario: Hill type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_hill`
- **THEN** the type SHALL exist
- **AND** nodes tagged `natural=hill` SHALL be importable as that type

#### Scenario: Ridge type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_ridge`
- **THEN** the type SHALL exist
- **AND** ways tagged `natural=ridge` SHALL be importable as that type

#### Scenario: Rock type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_rock`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `natural=rock` SHALL be importable as that type

#### Scenario: Saddle type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_saddle`
- **THEN** the type SHALL exist
- **AND** nodes tagged `natural=saddle` SHALL be importable as that type

#### Scenario: Sinkhole type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_sinkhole`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `natural=sinkhole` SHALL be importable as that type

#### Scenario: Stone type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_stone`
- **THEN** the type SHALL exist
- **AND** nodes tagged `natural=stone` SHALL be importable as that type

#### Scenario: Valley type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_valley`
- **THEN** the type SHALL exist
- **AND** nodes and ways tagged `natural=valley` SHALL be importable as that type

### Requirement: Elevation feature on elevation natural types

The import-time stylesheet SHALL assign the `Ele` feature to the types `natural_hill`, `natural_saddle`, and `natural_volcano` (volcano already has it), matching the wiki's use of `ele=*` on these features.

#### Scenario: Hill type carries elevation feature
- **GIVEN** a database imported with the type definitions
- **WHEN** a node tagged `natural=hill` with `ele=1234` is imported
- **THEN** the imported object SHALL carry the `Ele` feature with value `1234`

#### Scenario: Saddle type carries elevation feature
- **GIVEN** a database imported with the type definitions
- **WHEN** a node tagged `natural=saddle` with `ele=567` is imported
- **THEN** the imported object SHALL carry the `Ele` feature with value `567`

### Requirement: Rendering rules for new natural types

The rendering stylesheets SHALL define rendering rules for all newly added natural types, so they are visible on maps.

#### Scenario: New natural area types are rendered
- **GIVEN** a rendering stylesheet that includes the natural rendering module
- **WHEN** a map is rendered containing areas of the new area-capable natural types (`natural_shrubbery`, `natural_reef`, `natural_shoal`, `natural_strait`, `natural_isthmus`, `natural_peninsula`, `natural_blowhole`, `natural_arch`, `natural_blockfield`, `natural_crevasse`, `natural_dune`, `natural_rock`, `natural_sinkhole`, `natural_moor`, `natural_tundra`, `natural_tree_group`)
- **THEN** the areas SHALL be drawn with a fill color

#### Scenario: New natural way types are rendered
- **GIVEN** a rendering stylesheet that includes the natural rendering module
- **WHEN** a map is rendered containing ways of the new way-capable natural types (`natural_tree_row`, `natural_arch`, `natural_arete`, `natural_crevasse`, `natural_dune`, `natural_earth_bank`, `natural_gorge`, `natural_gully`, `natural_ridge`, `natural_valley`)
- **THEN** the ways SHALL be drawn as lines

#### Scenario: New natural node types are rendered
- **GIVEN** a rendering stylesheet that includes the natural rendering module
- **WHEN** a map is rendered containing nodes of the new node-capable natural types (`natural_shrub`, `natural_tree_stump`, `natural_cape`, `natural_hot_spring`, `natural_geyser`, `natural_fumarole`, `natural_hill`, `natural_saddle`, `natural_stone`, `natural_rock`, `natural_valley`, `natural_shoal`, `natural_strait`, `natural_isthmus`, `natural_peninsula`, `natural_blowhole`, `natural_arch`, `natural_dune`, `natural_sinkhole`, `natural_moor`, `natural_tundra`, `natural_tree_group`)
- **THEN** the nodes SHALL be rendered with a symbol or text label
