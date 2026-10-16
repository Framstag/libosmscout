# Spec Delta

## MODIFIED Requirements

### Requirement: LocationEntry data class

New Java class `com.framstag.libosmscout.client.LocationEntry` SHALL represent a single search result.
Alongside the object type, a result SHALL expose the description of that object type in the language
the caller asked for, and SHALL report it as empty when no description is available.

#### Scenario: LocationEntry structure

- **WHEN** a `LocationEntry` is returned from `searchLocations`
- **THEN** it SHALL expose fields: `label` (String), `type` (String), `objectType` (String),
  `objectTypeDescription` (String, empty when no description is available), `lat` (double),
  `lon` (double), `region` (String[])

#### Scenario: The object type description is reported

- **WHEN** a `LocationEntry` is returned for an object whose type carries a description in the requested language
- **THEN** `objectTypeDescription` SHALL carry that description
- **AND** `objectType` SHALL still carry the technical type name

#### Scenario: An undescribed object type reports an empty description

- **WHEN** a `LocationEntry` is returned for an object whose type carries no description in the requested language
- **THEN** `objectTypeDescription` SHALL be empty and non-null
- **AND** no error SHALL be raised
