## ADDED Requirements

### Requirement: Mapper converts LocationCoordDescription to JSON

The `LocationDescriptionMapper` SHALL convert `LocationCoordDescription` to both `content` text and `structuredContent` JSON fields.

#### Scenario: Coord description present
- **GIVEN** a `LocationDescription` with a valid `LocationCoordDescription`
- **WHEN** `ToJson()` is called
- **THEN** `content` contains a text entry with the coordinate's `GetDisplayText()`
- **AND** `structuredContent["coordinateDescription"]` contains `latitude`, `longitude`, and `displayText`

#### Scenario: Coord description absent
- **GIVEN** a `LocationDescription` with no `LocationCoordDescription`
- **WHEN** `ToJson()` is called
- **THEN** no coordinate-related entry appears in `content` or `structuredContent`

### Requirement: Mapper converts LocationAtPlaceDescription to JSON

The `LocationDescriptionMapper` SHALL convert all three `LocationAtPlaceDescription` instances (atName, atAddress, atPOI) using a shared helper.

#### Scenario: AtName description present
- **GIVEN** a `LocationDescription` with a valid `atNameDescription`
- **WHEN** `ToJson()` is called
- **THEN** `structuredContent["atNameDescription"]` contains `location`, `distanceInMeter`, `bearing`, `atPlace`
- **AND** if `adminRegion` is set, the object includes `adminRegion` with its `name`, `altName`, and optional `postalAreas`
- **AND** if `poi` is set, the object includes `poi` with `name`
- **AND** if `location` (street) is set, the object includes `street` with `name`
- **AND** if `address` is set, the object includes `houseNumber`
- **AND** `content` contains a human-readable string built from the place's `GetDisplayString()`

#### Scenario: AtAddress description present
- **GIVEN** a `LocationDescription` with a valid `atAddressDescription`
- **WHEN** `ToJson()` is called
- **THEN** `structuredContent["atAddressDescription"]` follows the same structure as `atNameDescription`
- **AND** `content` contains a text entry

#### Scenario: AtPOI description present
- **GIVEN** a `LocationDescription` with a valid `atPOIDescription`
- **WHEN** `ToJson()` is called
- **THEN** `structuredContent["atPOIDescription"]` follows the same structure as `atNameDescription`
- **AND** `content` contains a text entry

#### Scenario: Multiple at-place descriptions present
- **GIVEN** a `LocationDescription` with both `atNameDescription` and `atAddressDescription` set
- **WHEN** `ToJson()` is called
- **THEN** both `atNameDescription` and `atAddressDescription` appear in `structuredContent`
- **AND** both get separate entries in `content`

### Requirement: Mapper converts LocationWayDescription to JSON

The `LocationDescriptionMapper` SHALL convert `LocationWayDescription` to JSON.

#### Scenario: Way description present
- **GIVEN** a `LocationDescription` with a valid `LocationWayDescription`
- **WHEN** `ToJson()` is called
- **THEN** `structuredContent["wayDescription"]` contains `way` (from `GetWay().GetDisplayString()`), `distanceInMeter`
- **AND** `content` contains a text entry

#### Scenario: Way description absent
- **GIVEN** a `LocationDescription` with no `LocationWayDescription`
- **WHEN** `ToJson()` is called
- **THEN** no way-related entry appears

### Requirement: Mapper converts LocationCrossingDescription to JSON

The `LocationDescriptionMapper` SHALL convert `LocationCrossingDescription` to JSON.

#### Scenario: Crossing description present
- **GIVEN** a `LocationDescription` with a valid `LocationCrossingDescription`
- **WHEN** `ToJson()` is called
- **THEN** `structuredContent["crossingDescription"]` contains `crossingLatitude`, `crossingLongitude`, `ways` (array of way display strings), `distanceInMeter`, `bearing`, `atPlace`
- **AND** `content` contains a text entry

### Requirement: Mapper converts LocationHighwayMilestoneDescription to JSON

The `LocationDescriptionMapper` SHALL convert `LocationHighwayMilestoneDescription` to JSON.

#### Scenario: Highway milestone description present
- **GIVEN** a `LocationDescription` with a valid `LocationHighwayMilestoneDescription`
- **WHEN** `ToJson()` is called
- **THEN** `structuredContent["highwayMilestoneDescription"]` contains `milestoneDistance`, `milestoneRef`, `carriagewayRef`, `distanceInMeter`, `bearing`, `atPlace`
- **AND** `content` contains a text entry