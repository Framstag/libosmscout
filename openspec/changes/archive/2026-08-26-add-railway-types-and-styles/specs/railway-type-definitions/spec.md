# railway-type-definitions Specification

## Purpose

Defines the import-time OSM feature types for documented `railway=*` values missing from `stylesheets/map.ost`, so these features exist in the database `TypeConfig` and are importable, searchable, and renderable. Element types follow the OSM wiki [Key:railway](https://wiki.openstreetmap.org/wiki/Key:railway) element table. Only values that are documented, have relevant usage (>= 0.02% per taginfo), and are not discouraged/deprecated are added. Corresponding style definitions are provided where visualisation is obvious.

## ADDED Requirements

### Requirement: Track railway types

The import-time stylesheet SHALL define feature types for the following `railway=*` track values, with element type WAY as specified:

| OSM tag | Type name |
|---------|-----------|
| `railway=construction` | `railway_construction` |
| `railway=proposed` | `railway_proposed` |
| `railway=miniature` | `railway_miniature` |

#### Scenario: Construction type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `railway_construction`
- **THEN** the type SHALL exist
- **AND** ways tagged `railway=construction` SHALL be importable as that type
- **AND** nodes and areas tagged `railway=construction` SHALL NOT be importable as that type

#### Scenario: Proposed type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `railway_proposed`
- **THEN** the type SHALL exist
- **AND** ways tagged `railway=proposed` SHALL be importable as that type

#### Scenario: Miniature type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `railway_miniature`
- **THEN** the type SHALL exist
- **AND** ways tagged `railway=miniature` SHALL be importable as that type

### Requirement: Stop railway types

The import-time stylesheet SHALL define feature types for the following `railway=*` stop values, with element type NODE as specified:

| OSM tag | Type name |
|---------|-----------|
| `railway=stop` | `railway_stop` |
| `railway=tram_crossing` | `railway_tram_crossing` |
| `railway=tram_level_crossing` | `railway_tram_level_crossing` |
| `railway=train_station_entrance` | `railway_train_station_entrance` |

#### Scenario: Stop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `railway_stop`
- **THEN** the type SHALL exist
- **AND** nodes tagged `railway=stop` SHALL be importable as that type

#### Scenario: Tram crossing types exist in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `railway_tram_crossing` and `railway_tram_level_crossing`
- **THEN** both types SHALL exist
- **AND** nodes tagged `railway=tram_crossing` SHALL be importable as `railway_tram_crossing`
- **AND** nodes tagged `railway=tram_level_crossing` SHALL be importable as `railway_tram_level_crossing`

#### Scenario: Train station entrance type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `railway_train_station_entrance`
- **THEN** the type SHALL exist
- **AND** nodes tagged `railway=train_station_entrance` SHALL be importable as that type

### Requirement: Node-area railway types

The import-time stylesheet SHALL define feature types for the following `railway=*` values accepting NODE and AREA elements, as specified:

| OSM tag | Type name |
|---------|-----------|
| `railway=service_station` | `railway_service_station` |
| `railway=signal_box` | `railway_signal_box` |
| `railway=yard` | `railway_yard` |

#### Scenario: Service station type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `railway_service_station`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `railway=service_station` SHALL be importable as that type

#### Scenario: Signal box type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `railway_signal_box`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `railway=signal_box` SHALL be importable as that type

#### Scenario: Yard type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `railway_yard`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `railway=yard` SHALL be importable as that type

### Requirement: Infrastructure node railway types

The import-time stylesheet SHALL define feature types for the following `railway=*` infrastructure values, with element type NODE as specified:

| OSM tag | Type name |
|---------|-----------|
| `railway=switch` | `railway_switch` |
| `railway=signal` | `railway_signal` |
| `railway=buffer_stop` | `railway_buffer_stop` |
| `railway=milestone` | `railway_milestone` |
| `railway=railway_crossing` | `railway_railway_crossing` |
| `railway=derail` | `railway_derail` |
| `railway=junction` | `railway_junction` |
| `railway=radio` | `railway_radio` |
| `railway=vacancy_detection` | `railway_vacancy_detection` |
| `railway=phone` | `railway_phone` |
| `railway=spur_junction` | `railway_spur_junction` |
| `railway=rail_brake` | `railway_rail_brake` |
| `railway=defect_detector` | `railway_defect_detector` |
| `railway=power_supply` | `railway_power_supply` |
| `railway=owner_change` | `railway_owner_change` |
| `railway=crossover` | `railway_crossover` |
| `railway=platform_marker` | `railway_platform_marker` |

#### Scenario: Switch type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `railway_switch`
- **THEN** the type SHALL exist
- **AND** nodes tagged `railway=switch` SHALL be importable as that type
- **AND** ways and areas tagged `railway=switch` SHALL NOT be importable as that type

#### Scenario: Signal type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `railway_signal`
- **THEN** the type SHALL exist
- **AND** nodes tagged `railway=signal` SHALL be importable as that type

#### Scenario: Buffer stop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `railway_buffer_stop`
- **THEN** the type SHALL exist
- **AND** nodes tagged `railway=buffer_stop` SHALL be importable as that type

#### Scenario: Milestone type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `railway_milestone`
- **THEN** the type SHALL exist
- **AND** nodes tagged `railway=milestone` SHALL be importable as that type

#### Scenario: Railway crossing type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `railway_railway_crossing`
- **THEN** the type SHALL exist
- **AND** nodes tagged `railway=railway_crossing` SHALL be importable as that type

#### Scenario: Derail type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `railway_derail`
- **THEN** the type SHALL exist
- **AND** nodes tagged `railway=derail` SHALL be importable as that type

#### Scenario: Junction type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `railway_junction`
- **THEN** the type SHALL exist
- **AND** nodes tagged `railway=junction` SHALL be importable as that type

#### Scenario: Remaining infrastructure node types exist in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for each of `railway_radio`, `railway_vacancy_detection`, `railway_phone`, `railway_spur_junction`, `railway_rail_brake`, `railway_defect_detector`, `railway_power_supply`, `railway_owner_change`, `railway_crossover`, `railway_platform_marker`
- **THEN** each type SHALL exist
- **AND** nodes tagged with the corresponding `railway=*` value SHALL be importable as that type

### Requirement: Multi-element railway types

The import-time stylesheet SHALL define feature types for the following `railway=*` values with the element types as specified:

| OSM tag | Type name | Element types |
|---------|-----------|---------------|
| `railway=loading_ramp` | `railway_loading_ramp` | NODE, WAY, AREA |
| `railway=ventilation_shaft` | `railway_ventilation_shaft` | NODE, WAY |
| `railway=workshop` | `railway_workshop` | AREA |
| `railway=platform_edge` | `railway_platform_edge` | WAY |

#### Scenario: Loading ramp type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `railway_loading_ramp`
- **THEN** the type SHALL exist
- **AND** nodes, ways, and areas tagged `railway=loading_ramp` SHALL be importable as that type

#### Scenario: Ventilation shaft type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `railway_ventilation_shaft`
- **THEN** the type SHALL exist
- **AND** nodes and ways tagged `railway=ventilation_shaft` SHALL be importable as that type

#### Scenario: Workshop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `railway_workshop`
- **THEN** the type SHALL exist
- **AND** areas tagged `railway=workshop` SHALL be importable as that type

#### Scenario: Platform edge type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `railway_platform_edge`
- **THEN** the type SHALL exist
- **AND** ways tagged `railway=platform_edge` SHALL be importable as that type

### Requirement: Railway infrastructure symbols

The style sheet SHALL define distinct symbols for the following railway infrastructure node types, rendered at close zoom levels:

| Type | Symbol |
|------|--------|
| `railway_switch` | `railway_switch` |
| `railway_signal` | `railway_signal` |
| `railway_buffer_stop` | `railway_buffer_stop` |
| `railway_milestone` | `railway_milestone` |
| `railway_stop` | `railway_stop` |

#### Scenario: Switch symbol renders
- **GIVEN** a map rendered with the style sheet at very close zoom
- **WHEN** a node of type `railway_switch` is drawn
- **THEN** the `railway_switch` symbol SHALL be rendered at the node position

#### Scenario: Signal symbol renders upright
- **GIVEN** a map rendered with the style sheet at very close zoom
- **WHEN** a node of type `railway_signal` is drawn
- **THEN** the `railway_signal` symbol SHALL be rendered with the light above the pole

#### Scenario: Buffer stop symbol renders
- **GIVEN** a map rendered with the style sheet at very close zoom
- **WHEN** a node of type `railway_buffer_stop` is drawn
- **THEN** the `railway_buffer_stop` symbol SHALL be rendered at the node position

#### Scenario: Milestone symbol renders
- **GIVEN** a map rendered with the style sheet at very close zoom
- **WHEN** a node of type `railway_milestone` is drawn
- **THEN** the `railway_milestone` symbol SHALL be rendered at the node position

#### Scenario: Stop symbol renders
- **GIVEN** a map rendered with the style sheet at very close zoom
- **WHEN** a node of type `railway_stop` is drawn
- **THEN** the `railway_stop` symbol SHALL be rendered at the node position

### Requirement: Reused crossing and entrance symbols

The style sheet SHALL render the following railway node types using existing symbols:

| Type | Reused symbol |
|------|---------------|
| `railway_tram_crossing` | `railway_crossing` |
| `railway_tram_level_crossing` | `railway_level_crossing` |
| `railway_railway_crossing` | `railway_crossing` |
| `railway_train_station_entrance` | `railway_subway_entrance` |

#### Scenario: Tram crossing reuses crossing symbol
- **GIVEN** a map rendered with the style sheet at very close zoom
- **WHEN** a node of type `railway_tram_crossing` is drawn
- **THEN** the `railway_crossing` symbol SHALL be rendered at the node position

#### Scenario: Tram level crossing reuses level crossing symbol
- **GIVEN** a map rendered with the style sheet at very close zoom
- **WHEN** a node of type `railway_tram_level_crossing` is drawn
- **THEN** the `railway_level_crossing` symbol SHALL be rendered at the node position

#### Scenario: Train station entrance reuses subway entrance symbol
- **GIVEN** a map rendered with the style sheet at very close zoom
- **WHEN** a node of type `railway_train_station_entrance` is drawn
- **THEN** the `railway_subway_entrance` symbol SHALL be rendered at the node position

### Requirement: Distinct station, halt, and tram stop symbols

The style sheet SHALL render `railway_station`, `railway_halt`, and `railway_tram_stop` with visually distinct symbols: a square with a white center dot for stations, a smaller solid square for halts, and a solid circle for tram stops.

#### Scenario: Station symbol is a square with center dot
- **GIVEN** a map rendered with the style sheet
- **WHEN** a node of type `railway_station` is drawn
- **THEN** the rendered symbol SHALL be a filled square with a white dot at its center

#### Scenario: Halt symbol is a smaller square
- **GIVEN** a map rendered with the style sheet
- **WHEN** a node of type `railway_halt` is drawn
- **THEN** the rendered symbol SHALL be a filled square smaller than the station symbol

#### Scenario: Tram stop symbol is a circle
- **GIVEN** a map rendered with the style sheet
- **WHEN** a node of type `railway_tram_stop` is drawn
- **THEN** the rendered symbol SHALL be a filled circle

### Requirement: Track and area styles for new types

The style sheet SHALL render the following new types with line or area styles:

| Type | Style |
|------|-------|
| `railway_construction` | dashed line |
| `railway_proposed` | dashed line, lighter than construction |
| `railway_miniature` | thin line like narrow gauge |
| `railway_platform_edge` | thin line |
| `railway_yard` | area fill matching `landuse_railway` |
| `railway_workshop` | building area fill |
| `railway_signal_box` | building area fill |

#### Scenario: Construction renders as dashed line
- **GIVEN** a map rendered with the style sheet at close zoom
- **WHEN** a way of type `railway_construction` is drawn
- **THEN** a dashed line SHALL be rendered

#### Scenario: Proposed renders as dashed line
- **GIVEN** a map rendered with the style sheet at close zoom
- **WHEN** a way of type `railway_proposed` is drawn
- **THEN** a dashed line SHALL be rendered

#### Scenario: Miniature renders as thin line
- **GIVEN** a map rendered with the style sheet at close zoom
- **WHEN** a way of type `railway_miniature` is drawn
- **THEN** a thin line SHALL be rendered

#### Scenario: Yard renders as area
- **GIVEN** a map rendered with the style sheet at close zoom
- **WHEN** an area of type `railway_yard` is drawn
- **THEN** an area fill SHALL be rendered

#### Scenario: Workshop and signal box render as buildings
- **GIVEN** a map rendered with the style sheet
- **WHEN** an area of type `railway_workshop` or `railway_signal_box` is drawn
- **THEN** a building area fill SHALL be rendered
