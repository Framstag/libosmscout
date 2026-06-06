## MODIFIED Requirements

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
4. SHALL build a connectivity map by matching way start/end node **IDs** (`GetId()`). Nodes with `Id==0` (intermediate generated nodes) SHALL NOT be indexed.
5. SHALL load all milestone nodes (node types with `HighwayMilestoneFeature`) within `milestoneLookupDistance` via `Database::LoadNodesInRadius`. Milestone nodes SHALL be indexed by `FileOffset` (node serial) for O(1) lookup during traversal.
6. SHALL project the location onto the closest way to find the nearest segment and parametric position `t`.
7. SHALL traverse the closest way's nodes in **both directions** from the projection:
   - **Forward** (increasing node index toward way end) → finds the **next** milestone
   - **Backward** (decreasing node index toward way start) → finds the **previous** milestone
8. At each node during traversal, SHALL check if the node's `FileOffset` (`GetSerial()`) exists in the milestone index. If found, the node is a milestone → return it.
9. At way endpoints, SHALL look up the endpoint node `Id` in the connectivity map:
   - If only the current way is found at that node (no junction) → stop traversal (return nullptr)
   - If exactly one other way connects at that node (T-junction) → follow it ONLY if it has the **same name** as the current way AND its first node is within `milestoneLookupDistance`
   - If two or more other ways connect (crossing with >2 ways) → stop traversal (ambiguous direction)
10. SHALL track visited node `Id`s in a `std::set<Id>` to prevent cycles.
11. SHALL NOT use routing data for connectivity — only node ID-based matching.
12. SHALL NOT call `ReverseLookupObject` — highway milestones never resolve to a `Place`. Use `ObjectFileRef` and `FeatureValueBuffer` from the node directly.
13. If both a previous and next milestone are found and they are different nodes → SHALL set `LocationHighwayMilestoneDescription` with both `previous*` and `next*` fields populated.
14. If only one milestone is found, or both point to the same node → SHALL fallback to single-milestone description using that milestone's data in `previous*` fields.
15. If the location coordinate matches a milestone node's coordinate → `IsAtPlace()` SHALL return `true`.

#### Scenario: Precondition — no way description available
- **WHEN** `DescribeLocationByHighwayMilestone` is called but `description.GetWayDescription()` is null
- **THEN** method returns `true` and `GetHighwayMilestoneDescription()` remains null

#### Scenario: Milestone found on closest way
- **WHEN** the closest way has a node with `HighwayMilestoneFeature` 50m from the location
- **THEN** `GetHighwayMilestoneDescription()` SHALL be non-null with `GetDistance()` returning 50m

#### Scenario: No milestones in connected network
- **WHEN** no way in the connected network (within 2000m) has nodes with `HighwayMilestoneFeature`
- **THEN** method returns `true` and `GetHighwayMilestoneDescription()` remains null

#### Scenario: Cycle protection — same node visited twice
- **WHEN** the road network has a loop (way A and way B share both start and end nodes)
- **THEN** traversal SHALL NOT revisit already-traversed nodes, SHALL terminate without infinite loop

#### Scenario: Milestone beyond milestoneLookupDistance
- **WHEN** the closest milestone is 3000m away but `milestoneLookupDistance` is 2000m
- **THEN** the milestone is not included and `GetHighwayMilestoneDescription()` remains null

#### Scenario: Crossing boundary — stop at >2 way junction
- **WHEN** the road network has a crossing where 3+ ways share a node
- **THEN** traversal SHALL NOT cross that node; milestones on the other side SHALL NOT be considered

#### Scenario: Both previous and next milestones found
- **WHEN** the location projection is between two milestone nodes on the same way
- **THEN** `GetPreviousMilestoneDistance()` returns the backward milestone's distance and `GetNextMilestoneDistance()` returns the forward milestone's distance, and `IsBetweenMilestones()` returns `true`

#### Scenario: Only one milestone on way
- **WHEN** only one node with `HighwayMilestoneFeature` exists on the traversed way network
- **THEN** single-milestone fallback: `GetPreviousMilestoneRef()` contains the milestone ref and `IsBetweenMilestones()` returns `false`

## REMOVED Requirements

### Requirement: Milestone found on connected way via BFS
**Reason**: Replaced by forward/backward node traversal with same-name way following. The new algorithm traverses connected ways at T-junctions when they share the same road name, which covers the connected-way scenario more precisely.
**Migration**: Tests that assumed BFS-based connected-way discovery (any connected way regardless of name) should be updated to test same-name way following instead.

### Requirement: Use of `lookupDistance` parameter
**Reason**: The method signature has only 3 parameters — the `lookupDistance` parameter from the original spec was incorrect. The method uses `milestoneLookupDistance` for all distance checks.
**Migration**: Remove any references to the 4-parameter signature. Use `milestoneLookupDistance` (default 2000m) exclusively.