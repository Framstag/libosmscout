## MODIFIED Requirements

### Requirement: HighwayMilestone feature stores distance as uint32_t in meters

The system SHALL store the OSM `distance` tag value as a `uint32_t` representing **meters**. The OSM tag value is in kilometers — the parser SHALL multiply the parsed numeric value by 1000 before storing. Only simple decimal numbers with "." as separator are accepted. Other formats (unit suffixes, km+meters format, comma separator) SHALL log a warning and skip.

#### Scenario: Milestone with simple numeric distance in kilometers
- **WHEN** a node has `highway=milestone`, `distance=35`, and `ref=A2`
- **THEN** the distance field SHALL be `35000` (35 km × 1000)

#### Scenario: Milestone with decimal distance in kilometers
- **WHEN** a node has `highway=milestone`, `distance=35.5`, and `ref=A2`
- **THEN** the distance field SHALL be `35500` (35.5 km × 1000, decimal truncated after multiplication)

#### Scenario: Milestone with unit suffix logs warning and skips
- **WHEN** a node has `highway=milestone` and `distance=35.0 mi`
- **THEN** a warning SHALL be logged AND no feature value SHALL be allocated

#### Scenario: Milestone with combined km+meters format logs warning and skips
- **WHEN** a node has `highway=milestone` and `distance=45 + 5`
- **THEN** a warning SHALL be logged AND no feature value SHALL be allocated

#### Scenario: Milestone with comma separator logs warning and skips
- **WHEN** a node has `highway=milestone` and `distance=35,5`
- **THEN** a warning SHALL be logged AND no feature value SHALL be allocated

### Requirement: GetLabel returns distance formatted with locale

The `GetLabel()` method on the value SHALL return the distance value in kilometers (internal meters / 1000) formatted using the locale with a "km" unit suffix.

#### Scenario: GetLabel returns distance with kilometer unit
- **WHEN** `GetLabel(locale, 0)` is called on a HighwayMilestoneFeatureValue with distance=35000
- **THEN** it SHALL return `"35 km"` (formatted per locale)

### Requirement: HighwayMilestoneDescriptionProcessor renders feature values in DumpData

The system SHALL include a `HighwayMilestoneDescriptionProcessor` registered in `DescriptionService` that adds description entries for distance (in kilometers), ref, carriageway_ref, and marker values when a `HighwayMilestoneFeatureValue` is present.

#### Scenario: Processor renders distance (in km) and ref
- **WHEN** a buffer contains HighwayMilestoneFeatureValue with distance=35000 and ref="A2"
- **THEN** the description SHALL contain a "HighwayMilestone" section with entries for distance ("35 km") and ref ("A2")

#### Scenario: Processor skips empty carriageway_ref and marker
- **WHEN** a buffer contains HighwayMilestoneFeatureValue with distance=35000 and ref="A2" but empty carriageway_ref and marker
- **THEN** the description SHALL NOT contain entries for carriageway_ref or marker

#### Scenario: Processor includes carriageway_ref and marker when present
- **WHEN** a buffer contains HighwayMilestoneFeatureValue with distance=35000, ref="A2", carriageway_ref="W", and marker="stone"
- **THEN** the description SHALL contain entries for carriageway_ref ("W") and marker ("stone")