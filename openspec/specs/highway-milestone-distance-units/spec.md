## Purpose

The `highway-milestone-distance-units` capability extends `HighwayMilestoneFeature` distance parsing to support explicit unit suffixes (`"km"` and `"mi"`) in the OSM `distance` tag value, with conversion to internal meter representation. The implementation uses an extensible unit table so that new units can be added with a single table entry.

## Requirements

### Requirement: Distance tag parsed with unit suffix support

The system SHALL parse the OSM `distance` tag value with optional unit suffix (`"km"` or `"mi"`). When a unit suffix is present, the value SHALL be converted to meters using the appropriate conversion factor (km × 1000, mi × 1609.344). When no unit suffix is present, the value SHALL be treated as kilometers (backward compatible). The implementation SHALL use an extensible table of unit definitions so that adding a new unit requires only adding an entry to the table.

#### Scenario: Milestone with explicit km unit
- **WHEN** a node has `highway=milestone`, `distance=35.0 km`, and `ref=A2`
- **THEN** the distance field SHALL be `35000` (35 km × 1000)

#### Scenario: Milestone with explicit mi unit
- **WHEN** a node has `highway=milestone`, `distance=10 mi`, and `ref=A2`
- **THEN** the distance field SHALL be `16093` (10 mi × 1609.344, truncated to uint32_t)

#### Scenario: Milestone with decimal mi unit
- **WHEN** a node has `highway=milestone`, `distance=10.5 mi`, and `ref=A2`
- **THEN** the distance field SHALL be `16898` (10.5 mi × 1609.344, truncated to uint32_t)

#### Scenario: Milestone without unit defaults to km
- **WHEN** a node has `highway=milestone`, `distance=35`, and `ref=A2`
- **THEN** the distance field SHALL be `35000` (35 km × 1000, default behavior)

#### Scenario: Unknown unit suffix logs warning and skips distance
- **WHEN** a node has `highway=milestone`, `distance=35.0 nmi`, and `ref=A2`
- **THEN** a warning SHALL be logged AND the distance field SHALL remain at 0 (parsing continues for other tags)
