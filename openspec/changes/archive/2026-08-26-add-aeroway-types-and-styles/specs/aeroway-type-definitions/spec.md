# aeroway-type-definitions Specification

## Purpose

Defines the import-time OSM feature types for documented `aeroway=*` values missing from `stylesheets/map.ost`, so these features exist in the database `TypeConfig` and are importable, searchable, and renderable. Element types follow the OSM wiki [Key:aeroway](https://wiki.openstreetmap.org/wiki/Key:aeroway) element table, cross-checked against the individual `Tag:aeroway=*` pages. Only values that are documented, have relevant usage (>= 0.05% per taginfo), and are not discouraged/deprecated are added. Corresponding style definitions are provided where visualisation is obvious.

## ADDED Requirements

### Requirement: Node aeroway types

The import-time stylesheet SHALL define feature types for the following `aeroway=*` node values, with element type NODE as specified:

| OSM tag | Type name |
|---------|-----------|
| `aeroway=navigationaid` | `aeroway_navigationaid` |
| `aeroway=windsock` | `aeroway_windsock` |
| `aeroway=threshold` | `aeroway_threshold` |
| `aeroway=aircraft_crossing` | `aeroway_aircraft_crossing` |

#### Scenario: Navigation aid type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `aeroway_navigationaid`
- **THEN** the type SHALL exist
- **AND** nodes tagged `aeroway=navigationaid` SHALL be importable as that type
- **AND** ways and areas tagged `aeroway=navigationaid` SHALL NOT be importable as that type

#### Scenario: Windsock type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `aeroway_windsock`
- **THEN** the type SHALL exist
- **AND** nodes tagged `aeroway=windsock` SHALL be importable as that type

#### Scenario: Threshold type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `aeroway_threshold`
- **THEN** the type SHALL exist
- **AND** nodes tagged `aeroway=threshold` SHALL be importable as that type

#### Scenario: Aircraft crossing type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `aeroway_aircraft_crossing`
- **THEN** the type SHALL exist
- **AND** nodes tagged `aeroway=aircraft_crossing` SHALL be importable as that type

### Requirement: Node-way aeroway types

The import-time stylesheet SHALL define feature types for the following `aeroway=*` values accepting NODE and WAY elements, as specified:

| OSM tag | Type name |
|---------|-----------|
| `aeroway=parking_position` | `aeroway_parking_position` |
| `aeroway=holding_position` | `aeroway_holding_position` |

#### Scenario: Parking position type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `aeroway_parking_position`
- **THEN** the type SHALL exist
- **AND** nodes and ways tagged `aeroway=parking_position` SHALL be importable as that type
- **AND** areas tagged `aeroway=parking_position` SHALL NOT be importable as that type

#### Scenario: Holding position type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `aeroway_holding_position`
- **THEN** the type SHALL exist
- **AND** nodes and ways tagged `aeroway=holding_position` SHALL be importable as that type

### Requirement: Node-area aeroway types

The import-time stylesheet SHALL define feature types for the following `aeroway=*` values accepting NODE and AREA elements, as specified:

| OSM tag | Type name |
|---------|-----------|
| `aeroway=hangar` | `aeroway_hangar` |
| `aeroway=airstrip` | `aeroway_airstrip` |
| `aeroway=tower` | `aeroway_tower` |
| `aeroway=heliport` | `aeroway_heliport` |
| `aeroway=fuel` | `aeroway_fuel` |

#### Scenario: Hangar type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `aeroway_hangar`
- **THEN** the type SHALL exist
- **AND** areas tagged `aeroway=hangar` SHALL be importable as that type
- **AND** nodes and ways tagged `aeroway=hangar` SHALL NOT be importable as that type

#### Scenario: Airstrip type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `aeroway_airstrip`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `aeroway=airstrip` SHALL be importable as that type

#### Scenario: Tower type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `aeroway_tower`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `aeroway=tower` SHALL be importable as that type

#### Scenario: Heliport type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `aeroway_heliport`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `aeroway=heliport` SHALL be importable as that type

#### Scenario: Fuel type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `aeroway_fuel`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `aeroway=fuel` SHALL be importable as that type

### Requirement: Way aeroway types

The import-time stylesheet SHALL define feature types for the following `aeroway=*` way values, with element type WAY as specified:

| OSM tag | Type name |
|---------|-----------|
| `aeroway=taxilane` | `aeroway_taxilane` |
| `aeroway=jet_bridge` | `aeroway_jet_bridge` |
| `aeroway=stopway` | `aeroway_stopway` |
| `aeroway=model_runway` | `aeroway_model_runway` |

#### Scenario: Taxilane type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `aeroway_taxilane`
- **THEN** the type SHALL exist
- **AND** ways tagged `aeroway=taxilane` SHALL be importable as that type
- **AND** nodes and areas tagged `aeroway=taxilane` SHALL NOT be importable as that type

#### Scenario: Jet bridge type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `aeroway_jet_bridge`
- **THEN** the type SHALL exist
- **AND** ways tagged `aeroway=jet_bridge` SHALL be importable as that type

#### Scenario: Stopway type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `aeroway_stopway`
- **THEN** the type SHALL exist
- **AND** ways tagged `aeroway=stopway` SHALL be importable as that type

#### Scenario: Model runway type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `aeroway_model_runway`
- **THEN** the type SHALL exist
- **AND** ways tagged `aeroway=model_runway` SHALL be importable as that type

### Requirement: Aeroway style definitions

The stylesheet SHALL define style rules for the new aeroway types where visualisation is obvious, so the features are visible on the rendered map.

#### Scenario: New node types have icons at very close zoom
- **GIVEN** the standard stylesheet with the new type definitions
- **WHEN** the map is rendered at veryClose zoom
- **THEN** nodes of types `aeroway_navigationaid`, `aeroway_windsock`, `aeroway_threshold`, `aeroway_aircraft_crossing`, `aeroway_holding_position`, `aeroway_parking_position`, `aeroway_tower`, `aeroway_heliport`, `aeroway_fuel` SHALL be rendered with a symbol or text label

#### Scenario: New way types are rendered
- **GIVEN** the standard stylesheet with the new type definitions
- **WHEN** the map is rendered at close zoom
- **THEN** ways of types `aeroway_taxilane`, `aeroway_jet_bridge`, `aeroway_stopway`, `aeroway_model_runway` SHALL be rendered as lines

#### Scenario: New area types are rendered
- **GIVEN** the standard stylesheet with the new type definitions
- **WHEN** the map is rendered at city zoom
- **THEN** areas of types `aeroway_hangar`, `aeroway_airstrip`, `aeroway_heliport`, `aeroway_tower`, `aeroway_fuel` SHALL be rendered with a fill

#### Scenario: Building-like areas keep building rendering
- **GIVEN** an area tagged `aeroway=hangar` and `building=yes`
- **WHEN** the map is rendered at close zoom
- **THEN** the area SHALL be rendered with a building outline in addition to the aeroway fill
