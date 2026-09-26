# office-type-definitions Specification

## Purpose

Defines the import-time OSM feature types for documented `office=*` values missing from `stylesheets/map.ost`, so these features exist in the database `TypeConfig` and are importable, searchable, and renderable. Element types follow the OSM wiki [Key:office](https://wiki.openstreetmap.org/wiki/Key:office) element table. Only values that are documented, have relevant usage (>=0.05% per taginfo), and are not discouraged/deprecated are added.

## Requirements

### Requirement: Office types with relevant usage

The import-time stylesheet SHALL define feature types for the following `office=*` values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `office=accountant` | `office_accountant` | node, area |
| `office=advertising_agency` | `office_advertising_agency` | node, area |
| `office=architect` | `office_architect` | node, area |
| `office=association` | `office_association` | node, area |
| `office=charity` | `office_charity` | node, area |
| `office=company` | `office_company` | node, area |
| `office=construction_company` | `office_construction_company` | node, area |
| `office=consulting` | `office_consulting` | node, area |
| `office=cooperative` | `office_cooperative` | node, area |
| `office=courier` | `office_courier` | node, area |
| `office=coworking` | `office_coworking` | node, area |
| `office=diplomatic` | `office_diplomatic` | node, area |
| `office=educational_institution` | `office_educational_institution` | node, area |
| `office=employment_agency` | `office_employment_agency` | node, area |
| `office=energy_supplier` | `office_energy_supplier` | node, area |
| `office=engineer` | `office_engineer` | node, area |
| `office=estate_agent` | `office_estate_agent` | node, area |
| `office=financial` | `office_financial` | node, area |
| `office=financial_advisor` | `office_financial_advisor` | node, area |
| `office=forestry` | `office_forestry` | node, area |
| `office=foundation` | `office_foundation` | node, area |
| `office=government` | `office_government` | node, area |
| `office=graphic_design` | `office_graphic_design` | node, area |
| `office=guide` | `office_guide` | node, area |
| `office=insurance` | `office_insurance` | node, area |
| `office=it` | `office_it` | node, area |
| `office=lawyer` | `office_lawyer` | node, area |
| `office=logistics` | `office_logistics` | node, area |
| `office=moving_company` | `office_moving_company` | node, area |
| `office=newspaper` | `office_newspaper` | node, area |
| `office=ngo` | `office_ngo` | node, area |
| `office=notary` | `office_notary` | node, area |
| `office=physician` | `office_physician` | node, area |
| `office=political_party` | `office_political_party` | node, area |
| `office=property_management` | `office_property_management` | node, area |
| `office=publisher` | `office_publisher` | node, area |
| `office=quango` | `office_quango` | node, area |
| `office=religion` | `office_religion` | node, area |
| `office=research` | `office_research` | node, area |
| `office=security` | `office_security` | node, area |
| `office=surveyor` | `office_surveyor` | node, area |
| `office=tax_advisor` | `office_tax_advisor` | node, area |
| `office=telecommunication` | `office_telecommunication` | node, area |
| `office=therapist` | `office_therapist` | node, area |
| `office=translator` | `office_translator` | node, area |
| `office=transport` | `office_transport` | node, area |
| `office=travel_agent` | `office_travel_agent` | node, area |
| `office=union` | `office_union` | node, area |
| `office=university` | `office_university` | node, area |
| `office=water_utility` | `office_water_utility` | node, area |

For each value, the stylesheet SHALL additionally define an area-only `_building` variant matching objects that also carry a `building=*` tag other than `no`/`false`/`0`, following the convention of existing shop/amenity types (e.g. `shop_alcohol_building`). The `_building` variant SHALL be defined before its generic counterpart so that first-match type resolution assigns building areas to the `_building` type.

#### Scenario: Government office type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `office_government`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `office=government` SHALL be importable as that type
- **AND** areas additionally tagged with a `building=*` value other than `no`/`false`/`0` SHALL be importable as `office_government_building`

#### Scenario: Company office type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `office_company`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `office=company` SHALL be importable as that type
- **AND** areas additionally tagged with a `building=*` value other than `no`/`false`/`0` SHALL be importable as `office_company_building`

#### Scenario: All listed office types exist in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for each type listed in the table above
- **THEN** every listed type SHALL exist
- **AND** every listed type SHALL support node and area elements
- **AND** every `_building` variant SHALL support area elements only

### Requirement: Discouraged and undocumented office values

The import-time stylesheet SHALL NOT define dedicated feature types for discouraged, deprecated, or undocumented `office=*` values, including but not limited to `office=administrative` (deprecated, use `office=government`), `office=camping`, `office=parish` (use `office=religion`), `office=medical` (use more specific tags), `office=vacant`, and `office=taxi`. Such objects remain covered by the generic `office` catch-all type. The generic `office=yes` value SHALL NOT get a dedicated type either, as it is the catch-all itself.

#### Scenario: Discouraged values have no dedicated type
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `office_administrative`, `office_camping`, `office_parish`, `office_medical`, `office_vacant`, `office_taxi`, or `office_yes`
- **THEN** none of these types SHALL exist
- **AND** objects carrying the corresponding tags SHALL still be importable via the generic `office` type

### Requirement: Rendering of new office types

The rendering stylesheet SHALL render the new office types through the existing office rules: `_building` variants SHALL get the office building fill, border, and label via the `GROUP office, building` rules in `stylesheets/include/office.oss`; base types SHALL get the office node label and icon via the `GROUP office` rules.

The rendering stylesheet SHALL define distinct symbols for the most important office types (by taginfo usage) and SHALL assign them via `NODE.ICON`/`AREA.ICON` rules at closer zoom, overriding the generic office icon. The following types SHALL have a distinct symbol: government, estate_agent, insurance, lawyer, educational_institution, telecommunication, it, accountant, diplomatic, employment_agency, research, architect, tax_advisor, financial, logistics, advertising_agency, notary, energy_supplier, security, newspaper, water_utility, construction_company, forestry, charity, physician, publisher, translator, courier, travel_agent. `office_university` SHALL reuse the `office_educational_institution` symbol. All other office types SHALL keep the generic `office` symbol.

#### Scenario: Building variants use office building rendering
- **GIVEN** a map rendered with the stylesheet
- **WHEN** an area of type `office_government_building` or `office_company_building` is rendered
- **THEN** the area SHALL be rendered with the office building fill, border, and label rules

#### Scenario: Base types use office node rendering
- **GIVEN** a map rendered with the stylesheet
- **WHEN** a node of type `office_government` or `office_company` is rendered at close zoom
- **THEN** the node SHALL be rendered with the office label and office icon

#### Scenario: Important office types have distinct symbols
- **GIVEN** a map rendered with the stylesheet at closer zoom
- **WHEN** a node of type `office_government`, `office_lawyer`, or `office_insurance` is rendered
- **THEN** the node SHALL be rendered with the distinct symbol for that type (`office_government`, `office_lawyer`, `office_insurance`)

#### Scenario: University reuses education symbol
- **GIVEN** a map rendered with the stylesheet at closer zoom
- **WHEN** a node of type `office_university` is rendered
- **THEN** the node SHALL be rendered with the `office_educational_institution` symbol

#### Scenario: Remaining office types keep generic symbol
- **GIVEN** a map rendered with the stylesheet at closer zoom
- **WHEN** a node of type `office_company` or `office_ngo` is rendered
- **THEN** the node SHALL be rendered with the generic `office` symbol
