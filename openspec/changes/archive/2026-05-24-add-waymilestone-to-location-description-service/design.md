## Context

`LocationDescriptionService` has `DescribeLocationByWay()` which finds the closest named way and stores it in `LocationDescription::wayDescription`. A previous change added `HighwayMilestoneFeature` to capture OSM `highway=milestone` node data (`distance`, `ref`, `carriageway_ref`, `marker`). Currently no code connects these two: you can find a nearby way or a nearby POI, but you cannot answer "where am I relative to the nearest highway milestone on this road".

## Goals / Non-Goals

**Goals:**
- New `LocationHighwayMilestoneDescription` class storing the milestone `Place`, distance, bearing, and `HighwayMilestoneFeatureValue` data
- New `DescribeLocationByHighwayMilestone()` method on `LocationDescriptionService` with new `milestoneLookupDistance` parameter
- Precondition: only runs if `DescribeLocationByWay` already populated `wayDescription`; reuses that way
- Way network traversal by matching start/end node coordinates (no routing engine), limited by distance from location, no hop limit
- Track visited nodes in set to avoid cycles
- Find nearest node with `HighwayMilestoneFeature` along the connected way network
- New `SetHighwayMilestoneDescription()`/`GetHighwayMilestoneDescription()` on `LocationDescription`
- Update `DescribeLocation()` to call new method after `DescribeLocationByWay`
- Update demo app to dump the new description type

**Non-Goals:**
- No changes to `HighwayMilestoneFeature` itself
- No routing-based traversal — purely coordinate-based node matching
- No indexing changes — milestones are not indexed as addresses or POIs
- No changes to map rendering or style sheets

## Decisions

### Decision 1: New class vs inline in existing types

**Chosen**: New `LocationHighwayMilestoneDescription` class alongside existing `LocationWayDescription`, `LocationCrossingDescription`.

- Follows existing pattern — each description variant is its own class
- Parallel structure means consistent getters (`GetPlace`, `GetDistance`, `GetBearing`) plus milestone-specific accessors
- Alternative (inline in a generic class) breaks the pattern and loses type safety

### Decision 2: Way network traversal by coordinate matching with visited-set cycle detection

```
sequenceDiagram
    participant Caller
    participant LDS as LocationDescriptionService
    participant DB as Database

    Caller->>LDS: DescribeLocationByHighwayMilestone(loc, desc, milestoneLookupDistance)
    LDS->>LDS: Check desc.wayDescription exists
    alt No way description
        LDS-->>Caller: return true (no milestone result)
    end
    LDS->>LDS: Get closest way from wayDescription
    LDS->>DB: LoadWaysInRadius(loc, wayTypes, lookupDistance)
    DB-->>LDS: waySearchResult
    LDS->>LDS: Build map: node coord → set of WayRef
    LDS->>LDS: BFS from closest way's endpoints
    Note over LDS: Track visited nodes in std::set<Point><br/>Skip already-visited nodes<br/>Stop exploring way if all its nodes beyond milestoneLookupDistance
    LDS->>DB: LoadNodesInRadius (only milestone types)
    DB-->>LDS: nodeSearchResult
    LDS->>LDS: Filter nodes: must be on connected ways and within milestoneLookupDistance
    LDS->>LDS: Pick closest by distance
    LDS->>DB: ReverseLookupObject(milestoneNode)
    DB-->>LDS: lookupResult
    LDS->>LDS: Build LocationHighwayMilestoneDescription
    LDS-->>Caller: return true
```

- Load all ways in the radius
- Build a map from node coordinate to set of ways sharing that node
- The closest way's start/end nodes are the seed for BFS
- BFS follows connected ways in both directions, tracking visited nodes in a set
- If a node has been visited, skip it (cycle protection)
- Only traverse ways whose closest segment distance to location is within `milestoneLookupDistance`
- Stop traversal at crossings: a node where >2 ways connect is a crossing. Traversal does not cross it. A milestone beyond a crossing has ambiguous directionality — you may not have passed or may not pass it.
- No routing required — just coordinate-based topology

### Decision 3: `LocationHighwayMilestoneDescription` stores the milestone feature value

**Chosen**: Use `ObjectFileRef` + `FeatureValueBuffer` instead of `Place`. Highway milestones never resolve to a `Place` via reverse lookup — the `ReverseLookupObject` call crashes on empty result. Store raw object ref and feature buffer instead.

- Callers need the milestone distance/ref without extra database lookups
- API consumers don't need to include the feature header

### Decision 4: Only node search for milestone types

**Chosen**: Search only node types that have `HighwayMilestoneFeature` (not all nodes).

```cpp
TypeInfoSet milestoneTypes;
for (const auto& type : typeConfig->GetTypes()) {
  if (type->CanBeNode() &&
      type->HasFeature(HighwayMilestoneFeature::NAME)) {
    milestoneTypes.Set(type);
  }
}
```

- Milestones are node-only per OSM wiki (`highway=milestone`)
- Narrow search = faster, fewer false positives

### Decision 5: New `milestoneLookupDistance` parameter

**Chosen**: Add `milestoneLookupDistance` parameter to `DescribeLocationByHighwayMilestone()`. Default: 2000m (2km) — milestones are sparser than ways, so a wider search radius is needed.

- Controls how far along the connected way network to explore for milestones
- BFS cutoff: a way is included if any part of it (closest segment distance to location) is within this distance
- Independent from the way lookup distance — milestones may be farther than the way itself

## Risks / Trade-offs

- **Cyclic traversal** → Track visited nodes in set, skip already-traversed nodes. Break when all reachable nodes within distance limit exhausted.
- **Limit by distance from location** → BFS explores connected ways but only considers milestone nodes within `milestoneLookupDistance` of the original location. Once a way's nearest point is beyond this distance, stop traversing further along that branch.
- **Coordinate tolerance** → Node coordinates from different ways may not match exactly due to rounding. Mitigation: use `IsIdentical()` on the `Point` type or coordinate comparison within epsilon.
- **No milestone found in connected network** → Method returns true with no description set (graceful degradation).
- **Performance** → Loading all ways in radius + BFS adds latency vs simple radius search. Mitigation: the operation is already bounded by `milestoneLookupDistance` (new parameter).