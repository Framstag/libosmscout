# Merge waterfall type definitions

## What Changes

OSM discourages `natural=waterfall` (preferred tag: `waterway=waterfall`), but the tag still occurs in data. libosmscout currently defines two import-time types for the same feature:

- `waterway_waterfall` — `waterway=waterfall` (node, way, area)
- `natural_waterfall` — `natural=waterfall` (node, way, area)

Per Karry's review comment on PR #1769 (and Framstag's agreement): "maybe merge with waterway_waterfall, `natural=waterfall` should not be used and it pretty rare... both types can get merged."

This change merges both variants into the single official type `waterway_waterfall`:

- `stylesheets/map.ost`: `waterway_waterfall` gets an additional condition matching `natural=waterfall` (with a comment explaining the merge). The separate `natural_waterfall` type definition is removed and replaced by a comment explaining why it is deliberately not a regular import type (same pattern as the existing `natural_coastline` comment).
- Rendering styles: `natural_waterfall` rules are removed from `include/natural.oss` and `cycle.oss`; `waterway_waterfall` rules in `include/waterway.oss` are adjusted so waterfall area fill, labels, and way rendering stay visible at the zoom levels previously covered by the natural rules.
- The `natural-type-definitions` spec is updated: `natural=waterfall` no longer maps to a dedicated `natural_waterfall` type.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `natural-type-definitions`: `natural=waterfall` SHALL NOT map to a dedicated `natural_waterfall` type. Waterfall objects tagged `natural=waterfall` are imported under the `waterway_waterfall` type instead; `natural_waterfall` SHALL NOT exist in the type configuration.

## Impact

- `stylesheets/map.ost` — extend `waterway_waterfall` condition; remove `natural_waterfall` type def, add explanatory comment.
- `stylesheets/include/natural.oss` — drop `natural_waterfall` rules (area fill, area/node text, way).
- `stylesheets/include/waterway.oss` — adjust `waterway_waterfall` rules so merged type stays visible (area fill at low zoom, labels) where the natural rules previously rendered.
- `stylesheets/cycle.oss` — drop the two duplicated `natural_waterfall` rules.
- `openspec/specs/natural-type-definitions` — delta spec (requirement + scenarios).
- No C++ changes; import matching is declaration-order based, and a single merged type avoids ambiguity.
- Databases imported with the old `natural_waterfall` type retain it until re-import; old-data waterfall objects no longer match a rendering type. Re-import with updated stylesheet required for full rendering.
