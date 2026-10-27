# Spec Delta

## MODIFIED Requirements

### Requirement: POI result data class

New Java class `com.framstag.libosmscout.client.PoiEntry` SHALL represent a single POI search result.
Alongside the label, the object type, the coordinate and the distance, a result SHALL expose the
operator and the brand of the found object when the object carries them, and the description of the
object type in the language the caller asked for.

#### Scenario: PoiEntry structure

- **WHEN** a `PoiEntry` is returned from `searchPOIs`
- **THEN** it SHALL expose fields: `label` (String), `operator` (String, empty when the found object
  carries no operator), `brand` (String, empty when the found object carries no brand),
  `objectType` (String), `objectTypeDescription` (String, empty when no description is available),
  `lat` (double), `lon` (double), `distance` (double, meters from search center)

#### Scenario: PoiEntry fields populated

- **WHEN** user calls `searchPOIs("restaurants", 52.0, 8.0, 5000, 50)` on a database containing a restaurant
- **THEN** each returned `PoiEntry` SHALL have non-null `label` and `objectType`
- **AND** `operator` and `brand` SHALL be non-null, and empty when the found object carries no such
  information
- **AND** `distance` SHALL be the distance in meters from the search center

#### Scenario: The object type description is reported

- **WHEN** a `PoiEntry` is returned for an object whose type carries a description in the requested language
- **THEN** `objectTypeDescription` SHALL carry that description
- **AND** `objectType` SHALL still carry the technical type name

#### Scenario: An undescribed object type reports an empty description

- **WHEN** a `PoiEntry` is returned for an object whose type carries no description in the requested language
- **THEN** `objectTypeDescription` SHALL be empty and non-null
- **AND** no error SHALL be raised
