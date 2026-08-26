# aerialway-type-definitions Specification

## Purpose

Defines the import-time OSM feature types for documented `aerialway=*` values missing from `stylesheets/map.ost`, so these features exist in the database `TypeConfig` and are importable, searchable, and renderable. Element types follow the OSM wiki [Key:aerialway](https://wiki.openstreetmap.org/wiki/Key:aerialway) element table. Only values that are documented, have relevant usage (>= 100 per taginfo), and are not discouraged/deprecated are added. Corresponding style definitions are provided where visualisation is obvious.

## Requirements

### Requirement: Pylon type

The import-time stylesheet SHALL define a feature type for `aerialway=pylon` with element type NODE, as specified by the OSM wiki [Tag:aerialway=pylon](https://wiki.openstreetmap.org/wiki/Tag:aerialway=pylon).

#### Scenario: Pylon type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `aerialway_pylon`
- **THEN** the type SHALL exist
- **AND** nodes tagged `aerialway=pylon` SHALL be importable as that type
- **AND** ways and areas tagged `aerialway=pylon` SHALL NOT be importable as that type

#### Scenario: Pylon renders as node symbol
- **GIVEN** a map rendered with the style sheet at close zoom
- **WHEN** a node of type `aerialway_pylon` is drawn
- **THEN** a small node symbol SHALL be rendered

### Requirement: Station type

The import-time stylesheet SHALL define a feature type for `aerialway=station` with element types NODE and AREA, as specified by the OSM wiki [Tag:aerialway=station](https://wiki.openstreetmap.org/wiki/Tag:aerialway=station). The type SHALL be indexed as a POI by name.

#### Scenario: Station type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `aerialway_station`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `aerialway=station` SHALL be importable as that type
- **AND** ways tagged `aerialway=station` SHALL NOT be importable as that type

#### Scenario: Station is searchable as POI
- **GIVEN** a database imported with the type definitions
- **WHEN** a location search is performed for a named `aerialway=station` object
- **THEN** the object SHALL be found by its name

#### Scenario: Station renders as node symbol with label
- **GIVEN** a map rendered with the style sheet at close zoom
- **WHEN** a node of type `aerialway_station` is drawn
- **THEN** a node symbol SHALL be rendered
- **AND** the station name SHALL be rendered as a label

### Requirement: Zip line type

The import-time stylesheet SHALL define a feature type for `aerialway=zip_line` with element type WAY, as specified by the OSM wiki [Tag:aerialway=zip_line](https://wiki.openstreetmap.org/wiki/Tag:aerialway=zip_line).

#### Scenario: Zip line type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `aerialway_zip_line`
- **THEN** the type SHALL exist
- **AND** ways tagged `aerialway=zip_line` SHALL be importable as that type
- **AND** nodes and areas tagged `aerialway=zip_line` SHALL NOT be importable as that type

#### Scenario: Zip line renders as dashed line
- **GIVEN** a map rendered with the style sheet at close zoom
- **WHEN** a way of type `aerialway_zip_line` is drawn
- **THEN** a dashed line SHALL be rendered

### Requirement: Goods type

The import-time stylesheet SHALL define a feature type for `aerialway=goods` with element type WAY, as specified by the OSM wiki [Tag:aerialway=goods](https://wiki.openstreetmap.org/wiki/Tag:aerialway=goods).

#### Scenario: Goods type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `aerialway_goods`
- **THEN** the type SHALL exist
- **AND** ways tagged `aerialway=goods` SHALL be importable as that type
- **AND** nodes and areas tagged `aerialway=goods` SHALL NOT be importable as that type

#### Scenario: Goods renders as dashed line
- **GIVEN** a map rendered with the style sheet at close zoom
- **WHEN** a way of type `aerialway_goods` is drawn
- **THEN** a dashed line SHALL be rendered

### Requirement: Aerialway way types render in consistent order

The style sheets SHALL draw all aerialway way types (`aerialway_gondola`, `aerialway_chair_lift`, `aerialway_drag_lift`, `aerialway_zip_line`, `aerialway_goods`) in the same rendering order group, so they do not interleave with roads or other transport ways.

#### Scenario: Aerialway ways share rendering order group
- **GIVEN** a style sheet with an `ORDER WAYS` section
- **WHEN** the aerialway way types are listed
- **THEN** all five aerialway way types SHALL be in the same group
