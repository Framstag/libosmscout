## 1. LocationHighwayMilestoneDescription class

- [x] 1.1 Add `LocationHighwayMilestoneDescription` class declaration — with `ObjectFileRef`, `FeatureValueBuffer`, `Distance`, `Bearing`, milestone fields, plus `GetObject()`, `GetObjectFeatures()`, `GetDistance()`, `GetBearing()`, `GetMilestoneDistance()`, `GetMilestoneRef()`, `GetMilestoneCarriagewayRef()`, `IsAtPlace()` getters and three constructors
- [x] 1.2 Add `LocationHighwayMilestoneDescriptionRef` typedef (`std::shared_ptr<LocationHighwayMilestoneDescription>`) near the class declaration
- [x] 1.3 Add `atHighwayMilestoneDescription` field + `SetHighwayMilestoneDescription()`/`GetHighwayMilestoneDescription()` to `LocationDescription` class
- [x] 1.4 Implement `LocationHighwayMilestoneDescription` constructors and getters in `src/osmscout/location/LocationDescriptionService.cpp`

## 2. DescribeLocationByHighwayMilestone method

- [x] 2.1 Declare `DescribeLocationByHighwayMilestone()` method in `LocationDescriptionService` class — signature with `milestoneLookupDistance` parameter defaulting to `Distance::Of<Meter>(2000)`
- [x] 2.2 Implement precondition check: return `true` early if `description.GetWayDescription()` is null
- [x] 2.3 Implement way type collection: iterate `typeConfig->GetTypes()` collecting types that `CanBeWay()` and have `NameFeature::NAME` or `RefFeature::NAME` (same filter as `DescribeLocationByWay`)
- [x] 2.4 Implement way loading via `database->LoadWaysInRadius()` with collected types and `lookupDistance` parameter
- [x] 2.5 Implement connectivity graph: build `std::map<Point, std::set<WayRef>>` from candidate way node coordinates
- [x] 2.6 Implement BFS traversal from closest way's endpoints: process queue of nodes, for each node look up connected ways in map, for each way find unvisited endpoint nodes (stop at crossings where >2 ways share node), track visited nodes in `std::set<Point>` to prevent cycles
- [x] 2.7 Implement distance cutoff: only traverse a way if its closest segment distance to the original location is within `milestoneLookupDistance`
- [x] 2.8 Implement milestone type collection: collect node types that have `HighwayMilestoneFeature` via `type->HasFeature(HighwayMilestoneFeature::NAME)`
- [x] 2.9 Implement milestone node loading via `database->LoadNodesInRadius()` with milestone types
- [x] 2.10 Implement milestone filtering: keep only milestone nodes that belong to a traversed connected way
- [x] 2.11 Implement closest milestone selection: sort candidate milestones by distance from location, pick the closest
- [x] 2.12 Implement result construction: build `LocationHighwayMilestoneDescription` from node's `ObjectFileRef` + `FeatureValueBuffer` directly (no reverse lookup — milestones never resolve to Place)

## 3. Integration with DescribeLocation

- [x] 3.1 Add `DescribeLocationByHighwayMilestone()` call in `DescribeLocation()`, after `DescribeLocationByWay()` call and before `DescribeLocationByCrossing()` call, passing through `lookupDistance` and `sizeFilter`

## 4. Demo app update

- [x] 4.1 Add `DumpHighwayMilestoneDescription()` function in `Demos/src/LocationDescription.cpp` following the pattern of `DumpWayDescription()` — display place info, distance, bearing, and milestone feature values (distance, ref, carriageway_ref, marker from `HighwayMilestoneFeatureValue`)
- [x] 4.2 Add `GetHighwayMilestoneDescription()` check and dump call in `main()`, after way description dump

## 5. Build system

- [x] 5.1 Verify `libosmscout/CMakeLists.txt` includes any new header if placed in a separate file (not needed if added to existing header)
- [x] 5.2 Verify `libosmscout/include/meson.build` includes any new header if placed in a separate file

## 6. Tests

- [x] 6.1 Add test case for `LocationHighwayMilestoneDescription` constructors — at-place and at-distance with milestone feature value
- [x] 6.2 Add test case for precondition: no way description → no highway milestone description
- [x] 6.3 Add test case for milestone found on closest way
- [x] 6.4 Add test case for milestone found on connected way via BFS
- [x] 6.5 Add test case for no milestones in connected network
- [x] 6.6 Add test case for cycle protection (looping road network)
- [x] 6.7 Add test case for crossing boundary (node with >2 ways stops traversal)
- [x] 6.8 Add test case for milestone beyond `milestoneLookupDistance`