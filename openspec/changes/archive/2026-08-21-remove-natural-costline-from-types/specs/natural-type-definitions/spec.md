# natural-type-definitions Delta

## MODIFIED Requirements

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
