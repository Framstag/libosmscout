## Context

`DescribeLocationByHighwayMilestone` in `LocationDescriptionService.cpp` (lines 1508-1826) has a compile-error bug: `nodeSearchResult` at line 1572 is undefined in function scope. Beyond that, the algorithm is hard to read and maintain:

- The `walkDirection` lambda (100+ lines) mixes node iteration with way-jumping at junctions, same-name filtering, and distance checks in a single deeply-nested structure
- `RefFeatureLabelReader` is constructed but never used
- Seed node selection for forward/backward walks is confusing (uses `closestSegmentIdx +/- 1` depending on `closestR`)
- Two nearly-identical fallback loops for finding connected ways at junctions (lines 1659-1686 and 1694-1715)

The algorithm SHOULD: find the closest way, project location onto it, then walk its nodes in both directions looking for milestones. At way endpoints, check if connected ways (sharing the endpoint node ID) exist. If exactly one other way connects and shares the same road name, continue traversal on that way. Stop at crossings (>2 ways at one node). Stop when all nodes within the distance threshold are exhausted.

## Goals / Non-Goals

**Goals:**
- Fix the `nodeSearchResult` compile error — add proper `LoadNodesInRadius` call with milestone types
- Clean rewrite of `walkDirection` with simpler structure
- Keep connectivity traversal: walk nodes on same way + follow same-name connected ways at junctions
- Traverse both directions (forward toward way end, backward toward way start) from the projection point
- Track visited node `Id`s to prevent cycles
- Stop at crossings (>2 ways share a node) — milestone past crossing has ambiguous direction
- Only consider ways whose segment distance to location is within `milestoneLookupDistance`
- Same method signature — no public API change
- All existing milestone tests pass; update tests that relied on implementation details

**Non-Goals:**
- No new public API or class changes
- No changes to `LocationHighwayMilestoneDescription` data model (already has previous/next fields)
- No changes to other description types (way, crossing, at-place, etc.)
- No routing data usage — only coordinate/node-ID based connectivity

## Decisions

### D1: Walk helper returns milestone node pointer, not iterator position

**Decision**: `walkDirection` returns `const Node*` (pointer to milestone node in `milestoneNodes` map) or `nullptr` if none found.

**Rationale**: Current code already does this. The caller needs the node pointer to read milestone feature values (distance, ref) and compute bearing/distance. A node index would require a second lookup.

**Alternative considered**: Return `FileOffset` and look up later. Rejected — adds indirection with no benefit.

### D2: `milestoneNodes` indexed by `FileOffset` (node serial), not node `Id`

**Decision**: `milestoneNodes` uses `std::map<FileOffset, const Node*>` keyed by `GetSerial()`.

**Rationale**: FileOffset is the unique persistent identifier for the loaded node object. Node `Id` (OSM node ID) is used only for connectivity matching between ways. Using FileOffset for milestone lookup avoids confusion between the two ID systems.

**Alternative considered**: Index by `Id`. Rejected — `Id` is 0 for generated nodes (not in OSM source), and milestone nodes always have non-zero serial from loading.

### D3: Connectivity map by node `Id`, not coordinate matching

**Decision**: `nodeConnections` maps `Id` (OSM node ID from `GetId()`) to `vector<WayEntry>`. Only nodes with non-zero `Id` are indexed.

**Rationale**: Coordinate matching is imprecise (floating point). OSM node IDs are the canonical way to identify shared junction nodes between ways. Nodes with `Id==0` are intermediate generated points that don't form junctions.

**Alternative considered**: Coordinate-based matching. Rejected — floating point precision issues at junctions; OSM node IDs are exact.

### D4: Crossing detection = >2 ways at same node

**Decision**: A crossing is when `nodeConnections[nodeId].size() > 2` (the current way + 2+ other ways). At crossings, traversal stops.

**Rationale**: A crossing with 3+ ways (including current) means the location heading is ambiguous. The current way's connection count is 1 (itself), so `size() > 2` means at least 2 other ways connect.

