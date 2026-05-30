## Why

OSM `highway=milestone` nodes can specify `distance` in miles (e.g. `"35.0 mi"`) or kilometers (`"35.0 km"`). Current implementation assumes all values are kilometers, silently misinterpreting mile values. This produces incorrect stored distances and wrong label output.

## What Changes

- **Parse** `"km"` and `"mi"` unit suffixes from `distance` tag value
- Convert to internal meter representation using appropriate factor
- Keep default assumption of kilometers when no unit given (backward compatible)
- Structure code for easy addition of future units (extensible table/pattern)
- **Do NOT** add support for `"45 + 5"` combined format
- Update `GetLabel()` and description processor to reflect actual stored unit
- Update tests for new parsing scenarios

## Capabilities

### New Capabilities
- `highway-milestone-distance-units`: Extend `HighwayMilestoneFeature` distance parsing to handle "km" and "mi" units with conversion to meters, designed for extensibility to future units.

### Modified Capabilities
- `highway-milestone-feature` (existing): Modify `Parse()` behavior — previously assumed all values are km, now checks for unit suffix. `GetLabel()` and `DescriptionService` entry updated to reflect actual stored meter value.

## Impact

- `libosmscout/src/osmscout/feature/HighwayMilestoneFeature.cpp` — Parse(), GetLabel()
- `libosmscout/description/DescriptionService.cpp` — milestone description display
- `Tests/` — existing milestone feature tests need new scenarios
- Binary format — no change, distance stays `uint32_t` meters
