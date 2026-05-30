## Context

The `HighwayMilestoneFeature` parser in `libosmscout/src/osmscout/feature/HighwayMilestoneFeature.cpp` currently gates all feature value allocation on presence of both `distance` AND `ref` tags. Per OSM wiki, only `highway=milestone` itself is mandatory — all sub-tags are independently optional. The fix requires removing a single guard condition; no architectural changes needed.

## Goals / Non-Goals

**Goals:**
- Feature value allocated for every `highway=milestone` node, regardless of sub-tags
- Each sub-tag (`distance`, `ref`, `carriageway_ref`, `marker`) parsed independently
- Malformed `distance` value logs warning but doesn't discard the node — other fields preserved
- Update spec, parser, and tests

**Non-Goals:**
- No new data fields or structural changes to `HighwayMilestoneFeatureValue`
- No changes to serialization format (already handles empty defaults: distance=0, strings empty)
- No changes to other feature implementations

## Decisions

1. **Remove early-return guard, keep independent parsing**: The `Parse()` method already parses `carriageway_ref` and `marker` independently after the guard. We simply delete the `if (distanceTag==tags.end() || refTag==tags.end()) { return; }` block and parse each field separately.

2. **Malformed distance: warn, skip distance, keep node**: When distance tag is present but unparseable, warn and skip only the distance field — still allocate the feature value and parse other tags. This preserves `ref`, `carriageway_ref`, and `marker` data even when distance is garbage.

3. **No feature value changes needed**: `distance` defaults to 0, string fields default to empty — serialization already handles this correctly for minimal-attribute nodes.

## Risks / Trade-offs

- [Data inconsistency] Nodes with only `ref` will have distance=0 — fine for rendering but consumers must handle zero-distance meaning "unknown" not "zero km".
- [Spec drift] Existing test expectations for invalid-format scenarios assume no value allocated — those tests must change to expect allocated value with distance=0 and warning logged.
- [Performance] Negligible — one extra iteration through tags for nodes that previously skipped entirely.
