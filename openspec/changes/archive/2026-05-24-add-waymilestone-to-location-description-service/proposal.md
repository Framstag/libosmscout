## Why

`HighwayMilestoneFeature` exists (previous change) but location reverse lookup cannot describe a location in relation to a nearby highway milestone node. Drivers approaching a highway exit need structured position info like "50m before milestone 35 on A2" rather than just "near A2 road". Adding `DescribeLocationByHighwayMilestone` fills this gap using the existing `DescribeLocationByWay` foundation.

## What Changes

- New `LocationHighwayMilestoneDescription` class (parallel to `LocationWayDescription`, `LocationCrossingDescription`) holding `ObjectFileRef`, `FeatureValueBuffer`, distance, bearing, and milestone parameters (distance/ref/carriageway_ref) as plain fields — no `Place`, no dependency on `HighwayMilestoneFeatureValue`
- New `DescribeLocationByHighwayMilestone()` method on `LocationDescriptionService`. Called from `DescribeLocation()` only after `DescribeLocationByWay()` sets a way description.
- New `atHighwayMilestoneDescription` field on `LocationDescription` with setter/getter
- Precondition: `HighwayMilestoneDescription` only generated if `LocationWayDescription` exists. Reuses its closest way — no recalculation.
- Logic: take closest way from `DescribeLocationByWay`, load all ways in area by matching start/end nodes (no routing), traverse connected ways via BFS. At crossings (node with >2 connected ways) stop traversal — a milestone past a crossing has ambiguous directionality. Find nodes with `HighwayMilestoneFeature` on traversed ways, pick closest, return relative position. Do NOT use `ReverseLookupObject` — highway milestones never resolve to a `Place`.
- Tests: new test cases in `Tests/`

## Capabilities

### New Capabilities
- `highway-milestone-description`: Describe location relative to nearest node with `HighwayMilestoneFeature` that is part of a traversable way network connected to the closest way

### Modified Capabilities
<!-- No existing specs change -->

## Impact

- **New header**: `include/osmscout/location/LocationHighwayMilestoneDescription.h` (or add to existing `LocationDescriptionService.h`)
- **Modified header**: `include/osmscout/location/LocationDescriptionService.h` — new class + method + `LocationDescription` field
- **Modified source**: `src/osmscout/location/LocationDescriptionService.cpp` — implementation
- **Modified demo**: `Demos/src/LocationDescription.cpp` — dump new description type
- **Modified `LocationDescription`**: new `SetHighwayMilestoneDescription`/`GetHighwayMilestoneDescription`
- **Dependencies**: `HighwayMilestoneFeature` (internal only, not exposed in public API), `DescribeLocationByWay`, `Database::LoadNodesInRadius`, `Database::GetWayByOffset`