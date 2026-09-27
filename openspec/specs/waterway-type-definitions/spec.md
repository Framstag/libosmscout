# waterway-type-definitions Specification

## Purpose

Defines the complete set of `waterway` type and style definitions in the libosmscout stylesheets so that all relevant `waterway`-tagged OSM objects (per the OSM wiki and taginfo usage) are recognized and rendered.

## Requirements

### Requirement: Waterway type definitions cover all relevant wiki values
The stylesheet type definitions SHALL include a type for every `waterway` value documented on the OSM wiki Key:waterway page that has at least 500 uses on taginfo and is not marked as discouraged or deprecated. The following values SHALL be added: `flowline`, `tidal_channel`, `drystream`, `pressurised`, `derelict_canal`, `drainage_channel`, `link`, `fairway`, `fish_pass`, `fuel`, `water_point`, `sanitary_dump_station`, `access_point`, `milestone`, `rapids`, `sluice_gate`, `floodgate`, `check_dam`, `floating_barrier`, `flow_control`, `stream_end`, `soakhole`.

#### Scenario: New waterway value has a type definition
- **WHEN** a stylesheet consumer looks up the type for tag `waterway=rapids`
- **THEN** a type definition exists for it in the waterway section of `map.ost`

#### Scenario: Discouraged values are excluded
- **WHEN** a stylesheet consumer looks up the type for tag `waterway=wadi`
- **THEN** no type definition exists for it

#### Scenario: Deprecated values are excluded
- **WHEN** a stylesheet consumer looks up the type for tag `waterway=brook`
- **THEN** no type definition exists for it

#### Scenario: Values not documented on the wiki are excluded
- **WHEN** a stylesheet consumer looks up the type for tag `waterway=yes`
- **THEN** no type definition exists for it

### Requirement: Object types match OSM wiki documentation
Each new waterway type SHALL be declared with the object types (NODE, WAY, AREA) documented on the OSM wiki for that value: `flowline` (WAY), `tidal_channel` (WAY), `drystream` (WAY), `pressurised` (WAY), `derelict_canal` (WAY), `drainage_channel` (WAY), `link` (WAY), `fairway` (WAY), `fish_pass` (WAY), `fuel` (NODE AREA), `water_point` (NODE), `sanitary_dump_station` (NODE AREA), `access_point` (NODE), `milestone` (NODE), `rapids` (NODE WAY AREA), `sluice_gate` (NODE WAY AREA), `floodgate` (NODE WAY AREA), `check_dam` (NODE WAY), `floating_barrier` (NODE WAY), `flow_control` (NODE WAY), `stream_end` (NODE), `soakhole` (NODE).

#### Scenario: Way-only value declared as way
- **WHEN** a stylesheet consumer loads the type for `waterway=flowline`
- **THEN** the type is declared for WAY objects only

#### Scenario: Node-and-area value declared for both
- **WHEN** a stylesheet consumer loads the type for `waterway=fuel`
- **THEN** the type is declared for both NODE and AREA objects

#### Scenario: Node-way-area value declared for all three
- **WHEN** a stylesheet consumer loads the type for `waterway=rapids`
- **THEN** the type is declared for NODE, WAY, and AREA objects

### Requirement: Style definitions for new waterway types
Each new waterway type SHALL have a corresponding style definition in `include/waterway.oss` where visualization is obvious, reusing existing type/style definitions for similar types (e.g. `waterway_stream`, `waterway_drain`, `waterway_weir`, `waterway_boatyard`) as templates.

#### Scenario: Linear waterway has a style
- **WHEN** a map is rendered with a `waterway=tidal_channel` way
- **THEN** the way is drawn with a style derived from the existing `waterway_stream` style

#### Scenario: Barrier has a style
- **WHEN** a map is rendered with a `waterway=sluice_gate` way
- **THEN** the way is drawn with a style derived from the existing `waterway_weir` style

#### Scenario: Facility node has a style
- **WHEN** a map is rendered with a `waterway=fuel` node
- **THEN** the node is drawn with a dedicated icon symbol

#### Scenario: Facility area has a style
- **WHEN** a map is rendered with a `waterway=sanitary_dump_station` area
- **THEN** the area is drawn with a style derived from the existing `waterway_boatyard` style

### Requirement: New waterway types participate in rendering priority groups
The new waterway types SHALL be added to the GROUP priority definitions in `standard.oss`, `cycle.oss`, `winter-sports.oss`, and `public-transport.oss` so they render at the same priority as the existing waterway types they resemble.

#### Scenario: Linear types share priority with existing waterways
- **WHEN** a stylesheet consumer loads the GROUP definitions of `standard.oss`
- **THEN** `waterway_flowline`, `waterway_tidal_channel`, `waterway_drystream`, `waterway_pressurised`, `waterway_derelict_canal`, `waterway_drainage_channel`, `waterway_link`, and `waterway_fairway` are in the same GROUP as `waterway_river` and `waterway_canal`

#### Scenario: Barrier types share priority with weir
- **WHEN** a stylesheet consumer loads the GROUP definitions of `standard.oss`
- **THEN** `waterway_rapids`, `waterway_sluice_gate`, `waterway_floodgate`, `waterway_check_dam`, `waterway_floating_barrier`, `waterway_flow_control`, and `waterway_fish_pass` are in the same GROUP as `waterway_weir`

### Requirement: Existing waterway types and styles remain unchanged
The existing type definitions for `waterway_stream`, `waterway_river`, `waterway_riverbank`, `waterway_canal`, `waterway_ditch`, `waterway_drain`, `waterway_dock`, `waterway_lock_gate`, `waterway_turning_point`, `waterway_boatyard`, `waterway_weir`, `waterway_waterfall`, and `waterway_dam` SHALL keep their current names and behavior; the change SHALL only add new definitions.

#### Scenario: Existing types still resolve
- **WHEN** a stylesheet consumer loads the type for `waterway=river`
- **THEN** the existing `waterway_river` type definition is still present and unchanged
