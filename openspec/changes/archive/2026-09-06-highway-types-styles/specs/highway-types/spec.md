## Purpose

Defines complete coverage of documented `highway=*` values in the type definition file, with correct object types per the OSM wiki and style definitions for obvious visualizations.

## ADDED Requirements

### Requirement: Highway value coverage
The type definition file SHALL define a type for every documented `highway=*` value that has at least 1000 uses in OSM data and is not marked as discouraged.

#### Scenario: Common highway values are covered
- **WHEN** the type definition file is loaded
- **THEN** it defines types for the documented road, path, and highway-feature values including crossing, stop, give_way, turning_circle, turning_loop, passing_place, busway, raceway, escape, corridor, proposed, platform, elevator, rest_area, traffic_mirror, emergency_bay, emergency_access_point, trailhead, traffic_sign, toll_gantry, priority, speed_display, ladder, hitchhiking, and cyclist_waiting_aid

#### Scenario: Discouraged values are excluded
- **WHEN** the type definition file is loaded
- **THEN** it does not define types for discouraged values such as no, yes, planned, disused, abandoned, razed, piste, ford, turntable, residential_link, traffic_calming, sidewalk, and traffic_island

### Requirement: Object types match OSM wiki
Each highway type SHALL support the object types (NODE, WAY, AREA) documented for its value in the OSM wiki element tables.

#### Scenario: Node-only values
- **WHEN** a highway value documented as node-only, such as crossing, stop, or give_way, is imported
- **THEN** the type supports NODE objects

#### Scenario: Way values
- **WHEN** a highway value documented as way, such as busway, raceway, escape, proposed, or corridor, is imported
- **THEN** the type supports WAY objects

#### Scenario: Multi-object values
- **WHEN** a highway value documented for multiple object types, such as elevator (node and way), rest_area (node and area), or platform (node, way, and area), is imported
- **THEN** the type supports all documented object types

### Requirement: Style definitions for obvious visualizations
Highway types with an obvious visualization SHALL have corresponding style definitions in the standard stylesheet.

#### Scenario: Way types have line styles
- **WHEN** the standard stylesheet is loaded
- **THEN** busway, raceway, escape, proposed, and corridor have WAY style rules

#### Scenario: Node types have icons
- **WHEN** the standard stylesheet is loaded
- **THEN** stop, give_way, crossing, turning_circle, turning_loop, passing_place, priority, elevator, emergency_access_point, trailhead, rest_area, toll_gantry, traffic_mirror, speed_display, ladder, hitchhiking, cyclist_waiting_aid, emergency_bay, traffic_sign, and platform have NODE.ICON rules

### Requirement: Street lamp day/night rendering
Street lamps SHALL render with a light-off symbol during daylight and a light-on symbol at night.

#### Scenario: Daylight rendering
- **WHEN** the daylight flag is set
- **THEN** street lamps render with the light-off symbol

#### Scenario: Night rendering
- **WHEN** the daylight flag is not set
- **THEN** street lamps render with the light-on symbol
