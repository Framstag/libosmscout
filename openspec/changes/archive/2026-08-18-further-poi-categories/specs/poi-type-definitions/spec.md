## Purpose

Defines the import-time OSM feature types for police stations and doctors offices, so these POIs exist in the database `TypeConfig` and are searchable by the POI search API.

## ADDED Requirements

### Requirement: Police station type definition

The import-time stylesheet SHALL define an `amenity_police` feature type matching OSM objects tagged `amenity=police`.

#### Scenario: Police station type exists in type config
- **WHEN** a database is imported with a stylesheet that includes the type definitions
- **THEN** the database `TypeConfig` SHALL contain an `amenity_police` type
- **AND** POIs tagged `amenity=police` SHALL be importable as that type

### Requirement: Doctors office type definition

The import-time stylesheet SHALL define an `amenity_doctors` feature type matching OSM objects tagged `amenity=doctors`.

#### Scenario: Doctors office type exists in type config
- **WHEN** a database is imported with a stylesheet that includes the type definitions
- **THEN** the database `TypeConfig` SHALL contain an `amenity_doctors` type
- **AND** POIs tagged `amenity=doctors` SHALL be importable as that type
