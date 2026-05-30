## Purpose

The `HighwayMilestoneFeature` provides read/write support for OSM `highway=milestone` node data. It captures `distance` (in meters), `ref` (route identifier), `carriageway_ref` (carriageway variant), and `marker` (physical marker shape) tags, making them available for indexing, rendering, routing display, and DumpData description output.

## Requirements

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
- **THEN** a warning SHALL be logged AND a feature value SHALL be allocated with distance=0 and ref="A2"

#### Scenario: Milestone with comma separator logs warning and skips distance
- **WHEN** a node has `highway=milestone`, `distance=35,5`, and `ref=A2`
- **THEN** a warning SHALL be logged AND a feature value SHALL be allocated with distance=0 and ref="A2"

### Requirement: Feature allocated for any highway=milestone node

The system SHALL allocate a HighwayMilestoneFeatureValue for every node with `highway=milestone`, regardless of which sub-tags are present. Each sub-tag (`distance`, `ref`, `carriageway_ref`, `marker`) SHALL be parsed independently — missing or invalid sub-tags SHALL NOT prevent allocation.

#### Scenario: Only ref tag present, feature allocated
- **WHEN** a node has `highway=milestone` and `ref=A2` but no `distance` tag
- **THEN** a feature value SHALL be allocated with ref="A2" and distance=0

#### Scenario: Only distance tag present, feature allocated
- **WHEN** a node has `highway=milestone` and `distance=35` but no `ref` tag
- **THEN** a feature value SHALL be allocated with distance=35000 and ref="" (empty)

#### Scenario: No sub-tags present, feature allocated with defaults
- **WHEN** a node has `highway=milestone` but no `distance`, `ref`, `carriageway_ref`, or `marker` tags
- **THEN** a feature value SHALL be allocated with distance=0, ref="", carriageway_ref="", and marker=""

### Requirement: HighwayMilestone feature stores OSM ref tag as string

The system SHALL store the OSM `ref` tag value as a string field in the feature value.

#### Scenario: Milestone with ref tag
- **WHEN** a node has `highway=milestone`, `distance=35`, and `ref=A2`
- **THEN** the ref field SHALL contain `"A2"`

### Requirement: HighwayMilestone feature stores OSM carriageway_ref tag as string

The system SHALL store the OSM `carriageway_ref` tag value as a string field in the feature value when present.

#### Scenario: Milestone with carriageway_ref tag
- **WHEN** a node has `highway=milestone`, `distance=35`, `ref=A2`, and `carriageway_ref=W`
- **THEN** the carriageway_ref field SHALL contain `"W"`

#### Scenario: Milestone without carriageway_ref tag
- **WHEN** a node has `highway=milestone`, `distance=35`, and `ref=A2` but no `carriageway_ref`
- **THEN** the carriageway_ref field SHALL be empty

### Requirement: HighwayMilestone feature stores OSM marker tag as string

The system SHALL store the OSM `marker` tag value as a string field in the feature value when present.

#### Scenario: Milestone with marker tag
- **WHEN** a node has `highway=milestone`, `distance=35`, `ref=A2`, and `marker=stone`
- **THEN** the marker field SHALL contain `"stone"`

### Requirement: HighwayMilestone feature value serialization

The feature value SHALL support binary Read/Write for all stored fields (uint32_t distance, string ref, string carriageway_ref, string marker) using `FileScanner`/`FileWriter`.

#### Scenario: Round-trip serialization with all fields
- **WHEN** a HighwayMilestoneFeatureValue with distance=35, ref="A2", carriageway_ref="W", marker="stone" is written to a FileWriter and read back with FileScanner
- **THEN** all fields SHALL match the original values

#### Scenario: Round-trip serialization with only ref set
- **WHEN** a HighwayMilestoneFeatureValue with distance=0, ref="A2", carriageway_ref="", marker="" is written to a FileWriter and read back with FileScanner
- **THEN** all fields SHALL match the original values

#### Scenario: Round-trip serialization with all defaults
- **WHEN** a HighwayMilestoneFeatureValue with all default values is written to a FileWriter and read back with FileScanner
- **THEN** all fields SHALL match the original defaults

### Requirement: GetLabel returns distance formatted with locale

The `GetLabel()` method on the value SHALL return the distance value in kilometers (internal meters / 1000) formatted using the locale with a "km" unit suffix.

#### Scenario: GetLabel returns distance with kilometer unit
- **WHEN** `GetLabel(locale, 0)` is called on a HighwayMilestoneFeatureValue with distance=35000
- **THEN** it SHALL return `"35 km"` (formatted per locale)

### Requirement: HighwayMilestone feature registration in TypeConfig

The `HighwayMilestoneFeature` SHALL be registered in `TypeConfig` constructor so OST stylesheets can reference `HIGHWAYMILESTONE` feature.

#### Scenario: Feature registered in TypeConfig
- **WHEN** a TypeConfig is constructed
- **THEN** `GetFeature("HighwayMilestone")` SHALL return a valid FeatureRef

### Requirement: HighwayMilestone feature files added to build systems

The new header and source files SHALL be added to both CMakeLists.txt and meson.build.

#### Scenario: Header listed in CMakeLists.txt
- **WHEN** checking `libosmscout/CMakeLists.txt`
- **THEN** `include/osmscout/feature/HighwayMilestoneFeature.h` SHALL appear in `HEADER_FILES_FEATURE`

#### Scenario: Source listed in CMakeLists.txt
- **WHEN** checking `libosmscout/CMakeLists.txt`
- **THEN** `src/osmscout/feature/HighwayMilestoneFeature.cpp` SHALL appear in the source files list

#### Scenario: Header listed in meson.build
- **WHEN** checking `libosmscout/include/meson.build`
- **THEN** `osmscout/feature/HighwayMilestoneFeature.h` SHALL appear in the header list

#### Scenario: Source listed in meson.build
- **WHEN** checking `libosmscout/src/meson.build`
- **THEN** `src/osmscout/feature/HighwayMilestoneFeature.cpp` SHALL appear in the source list

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