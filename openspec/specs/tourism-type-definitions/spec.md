# tourism-type-definitions Specification

## Purpose

Defines the import-time OSM feature types for documented `tourism=*` values missing from `stylesheets/map.ost`, so these features exist in the database `TypeConfig` and are importable, searchable, and renderable. Element types follow the OSM wiki [Key:tourism](https://wiki.openstreetmap.org/wiki/Key:tourism) element table. Only values that are documented, have relevant usage (>=50 per taginfo), and are not discouraged/deprecated are added.

## Requirements

### Requirement: Accommodation tourism types

The import-time stylesheet SHALL define feature types for the following accommodation `tourism=*` values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `tourism=apartment` | `tourism_apartment` | node, area |
| `tourism=wilderness_hut` | `tourism_wilderness_hut` | node, area |
| `tourism=trail_riding_station` | `tourism_trail_riding_station` | node, area |

For building-like values (`apartment`, `wilderness_hut`), the stylesheet SHALL additionally define an area-only `_building` variant matching objects that also carry a `building=*` tag other than `no`/`false`/`0`, following the convention of existing tourism accommodation types (e.g. `tourism_hotel_building`).

#### Scenario: Apartment type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `tourism_apartment`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `tourism=apartment` SHALL be importable as that type
- **AND** areas additionally tagged with a `building=*` value other than `no`/`false`/`0` SHALL be importable as `tourism_apartment_building`

#### Scenario: Wilderness hut type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `tourism_wilderness_hut`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `tourism=wilderness_hut` SHALL be importable as that type
- **AND** areas additionally tagged with a `building=*` value other than `no`/`false`/`0` SHALL be importable as `tourism_wilderness_hut_building`

#### Scenario: Trail riding station type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `tourism_trail_riding_station`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `tourism=trail_riding_station` SHALL be importable as that type

### Requirement: Attraction tourism types

The import-time stylesheet SHALL define feature types for the following attraction `tourism=*` values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `tourism=gallery` | `tourism_gallery` | node, area |
| `tourism=camp_pitch` | `tourism_camp_pitch` | node, area |

The stylesheet SHALL additionally define an area-only `tourism_gallery_building` variant matching objects tagged `tourism=gallery` that also carry a `building=*` tag other than `no`/`false`/`0`, following the `tourism_museum_building` convention.

#### Scenario: Gallery type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `tourism_gallery`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `tourism=gallery` SHALL be importable as that type
- **AND** areas additionally tagged with a `building=*` value other than `no`/`false`/`0` SHALL be importable as `tourism_gallery_building`

#### Scenario: Camp pitch type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `tourism_camp_pitch`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `tourism=camp_pitch` SHALL be importable as that type

### Requirement: Discouraged and undocumented tourism values

The import-time stylesheet SHALL NOT define dedicated feature types for discouraged, deprecated, or undocumented `tourism=*` values, including but not limited to `tourism=resort`, `tourism=winery`, `tourism=checkpoint`, `tourism=cabin`, `tourism=wine_cellar`, `tourism=hunting_lodge`, `tourism=lean_to`, `tourism=spa_resort`, `tourism=holiday_village`. Such objects remain covered by the generic `tourism` catch-all type.

#### Scenario: Discouraged values have no dedicated type
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `tourism_resort`, `tourism_winery`, `tourism_cabin`, `tourism_spa_resort`, or `tourism_holiday_village`
- **THEN** none of these types SHALL exist
- **AND** objects carrying the corresponding tags SHALL still be importable via the generic `tourism` type

### Requirement: Area rendering for new tourism types

The rendering stylesheet SHALL give areas of the new tourism types (`tourism_apartment`, `tourism_gallery`, `tourism_camp_pitch`, `tourism_trail_riding_station`, `tourism_wilderness_hut`) the standard tourism area fill at detail zoom, matching the existing rendering of `tourism_museum` / `tourism_camp_site` areas. The `_building` variants SHALL be rendered through the existing tourism building rules.

#### Scenario: New tourism areas get tourism fill
- **GIVEN** a map rendered with the stylesheet at detail zoom
- **WHEN** an area of type `tourism_gallery`, `tourism_camp_pitch`, `tourism_apartment`, `tourism_trail_riding_station`, or `tourism_wilderness_hut` is rendered
- **THEN** the area SHALL be filled with the standard tourism area color

#### Scenario: Building variants use tourism building rendering
- **GIVEN** a map rendered with the stylesheet
- **WHEN** an area of type `tourism_apartment_building`, `tourism_gallery_building`, or `tourism_wilderness_hut_building` is rendered
- **THEN** the area SHALL be rendered with the tourism building fill, border, and label rules
