## Purpose

The `HighwayMilestoneFeature` provides read/write support for OSM `highway=milestone` node data. It captures `distance` (in meters), `ref` (route identifier), `carriageway_ref` (carriageway variant), and `marker` (physical marker shape) tags, making them available for indexing, rendering, routing display, and DumpData description output.

## Requirements

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

### Requirement: Feature only allocated when both distance and ref are present

The system SHALL only allocate a HighwayMilestoneFeatureValue if both `distance` AND `ref` tags are present on the node. If either is missing, parsing SHALL silently return without allocating a value.

#### Scenario: Only distance tag present, no value allocated
- **WHEN** a node has `highway=milestone` and `distance=35` but no `ref` tag
- **THEN** the feature SHALL NOT allocate a value

#### Scenario: Only ref tag present, no value allocated
- **WHEN** a node has `highway=milestone` and `ref=A2` but no `distance` tag
- **THEN** the feature SHALL NOT allocate a value

#### Scenario: Neither tag present, no value allocated
- **WHEN** a node has `highway=milestone` but no `distance` and no `ref` tag
- **THEN** the feature SHALL NOT allocate a value

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

#### Scenario: Round-trip serialization
- **WHEN** a HighwayMilestoneFeatureValue with distance=35, ref="A2", carriageway_ref="W", marker="stone" is written to a FileWriter and read back with FileScanner
- **THEN** all fields SHALL match the original values

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