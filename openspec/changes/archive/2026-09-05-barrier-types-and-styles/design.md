# Design: barrier-types-and-styles

## Context

`stylesheets/map.ost` defines import-time feature types; `stylesheets/include/man_made.oss` defines their rendering (the barrier styles live in the man_made module). The barrier section currently covers 17 `barrier=*` values. The OSM wiki documents 74 values; taginfo shows 19 additional documented values with usage >= 0.01% that are not discouraged. See proposal.md - Why for motivation and the spec for the full requirement list.

The `TypeConfig` is generated from the stylesheet at import time — no C++ code changes are needed. This is a pure stylesheet change, same pattern as the earlier `leisure-type-definitions`, `natural-type-definitions`, `aeroway-type-definitions`, and `amenity-type-definitions` changes.

## Goals / Non-Goals

**Goals:**
- Add 19 new `TYPE` definitions to the barrier section of `stylesheets/map.ost`.
- Add rendering rules in `stylesheets/include/man_made.oss` for the new linear barrier types where visualisation is obvious.
- Follow existing type/style conventions so the new definitions are indistinguishable in style from the current ones.

**Non-Goals:**
- No changes to existing type or style definitions.
- No new symbols/icons — only line rendering, reusing existing colors and patterns.
- No C++/import-pipeline changes.
- No changes to other stylesheets (`standard.oss`, `cycle.oss`, `winter-sports.oss`).

## Decisions

### D1: Add types to the existing barrier section of map.ost

New `TYPE` definitions go into the `// Barriers` section of `stylesheets/map.ost`, after `barrier_kent_carriage_gate` and before the `// Types we currently ignore` section.

- **Alternative considered**: a separate `barrier.ost` include file. Rejected — `map.ost` is the single source of type definitions; the prior type-definition changes all extended `map.ost` in place.
- **Rationale**: keeps type definitions discoverable in one place and matches established project convention.

### D2: Element types follow the OSM wiki element table

Each new type uses the element set from the wiki [Key:barrier](https://wiki.openstreetmap.org/wiki/Key:barrier) table and the individual tag pages: `WAY` only for `guard_rail` and `handrail`, `NODE WAY` for `kerb`, `chain`, `jersey_barrier`, `log`, and `rope`, `NODE WAY AREA` for `avalanche_protection` (per its tag page), and `NODE` only for the 11 node barrier types.

- **Alternative considered**: `NODE WAY AREA` for everything. Rejected — would import node-tagged `guard_rail`/`handrail` and area-tagged `turnstile` objects that the wiki does not document, polluting the type config.
- **Rationale**: the spec requires wiki-conformant element types; import-time filtering is the cheapest place to enforce them.

### D3: Node barrier types are marked IGNORE

The 11 node-only barrier types (`swing_gate`, `wicket_gate`, `kissing_gate`, `height_restrictor`, `turnstile`, `sliding_gate`, `hampshire_gate`, `border_control`, `planter`, `debris`, `full-height_turnstile`) are defined with `IGNORE`, mirroring the existing node barriers (`barrier_gate`, `barrier_lift_gate`, `barrier_stile`, `barrier_block`, `barrier_cattle_grid`, `barrier_toll_booth`, `barrier_entrance`, `barrier_cycle_barrier`, `barrier_sally_port`, `barrier_kent_carriage_gate`). Per the typedef documentation, `IGNORE` still defines the type so matching objects do not mismatch with other types, but the objects are not stored.

- **Alternative considered**: define them without `IGNORE` so nodes are stored as POIs. Rejected — inconsistent with every existing node barrier, and the nodes have no rendering, so storing them only bloats the database.
- **Rationale**: `IGNORE` is the established convention for node barriers in this stylesheet.

### D4: Linear barrier types are stored and rendered

The 8 linear barrier types (`kerb`, `guard_rail`, `handrail`, `chain`, `jersey_barrier`, `log`, `rope`, `avalanche_protection`) are defined without `IGNORE`, mirroring the rendered barriers (`barrier_fence`, `barrier_wall`, `barrier_retaining_wall`, `barrier_city_wall`). `NODE WAY` types store their nodes too — the same pattern as `waterway_check_dam`, `leisure_slipway`, and `natural_valley`.

- **Alternative considered**: `IGNORE` all new types. Rejected — the linear barriers are common map features (kerbs, guard rails) that should render.
- **Rationale**: matches the existing split between rendered and ignored barriers.

### D5: Styles grouped by visual nature in man_made.oss

New style rules in `stylesheets/include/man_made.oss` reuse existing colors and patterns:

- Wall-like line (`@wallColor`, `[MAG closer-]`): `barrier_kerb` (low solid barrier, like `barrier_retaining_wall`), `barrier_jersey_barrier` (heavy prefabricated blocks, like `barrier_wall`).
- Fence-like line (`#aaaaaa`, `[MAG veryClose-]`): `barrier_guard_rail`, `barrier_handrail`, `barrier_log`, `barrier_avalanche_protection` (light linear structures, like `barrier_fence`).
- Dashed line (`#aaaaaa` with `dash: 1,1`, `[MAG veryClose-]`): `barrier_chain`, `barrier_rope` (flexible, non-continuous barriers).

- **Alternative considered**: one generic line for all new types. Rejected — chains/ropes are visually distinct from solid kerbs; the spec requires nature-appropriate rendering.
- **Rationale**: reusing existing colors keeps the palette coherent and avoids new visual noise.

## Risks / Trade-offs

- [New types may collide with existing type names] → Verified against current `map.ost`; all 19 names are new. Import fails loudly on duplicate type names, so a collision would be caught in CI.
- [Over-rendering at low zoom clutters the map] → All new line rules are gated behind `[MAG closer-]`/`[MAG veryClose-]`, matching the existing barrier rules; nothing renders at low zoom.
- [Kerb nodes stored as POIs add data bloat] → Kerb nodes are a small fraction of kerb objects; the `NODE WAY` pattern is already used by other types. Acceptable.
- [Import-time behavior change requires database re-import] → Stylesheet-only change; existing databases keep working, new types appear after re-import. Rollback = revert the two stylesheet files.

## Migration Plan

1. Edit `stylesheets/map.ost` (barrier section) and `stylesheets/include/man_made.oss`.
2. Validate stylesheets parse (run the existing style validation in CI).
3. Re-import a test database and verify new types appear in `TypeConfig` and render.
4. Rollback: revert the two files; no data migration needed.

## Open Questions

None.
