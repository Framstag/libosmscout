## Purpose

Defines the complete set of `power` type and style definitions in the libosmscout stylesheets so that all relevant `power`-tagged OSM objects (per the OSM wiki and taginfo usage) are recognized and rendered.

## ADDED Requirements

### Requirement: Power type definitions cover all relevant wiki values
The stylesheet type definitions SHALL include a type for every `power` value documented on the OSM wiki Key:power page that has at least 0.01% usage on taginfo and is not marked as discouraged. The following values SHALL be added: `catenary_mast`, `portal`, `transformer`, `cable`, `switch`, `plant`, `terminal`, `heliostat`, `insulator`, `circuit`, `connection`, `catenary_portal`, `compensator`, `inverter`, `switchgear`, `converter`.

#### Scenario: New power value has a type definition
- **WHEN** a stylesheet consumer looks up the type for tag `power=catenary_mast`
- **THEN** a type definition exists for it in the power section of `map.ost`

#### Scenario: Discouraged values are excluded
- **WHEN** a stylesheet consumer looks up the type for tag `power=cable_distribution_cabinet`
- **THEN** no type definition exists for it

### Requirement: Object types match OSM wiki documentation
Each new power type SHALL be declared with the object types (NODE, WAY, AREA) documented on the OSM wiki for that value: `catenary_mast` (NODE), `portal` (NODE), `transformer` (NODE), `cable` (WAY), `switch` (NODE), `plant` (NODE AREA), `terminal` (NODE), `heliostat` (NODE AREA), `insulator` (NODE), `circuit` (WAY), `connection` (NODE), `catenary_portal` (NODE), `compensator` (NODE AREA), `inverter` (NODE AREA), `switchgear` (NODE AREA), `converter` (NODE AREA).

#### Scenario: Way-only value declared as way
- **WHEN** a stylesheet consumer loads the type for `power=cable`
- **THEN** the type is declared for WAY objects only

#### Scenario: Node-and-area value declared for both
- **WHEN** a stylesheet consumer loads the type for `power=plant`
- **THEN** the type is declared for both NODE and AREA objects

### Requirement: Style definitions for new power types
Each new power type SHALL have a corresponding style definition in `include/power.oss` where visualization is obvious, reusing existing type/style definitions for similar types (e.g. `power_tower`, `power_pole`, `power_line`, `power_sub_station`, `power_generator`) as templates.

#### Scenario: Node support structure has a style
- **WHEN** a map is rendered with a `power=catenary_mast` node
- **THEN** the node is drawn with a style derived from the existing `power_tower`/`power_pole` styles

#### Scenario: Way cable has a style
- **WHEN** a map is rendered with a `power=cable` way
- **THEN** the way is drawn with a style derived from the existing `power_line`/`power_minor_line` styles

#### Scenario: Area facility has a style
- **WHEN** a map is rendered with a `power=plant` area
- **THEN** the area is drawn with a style derived from the existing `power_sub_station`/`power_generator` styles

### Requirement: Existing power types and styles remain unchanged
The existing type definitions for `power_tower`, `power_pole`, `power_line`, `power_minor_line`, `power_sub_station`, and `power_generator` SHALL keep their current names and behavior; the change SHALL only add new definitions and resolve the existing TODO comment for `power_plant`.

#### Scenario: Existing types still resolve
- **WHEN** a stylesheet consumer loads the type for `power=tower`
- **THEN** the existing `power_tower` type definition is still present and unchanged
