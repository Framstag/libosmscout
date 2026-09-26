# military-type-definitions Specification

## Purpose

Defines the import-time OSM feature types for `military=*` values missing from `stylesheets/map.ost`, so these features exist in the database `TypeConfig` and are importable, searchable, and renderable. Element types follow the OSM wiki [Key:military](https://wiki.openstreetmap.org/wiki/Key:military) element table for documented values; for undocumented values they follow taginfo node/way/relation usage statistics. Only values with relevant usage (>= 0.10% of all `military=*` objects per taginfo) that are not discouraged/deprecated are added.

## ADDED Requirements

### Requirement: Node-only military types

The import-time stylesheet SHALL define feature types for the following node-only `military=*` values, with element type NODE as specified:

| OSM tag | Type name |
|---------|-----------|
| `military=cannon` | `military_cannon` |
| `military=checkpoint` | `military_checkpoint` |
| `military=embrasure` | `military_embrasure` |

#### Scenario: Cannon type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `military_cannon`
- **THEN** the type SHALL exist
- **AND** nodes tagged `military=cannon` SHALL be importable as that type
- **AND** ways and areas tagged `military=cannon` SHALL NOT be importable as that type

#### Scenario: Checkpoint type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `military_checkpoint`
- **THEN** the type SHALL exist
- **AND** nodes tagged `military=checkpoint` SHALL be importable as that type
- **AND** ways and areas tagged `military=checkpoint` SHALL NOT be importable as that type

#### Scenario: Embrasure type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `military_embrasure`
- **THEN** the type SHALL exist
- **AND** nodes tagged `military=embrasure` SHALL be importable as that type

### Requirement: Node-area military types

The import-time stylesheet SHALL define feature types for the following `military=*` values accepting NODE and AREA elements, as specified:

| OSM tag | Type name |
|---------|-----------|
| `military=ammunition` | `military_ammunition` |
| `military=base` | `military_base` |
| `military=nuclear_explosion_site` | `military_nuclear_explosion_site` |
| `military=office` | `military_office` |
| `military=police` | `military_police` |
| `military=radar` | `military_radar` |

#### Scenario: Base type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `military_base`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `military=base` SHALL be importable as that type

#### Scenario: Ammunition type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `military_ammunition`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `military=ammunition` SHALL be importable as that type

#### Scenario: Nuclear explosion site type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `military_nuclear_explosion_site`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `military=nuclear_explosion_site` SHALL be importable as that type

#### Scenario: Office type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `military_office`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `military=office` SHALL be importable as that type
- **AND** the type SHALL be a member of the `office` type group

#### Scenario: Police type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `military_police`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `military=police` SHALL be importable as that type

#### Scenario: Radar type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `military_radar`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `military=radar` SHALL be importable as that type

### Requirement: Way and way-area military types

The import-time stylesheet SHALL define feature types for the following `military=*` values involving WAY elements, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `military=cordon` | `military_cordon` | way |
| `military=road` | `military_road` | way |
| `military=obstacle_course` | `military_obstacle_course` | way, area |
| `military=shelter` | `military_shelter` | way, area |
| `military=trench` | `military_trench` | node, way |

#### Scenario: Cordon type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `military_cordon`
- **THEN** the type SHALL exist
- **AND** ways tagged `military=cordon` SHALL be importable as that type
- **AND** nodes and areas tagged `military=cordon` SHALL NOT be importable as that type

#### Scenario: Road type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `military_road`
- **THEN** the type SHALL exist
- **AND** ways tagged `military=road` SHALL be importable as that type

#### Scenario: Obstacle course type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `military_obstacle_course`
- **THEN** the type SHALL exist
- **AND** ways and areas tagged `military=obstacle_course` SHALL be importable as that type
- **AND** nodes tagged `military=obstacle_course` SHALL NOT be importable as that type

#### Scenario: Shelter type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `military_shelter`
- **THEN** the type SHALL exist
- **AND** ways and areas tagged `military=shelter` SHALL be importable as that type

#### Scenario: Trench type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `military_trench`
- **THEN** the type SHALL exist
- **AND** nodes and ways tagged `military=trench` SHALL be importable as that type
- **AND** areas tagged `military=trench` SHALL NOT be importable as that type

### Requirement: Area-only military types

The import-time stylesheet SHALL define an AREA-only feature type for `military=training_area`, as specified:

| OSM tag | Type name |
|---------|-----------|
| `military=training_area` | `military_training_area` |

#### Scenario: Training area type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `military_training_area`
- **THEN** the type SHALL exist
- **AND** areas tagged `military=training_area` SHALL be importable as that type
- **AND** nodes and ways tagged `military=training_area` SHALL NOT be importable as that type

### Requirement: Discouraged and undocumented military values

The import-time stylesheet SHALL NOT define dedicated feature types for discouraged or deprecated `military=*` values, including but not limited to `military=yes` (replace with a more specific value), `military=abandoned` (use `abandoned=*`), and `military=naval_base` (deprecated in favor of `military=base` + `military_service=navy`). The pre-existing `military_naval_base` type SHALL remain unchanged. Undocumented values with taginfo usage below 0.10% SHALL NOT get dedicated types.

#### Scenario: Discouraged values have no new dedicated type
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `military_yes` or `military_abandoned`
- **THEN** neither type SHALL exist

#### Scenario: Naval base type remains unchanged
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `military_naval_base`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `military=naval_base` SHALL be importable as that type

### Requirement: Area rendering for new military types

The rendering stylesheet SHALL give areas of the new installation types (`military_ammunition`, `military_base`, `military_nuclear_explosion_site`, `military_training_area`) a fill matching the existing military area color at city zoom, SHALL render labels for these types at close zoom following the existing military label rules, and SHALL render the way-capable types (`military_cordon`, `military_road`, `military_shelter`, `military_trench`) with a dashed line at close zoom. `military_obstacle_course` SHALL get an area fill and dashed line at detail zoom.

#### Scenario: New military areas get fill at city zoom
- **GIVEN** a map rendered with the stylesheet at city zoom
- **WHEN** an area of type `military_base`, `military_training_area`, `military_nuclear_explosion_site`, or `military_ammunition` is rendered
- **THEN** the area SHALL be filled with the military area color

#### Scenario: New military areas get labels at close zoom
- **GIVEN** a map rendered with the stylesheet at close zoom
- **WHEN** an area of type `military_base`, `military_training_area`, `military_nuclear_explosion_site`, or `military_ammunition` is rendered
- **THEN** the area SHALL be labeled with its name following the military label rules

#### Scenario: Linear military types get dashed rendering
- **GIVEN** a map rendered with the stylesheet at close zoom
- **WHEN** a way of type `military_trench`, `military_cordon`, `military_road`, or `military_shelter` is rendered
- **THEN** the way SHALL be rendered with a dashed line

#### Scenario: Obstacle course gets area and line rendering
- **GIVEN** a map rendered with the stylesheet at detail zoom
- **WHEN** an area or way of type `military_obstacle_course` is rendered
- **THEN** the area SHALL be filled with the military area color
- **AND** the way SHALL be rendered with a dashed line

### Requirement: Node icon rendering for new military types

The rendering stylesheet SHALL define a dedicated vector symbol for each new military type that accepts NODE elements where an obvious pictogram can be drawn, and SHALL render nodes of those types with the corresponding symbol icon at very close zoom. Types without a dedicated symbol SHALL fall back to the generic `military` symbol. The generic `military` symbol SHALL remain the fallback for all military types.

#### Scenario: Checkpoint node gets checkpoint icon
- **GIVEN** a map rendered with the stylesheet at very close zoom
- **WHEN** a node of type `military_checkpoint` is rendered
- **THEN** the node SHALL be rendered with the `military_checkpoint` symbol icon

#### Scenario: Cannon node gets cannon icon
- **GIVEN** a map rendered with the stylesheet at very close zoom
- **WHEN** a node of type `military_cannon` is rendered
- **THEN** the node SHALL be rendered with the `military_cannon` symbol icon

#### Scenario: Radar node gets radar icon
- **GIVEN** a map rendered with the stylesheet at very close zoom
- **WHEN** a node of type `military_radar` is rendered
- **THEN** the node SHALL be rendered with the `military_radar` symbol icon

#### Scenario: Trench node gets trench icon
- **GIVEN** a map rendered with the stylesheet at very close zoom
- **WHEN** a node of type `military_trench` is rendered
- **THEN** the node SHALL be rendered with the `military_trench` symbol icon

#### Scenario: Training area node falls back to generic icon
- **GIVEN** a map rendered with the stylesheet at very close zoom
- **WHEN** a node of type `military_training_area` is rendered
- **THEN** the node SHALL be rendered with the generic `military` symbol icon
