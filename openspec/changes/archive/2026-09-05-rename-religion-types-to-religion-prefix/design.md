# Design: Rename religion types to religion_ prefix

## Context

See proposal.md - Why. The stylesheets use a `<key>_<value>` type naming scheme (`waterway_stream`, `military_cannon`, `power_tower`, `shop_*`, `amenity_*`, ...). The 37 religion types in `stylesheets/map.ost` break it: 29 worship node types use `<religion>_worship`, 8 building types use `<religion>_<building>_building` or bare names (`temple_building`, `shrine_building`, `worship_building`). The stylesheet `stylesheets/include/religious.oss` references all 37 in `[TYPE ...]` rule lists; its symbols already use the `religion_` prefix and are unaffected.

## Goals / Non-Goals

**Goals:**
- Rename all 37 religion types to the `religion_` prefix scheme
- Update every stylesheet reference so rendering behavior is unchanged
- Update the religion-type-definitions spec to the new names

**Non-Goals:**
- No behavior change: types match the same OSM tags and render identically
- No symbol changes (already `religion_*`)
- No changes to other type families
- No code changes (C++ never references these type names)

## Decisions

### D1: Worship types drop the "worship" suffix (`religion_<value>`)

Chosen: `christian_worship` -> `religion_christian`, `buddhist_worship` -> `religion_buddhist`, ... (29 types).

- Alternative A: `religion_<value>_worship` (keep suffix, add prefix). Rejected: the scheme is `<key>_<value>`; "worship" is redundant since every religion type is a place-of-worship node, and it would make names longer than any other family.
- Alternative B: `place_of_worship_<value>` (prefix from the primary `amenity` key). Rejected: the distinguishing attribute is `religion`; `place_of_worship_*` would be longer and less recognizable, and the user explicitly wants the `religion_` prefix.

### D2: Edge-case building types are renamed too

Chosen: `temple_building` -> `religion_temple_building`, `shrine_building` -> `religion_shrine_building`, `worship_building` -> `religion_building`, plus the five religion-qualified building types (`christian_church_building` -> `religion_christian_church_building`, etc.).

- Alternative: leave the generic building types unchanged. Rejected: they live in the Religious section and would remain the only non-`religion_` types, keeping the inconsistency the change exists to fix.

### D3: Spec delta, not skip_specs

Chosen: MODIFIED requirements 1-3 (name references), REMOVED requirement 5 ("remain unchanged" is inverted by the rename), ADDED a naming-scheme requirement.

- Alternative: `skip_specs: true` (pure refactor). Rejected: type names are part of the observable contract — they appear in the database `TypeConfig` and are referenced by consumers; the existing spec names the old types in scenarios and would be stale.

### D4: Direct rename, no aliases

Chosen: rename the `TYPE` definitions in `map.ost` and update `[TYPE ...]` lists in `religious.oss` in one pass.

- Alternative: keep old types as aliases. Rejected: the stylesheet language has no alias mechanism; duplicate types would double-match the same tags and change rendering priority.

## Risks / Trade-offs

- [Imported databases reference old type names] -> Mitigation: documented as **BREAKING** in the proposal; databases must be re-imported after the change.
- [Missed reference leaves a dangling type name in a rule list] -> Mitigation: grep all 37 old names across the repo before and after the rename; `CheckStyleSheet` tests validate every `.oss` against `map.ost` with `--warning-as-error`.
- [Spec text drifts from implementation] -> Mitigation: the delta spec is the source of truth; sync to main spec on archive.

## Migration Plan

1. Rename the 37 `TYPE` definitions in `stylesheets/map.ost` (Religious section, lines ~2565-2840)
2. Update `[TYPE ...]` rule lists in `stylesheets/include/religious.oss` (3 building lists + 29 worship rules)
3. Verify no old names remain: `grep -rn "christian_worship\|...\|worship_building" stylesheets/`
4. Run `cd build && ctest -R CheckStyleSheet --output-on-failure` to validate stylesheets
5. Re-import any databases (BREAKING)
6. Archive the change; sync delta spec to main spec
