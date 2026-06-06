## Why

Location description service currently describes location relative to the single nearest highway milestone (e.g., "near milestone A2, 35m"). This misses natural human phrasing like "between milestone A2 (35m) and milestone A3 (50m)". Two-milestone description gives much better spatial context for navigation and location sharing.

## What Changes

- Extend `LocationHighwayMilestoneDescription` (or create new class) to hold **two** milestones instead of one — the milestone before and the milestone after the location
- Rewrite `DescribeLocationByHighwayMilestone` algorithm:
  - Find two nearest milestones on the same way (or connected way network) that bracket the location
  - Compute distances along way to each milestone
- Handle corner cases:
  - Location before first milestone on way → describe relative to first milestone only (fallback to single-milestone)
  - Location after last milestone on way → same fallback
  - Only one milestone found within lookup range → single-milestone fallback
  - Milestones are on different ways but ways are connected → describe as "between milestone X on [way name] and milestone Y on [way name]"
  - Location at same coordinate as a milestone → "at milestone X"
- Update `LocationHighwayMilestoneDescription` API:
  - Add `GetMilestoneBackward()` / `GetMilestoneForward()` (or restructure as a pair)
  - Keep backward-compatible single-milestone getters where possible
- Update `MCPServer/src/LocationDescriptionMapper.cpp`:
  - Include both milestones in JSON output
  - Add human-readable text like "between milestone A2 (35m) and milestone A3 (50m)"
- Update `Demos/src/LocationDescription.cpp` — print both milestones
- Update `Tests/src/LocationDescriptionServiceTest.cpp` — add test cases for two-milestone scenarios

## Capabilities

### New Capabilities
- `two-milestone-description`: Describe location as between two highway milestones on a way, with forward/backward distances

### Modified Capabilities
*(no existing spec-level changes — this is a new capability within the existing `DescribeLocationByHighwayMilestone` method)*

## Impact

- **libosmscout** (`libosmscout/src/osmscout/location/`):
  - `LocationDescriptionService.h` — extend `LocationHighwayMilestoneDescription` class (add second milestone fields)
  - `LocationDescriptionService.cpp` — rewrite `DescribeLocationByHighwayMilestone` algorithm
- **MCPServer** (`MCPServer/src/LocationDescriptionMapper.cpp`):
  - Update `ToJson(const LocationHighwayMilestoneDescription&)` — add `milestoneForward`/`milestoneBackward` fields
  - Update text generation to describe two-milestone scenario
- **Demos** (`Demos/src/LocationDescription.cpp`):
  - Update `DumpHighwayMilestoneDescription` to print both milestones
- **Tests** (`Tests/src/LocationDescriptionServiceTest.cpp`):
  - Add unit tests for: between-two-milestones, before-first-milestone, after-last-milestone, single-milestone-fallback, same-coordinate
- **Backward compatibility**: existing single-milestone JSON consumers see no breakage if old fields remain populated when only one milestone found