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

`natural=coastline` SHALL NOT be defined as an import-time type: coastline ways are handled by the separate water/land index and coastline baseline pipeline, not by regular type-based import.

`natural=waterfall` SHALL NOT be defined as a dedicated `natural_waterfall` import-time type: the tag is deprecated by OSM and is merged into the existing `waterway_waterfall` type (see merge-waterfall-type-definitions), so a single type definition handles both `waterway=waterfall` and `natural=waterfall`.

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
- **THEN** the type SHALL NOT exist
- **AND** nodes, ways, and areas tagged `natural=waterfall` SHALL be importable as `waterway_waterfall` instead

#### Scenario: Natural waterfall objects import as merged waterway type
- **GIVEN** a database imported with the type definitions
- **WHEN** a node, way, or area tagged `natural=waterfall` is imported
- **THEN** the object SHALL be importable as the `waterway_waterfall` type

#### Scenario: Coastline type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `natural_coastline`
- **THEN** the type SHALL NOT exist
- **AND** ways tagged `natural=coastline` SHALL NOT be importable as a regular natural way type
