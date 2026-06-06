## Why

`DescribeLocationByHighwayMilestone` has a critical bug: line 1572 references `nodeSearchResult` which is never defined in the function scope — this code does not compile. Beyond the compile error, the `walkDirection` lambda is overly complex: it mixes node walking with way-jumping logic, uses unclear direction-to-node-index mapping, and re-loads way data in a way that's fragile. The algorithm needs a clean rewrite that properly loads milestone nodes, traverses the closest way's nodes in both directions, and follows connected ways with the same name at junctions.

## What Changes

- **Fix bug**: Add `database->LoadNodesInRadius(location, milestoneTypes, milestoneLookupDistance)` to properly load milestone nodes
- **Rewrite** `walkDirection` logic:
  - Traverse closest way nodes forward (toward end) and backward (toward start) from the projection point
  - At way endpoints, look up connected ways by **node ID** (`GetId()`) — match start/end nodes
  - At crossings with >2 ways sharing a node, stop traversal (ambiguous direction)
  - At crossings with exactly one other way, follow it if it has the **same name** as the currently traversed way
  - Track visited nodes (`std::set<Id>`) to prevent cycles
  - Only traverse ways whose segments are within `milestoneLookupDistance`
- **Remove** unnecessary components: `RefFeatureLabelReader`, unused struct fields
- Keep same API signature — no public API changes
- Update tests: ensure all corner cases pass with rewritten logic

## Capabilities

### New Capabilities
*(none)*

### Modified Capabilities
- `highway-milestone-description`: Requirements reorganized. BFS removed in favor of simple forward/backward node walk with same-name way following at junctions. New requirement: milestone nodes loaded via `LoadNodesInRadius` with milestone types.

## Impact

- `libosmscout/src/osmscout/location/LocationDescriptionService.cpp` — rewrite `DescribeLocationByHighwayMilestone` body
- `Tests/src/LocationDescriptionServiceTest.cpp` — update tests for fixed algorithm
- `openspec/specs/highway-milestone-description/spec.md` — delta spec reflecting corrected requirements