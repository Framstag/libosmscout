# man-made-type-definitions Specification

## Purpose

Defines the import-time OSM feature types for documented `man_made=*` values missing from `stylesheets/map.ost`, so these features exist in the database `TypeConfig` and are importable and renderable. Element types follow the OSM wiki [Key:man_made](https://wiki.openstreetmap.org/wiki/Key:man_made) element table and the individual tag pages. Only values that are documented, have relevant usage (>= 0.05% per taginfo), and are not discouraged/deprecated are added. Corresponding style definitions are provided where visualisation is obvious.

## ADDED Requirements

### Requirement: Way-only man made types

The import-time stylesheet SHALL define feature types for the following way-only `man_made=*` values:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `man_made=cutline` | `man_made_cutline` | way |
| `man_made=pipeline` | `man_made_pipeline` | way |
| `man_made=embankment` | `man_made_embankment` | way |
| `man_made=goods_conveyor` | `man_made_goods_conveyor` | way |
| `man_made=cooling` | `man_made_cooling` | way |
| `man_made=snow_fence` | `man_made_snow_fence` | way |

#### Scenario: Cutline type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_cutline`
- **THEN** the type SHALL exist
- **AND** ways tagged `man_made=cutline` SHALL be importable as that type
- **AND** nodes tagged `man_made=cutline` SHALL NOT be importable as that type

#### Scenario: Pipeline type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_pipeline`
- **THEN** the type SHALL exist
- **AND** ways tagged `man_made=pipeline` SHALL be importable as that type

#### Scenario: Embankment type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_embankment`
- **THEN** the type SHALL exist
- **AND** ways tagged `man_made=embankment` SHALL be importable as that type

#### Scenario: Goods conveyor type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_goods_conveyor`
- **THEN** the type SHALL exist
- **AND** ways tagged `man_made=goods_conveyor` SHALL be importable as that type

#### Scenario: Cooling type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_cooling`
- **THEN** the type SHALL exist
- **AND** ways tagged `man_made=cooling` SHALL be importable as that type

#### Scenario: Snow fence type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made=snow_fence`
- **THEN** the type SHALL exist
- **AND** ways tagged `man_made=snow_fence` SHALL be importable as that type

### Requirement: Node-only man made types

The import-time stylesheet SHALL define feature types for the following node-only `man_made=*` values:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `man_made=surveillance` | `man_made_surveillance` | node |
| `man_made=manhole` | `man_made_manhole` | node |
| `man_made=mast` | `man_made_mast` | node |
| `man_made=utility_pole` | `man_made_utility_pole` | node |
| `man_made=survey_point` | `man_made_survey_point` | node |
| `man_made=petroleum_well` | `man_made_petroleum_well` | node |
| `man_made=flagpole` | `man_made_flagpole` | node |
| `man_made=chimney` | `man_made_chimney` | node |
| `man_made=beehive` | `man_made_beehive` | node |
| `man_made=charge_point` | `man_made_charge_point` | node |
| `man_made=cross` | `man_made_cross` | node |
| `man_made=adit` | `man_made_adit` | node |
| `man_made=lighthouse` | `man_made_lighthouse` | node |
| `man_made=snow_cannon` | `man_made_snow_cannon` | node |
| `man_made=flare` | `man_made_flare` | node |
| `man_made=beacon` | `man_made_beacon` | node |
| `man_made=telephone_box` | `man_made_telephone_box` | node |
| `man_made=oil_gas_separator` | `man_made_oil_gas_separator` | node |
| `man_made=buoy` | `man_made_buoy` | node |
| `man_made=fuel_pump` | `man_made_fuel_pump` | node |
| `man_made=carpet_hanger` | `man_made_carpet_hanger` | node |

#### Scenario: Surveillance type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_surveillance`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=surveillance` SHALL be importable as that type
- **AND** ways tagged `man_made=surveillance` SHALL NOT be importable as that type

#### Scenario: Manhole type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_manhole`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=manhole` SHALL be importable as that type

#### Scenario: Mast type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_mast`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=mast` SHALL be importable as that type

#### Scenario: Utility pole type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_utility_pole`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=utility_pole` SHALL be importable as that type

#### Scenario: Survey point type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_survey_point`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=survey_point` SHALL be importable as that type

#### Scenario: Petroleum well type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_petroleum_well`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=petroleum_well` SHALL be importable as that type

#### Scenario: Flagpole type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_flagpole`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=flagpole` SHALL be importable as that type

#### Scenario: Chimney type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_chimney`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=chimney` SHALL be importable as that type

#### Scenario: Beehive type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_beehive`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=beehive` SHALL be importable as that type

#### Scenario: Charge point type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_charge_point`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=charge_point` SHALL be importable as that type

#### Scenario: Cross type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_cross`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=cross` SHALL be importable as that type

#### Scenario: Adit type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_adit`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=adit` SHALL be importable as that type

#### Scenario: Lighthouse type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_lighthouse`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=lighthouse` SHALL be importable as that type

#### Scenario: Snow cannon type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_snow_cannon`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=snow_cannon` SHALL be importable as that type

#### Scenario: Flare type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_flare`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=flare` SHALL be importable as that type

#### Scenario: Beacon type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_beacon`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=beacon` SHALL be importable as that type

#### Scenario: Telephone box type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_telephone_box`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=telephone_box` SHALL be importable as that type

#### Scenario: Oil gas separator type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_oil_gas_separator`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=oil_gas_separator` SHALL be importable as that type

#### Scenario: Buoy type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_buoy`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=buoy` SHALL be importable as that type

#### Scenario: Fuel pump type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_fuel_pump`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=fuel_pump` SHALL be importable as that type

#### Scenario: Carpet hanger type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_carpet_hanger`
- **THEN** the type SHALL exist
- **AND** nodes tagged `man_made=carpet_hanger` SHALL be importable as that type

### Requirement: Area-only man made types

The import-time stylesheet SHALL define feature types for the following area-only `man_made=*` values:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `man_made=clearcut` | `man_made_clearcut` | area |
| `man_made=tunnel` | `man_made_tunnel` | area |
| `man_made=spoil_heap` | `man_made_spoil_heap` | area |
| `man_made=tailings_pond` | `man_made_tailings_pond` | area |

#### Scenario: Clearcut type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_clearcut`
- **THEN** the type SHALL exist
- **AND** areas tagged `man_made=clearcut` SHALL be importable as that type
- **AND** nodes tagged `man_made=clearcut` SHALL NOT be importable as that type

#### Scenario: Tunnel type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_tunnel`
- **THEN** the type SHALL exist
- **AND** areas tagged `man_made=tunnel` SHALL be importable as that type

#### Scenario: Spoil heap type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_spoil_heap`
- **THEN** the type SHALL exist
- **AND** areas tagged `man_made=spoil_heap` SHALL be importable as that type

#### Scenario: Tailings pond type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_tailings_pond`
- **THEN** the type SHALL exist
- **AND** areas tagged `man_made=tailings_pond` SHALL be importable as that type

### Requirement: Node and area man made types

The import-time stylesheet SHALL define feature types for the following `man_made=*` values usable on both nodes and areas:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `man_made=storage_tank` | `man_made_storage_tank` | node, area |
| `man_made=tower` | `man_made_tower` | node, area |
| `man_made=water_well` | `man_made_water_well` | node, area |
| `man_made=silo` | `man_made_silo` | node, area |
| `man_made=street_cabinet` | `man_made_street_cabinet` | node, area |
| `man_made=works` | `man_made_works` | node, area |
| `man_made=water_tower` | `man_made_water_tower` | node, area |
| `man_made=water_tap` | `man_made_water_tap` | node, area |
| `man_made=bunker_silo` | `man_made_bunker_silo` | node, area |
| `man_made=monitoring_station` | `man_made_monitoring_station` | node, area |
| `man_made=reservoir_covered` | `man_made_reservoir_covered` | node, area |
| `man_made=heap` | `man_made_heap` | node, area |
| `man_made=planter` | `man_made_planter` | node, area |
| `man_made=water_works` | `man_made_water_works` | node, area |
| `man_made=pumping_station` | `man_made_pumping_station` | node, area |
| `man_made=kiln` | `man_made_kiln` | node, area |
| `man_made=courtyard` | `man_made_courtyard` | node, area |
| `man_made=crane` | `man_made_crane` | node, area |
| `man_made=gasometer` | `man_made_gasometer` | node, area |
| `man_made=mineshaft` | `man_made_mineshaft` | node, area |
| `man_made=watermill` | `man_made_watermill` | node, area |
| `man_made=cairn` | `man_made_cairn` | node, area |
| `man_made=windmill` | `man_made_windmill` | node, area |
| `man_made=windpump` | `man_made_windpump` | node, area |
| `man_made=clarifier` | `man_made_clarifier` | node, area |
| `man_made=dolphin` | `man_made_dolphin` | node, area |
| `man_made=satellite_dish` | `man_made_satellite_dish` | node, area |
| `man_made=stupa` | `man_made_stupa` | node, area |
| `man_made=communications_tower` | `man_made_communications_tower` | node, area |
| `man_made=bioswale` | `man_made_bioswale` | node, area |
| `man_made=column` | `man_made_column` | node, area |
| `man_made=telescope` | `man_made_telescope` | node, area |

#### Scenario: Storage tank type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_storage_tank`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=storage_tank` SHALL be importable as that type

#### Scenario: Tower type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_tower`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=tower` SHALL be importable as that type

#### Scenario: Water well type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_water_well`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=water_well` SHALL be importable as that type

#### Scenario: Silo type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_silo`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=silo` SHALL be importable as that type

#### Scenario: Street cabinet type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_street_cabinet`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=street_cabinet` SHALL be importable as that type

#### Scenario: Works type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_works`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=works` SHALL be importable as that type

#### Scenario: Water tower type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_water_tower`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=water_tower` SHALL be importable as that type

#### Scenario: Water tap type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_water_tap`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=water_tap` SHALL be importable as that type

#### Scenario: Bunker silo type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_bunker_silo`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=bunker_silo` SHALL be importable as that type

#### Scenario: Monitoring station type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_monitoring_station`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=monitoring_station` SHALL be importable as that type

#### Scenario: Covered reservoir type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_reservoir_covered`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=reservoir_covered` SHALL be importable as that type

#### Scenario: Heap type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_heap`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=heap` SHALL be importable as that type

#### Scenario: Planter type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_planter`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=planter` SHALL be importable as that type

#### Scenario: Water works type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_water_works`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=water_works` SHALL be importable as that type

#### Scenario: Pumping station type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_pumping_station`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=pumping_station` SHALL be importable as that type

#### Scenario: Kiln type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_kiln`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=kiln` SHALL be importable as that type

#### Scenario: Courtyard type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_courtyard`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=courtyard` SHALL be importable as that type

#### Scenario: Crane type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_crane`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=crane` SHALL be importable as that type

#### Scenario: Gasometer type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_gasometer`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=gasometer` SHALL be importable as that type

#### Scenario: Mineshaft type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_mineshaft`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=mineshaft` SHALL be importable as that type

#### Scenario: Watermill type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_watermill`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=watermill` SHALL be importable as that type

#### Scenario: Cairn type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_cairn`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=cairn` SHALL be importable as that type

#### Scenario: Windmill type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_windmill`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=windmill` SHALL be importable as that type

#### Scenario: Windpump type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_windpump`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=windpump` SHALL be importable as that type

#### Scenario: Clarifier type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_clarifier`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=clarifier` SHALL be importable as that type

#### Scenario: Dolphin type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_dolphin`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=dolphin` SHALL be importable as that type

#### Scenario: Satellite dish type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_satellite_dish`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=satellite_dish` SHALL be importable as that type

#### Scenario: Stupa type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_stupa`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=stupa` SHALL be importable as that type

#### Scenario: Communications tower type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_communications_tower`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=communications_tower` SHALL be importable as that type

#### Scenario: Bioswale type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_bioswale`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=bioswale` SHALL be importable as that type

#### Scenario: Column type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_column`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=column` SHALL be importable as that type

#### Scenario: Telescope type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_telescope`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `man_made=telescope` SHALL be importable as that type

### Requirement: Way and area man made types

The import-time stylesheet SHALL define feature types for the following `man_made=*` values usable on both ways and areas:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `man_made=breakwater` | `man_made_breakwater` | way, area |
| `man_made=groyne` | `man_made_groyne` | way, area |
| `man_made=dyke` | `man_made_dyke` | way, area |
| `man_made=quay` | `man_made_quay` | way, area |

#### Scenario: Breakwater type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_breakwater`
- **THEN** the type SHALL exist
- **AND** ways and areas tagged `man_made=breakwater` SHALL be importable as that type

#### Scenario: Groyne type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_groyne`
- **THEN** the type SHALL exist
- **AND** ways and areas tagged `man_made=groyne` SHALL be importable as that type

#### Scenario: Dyke type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_dyke`
- **THEN** the type SHALL exist
- **AND** ways and areas tagged `man_made=dyke` SHALL be importable as that type

#### Scenario: Quay type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_quay`
- **THEN** the type SHALL exist
- **AND** ways and areas tagged `man_made=quay` SHALL be importable as that type

### Requirement: Node and way man made types

The import-time stylesheet SHALL define feature types for the following `man_made=*` values usable on both nodes and ways:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `man_made=advertising` | `man_made_advertising` | node, way |
| `man_made=gantry` | `man_made_gantry` | node, way |

#### Scenario: Advertising type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_advertising`
- **THEN** the type SHALL exist
- **AND** nodes and ways tagged `man_made=advertising` SHALL be importable as that type

#### Scenario: Gantry type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_gantry`
- **THEN** the type SHALL exist
- **AND** nodes and ways tagged `man_made=gantry` SHALL be importable as that type

### Requirement: Node, way and area man made types

The import-time stylesheet SHALL define feature types for the following `man_made=*` values usable on nodes, ways, and areas:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `man_made=antenna` | `man_made_antenna` | node, way, area |
| `man_made=avalanche_protection` | `man_made_avalanche_protection` | node, way, area |
| `man_made=ceremonial_gate` | `man_made_ceremonial_gate` | node, way, area |
| `man_made=geoglyph` | `man_made_geoglyph` | node, way, area |

#### Scenario: Antenna type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_antenna`
- **THEN** the type SHALL exist
- **AND** nodes, ways, and areas tagged `man_made=antenna` SHALL be importable as that type

#### Scenario: Avalanche protection type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_avalanche_protection`
- **THEN** the type SHALL exist
- **AND** nodes, ways, and areas tagged `man_made=avalanche_protection` SHALL be importable as that type

#### Scenario: Ceremonial gate type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_ceremonial_gate`
- **THEN** the type SHALL exist
- **AND** nodes, ways, and areas tagged `man_made=ceremonial_gate` SHALL be importable as that type

#### Scenario: Geoglyph type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `man_made_geoglyph`
- **THEN** the type SHALL exist
- **AND** nodes, ways, and areas tagged `man_made=geoglyph` SHALL be importable as that type

### Requirement: Discouraged and undocumented values are excluded

The import-time stylesheet SHALL NOT define types for discouraged values (`man_made=yes`, `man_made=lamp`, `man_made=pumping_rig`, `man_made=launch_pad`) or undocumented values (`man_made=pond`, `man_made=tar_kiln`, `man_made=marsh_terrace`, `man_made=waterway`, `man_made=pole`).

#### Scenario: Discouraged values are excluded
- **WHEN** a stylesheet consumer looks up the type for tag `man_made=yes`
- **THEN** no type definition exists for it

#### Scenario: Undocumented values are excluded
- **WHEN** a stylesheet consumer looks up the type for tag `man_made=pond`
- **THEN** no type definition exists for it

### Requirement: Style definitions for new man made types

Each new man made type SHALL have a corresponding style definition in `include/man_made.oss` where visualization is obvious, reusing existing type/style definitions for similar types (e.g. `man_made_pier`, `man_made_wastewater_plant`, `waterway_dam`, `power_tower`, `power_pole`, `barrier_planter`, `barrier_avalanche_protection`) as templates.

#### Scenario: Node structure has a style
- **WHEN** a map is rendered with a `man_made=tower` node
- **THEN** the node is drawn with a style derived from the existing tower icon patterns

#### Scenario: Way structure has a style
- **WHEN** a map is rendered with a `man_made=cutline` way
- **THEN** the way is drawn with a style derived from the existing linear feature patterns

#### Scenario: Area facility has a style
- **WHEN** a map is rendered with a `man_made=works` area
- **THEN** the area is drawn with a style derived from the existing `man_made_wastewater_plant` area pattern

### Requirement: Existing man made types and styles remain unchanged

The existing type definitions for `man_made_bridge`, `man_made_pier`, and `man_made_wastewater_plant` SHALL keep their current names and behavior; the change SHALL only add new definitions.

#### Scenario: Existing types still resolve
- **WHEN** a stylesheet consumer loads the type for `man_made=pier`
- **THEN** the existing `man_made_pier` type definition is still present and unchanged
