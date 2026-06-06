## Purpose

The `highway-milestone-description` capability extends `LocationDescriptionService` with `DescribeLocationByHighwayMilestone()`, which finds the nearest node with `HighwayMilestoneFeature` along the connected road network near a location. The result is stored in `LocationDescription` as a `LocationHighwayMilestoneDescription` containing the node's `ObjectFileRef`, `FeatureValueBufferRef`, distance, bearing, and milestone data (distance, ref, carriageway_ref) as plain fields — no dependency on the feature system in the public API.

## Requirements

### Requirement: LocationHighwayMilestoneDescription class

The system SHALL provide a `LocationHighwayMilestoneDescription` class (parallel to `LocationWayDescription`, `LocationCrossingDescription`) that stores the description of a location relative to a nearby highway milestone node.

The class SHALL expose:
- `GetObject()` — the `ObjectFileRef` of the milestone node
- `GetObjectFeatures()` — the `FeatureValueBufferRef` of the milestone node
- `GetDistance()` — distance from location to the milestone node in meters
- `GetBearing()` — bearing from the milestone node toward the location
- `GetPreviousMilestoneDistance()` — the milestone's distance value (uint32_t)
- `GetPreviousMilestoneRef()` — the milestone's route reference (e.g., "A2")
- `GetPreviousMilestoneCarriagewayRef()` — the carriageway reference (optional)
- `GetNextMilestoneDistance()` — the forward milestone's distance value (uint32_t)
- `GetNextMilestoneRef()` — the forward milestone's route reference
- `GetNextMilestoneCarriagewayRef()` — the forward carriageway reference (optional)
- `IsBetweenMilestones()` — `true` when both previous and next milestones are set and refer to different physical nodes
- `IsAtPlace()` — `true` if the location is within epsilon of the milestone node itself

#### Scenario: Constructor with at-place coordinates
- **WHEN** the location equals the milestone node coordinate
- **THEN** `IsAtPlace()` SHALL return `true` and distance SHALL be zero

#### Scenario: Constructor with non-zero distance
- **WHEN** the location is 50m from the milestone node
- **THEN** `GetDistance()` SHALL return 50m and `IsAtPlace()` SHALL return `false`

#### Scenario: Milestone feature value accessible
- **WHEN** the milestone node has `HighwayMilestoneFeature` with distance=35, ref="A2"
- **THEN** `GetPreviousMilestoneDistance()` SHALL return 35 and `GetPreviousMilestoneRef()` SHALL return "A2"

### Requirement: DescribeLocationByHighwayMilestone method

The system SHALL provide `DescribeLocationByHighwayMilestone()` method on `LocationDescriptionService` with signature:

```cpp
bool DescribeLocationByHighwayMilestone(const GeoCoord& location,
                                        LocationDescription& description,
                                        const Distance& milestoneLookupDistance=Distance::Of<Meter>(2000));
```

The behavior:
1. SHALL check if `description.GetWayDescription()` is non-null (set by prior `DescribeLocationByWay` call). If null, return `true` with no highway milestone description set.
2. SHALL reuse the closest way from the way description — it SHALL NOT recalculate the closest way.
3. SHALL load all ways in the area of `milestoneLookupDistance` via `Database::LoadWaysInRadius`.
4. SHALL build a connectivity map by matching way node IDs (`GetId()`). Nodes with `Id==0` SHALL NOT be indexed.
5. SHALL load all milestone nodes (node types with `HighwayMilestoneFeature`) within `milestoneLookupDistance` via `Database::LoadNodesInRadius`.
6. SHALL project the location onto the closest way to find the nearest segment and parametric position `t`.
7. SHALL traverse the closest way's nodes in both directions from the projection:
   - Forward (increasing node index toward way end) — finds the next milestone
   - Backward (decreasing node index toward way start) — finds the previous milestone
8. At each node during traversal, SHALL check if the node's coordinate matches a milestone coordinate (epsilon match).
9. At way endpoints, SHALL look up the endpoint node Id in the connectivity map:
   - If only the current way is found → stop traversal
   - If exactly one other way connects (T-junction) → follow it ONLY if it has the same name as the current way AND its joining node is within `milestoneLookupDistance`. Can connect at any node position.
   - If two or more other ways connect (crossing) → stop traversal
10. SHALL track visited node Ids in a `std::set<Id>` to prevent cycles.
11. SHALL NOT use routing data for connectivity — only node ID-based matching.
12. SHALL NOT call `ReverseLookupObject` — highway milestones never resolve to a `Place`.
13. If both a previous and next milestone are found and they are different nodes → SHALL populate both `previous*` and `next*` fields.
14. If only one milestone is found, or both point to the same node → SHALL fallback to single-milestone description using `previous*` fields.
15. If the location matches a milestone node's coordinate → `IsAtPlace()` SHALL return true.

#### Scenario: Precondition — no way description available
- WHEN DescribeLocationByHighwayMilestone is called but way description is null
- THEN method returns true with no milestone description

#### Scenario: Milestone found on closest way
- WHEN the closest way has a node with HighwayMilestoneFeature 50m from the location
- THEN GetHighwayMilestoneDescription() is non-null with GetDistance() returning 50m

#### Scenario: No milestones in connected network
- WHEN no way within 2000m has nodes with HighwayMilestoneFeature
- THEN method returns true with no milestone description

#### Scenario: Cycle protection — same node visited twice
- WHEN the road network has a loop
- THEN traversal SHALL NOT revisit already-traversed nodes

#### Scenario: Milestone beyond milestoneLookupDistance
- WHEN the closest milestone is 3000m away but milestoneLookupDistance is 2000m
- THEN the milestone is not included

#### Scenario: Crossing boundary — stop at >2 way junction
- WHEN 3+ ways share a node
- THEN traversal SHALL NOT cross that node

#### Scenario: Both previous and next milestones found
- WHEN the location is between two milestone nodes on the same way
- THEN GetPreviousMilestoneDistance() and GetNextMilestoneDistance() are set, IsBetweenMilestones() returns true

#### Scenario: Only one milestone on way
- WHEN only one milestone exists on the traversed way network
- THEN single-milestone fallback: IsBetweenMilestones() returns false
### Requirement: LocationDescription extended with highway milestone

The system SHALL extend `LocationDescription` with:
- `void SetHighwayMilestoneDescription(const LocationHighwayMilestoneDescriptionRef& description)`
- `LocationHighwayMilestoneDescriptionRef GetHighwayMilestoneDescription() const`

The `DescribeLocation()` method SHALL call `DescribeLocationByHighwayMilestone()` after `DescribeLocationByWay()`.

#### Scenario: DescribeLocation composes highway milestone
- **WHEN** `DescribeLocation()` is called and a way description is found with a nearby milestone
- **THEN** `GetHighwayMilestoneDescription()` SHALL be non-null in the result

### Requirement: Demo app displays highway milestone description

The `LocationDescription.cpp` demo SHALL display the highway milestone description when present. Output SHALL include:
- Node id (file offset)
- Distance and bearing
- When `IsBetweenMilestones()` is true: previous and next milestone feature values (distance, ref, carriageway_ref)
- When `IsBetweenMilestones()` is false: single milestone feature values (distance, ref, carriageway_ref)

#### Scenario: Demo output with milestone
- **WHEN** the demo runs at a location with a valid highway milestone description
- **THEN** output SHALL include a "Nearest highway milestone" section with node id, distance, bearing, and milestone feature data