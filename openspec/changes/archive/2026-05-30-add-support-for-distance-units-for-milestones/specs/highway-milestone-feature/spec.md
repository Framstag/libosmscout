## MODIFIED Requirements

### Requirement: HighwayMilestone feature stores distance as uint32_t in meters

The system SHALL store the OSM `distance` tag value as a `uint32_t` representing **meters**. When the tag value has a unit suffix (`"km"` or `"mi"`), the system SHALL convert to meters using the appropriate factor. When no unit suffix is present, the value SHALL be treated as kilometers (backward compatible). Only simple decimal numbers with "." as separator are accepted, with optional unit suffix. Other formats (unknown unit suffixes, km+meters format, comma separator) SHALL log a warning, skip the distance field (leaving it at 0), and continue parsing remaining tags.

#### Scenario: Milestone with simple numeric distance in kilometers
- **WHEN** a node has `highway=milestone` and `distance=35`
- **THEN** the distance field SHALL be `35000` (35 km × 1000)

#### Scenario: Milestone with decimal distance in kilometers
- **WHEN** a node has `highway=milestone` and `distance=35.5`
- **THEN** the distance field SHALL be `35500` (35.5 km × 1000, decimal truncated after multiplication)

#### Scenario: Milestone with km unit suffix
- **WHEN** a node has `highway=milestone`, `distance=35.0 km`, and `ref=A2`
- **THEN** the distance field SHALL be `35000` (35 km × 1000)

#### Scenario: Milestone with mi unit suffix
- **WHEN** a node has `highway=milestone`, `distance=10 mi`, and `ref=A2`
- **THEN** the distance field SHALL be `16093` (10 mi × 1609.344, truncated to uint32_t)

#### Scenario: Milestone with unknown unit suffix logs warning and skips distance
- **WHEN** a node has `highway=milestone`, `distance=35.0 nmi`, and `ref=A2`
- **THEN** a warning SHALL be logged AND a feature value shall be allocated with distance=0 and ref="A2"

#### Scenario: Milestone with combined km+meters format logs warning and skips distance
- **WHEN** a node has `highway=milestone`, `distance=45 + 5`, and `ref=A2`
- **THEN** a warning SHALL be logged AND a feature value shall be allocated with distance=0 and ref="A2"

#### Scenario: Milestone with comma separator logs warning and skips distance
- **WHEN** a node has `highway=milestone`, `distance=35,5`, and `ref=A2`
- **THEN** a warning SHALL be logged AND a feature value shall be allocated with distance=0 and ref="A2"