# landuse-type-definitions Specification

## Purpose

Defines the import-time OSM feature types for `landuse=*` values missing from `stylesheets/map.ost`, so these features exist in the database `TypeConfig` and are renderable. Element types (node/area) follow the OSM wiki [Key:landuse](https://wiki.openstreetmap.org/wiki/Key:landuse) element table. Only values with taginfo usage >= 150 that are not discouraged, deprecated, or proposal-only are added.

## ADDED Requirements

### Requirement: Agricultural landuse types

The import-time stylesheet SHALL define feature types for the following `landuse=*` agricultural values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `landuse=animal_keeping` | `landuse_animal_keeping` | area |
| `landuse=apiary` | `landuse_apiary` | area |
| `landuse=aquaculture` | `landuse_aquaculture` | node, area |
| `landuse=plant_nursery` | `landuse_plant_nursery` | area |
| `landuse=logging` | `landuse_logging` | area |
| `landuse=forestry` | `landuse_forestry` | area |
| `landuse=peat_cutting` | `landuse_peat_cutting` | area |

#### Scenario: Animal keeping type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_animal_keeping`
- **THEN** the type SHALL exist
- **AND** areas tagged `landuse=animal_keeping` SHALL be importable as that type

#### Scenario: Apiary type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_apiary`
- **THEN** the type SHALL exist
- **AND** areas tagged `landuse=apiary` SHALL be importable as that type

#### Scenario: Aquaculture type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_aquaculture`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `landuse=aquaculture` SHALL be importable as that type

#### Scenario: Plant nursery type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_plant_nursery`
- **THEN** the type SHALL exist
- **AND** areas tagged `landuse=plant_nursery` SHALL be importable as that type

#### Scenario: Logging type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_logging`
- **THEN** the type SHALL exist
- **AND** areas tagged `landuse=logging` SHALL be importable as that type

#### Scenario: Forestry type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_forestry`
- **THEN** the type SHALL exist
- **AND** areas tagged `landuse=forestry` SHALL be importable as that type

#### Scenario: Peat cutting type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_peat_cutting`
- **THEN** the type SHALL exist
- **AND** areas tagged `landuse=peat_cutting` SHALL be importable as that type

### Requirement: Urban and institutional landuse types

The import-time stylesheet SHALL define feature types for the following `landuse=*` urban and institutional values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `landuse=education` | `landuse_education` | node, area |
| `landuse=healthcare` | `landuse_healthcare` | area |
| `landuse=culture` | `landuse_culture` | area |
| `landuse=institutional` | `landuse_institutional` | node, area |
| `landuse=religious` | `landuse_religious` | node, area |
| `landuse=fairground` | `landuse_fairground` | node, area |
| `landuse=highway` | `landuse_highway` | area |
| `landuse=static_caravan` | `landuse_static_caravan` | node, area |
| `landuse=winter_sports` | `landuse_winter_sports` | node, area |
| `landuse=flowerbed` | `landuse_flowerbed` | area |
| `landuse=greenery` | `landuse_greenery` | area |
| `landuse=tree_pit` | `landuse_tree_pit` | area |

#### Scenario: Education type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_education`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `landuse=education` SHALL be importable as that type

#### Scenario: Healthcare type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_healthcare`
- **THEN** the type SHALL exist
- **AND** areas tagged `landuse=healthcare` SHALL be importable as that type

#### Scenario: Culture type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_culture`
- **THEN** the type SHALL exist
- **AND** areas tagged `landuse=culture` SHALL be importable as that type

#### Scenario: Institutional type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_institutional`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `landuse=institutional` SHALL be importable as that type

#### Scenario: Religious type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_religious`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `landuse=religious` SHALL be importable as that type

#### Scenario: Fairground type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_fairground`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `landuse=fairground` SHALL be importable as that type

#### Scenario: Highway type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_highway`
- **THEN** the type SHALL exist
- **AND** areas tagged `landuse=highway` SHALL be importable as that type

#### Scenario: Static caravan type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_static_caravan`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `landuse=static_caravan` SHALL be importable as that type

#### Scenario: Winter sports type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_winter_sports`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `landuse=winter_sports` SHALL be importable as that type

#### Scenario: Flowerbed type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_flowerbed`
- **THEN** the type SHALL exist
- **AND** areas tagged `landuse=flowerbed` SHALL be importable as that type

#### Scenario: Greenery type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_greenery`
- **THEN** the type SHALL exist
- **AND** areas tagged `landuse=greenery` SHALL be importable as that type

#### Scenario: Tree pit type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_tree_pit`
- **THEN** the type SHALL exist
- **AND** areas tagged `landuse=tree_pit` SHALL be importable as that type

### Requirement: Discouraged landuse values are not defined

The import-time stylesheet SHALL NOT define feature types for `landuse=*` values that are deprecated, discouraged, or proposal-only, even when their taginfo usage is high. This includes at least `landuse=school` (deprecated, use `landuse=education`), `landuse=churchyard` (deprecated, use `landuse=religious`), `landuse=conservation` (deprecated), `landuse=farm` (discouraged), `landuse=agriculture` (discouraged), `landuse=livestock` (discouraged), `landuse=depot` (discouraged), `landuse=port` (discouraged), `landuse=harbour` (discouraged), `landuse=plantation` (discouraged), `landuse=paddy` (prefer `landuse=farmland` + `farmland=paddy`), `landuse=traffic_island` (abandoned proposal), `landuse=civic_admin` (proposal), `landuse=shrubs` (proposal), `landuse=wasteland` (proposal), `landuse=plot` (proposal), `landuse=pasture` (proposal), `landuse=governmental` (abandoned proposal), `landuse=civic` (synonym of abandoned proposal), `landuse=reservoir_watershed` (controversial), and `landuse=yes` (pointless).

#### Scenario: School type does not exist in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_school`
- **THEN** the type SHALL NOT exist

#### Scenario: Churchyard type does not exist in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_churchyard`
- **THEN** the type SHALL NOT exist

#### Scenario: Harbour type does not exist in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `landuse_harbour`
- **THEN** the type SHALL NOT exist

### Requirement: Rendering rules for new landuse types

The rendering stylesheets SHALL define rendering rules for all newly added landuse types, so they are visible on maps.

#### Scenario: New landuse area types are rendered
- **GIVEN** a rendering stylesheet that includes the landuse rendering module
- **WHEN** a map is rendered containing areas of the new area-capable landuse types
- **THEN** the areas SHALL be drawn with a fill color

#### Scenario: New landuse node types are rendered
- **GIVEN** a rendering stylesheet that includes the landuse rendering module
- **WHEN** a map is rendered containing nodes of the new node-capable landuse types (`landuse_aquaculture`, `landuse_education`, `landuse_fairground`, `landuse_institutional`, `landuse_religious`, `landuse_static_caravan`, `landuse_winter_sports`)
- **THEN** the nodes SHALL be labeled with their name

#### Scenario: New landuse types are labeled
- **GIVEN** a rendering stylesheet that includes the landuse rendering module
- **WHEN** a map is rendered at close magnification containing areas of the new landuse types with a name
- **THEN** the areas SHALL be labeled with their name

### Requirement: Pattern fills for landuse types

The rendering stylesheets SHALL define pattern fills for the new landuse types where a pattern tile exists, so structure and coverage are symbolized in addition to the fill color. Pattern tiles SHALL be named `<type>_<name>` (e.g. `landuse_apiary.png`) following the symbol naming convention, and SHALL be looked up as `<pattern-path>/<pattern-name>.png`.

#### Scenario: Landuse area renders with pattern fill
- **GIVEN** a rendering stylesheet that includes the landuse rendering module and a pattern tile for a new landuse type
- **WHEN** a map is rendered at a magnification at or above the type's `patternMinMag` containing an area of that type
- **THEN** the area SHALL be filled with the repeating pattern tile

#### Scenario: Landuse area falls back to solid color below pattern magnification
- **GIVEN** a rendering stylesheet that includes the landuse rendering module
- **WHEN** a map is rendered below the type's `patternMinMag` containing an area of a patterned landuse type
- **THEN** the area SHALL be filled with the solid fill color

#### Scenario: Religious pattern tile is a universal symbol
- **GIVEN** the `landuse_religious` pattern tile
- **THEN** the tile SHALL depict a universal place-of-worship symbol (building with spire) and SHALL NOT depict a cross
