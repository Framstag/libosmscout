# Rename religion types to religion_ prefix

## Why

The religion type definitions in the stylesheets break the general `<key>_<value>` naming scheme used by every other type family (`waterway_stream`, `military_cannon`, `power_tower`, `railway_*`, `shop_*`, `amenity_*`, ...). Religion types use `<religion-value>_worship` and `<religion-value>_<building>_building` instead of the `religion_` prefix, making them inconsistent and harder to find. The previous religion change explicitly kept the old names; this change aligns them with the scheme.

## What Changes

- Rename the 29 worship node types from `<religion>_worship` to `religion_<value>` (dropping the redundant "worship" suffix), e.g. `christian_worship` -> `religion_christian`, `buddhist_worship` -> `religion_buddhist`, `yazidi_worship` -> `religion_yazidi`
- Rename the 8 building types to the `religion_` prefix, e.g. `christian_church_building` -> `religion_christian_church_building`, `jewish_synagogue_building` -> `religion_jewish_synagogue_building`, `temple_building` -> `religion_temple_building`, `shrine_building` -> `religion_shrine_building`, `worship_building` -> `religion_building`
- Update all stylesheet rule references to the renamed types
- Update the religion type definitions spec to reference the new names
- **BREAKING**: type names are part of the imported database type registry; databases imported with the old names will not resolve the renamed types and must be re-imported

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `religion-type-definitions`: type names change from `<religion>_worship` / `<religion>_<building>_building` to `religion_<value>` / `religion_<value>_<building>_building`; all requirements and scenarios referencing type names are updated to the new scheme

## Impact

- `stylesheets/map.ost` — 37 `TYPE` definitions renamed (29 worship node types, 8 building types)
- `stylesheets/include/religious.oss` — `[TYPE ...]` rule lists updated to the renamed types (symbols already use the `religion_` prefix and are unchanged)
- `openspec/specs/religion-type-definitions/spec.md` — requirement and scenario text updated to the new type names
- No C++ code, tests, or other stylesheets affected
- **BREAKING**: existing imported databases reference the old type names and need re-import
