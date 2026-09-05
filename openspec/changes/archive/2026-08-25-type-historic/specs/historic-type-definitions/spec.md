# historic-type-definitions Specification

## Purpose

Defines the import-time OSM feature types for documented `historic=*` values missing from `stylesheets/map.ost`, so these features exist in the database `TypeConfig` and are importable, searchable, and renderable. Element types follow the OSM wiki [Key:historic](https://wiki.openstreetmap.org/wiki/Key:historic) element table. Only values that are documented, have relevant usage (>= 150 objects per taginfo), and are not discouraged/deprecated are added.

## ADDED Requirements

### Requirement: Node-only historic types

The import-time stylesheet SHALL define feature types for the following node-only `historic=*` values, with element type NODE as specified:

| OSM tag | Type name |
|---------|-----------|
| `historic=anchor` | `historic_anchor` |
| `historic=boundary_stone` | `historic_boundary_stone` |
| `historic=bullaun_stone` | `historic_bullaun_stone` |
| `historic=cannon` | `historic_cannon` |
| `historic=high_cross` | `historic_high_cross` |
| `historic=highwater_mark` | `historic_highwater_mark` |
| `historic=milestone` | `historic_milestone` |
| `historic=millstone` | `historic_millstone` |
| `historic=ogham_stone` | `historic_ogham_stone` |
| `historic=pillory` | `historic_pillory` |
| `historic=rune_stone` | `historic_rune_stone` |
| `historic=stone` | `historic_stone` |
| `historic=tree_shrine` | `historic_tree_shrine` |
| `historic=wayside_cross` | `historic_wayside_cross` |

#### Scenario: Anchor type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `historic_anchor`
- **THEN** the type SHALL exist
- **AND** nodes tagged `historic=anchor` SHALL be importable as that type
- **AND** ways and areas tagged `historic=anchor` SHALL NOT be importable as that type

#### Scenario: Milestone type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `historic_milestone`
- **THEN** the type SHALL exist
- **AND** nodes tagged `historic=milestone` SHALL be importable as that type

#### Scenario: Wayside cross type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `historic_wayside_cross`
- **THEN** the type SHALL exist
- **AND** nodes tagged `historic=wayside_cross` SHALL be importable as that type

### Requirement: Node-area historic types

The import-time stylesheet SHALL define feature types for the following `historic=*` values accepting NODE and AREA elements, as specified:

| OSM tag | Type name |
|---------|-----------|
| `historic=aircraft` | `historic_aircraft` |
| `historic=bomb_crater` | `historic_bomb_crater` |
| `historic=caravanserai` | `historic_caravanserai` |
| `historic=cattle_crush` | `historic_cattle_crush` |
| `historic=cemetery` | `historic_cemetery` |
| `historic=chapel` | `historic_chapel` |
| `historic=charcoal_pile` | `historic_charcoal_pile` |
| `historic=church` | `historic_church` |
| `historic=city_gate` | `historic_city_gate` |
| `historic=creamery` | `historic_creamery` |
| `historic=district` | `historic_district` |
| `historic=farm` | `historic_farm` |
| `historic=fort` | `historic_fort` |
| `historic=gallows` | `historic_gallows` |
| `historic=granary` | `historic_granary` |
| `historic=house` | `historic_house` |
| `historic=lavoir` | `historic_lavoir` |
| `historic=lime_kiln` | `historic_lime_kiln` |
| `historic=locomotive` | `historic_locomotive` |
| `historic=machine` | `historic_machine` |
| `historic=mine` | `historic_mine` |
| `historic=mine_adit` | `historic_mine_adit` |
| `historic=mine_shaft` | `historic_mine_shaft` |
| `historic=minecart` | `historic_minecart` |
| `historic=monastery` | `historic_monastery` |
| `historic=mosque` | `historic_mosque` |
| `historic=optical_telegraph` | `historic_optical_telegraph` |
| `historic=pound` | `historic_pound` |
| `historic=railway_car` | `historic_railway_car` |
| `historic=railway_station` | `historic_railway_station` |
| `historic=round_tower` | `historic_round_tower` |
| `historic=shieling` | `historic_shieling` |
| `historic=ship` | `historic_ship` |
| `historic=smithy` | `historic_smithy` |
| `historic=stećak` | `historic_stecak` |
| `historic=tank` | `historic_tank` |
| `historic=temple` | `historic_temple` |
| `historic=tomb` | `historic_tomb` |
| `historic=tower` | `historic_tower` |
| `historic=vehicle` | `historic_vehicle` |
| `historic=warehouse` | `historic_warehouse` |
| `historic=watermill` | `historic_watermill` |

#### Scenario: Church type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `historic_church`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `historic=church` SHALL be importable as that type

#### Scenario: Mine type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `historic_mine`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `historic=mine` SHALL be importable as that type

#### Scenario: Stecak type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `historic_stecak`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `historic=stećak` SHALL be importable as that type

#### Scenario: Tower type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `historic_tower`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `historic=tower` SHALL be importable as that type

### Requirement: Way and way-area historic types

The import-time stylesheet SHALL define feature types for the following `historic=*` values involving WAY elements, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `historic=aqueduct` | `historic_aqueduct` | way, area |
| `historic=castle_wall` | `historic_castle_wall` | way, area |
| `historic=hollow_way` | `historic_hollow_way` | way |
| `historic=road` | `historic_road` | way |
| `historic=roman_road` | `historic_roman_road` | way |
| `historic=railway` | `historic_railway` | node, way |
| `historic=epigraph` | `historic_epigraph` | node, way, area |
| `historic=folly` | `historic_folly` | node, way, area |
| `historic=wayside_shrine` | `historic_wayside_shrine` | node, way, area |

#### Scenario: Aqueduct type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `historic_aqueduct`
- **THEN** the type SHALL exist
- **AND** ways and areas tagged `historic=aqueduct` SHALL be importable as that type
- **AND** nodes tagged `historic=aqueduct` SHALL NOT be importable as that type

#### Scenario: Hollow way type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `historic_hollow_way`
- **THEN** the type SHALL exist
- **AND** ways tagged `historic=hollow_way` SHALL be importable as that type
- **AND** nodes and areas tagged `historic=hollow_way` SHALL NOT be importable as that type

#### Scenario: Railway type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `historic_railway`
- **THEN** the type SHALL exist
- **AND** nodes and ways tagged `historic=railway` SHALL be importable as that type

#### Scenario: Wayside shrine type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `historic_wayside_shrine`
- **THEN** the type SHALL exist
- **AND** nodes, ways, and areas tagged `historic=wayside_shrine` SHALL be importable as that type

### Requirement: Building variant types

The import-time stylesheet SHALL define AREA-only `_building` variant types for building-like `historic=*` values, matching objects tagged with both `historic=*` and a positive `building=*` tag, so such areas render as buildings rather than as the base node/area type:

| OSM tag | Type name |
|---------|-----------|
| `historic=caravanserai` | `historic_caravanserai_building` |
| `historic=cattle_crush` | `historic_cattle_crush_building` |
| `historic=chapel` | `historic_chapel_building` |
| `historic=church` | `historic_church_building` |
| `historic=creamery` | `historic_creamery_building` |
| `historic=farm` | `historic_farm_building` |
| `historic=folly` | `historic_folly_building` |
| `historic=granary` | `historic_granary_building` |
| `historic=house` | `historic_house_building` |
| `historic=lavoir` | `historic_lavoir_building` |
| `historic=monastery` | `historic_monastery_building` |
| `historic=mosque` | `historic_mosque_building` |
| `historic=optical_telegraph` | `historic_optical_telegraph_building` |
| `historic=railway_station` | `historic_railway_station_building` |
| `historic=round_tower` | `historic_round_tower_building` |
| `historic=shieling` | `historic_shieling_building` |
| `historic=smithy` | `historic_smithy_building` |
| `historic=temple` | `historic_temple_building` |
| `historic=tower` | `historic_tower_building` |
| `historic=warehouse` | `historic_warehouse_building` |
| `historic=watermill` | `historic_watermill_building` |

#### Scenario: Church building variant exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `historic_church_building`
- **THEN** the type SHALL exist
- **AND** areas tagged `historic=church` with a positive `building=*` tag SHALL be importable as that type
- **AND** areas tagged `historic=church` without a `building=*` tag SHALL be importable as `historic_church`

#### Scenario: Tower building variant exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `historic_tower_building`
- **THEN** the type SHALL exist
- **AND** areas tagged `historic=tower` with a positive `building=*` tag SHALL be importable as that type

### Requirement: Discouraged and undocumented historic values

The import-time stylesheet SHALL NOT define dedicated feature types for discouraged, deprecated, or undocumented `historic=*` values, including but not limited to `historic=heritage`, `historic=wayside_chapel`, `historic=coat_of_arms`, and values with taginfo usage below 150 objects. Such objects remain covered by the generic `historic` catch-all type.

#### Scenario: Discouraged values have no dedicated type
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `historic_heritage`, `historic_wayside_chapel`, or `historic_coat_of_arms`
- **THEN** none of these types SHALL exist
- **AND** objects carrying the corresponding tags SHALL still be importable via the generic `historic` type

### Requirement: Area rendering for new historic types

The rendering stylesheet SHALL give areas of the new historic types a fill matching the existing historic area color at detail zoom, SHALL render labels for the new types at close zoom following the existing historic label rules, and SHALL render way-only and way-capable types (`historic_aqueduct`, `historic_castle_wall`, `historic_epigraph`, `historic_folly`, `historic_hollow_way`, `historic_railway`, `historic_road`, `historic_roman_road`, `historic_wayside_shrine`) with a dashed line at close zoom.

#### Scenario: New historic areas get fill at detail zoom
- **GIVEN** a map rendered with the stylesheet at detail zoom
- **WHEN** an area of type `historic_church`, `historic_mine`, or `historic_tower` is rendered
- **THEN** the area SHALL be filled with the historic area color

#### Scenario: New historic areas get labels at close zoom
- **GIVEN** a map rendered with the stylesheet at close zoom
- **WHEN** an area of type `historic_church`, `historic_mine`, or `historic_tower` is rendered
- **THEN** the area SHALL be labeled with its name following the historic label rules

#### Scenario: Way types get dashed rendering
- **GIVEN** a map rendered with the stylesheet at close zoom
- **WHEN** a way of type `historic_roman_road` or `historic_castle_wall` is rendered
- **THEN** the way SHALL be rendered with a dashed line

### Requirement: Node icon rendering for new historic types

The rendering stylesheet SHALL define a dedicated vector symbol for each new historic type that accepts NODE elements where an obvious pictogram can be drawn, and SHALL render nodes of those types with the corresponding symbol icon at very close zoom. Types without a dedicated symbol SHALL fall back to the generic `historic` symbol. The generic `historic` symbol SHALL remain the fallback for all historic types.

#### Scenario: Castle node gets castle icon
- **GIVEN** a map rendered with the stylesheet at very close zoom
- **WHEN** a node of type `historic_castle` is rendered
- **THEN** the node SHALL be rendered with the `historic_castle` symbol icon

#### Scenario: Church node gets church icon
- **GIVEN** a map rendered with the stylesheet at very close zoom
- **WHEN** a node of type `historic_church` is rendered
- **THEN** the node SHALL be rendered with the `historic_church` symbol icon

#### Scenario: Anchor node gets anchor icon
- **GIVEN** a map rendered with the stylesheet at very close zoom
- **WHEN** a node of type `historic_anchor` is rendered
- **THEN** the node SHALL be rendered with the `historic_anchor` symbol icon

#### Scenario: District node falls back to generic icon
- **GIVEN** a map rendered with the stylesheet at very close zoom
- **WHEN** a node of type `historic_district` is rendered
- **THEN** the node SHALL be rendered with the generic `historic` symbol icon
