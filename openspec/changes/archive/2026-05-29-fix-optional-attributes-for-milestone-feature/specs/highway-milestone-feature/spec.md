## REMOVED Requirements

### Requirement: Feature only allocated when both distance and ref are present
**Reason**: OSM wiki specifies only `highway=milestone` is mandatory — `distance`, `ref`, `carriageway_ref`, and `marker` are all independently optional.
**Migration**: Replaced by new "Feature allocated for any highway=milestone node" requirement.

#### Scenario: Only distance tag present, no value allocated
- **WHEN** a node has `highway=milestone` and `distance=35` but no `ref` tag
- **THEN** the feature SHALL NOT allocate a value

#### Scenario: Only ref tag present, no value allocated
- **WHEN** a node has `highway=milestone` and `ref=A2` but no `distance` tag
- **THEN** the feature SHALL NOT allocate a value

#### Scenario: Neither tag present, no value allocated
- **WHEN** a node has `highway=milestone` but no `distance` and no `ref` tag
- **THEN** the feature SHALL NOT allocate a value

## MODIFIED Requirements

### Requirement: HighwayMilestone feature stores distance as uint32_t in meters

The system SHALL store the OSM `distance` tag value as a `uint32_t` representing **meters**. The OSM tag value is in kilometers — the parser SHALL multiply the parsed numeric value by 1000 before storing. Only simple decimal numbers with "." as separator are accepted. Other formats (unit suffixes, km+meters format, comma separator) SHALL log a warning, skip the distance field (leaving it at 0), and continue parsing remaining tags.

#### Scenario: Milestone with simple numeric distance in kilometers
- **WHEN** a node has `highway=milestone` and `distance=35`
- **THEN** the distance field SHALL be `35000` (35 km × 1000)

#### Scenario: Milestone with decimal distance in kilometers
- **WHEN** a node has `highway=milestone` and `distance=35.5`
- **THEN** the distance field SHALL be `35500` (35.5 km × 1000, decimal truncated after multiplication)

#### Scenario: Milestone with unit suffix logs warning and skips distance
- **WHEN** a node has `highway=milestone`, `distance=35.0 mi`, and `ref=A2`
- **THEN** a warning SHALL be logged AND a feature value SHALL be allocated with distance=0 and ref="A2"

#### Scenario: Milestone with combined km+meters format logs warning and skips distance
- **WHEN** a node has `highway=milestone`, `distance=45 + 5`, and `ref=A2`
- **THEN** a warning SHALL be logged AND a feature value SHALL be allocated with distance=0 and ref="A2"

#### Scenario: Milestone with comma separator logs warning and skips distance
- **WHEN** a node has `highway=milestone`, `distance=35,5`, and `ref=A2`
- **THEN** a warning SHALL be logged AND a feature value SHALL be allocated with distance=0 and ref="A2"

## ADDED Requirements

### Requirement: Feature allocated for any highway=milestone node
The system SHALL allocate a `HighwayMilestoneFeatureValue` for every node with `highway=milestone`, regardless of which sub-tags are present. Each sub-tag (`distance`, `ref`, `carriageway_ref`, `marker`) SHALL be parsed independently — missing or invalid sub-tags SHALL NOT prevent allocation.

#### Scenario: Only ref tag present, feature allocated
- **WHEN** a node has `highway=milestone` and `ref=A2` but no `distance` tag
- **THEN** a feature value SHALL be allocated with ref="A2" and distance=0

#### Scenario: Only distance tag present, feature allocated
- **WHEN** a node has `highway=milestone` and `distance=35` but no `ref` tag
- **THEN** a feature value SHALL be allocated with distance=35000 and ref="" (empty)

#### Scenario: No sub-tags present, feature allocated with defaults
- **WHEN** a node has `highway=milestone` but no `distance`, `ref`, `carriageway_ref`, or `marker` tags
- **THEN** a feature value SHALL be allocated with distance=0, ref="", carriageway_ref="", and marker=""

#### Scenario: Malformed distance with valid ref still allocates
- **WHEN** a node has `highway=milestone`, `distance=35.0 mi`, and `ref=A2`
- **THEN** a warning SHALL be logged AND a feature value SHALL be allocated with distance=0 and ref="A2"

### Requirement: Serialization round-trip with minimal fields
The feature value SHALL correctly serialize and deserialize when some fields are at default values.

#### Scenario: Round-trip serialization with only ref set
- **WHEN** a HighwayMilestoneFeatureValue with distance=0, ref="A2", carriageway_ref="", marker="" is written to a FileWriter and read back with FileScanner
- **THEN** all fields SHALL match the original values (distance=0, ref="A2", carriageway_ref="", marker="")

#### Scenario: Round-trip serialization with all defaults
- **WHEN** a HighwayMilestoneFeatureValue with all default values is written to a FileWriter and read back with FileScanner
- **THEN** all fields SHALL match the original defaults (distance=0, all strings empty)