## Why

OSM wiki (https://wiki.openstreetmap.org/wiki/Tag:highway=milestone) specifies that only `highway=milestone` is mandatory — all other tags (`distance`, `ref`, `carriageway_ref`, `marker`) are optional. Current `HighwayMilestoneFeature` parser incorrectly requires both `distance` AND `ref` to allocate a feature value, dropping nodes that have only `ref` (common pattern), only `distance`, or none of these sub-tags.

## What Changes

- Remove the requirement that `distance` AND `ref` must both be present for feature value allocation
- Make all sub-tags (`distance`, `ref`, `carriageway_ref`, `marker`) independently optional
- Feature value allocated for any `highway=milestone` node regardless of which sub-tags present
- Distance parsing still validates format — malformed `distance` tag logs warning, but feature value still allocated (just without distance) so other fields are preserved
- Update serialization round-trip test to cover minimal-attribute scenarios

## Capabilities

### New Capabilities
- `highway-milestone-feature`: Modified existing capability — requirements change to reflect truly optional sub-tags

### Modified Capabilities
- `highway-milestone-feature`: Change allocation requirement from "both distance and ref required" to "feature allocated for any highway=milestone node, all sub-tags optional independently"

## Impact

- `libosmscout/include/osmscout/feature/HighwayMilestoneFeature.h` — no structural changes needed (all fields already support defaults: distance=0, string fields empty)
- `libosmscout/src/osmscout/feature/HighwayMilestoneFeature.cpp` — `Parse()` method: remove early return when distance or ref missing; still parse each tag independently
- `Tests/src/HighwayMilestoneFeatureTest.cpp` — update existing test expectations; add new tests for nodes with only `ref`, only `distance`, and no sub-tags
- `openspec/specs/highway-milestone-feature/spec.md` — update requirement about allocation; update malformed-format scenarios
