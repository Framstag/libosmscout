# Spec Delta

## MODIFIED Requirements

### Requirement: POI result data class

New Java class `com.framstag.libosmscout.client.PoiEntry` SHALL represent a single POI search result.
Alongside the label, the object type, the coordinate and the distance, a result SHALL expose the
operator and the brand of the found object when the object carries them.

#### Scenario: PoiEntry structure

- **WHEN** a `PoiEntry` is returned from `searchPOIs`
- **THEN** it SHALL expose fields: `label` (String), `operator` (String, empty when the found object
  carries no operator), `brand` (String, empty when the found object carries no brand),
  `objectType` (String), `lat` (double), `lon` (double), `distance` (double, meters from search
  center)

#### Scenario: PoiEntry fields populated

- **WHEN** user calls `searchPOIs("restaurants", 52.0, 8.0, 5000, 50)` on a database containing a restaurant
- **THEN** each returned `PoiEntry` SHALL have non-null `label` and `objectType`
- **AND** `operator` and `brand` SHALL be non-null, and empty when the found object carries no such
  information
- **AND** `distance` SHALL be the distance in meters from the search center

## ADDED Requirements

### Requirement: POI results expose the operator and the brand of the found object

A POI search result SHALL expose the operator of the found object and the brand of the found object.
Each attribute SHALL be empty when the found object carries no such information; an empty attribute
SHALL NOT be an error and SHALL NOT be treated as a value. The two attributes SHALL be determined
independently of each other and independently of the display label, so that an object that has both
carries both, an object that has only one carries only that one, and an object that has neither is
still a valid result. Reading these attributes SHALL NOT change which objects a search returns, how
many it returns, or the order it returns them in.

#### Scenario: Operator and brand are carried by the result

- **GIVEN** a database region contains a POI that carries an operator and a brand in the map data
- **WHEN** a search returns that POI as a result
- **THEN** the result SHALL carry that operator
- **AND** the result SHALL carry that brand

#### Scenario: Only one of the two attributes has a value

- **GIVEN** a database region contains a POI that carries an operator but no brand
- **WHEN** a search returns that POI as a result
- **THEN** the result SHALL carry that operator
- **AND** the result SHALL report the brand as empty

#### Scenario: Neither attribute has a value

- **GIVEN** a database region contains a POI that carries neither an operator nor a brand
- **WHEN** a search returns that POI as a result
- **THEN** the result SHALL report both attributes as empty
- **AND** no error SHALL be raised

#### Scenario: The label fallback does not consume the operator

- **GIVEN** a POI whose display label is derived from its operator because the object carries no name
- **WHEN** a search returns that POI as a result
- **THEN** the result SHALL carry that operator in addition to the label

#### Scenario: The attributes do not influence which results are returned

- **GIVEN** a search over an area whose matching objects are a mix of objects that carry an operator
  or a brand and objects that carry neither
- **WHEN** the search is executed
- **THEN** every object that matches the category and the radius SHALL be returned regardless of
  whether it carries these attributes
- **AND** the returned results SHALL be ordered by distance ascending as before
