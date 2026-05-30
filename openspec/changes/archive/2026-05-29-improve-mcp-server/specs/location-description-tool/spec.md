## ADDED Requirements

### Requirement: LocationDescriptionTool calls DescribeLocation

The `LocationDescriptionTool` SHALL call `LocationDescriptionService::DescribeLocation()` with the latitude/longitude from the request.

#### Scenario: Successful location description
- **GIVEN** a valid `tools/call` request for tool `locationDescription` with `arguments.latitude` and `arguments.longitude`
- **WHEN** `HandleLocationDescription()` is called
- **THEN** the result SHALL have status 200
- **AND** the body SHALL contain `jsonrpc: "2.0"` and the request `id`
- **AND** the body SHALL contain all description types returned by the service, mapped via `LocationDescriptionMapper`
- **AND** `result.content` SHALL be a non-empty array of text entries
- **AND** `result.structuredContent` SHALL contain each present description type under its key

#### Scenario: Invalid coordinates
- **GIVEN** a request with non-numeric or out-of-range latitude/longitude
- **WHEN** `HandleLocationDescription()` is called
- **THEN** the result SHALL have status 400
- **AND** `result.error.message` SHALL describe the invalid parameter

#### Scenario: No description returned
- **GIVEN** a valid location with no nearby objects to describe
- **WHEN** `HandleLocationDescription()` is called
- **THEN** the result SHALL have status 200
- **AND** `result.content` SHALL contain at least the coordinate text
- **AND** `structuredContent` SHALL contain only `coordinateDescription`

### Requirement: LocationDescriptionTool validates input

The `LocationDescriptionTool` SHALL validate that required input parameters are present and correctly typed.

#### Scenario: Missing latitude
- **GIVEN** a request for `locationDescription` with no `latitude` in `arguments`
- **WHEN** `HandleLocationDescription()` is called
- **THEN** the result SHALL have status 400
- **AND** indicate that `latitude` is required

#### Scenario: Missing longitude
- **GIVEN** a request for `locationDescription` with no `longitude` in `arguments`
- **WHEN** `HandleLocationDescription()` is called
- **THEN** the result SHALL have status 400
- **AND** indicate that `longitude` is required

## REMOVED Requirements

### Requirement: LocationDescriptionTool passes TypeConfig to mapper

**Reason**: Design resolved that individual attribute getters (`GetMilestoneDistance()`, `GetMilestoneRef()`, `GetMilestoneCarriagewayRef()`) on `LocationHighwayMilestoneDescription` already provide all needed data. No `FeatureValueBuffer`/`TypeConfig` decoding needed — mapper uses these getters directly.

**Migration**: The `LocationHighwayMilestoneDescription` mapper reads fields from the description object's own getters rather than decoding `FeatureValueBuffer` via `TypeConfig`.