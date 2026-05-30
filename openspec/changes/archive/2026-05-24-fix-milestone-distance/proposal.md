## Why

OSM `highway=milestone` nodes use the `distance` tag in **kilometers** (e.g., `distance=35` means 35 km). The current `HighwayMilestoneFeature` implementation treats the tag value as meters — no km→m conversion on import, no m→km conversion on display. This causes incorrect internal storage (35 instead of 35000) and wrong user-facing output ("35 m" instead of "35 km").

## What Changes

- **Import pipeline**: Multiply parsed OSM `distance` value by 1000 to store as meters internally (consistent with libosmscout's metric-internal convention)
- **GetLabel()**: Divide internal meters by 1000, display with "km" unit suffix
- **DescriptionProcessor** (DumpData): Output distance in kilometers for human-readable display
- **Spec update**: Revise existing `highway-milestone-feature` spec to reflect km→m conversion on import and km display on output

## Capabilities

### New Capabilities
*(none — this is a bugfix to existing capability)*

### Modified Capabilities
- `highway-milestone-feature`: Distance parsing SHALL convert OSM km value to meters for internal storage. GetLabel() and DescriptionProcessor SHALL display in kilometers. Unit semantics corrected from "implicit meters" to "OSM km → internal meters, display km".

## Impact

- **Modified files**: `libosmscout/src/osmscout/feature/HighwayMilestoneFeature.cpp` (Parse + GetLabel), `libosmscout/src/osmscout/description/DescriptionService.cpp` (DescriptionProcessor output)
- **No API/ABI break**: Feature value storage format (uint32_t meters) unchanged — only semantics of what goes in changes
- **Existing databases**: Will need re-import because stored distance values change (35 → 35000)