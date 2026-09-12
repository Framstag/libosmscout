# place-type-definitions Specification

## Purpose
Defines the import-time OSM feature types for documented `place=*` values missing from `stylesheets/map.ost`, so these features exist in the database `TypeConfig` and are importable and renderable. Element types follow the OSM wiki [Key:place](https://wiki.openstreetmap.org/wiki/Key:place) element table and the individual tag pages. Only values that are documented, have relevant usage per taginfo, and are not discouraged/deprecated/proposals are added. Corresponding label style definitions are provided in the place style module.

## Requirements

### Requirement: Administrative place types

The import-time stylesheet SHALL define feature types for the following administratively declared `place=*` values:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `place=province` | `place_province` | node |
| `place=district` | `place_district` | node |
| `place=subdistrict` | `place_subdistrict` | node |
| `place=municipality` | `place_municipality` | node, relation |

#### Scenario: Administrative place types exist in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `place_province`, `place_district`, `place_subdistrict`, and `place_municipality`
- **THEN** all four types SHALL exist
- **AND** nodes tagged `place=province`, `place=district`, `place=subdistrict` SHALL be importable as the respective types
- **AND** relations tagged `place=municipality` SHALL be importable as `place_municipality`

### Requirement: Urban settlement place types

The import-time stylesheet SHALL define feature types for the following urban populated settlement `place=*` values:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `place=borough` | `place_borough` | node, area, relation |
| `place=quarter` | `place_quarter` | node, area, relation |
| `place=neighbourhood` | `place_neighbourhood` | node, area, relation |
| `place=city_block` | `place_city_block` | node, area |
| `place=plot` | `place_plot` | node, area, relation |

#### Scenario: Urban settlement place types exist in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `place_borough`, `place_quarter`, `place_neighbourhood`, `place_city_block`, and `place_plot`
- **THEN** all five types SHALL exist
- **AND** nodes and areas tagged with the respective `place=*` values SHALL be importable as the respective types

### Requirement: Rural settlement place types

The import-time stylesheet SHALL define feature types for the following rural populated settlement `place=*` values:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `place=isolated_dwelling` | `place_isolated_dwelling` | node, area, relation |
| `place=farm` | `place_farm` | node, area |
| `place=allotments` | `place_allotments` | node, area |

#### Scenario: Rural settlement place types exist in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `place_isolated_dwelling`, `place_farm`, and `place_allotments`
- **THEN** all three types SHALL exist
- **AND** nodes and areas tagged with the respective `place=*` values SHALL be importable as the respective types

### Requirement: Other named place types

The import-time stylesheet SHALL define feature types for the following other named `place=*` values:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `place=square` | `place_square` | node, area |
| `place=archipelago` | `place_archipelago` | area, relation |
| `place=polder` | `place_polder` | node, area |
| `place=sea` | `place_sea` | node, area |
| `place=ocean` | `place_ocean` | node |

#### Scenario: Other named place types exist in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `place_square`, `place_archipelago`, `place_polder`, `place_sea`, and `place_ocean`
- **THEN** all five types SHALL exist
- **AND** nodes tagged `place=ocean` SHALL be importable as `place_ocean`
- **AND** relations tagged `place=archipelago` SHALL be importable as `place_archipelago`

### Requirement: Remaining documented place types

The import-time stylesheet SHALL define feature types for the following documented `place=*` values that are still missing:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `place=cadastral_community` | `place_cadastral_community` | node, area |
| `place=subcounty` | `place_subcounty` | node, area, relation |
| `place=building_complex` | `place_building_complex` | node, area |

#### Scenario: Cadastral community type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `place_cadastral_community`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `place=cadastral_community` SHALL be importable as that type

#### Scenario: Subcounty type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `place_subcounty`
- **THEN** the type SHALL exist
- **AND** nodes, areas, and relations tagged `place=subcounty` SHALL be importable as that type

#### Scenario: Building complex type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `place_building_complex`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `place=building_complex` SHALL be importable as that type

### Requirement: Label rendering for new place types

The place style module SHALL render labels for all new place types: each type SHALL have a NODE.TEXT and/or AREA.TEXT rule at magnification levels appropriate to the type (administrative types at region/county level, settlement types at suburb/city level, small named places at close level).

#### Scenario: New place types have label rules
- **GIVEN** the stylesheet `stylesheets/standard.oss` with the place style module
- **WHEN** the style configuration is analyzed for the new place types
- **THEN** every new place type SHALL have at least one label rule
- **AND** the `OSTAndOSSTest --analyze` output SHALL NOT list any new place type as "without style"

### Requirement: Discouraged and undocumented place values are not defined

The import-time stylesheet SHALL NOT define types for discouraged, deprecated, proposal, or undocumented `place=*` values, including `place=yes`, `place=civil_parish` (draft proposal), and undocumented values such as `subdivision`, `block`, `field`, `department`, `zone`, `township`, `ward`, `community`, `camp`.

#### Scenario: Discouraged place values are not defined
- **GIVEN** the import-time stylesheet `stylesheets/map.ost`
- **WHEN** the stylesheet is searched for `place_yes`, `place_civil_parish`, `place_subdivision`, `place_block`, `place_field`, `place_department`, `place_zone`, `place_township`, `place_ward`, `place_community`, and `place_camp`
- **THEN** none of these types SHALL be defined
