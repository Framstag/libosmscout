## ADDED Requirements

### Requirement: LocationHighwayMilestoneDescription class

The system SHALL provide a `LocationHighwayMilestoneDescription` class (parallel to `LocationWayDescription`, `LocationCrossingDescription`) that stores the description of a location relative to a nearby highway milestone node.

The class SHALL expose:
- `GetObject()` — the `ObjectFileRef` of the milestone node
- `GetObjectFeatures()` — the `FeatureValueBufferRef` of the milestone node
- `GetDistance()` — distance from location to the milestone node in meters
- `GetBearing()` — bearing from the milestone node toward the location
- `GetMilestoneDistance()` — the milestone's distance value (uint32_t)
- `GetMilestoneRef()` — the milestone's route reference (e.g., "A2")
- `GetMilestoneCarriagewayRef()` — the carriageway reference (optional)
- `IsAtPlace()` — `true` if the location is within epsilon of the milestone node itself

#### Scenario: Constructor with at-place coordinates
- **WHEN** the location equals the milestone node coordinate
- **THEN** `IsAtPlace()` SHALL return `true` and distance SHALL be zero

#### Scenario: Constructor with non-zero distance
- **WHEN** the location is 50m from the milestone node
- **THEN** `GetDistance()` SHALL return 50m and `IsAtPlace()` SHALL return `false`

#### Scenario: Milestone feature value accessible
- **WHEN** the milestone node has `HighwayMilestoneFeature` with distance=35, ref="A2"
- **THEN** `GetMilestoneDistance()` SHALL return 35 and `GetMilestoneRef()` SHALL return "A2"

### Requirement: DescribeLocationByHighwayMilestone method

The system SHALL provide `DescribeLocationByHighwayMilestone()` method on `LocationDescriptionService` with signature:

```cpp
bool DescribeLocationByHighwayMilestone(const GeoCoord& location,
                                         LocationDescription& description,
                                         const Distance& lookupDistance=Distance::Of<Meter>(100),
                                         const Distance& milestoneLookupDistance=Distance::Of<Meter>(2000));
```

The behavior:
1. The method SHALL check if `description.GetWayDescription()` is non-null (set by prior `DescribeLocationByWay` call). If null, return `true` with no highway milestone description set.
2. The method SHALL reuse the closest way from the way description — it SHALL NOT recalculate the closest way.
3. The method SHALL load all ways in the area of the given `lookupDistance`.
4. The method SHALL build a connectivity map by matching way start/end node coordinates.
5. The method SHALL perform BFS traversal starting from the closest way's endpoints, tracking visited nodes in a set to avoid cycles. The method SHALL stop at crossings: a node where >2 ways connect SHALL NOT be traversed (milestone past crossing has ambiguous directionality).
6. The method SHALL only traverse ways whose closest segment distance to the location is within `milestoneLookupDistance` (default 2000m).
7. The method SHALL search for node types that have `HighwayMilestoneFeature` via `Database::LoadNodesInRadius`.
8. The method SHALL filter milestone nodes to those that belong to a traversed connected way.
9. The method SHALL pick the closest milestone node by distance.
10. The method SHALL set `description.SetHighwayMilestoneDescription()` if a milestone is found, or leave it unset (null) if none are found within range.
11. The method SHALL NOT use routing data for connectivity — only coordinate-based node matching.
12. The method SHALL NOT call `ReverseLookupObject` — highway milestones never resolve to a `Place`. Use `ObjectFileRef` and `FeatureValueBuffer` from the node directly.

#### Scenario: Precondition — no way description available
- **WHEN** `DescribeLocationByHighwayMilestone` is called but `description.GetWayDescription()` is null
- **THEN** method returns `true` and `GetHighwayMilestoneDescription()` remains null

#### Scenario: Milestone found on closest way
- **WHEN** the closest way has a node with `HighwayMilestoneFeature` 50m from the location
- **THEN** `GetHighwayMilestoneDescription()` SHALL be non-null with `GetDistance()` returning 50m

#### Scenario: Milestone found on connected way via BFS
- **WHEN** the closest way has no milestones, but a connected way (sharing a node) at 100m from location has a milestone
- **THEN** `GetHighwayMilestoneDescription()` SHALL be non-null and reference that milestone

#### Scenario: No milestones in connected network
- **WHEN** no way in the connected network (within 2000m) has nodes with `HighwayMilestoneFeature`
- **THEN** method returns `true` and `GetHighwayMilestoneDescription()` remains null

#### Scenario: Cycle protection — same node visited twice
- **WHEN** the road network has a loop (way A and way B share both start and end nodes)
- **THEN** the BFS SHALL NOT revisit already-traversed nodes, SHALL terminate without infinite loop

#### Scenario: Milestone beyond milestoneLookupDistance
- **WHEN** the closest milestone is 3000m away but `milestoneLookupDistance` is 2000m
- **THEN** the milestone is not included and `GetHighwayMilestoneDescription()` remains null

#### Scenario: Crossing boundary — stop at >2 way junction
- **WHEN** the road network has a crossing where 3+ ways share a node
- **THEN** traversal SHALL NOT cross that node; milestones on the other side SHALL NOT be considered

### Requirement: LocationDescription extended with highway milestone

The system SHALL extend `LocationDescription` with:
- `void SetHighwayMilestoneDescription(const LocationHighwayMilestoneDescriptionRef& description)`
- `LocationHighwayMilestoneDescriptionRef GetHighwayMilestoneDescription() const`

The `DescribeLocation()` method SHALL call `DescribeLocationByHighwayMilestone()` after `DescribeLocationByWay()`.

#### Scenario: DescribeLocation composes highway milestone
- **WHEN** `DescribeLocation()` is called and a way description is found with a nearby milestone
- **THEN** `GetHighwayMilestoneDescription()` SHALL be non-null in the result

### Requirement: Demo app displays highway milestone description

The `LocationDescription.cpp` demo SHALL display the highway milestone description when present, following the same pattern as way description and crossing description display. Output SHALL include:
- Node id (file offset)
- Distance and bearing
- Milestone feature values (distance, ref, carriageway_ref)

#### Scenario: Demo output with milestone
- **WHEN** the demo runs at a location with a valid highway milestone description
- **THEN** output SHALL include a "Nearest highway milestone" section with node id, distance, bearing, and milestone feature data