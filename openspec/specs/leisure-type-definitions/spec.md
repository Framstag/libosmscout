# leisure-type-definitions Specification

## Purpose

Defines the import-time OSM feature types for documented `leisure=*` values missing from `stylesheets/map.ost`, so these features exist in the database `TypeConfig` and are importable, searchable, and renderable. Element types follow the OSM wiki [Key:leisure](https://wiki.openstreetmap.org/wiki/Key:leisure) element table. Only values that are documented, have relevant usage (>= 0.01% per taginfo), and are not discouraged/deprecated are added.

## Requirements

### Requirement: Outdoor leisure area types

The import-time stylesheet SHALL define feature types for the following outdoor `leisure=*` values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `leisure=picnic_table` | `leisure_picnic_table` | node, area |
| `leisure=outdoor_seating` | `leisure_outdoor_seating` | node, area |
| `leisure=firepit` | `leisure_firepit` | node, area |
| `leisure=dog_park` | `leisure_dog_park` | node, area |
| `leisure=beach_resort` | `leisure_beach_resort` | node, area |
| `leisure=recreation_ground` | `leisure_recreation_ground` | node, area |
| `leisure=bathing_place` | `leisure_bathing_place` | node, area |
| `leisure=swimming_area` | `leisure_swimming_area` | area |
| `leisure=resort` | `leisure_resort` | node, area |
| `leisure=summer_camp` | `leisure_summer_camp` | node, area |
| `leisure=schoolyard` | `leisure_schoolyard` | node, area |

#### Scenario: Picnic table type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `leisure_picnic_table`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `leisure=picnic_table` SHALL be importable as that type

#### Scenario: Dog park type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `leisure_dog_park`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `leisure=dog_park` SHALL be importable as that type

#### Scenario: Swimming area type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `leisure_swimming_area`
- **THEN** the type SHALL exist
- **AND** areas tagged `leisure=swimming_area` SHALL be importable as that type
- **AND** nodes tagged `leisure=swimming_area` SHALL NOT be importable as that type

#### Scenario: Outdoor seating type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `leisure_outdoor_seating`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `leisure=outdoor_seating` SHALL be importable as that type

### Requirement: Sports facility types

The import-time stylesheet SHALL define feature types for the following sports-related `leisure=*` values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `leisure=fitness_centre` | `leisure_fitness_centre` | node, area |
| `leisure=sports_hall` | `leisure_sports_hall` | node, area |
| `leisure=bleachers` | `leisure_bleachers` | area |
| `leisure=miniature_golf` | `leisure_miniature_golf` | node, area |
| `leisure=bowling_alley` | `leisure_bowling_alley` | node, area |
| `leisure=disc_golf_course` | `leisure_disc_golf_course` | node, area |
| `leisure=climbing` | `leisure_climbing` | node, area |
| `leisure=horse_riding` | `leisure_horse_riding` | node, area |
| `leisure=trampoline_park` | `leisure_trampoline_park` | node, area |
| `leisure=dance` | `leisure_dance` | node, area |

#### Scenario: Fitness centre type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `leisure_fitness_centre`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `leisure=fitness_centre` SHALL be importable as that type

#### Scenario: Sports hall type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `leisure_sports_hall`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `leisure=sports_hall` SHALL be importable as that type

#### Scenario: Bleachers type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `leisure_bleachers`
- **THEN** the type SHALL exist
- **AND** areas tagged `leisure=bleachers` SHALL be importable as that type
- **AND** nodes tagged `leisure=bleachers` SHALL NOT be importable as that type

#### Scenario: Miniature golf type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `leisure_miniature_golf`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `leisure=miniature_golf` SHALL be importable as that type

#### Scenario: Disc golf course type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `leisure_disc_golf_course`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `leisure=disc_golf_course` SHALL be importable as that type

### Requirement: Entertainment and wellness types

The import-time stylesheet SHALL define feature types for the following entertainment and wellness `leisure=*` values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `leisure=amusement_arcade` | `leisure_amusement_arcade` | node, area |
| `leisure=adult_gaming_centre` | `leisure_adult_gaming_centre` | node, area |
| `leisure=bandstand` | `leisure_bandstand` | node, area |
| `leisure=escape_game` | `leisure_escape_game` | node, area |
| `leisure=indoor_play` | `leisure_indoor_play` | node, area |
| `leisure=hackerspace` | `leisure_hackerspace` | node, area |
| `leisure=sauna` | `leisure_sauna` | node, area |
| `leisure=hot_tub` | `leisure_hot_tub` | node, area |
| `leisure=tanning_salon` | `leisure_tanning_salon` | node, area |

#### Scenario: Amusement arcade type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `leisure_amusement_arcade`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `leisure=amusement_arcade` SHALL be importable as that type

#### Scenario: Sauna type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `leisure_sauna`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `leisure=sauna` SHALL be importable as that type

#### Scenario: Bandstand type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `leisure_bandstand`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `leisure=bandstand` SHALL be importable as that type

### Requirement: Slipway element extension

The import-time stylesheet SHALL extend the existing `leisure_slipway` type to accept WAY elements in addition to NODE elements, matching the OSM wiki element table for `leisure=slipway` (node, way).

#### Scenario: Slipway way importable
- **GIVEN** a database imported with the type definitions
- **WHEN** a way tagged `leisure=slipway` is imported
- **THEN** the way SHALL be importable as `leisure_slipway`

### Requirement: Discouraged and undocumented leisure values

The import-time stylesheet SHALL NOT define dedicated feature types for discouraged, deprecated, or undocumented `leisure=*` values, including but not limited to `leisure=yes`, `leisure=hot_spring`, `leisure=turkish_bath`, `leisure=arena`, `leisure=sailing_club`, `leisure=wildlife_hide`, `leisure=high_ropes_course`, `leisure=sunbathing`, `leisure=barefoot`, `leisure=hammock`. Such objects remain covered by the generic `leisure` catch-all type.

#### Scenario: Discouraged values have no dedicated type
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `leisure_yes`, `leisure_hot_spring`, `leisure_turkish_bath`, `leisure_arena`, or `leisure_sailing_club`
- **THEN** none of these types SHALL exist
- **AND** objects carrying the corresponding tags SHALL still be importable via the generic `leisure` type

### Requirement: Area rendering for new leisure types

The rendering stylesheet SHALL give areas of the new leisure types a fill matching their nature at detail zoom: green fills for park-like types (`leisure_dog_park`, `leisure_recreation_ground`, `leisure_disc_golf_course`, `leisure_miniature_golf`), water fills for water-related types (`leisure_swimming_area`, `leisure_hot_tub`, `leisure_bathing_place`), sand fill for `leisure_beach_resort`, and the standard leisure area fill for the remaining types. Labels for the new types SHALL be rendered at close zoom following the existing leisure label rules.

#### Scenario: Dog park area gets green fill
- **GIVEN** a map rendered with the stylesheet at detail zoom
- **WHEN** an area of type `leisure_dog_park` is rendered
- **THEN** the area SHALL be filled with a green park-like color

#### Scenario: Swimming area gets water fill
- **GIVEN** a map rendered with the stylesheet at detail zoom
- **WHEN** an area of type `leisure_swimming_area` is rendered
- **THEN** the area SHALL be filled with a water-like color

#### Scenario: New leisure areas get labels at close zoom
- **GIVEN** a map rendered with the stylesheet at close zoom
- **WHEN** an area of type `leisure_picnic_table`, `leisure_fitness_centre`, or `leisure_sports_hall` is rendered
- **THEN** the area SHALL be labeled with its name following the leisure label rules

### Requirement: Node icon rendering for new leisure types

The rendering stylesheet SHALL define a dedicated vector symbol for each new leisure type that accepts NODE elements, and SHALL render nodes of those types with the corresponding symbol icon at very close zoom. Area-only types (`leisure_bleachers`, `leisure_swimming_area`) SHALL NOT have node icon rules.

#### Scenario: Picnic table node gets icon
- **GIVEN** a map rendered with the stylesheet at very close zoom
- **WHEN** a node of type `leisure_picnic_table` is rendered
- **THEN** the node SHALL be rendered with the `leisure_picnic_table` symbol icon

#### Scenario: Fitness centre node gets icon
- **GIVEN** a map rendered with the stylesheet at very close zoom
- **WHEN** a node of type `leisure_fitness_centre` is rendered
- **THEN** the node SHALL be rendered with the `leisure_fitness_centre` symbol icon

#### Scenario: Area-only types have no node icon
- **GIVEN** a map rendered with the stylesheet
- **WHEN** the stylesheet is inspected for node icon rules for `leisure_bleachers` and `leisure_swimming_area`
- **THEN** no node icon rule SHALL exist for either type
