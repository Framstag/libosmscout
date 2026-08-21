# Proposal: Remove natural_coastline from natural type definitions

## Why

`natural=coastline` is handled by a separate, dedicated pipeline during import: it feeds the water/land tile index and the coastline baseline computation instead of being imported as a regular way. Defining `natural_coastline` as a normal import-time way type (added by the previous `additional-natural-types` change) conflicts with that handling — the type is not needed there and its presence in the `TypeConfig` is misleading. See PR discussion (comment by karry): coastline should not be handled at this point. The type definition should be removed, with a comment in the stylesheet documenting the reason.

## What Changes

- **BREAKING** Remove the `natural_coastline` import-time type definition from the import stylesheet.
- Add a comment in the stylesheet at the removal site stating why: coastline ways are handled separately by the water/land index and baseline generation, so no regular type definition is wanted.
- Remove the rendering rule for `natural_coastline` from the natural rendering module (it references the removed type).
- Update the `natural-type-definitions` spec: `natural=coastline` no longer appears in the water natural types table, its type-config scenario is dropped, and it is removed from the rendered way types scenario.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `natural-type-definitions`: remove `natural=coastline` / `natural_coastline` from the defined water natural types and from the way-rendering requirement, reflecting that coastline ways are not importable/rendered as a regular natural type.

## Impact

- `stylesheets/map.ost` — import-time type definitions (type removed, explanatory comment added).
- `stylesheets/include/natural.oss` — rendering rule for the removed type.
- `openspec/specs/natural-type-definitions/spec.md` — spec delta (water types table, coastline scenario, way-rendering scenario).
- `libosmscout-import` (`GenWaterIndex`, coastline baseline handling) — unaffected; this is the separate pipeline that continues to handle coastline data.
