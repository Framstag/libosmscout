## Why

OSM data has `highway=milestone` nodes marking distance along roads, with `distance`, `ref`, and `carriageway_ref` tags. libosmscout currently cannot import or render these — `distance` tag values are not read into any feature value buffer. Adding a `HighwayMilestoneFeature` makes this data available for indexing, rendering, and routing display.

## What Changes

- New `HighwayMilestoneFeature` class (header + implementation) following the same pattern as `EleFeature`, `LocationFeature`, etc.
- New `HighwayMilestoneFeatureValue` value holder storing `distance` as `uint32_t` and `ref`, `carriageway_ref`, `marker` as strings
- Read (`FileScanner`) / Write (`FileWriter`) serialization for the value
- OSM tag parsing: `distance`, `ref`, `carriageway_ref`, `marker`
- Registration in `TypeConfig` constructor
- `HighwayMilestoneDescriptionProcessor` for DumpData tool description output
- Add new files to CMakeLists.txt and meson.build
- OST stylesheet entries can then use `highway=milestone` and access feature values

## Capabilities

### New Capabilities
- `highway-milestone-feature`: Feature for OSM `highway=milestone` nodes, parsing `distance`, `ref`, `carriageway_ref`, `marker` tags and making them available as feature values

### Modified Capabilities
*(none — no existing capability changes)*

## Impact

- **New files**: `include/osmscout/feature/HighwayMilestoneFeature.h`, `src/osmscout/feature/HighwayMilestoneFeature.cpp`
- **Modified files**: `CMakeLists.txt`, `include/meson.build`, `src/meson.build`, `src/osmscout/TypeConfig.cpp`, `include/osmscout/description/DescriptionService.h`, `src/osmscout/description/DescriptionService.cpp`
- No breaking API changes — purely additive