**Alternative considered**: `size() >= 2` (any junction). Rejected — a T-junction has exactly one other way, which is safe to traverse if names match.

**Alternative considered**: Stop when name doesn't match. Rejected — name match is already checked; crossing detection is a separate safety stop.

### D5: Same-name matching at junctions

**Decision**: When reaching a way endpoint and a connected way exists (exactly one other way at that node), follow it ONLY if the connected way has the same name as the currently traversed way. Name is read via `NameFeatureLabelReader`.

**Rationale**: A road that changes name (e.g., "Main St" → "Oak Ave") likely represents a different road, not a continuation. Milestones reference the road name.

**Alternative considered**: Follow any connected way. Rejected — would lead to milestones on side roads that aren't the same route.

### D6: Forward/backward seed node selection

**Decision**: Seed nodes are the neighboring nodes of the projection segment:
- Forward walk starts from `closestSegmentIdx + 1` (end node of segment)
- Backward walk starts from `closestSegmentIdx` (start node of segment)
- The projection point lies between these two nodes

**Rationale**: The projection is somewhere along the segment between `closestSegmentIdx` and `closestSegmentIdx + 1`. Walking forward from `closestSegmentIdx + 1` moves toward the way's end; backward from `closestSegmentIdx` moves toward the way's start. This cleanly splits the node sequence at the projection point without needing to insert a virtual node.

**Why not use `closestR` to choose seeds?**: Current code swaps seeds based on `closestR < 0.5`, but this is unnecessary — the distance of the projection along the segment doesn't change which node indices bound it. The projection is always between segment nodes, so seeds are always the segment's start and end nodes.

### D7: Distance check on connected ways

**Decision**: When following a connected way, verify that the joining node's coordinate is within `milestoneLookupDistance` of the location. All nodes on the connected way are assumed to be at a similar distance.

**Rationale**: Simple heuristic to avoid traversing ways that are far from the location. The connected way's first node is at the junction point, which is close to the location if the junction is within range.

## Algorithm Outline

```
1. Precondition: wayDescription exists, else return true (no milestone description)
2. Collect milestone types (node types with HighwayMilestoneFeature)
   → if none exist, return true
3. Load milestone nodes in radius:
   database->LoadNodesInRadius(location, milestoneTypes, milestoneLookupDistance)
   → build milestoneNodes map: FileOffset → Node*
4. Load ALL ways in radius:
   database->LoadWaysInRadius(location, allWayTypes, milestoneLookupDistance)
   → build nodeConnections map: Id → [(WayRef, nodeIndex, name), ...]
5. Get closest way from wayDescription
6. Project location onto closest way → find closest segment + parametric t
7. Seed nodes: startForward = segmentEndIdx, startBackward = segmentStartIdx
8. Walk forward from startForward:
   - For each node, check if FileOffset is in milestoneNodes → return milestone
   - Move to next node (increasing index)
   - At way end: look up last node Id in nodeConnections
   - If only current way found → return nullptr (dead end)
   - If 2+ other ways → return nullptr (crossing)
   - If exactly 1 other way with same name AND within distance → switch to that way
   - If no same-name way found → return nullptr
9. Walk backward (same logic, decreasing index)
10. Construct LocationHighwayMilestoneDescription:
    - If both previous and next found and different → two-milestone
    - If only one → single-milestone fallback
    - If location == milestone coord → at-place
```

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| `LoadNodesInRadius` returns many non-milestone nodes | Filter by `milestoneTypes` type set before loading; milestone types are sparse |
| Node `Id` == 0 (generated/intermediate nodes) | Connectivity map only indexes nodes with `Id != 0`; walks continue past generated nodes |
| Connected way has different name at junction but same route (e.g., ref-based matching) | Milestone spec references route ref, not name. If needed, future enhancement could add ref matching. For now, name matching is correct for same-road detection |
| Cyclic way connections (way A→B→A) | `visitedNodeIds` set tracking prevents revisiting nodes |
| `milestoneLookupDistance` includes too many ways | The outer distance threshold bounds the search; connected ways are only followed if their first node is within this distance |