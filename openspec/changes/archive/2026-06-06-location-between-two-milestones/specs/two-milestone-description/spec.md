## ADDED Requirements

### Requirement: Describe location between two highway milestones
The system SHALL describe a geographic location as between two highway milestones on a way, when two milestones bracket the location along the way topology.

#### Scenario: Location between two milestones on same way
- **WHEN** `DescribeLocationByHighwayMilestone` is called with a location that lies between two highway milestones on the same way
- **THEN** the result SHALL include `previousMilestoneDistance`, `previousMilestoneRef`, `previousMilestoneCarriagewayRef` (behind location) and `nextMilestoneDistance`, `nextMilestoneRef`, `nextMilestoneCarriagewayRef` (ahead of location)

#### Scenario: Location before first milestone
- **WHEN** the location projects before the first highway milestone on a way (no milestone behind it)
- **THEN** the result SHALL fall back to single-milestone description using the first milestone ahead

#### Scenario: Location after last milestone
- **WHEN** the location projects past the last highway milestone on a way (no milestone ahead)
- **THEN** the result SHALL fall back to single-milestone description using the last milestone behind

#### Scenario: Only one milestone found
- **WHEN** only one highway milestone is found within `milestoneLookupDistance`
- **THEN** the result SHALL fall back to single-milestone description

#### Scenario: Location exactly at a milestone
- **WHEN** the location coordinate matches a highway milestone node coordinate
- **THEN** the result SHALL indicate "at milestone X" (previous and next milestone are the same; `IsBetweenMilestones()` returns false)

### Requirement: Milestone ordering by way topology
Milestone ordering SHALL use way node traversal order, not straight-line distance from location.

#### Scenario: Previous and next determined by way node order
- **WHEN** two milestones exist on the same way but the farther-by-way milestone is closer by Euclidean distance
- **THEN** the previous milestone SHALL be the one preceding the location in way node order, and the next milestone SHALL be the one following it, regardless of Euclidean distance



### Requirement: Milestone distance display
The human-readable description SHALL display the milestone distance value (from OSM highway=milestone `distance` tag) for each milestone.

#### Scenario: Both milestones have distance tags
- **WHEN** both milestones have `distance` tag values
- **THEN** display as "between milestone A2 (35m) and milestone A3 (50m)"

#### Scenario: One or both milestones missing distance tag
- **WHEN** a milestone has no `distance` tag
- **THEN** display the milestone ref without a distance value

### Requirement: MCP server JSON output
The MCP server SHALL serialize two-milestone descriptions in JSON, maintaining backward compatibility for single-milestone fields.

#### Scenario: Two-milestone JSON structure
- **WHEN** two milestones are found on the same way
- **THEN** the JSON SHALL include `previousMilestoneDistance`, `previousMilestoneRef`, `previousMilestoneCarriagewayRef`
- **AND** SHALL include `nextMilestoneDistance`, `nextMilestoneRef`, `nextMilestoneCarriagewayRef`

#### Scenario: Single-milestone fallback JSON
- **WHEN** only one milestone is found (fallback)
- **THEN** the JSON SHALL include `previousMilestoneDistance`, `previousMilestoneRef`, `previousMilestoneCarriagewayRef`
- **AND** SHALL NOT include `next*` milestone fields

### Requirement: HighwayMilestoneDescription class API
The `LocationHighwayMilestoneDescription` class SHALL support both single and two-milestone states.

#### Scenario: Two-milestone state
- **WHEN** a two-milestone description is created with both previous and next milestones
- **THEN** `IsBetweenMilestones()` SHALL return true (computed: both milestones set and different)
- **AND** `GetPreviousMilestoneDistance()`, `GetPreviousMilestoneRef()`, `GetPreviousMilestoneCarriagewayRef()` SHALL return the previous milestone data
- **AND** `GetNextMilestoneDistance()`, `GetNextMilestoneRef()`, `GetNextMilestoneCarriagewayRef()` SHALL return the next milestone data

#### Scenario: Single-milestone state (backward compatible)
- **WHEN** only a single milestone is available
- **THEN** `IsBetweenMilestones()` SHALL return false
- **AND** `GetPreviousMilestoneDistance()`, `GetPreviousMilestoneRef()`, `GetPreviousMilestoneCarriagewayRef()` SHALL return that milestone's data
- **AND** `GetNextMilestoneDistance()`, `GetNextMilestoneRef()`, `GetNextMilestoneCarriagewayRef()` SHALL return empty/zero

### Requirement: Demo output update
The `LocationDescription` demo SHALL display both milestones when two-milestone description is available.

#### Scenario: Demo prints both milestones
- **WHEN** `DumpHighwayMilestoneDescription` receives a two-milestone description
- **THEN** it SHALL print both previous and next milestone info