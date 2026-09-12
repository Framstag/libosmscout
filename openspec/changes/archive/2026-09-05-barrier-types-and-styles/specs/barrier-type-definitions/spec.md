# barrier-type-definitions Specification

## Purpose

Defines the import-time OSM feature types for documented `barrier=*` values missing from `stylesheets/map.ost`, so these features exist in the database `TypeConfig` and are importable and renderable. Element types follow the OSM wiki [Key:barrier](https://wiki.openstreetmap.org/wiki/Key:barrier) element table and the individual tag pages. Only values that are documented, have relevant usage (>= 0.01% per taginfo), and are not discouraged/deprecated are added. Corresponding style definitions are provided where visualisation is obvious.

## ADDED Requirements

### Requirement: Linear barrier types

The import-time stylesheet SHALL define feature types for the following linear `barrier=*` values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `barrier=kerb` | `barrier_kerb` | node, way |
| `barrier=guard_rail` | `barrier_guard_rail` | way |
| `barrier=handrail` | `barrier_handrail` | way |
| `barrier=chain` | `barrier_chain` | node, way |
| `barrier=jersey_barrier` | `barrier_jersey_barrier` | node, way |
| `barrier=log` | `barrier_log` | node, way |
| `barrier=rope` | `barrier_rope` | node, way |
| `barrier=avalanche_protection` | `barrier_avalanche_protection` | node, way, area |

#### Scenario: Kerb type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `barrier_kerb`
- **THEN** the type SHALL exist
- **AND** nodes and ways tagged `barrier=kerb` SHALL be importable as that type

#### Scenario: Guard rail type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `barrier_guard_rail`
- **THEN** the type SHALL exist
- **AND** ways tagged `barrier=guard_rail` SHALL be importable as that type
- **AND** nodes tagged `barrier=guard_rail` SHALL NOT be importable as that type

#### Scenario: Handrail type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `barrier_handrail`
- **THEN** the type SHALL exist
- **AND** ways tagged `barrier=handrail` SHALL be importable as that type
- **AND** nodes tagged `barrier=handrail` SHALL NOT be importable as that type

#### Scenario: Chain type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `barrier_chain`
- **THEN** the type SHALL exist
- **AND** nodes and ways tagged `barrier=chain` SHALL be importable as that type

#### Scenario: Jersey barrier type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `barrier_jersey_barrier`
- **THEN** the type SHALL exist
- **AND** nodes and ways tagged `barrier=jersey_barrier` SHALL be importable as that type

#### Scenario: Log type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `barrier_log`
- **THEN** the type SHALL exist
- **AND** nodes and ways tagged `barrier=log` SHALL be importable as that type

#### Scenario: Rope type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `barrier_rope`
- **THEN** the type SHALL exist
- **AND** nodes and ways tagged `barrier=rope` SHALL be importable as that type

#### Scenario: Avalanche protection type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `barrier_avalanche_protection`
- **THEN** the type SHALL exist
- **AND** nodes, ways, and areas tagged `barrier=avalanche_protection` SHALL be importable as that type

### Requirement: Node barrier types

The import-time stylesheet SHALL define feature types for the following node-only `barrier=*` values, marked `IGNORE` following the existing convention for node barriers (`barrier_gate`, `barrier_lift_gate`, `barrier_stile`), so that matching objects do not mismatch with other types:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `barrier=swing_gate` | `barrier_swing_gate` | node |
| `barrier=wicket_gate` | `barrier_wicket_gate` | node |
| `barrier=kissing_gate` | `barrier_kissing_gate` | node |
| `barrier=height_restrictor` | `barrier_height_restrictor` | node |
| `barrier=turnstile` | `barrier_turnstile` | node |
| `barrier=sliding_gate` | `barrier_sliding_gate` | node |
| `barrier=hampshire_gate` | `barrier_hampshire_gate` | node |
| `barrier=border_control` | `barrier_border_control` | node |
| `barrier=planter` | `barrier_planter` | node |
| `barrier=debris` | `barrier_debris` | node |
| `barrier=full-height_turnstile` | `barrier_full_height_turnstile` | node |

#### Scenario: Swing gate type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `barrier_swing_gate`
- **THEN** the type SHALL exist
- **AND** nodes tagged `barrier=swing_gate` SHALL be importable as that type

#### Scenario: Wicket gate type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `barrier_wicket_gate`
- **THEN** the type SHALL exist
- **AND** nodes tagged `barrier=wicket_gate` SHALL be importable as that type

#### Scenario: Kissing gate type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `barrier_kissing_gate`
- **THEN** the type SHALL exist
- **AND** nodes tagged `barrier=kissing_gate` SHALL be importable as that type

#### Scenario: Height restrictor type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `barrier_height_restrictor`
- **THEN** the type SHALL exist
- **AND** nodes tagged `barrier=height_restrictor` SHALL be importable as that type

#### Scenario: Turnstile type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `barrier_turnstile`
- **THEN** the type SHALL exist
- **AND** nodes tagged `barrier=turnstile` SHALL be importable as that type

#### Scenario: Sliding gate type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `barrier_sliding_gate`
- **THEN** the type SHALL exist
- **AND** nodes tagged `barrier=sliding_gate` SHALL be importable as that type

#### Scenario: Hampshire gate type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `barrier_hampshire_gate`
- **THEN** the type SHALL exist
- **AND** nodes tagged `barrier=hampshire_gate` SHALL be importable as that type

#### Scenario: Border control type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `barrier_border_control`
- **THEN** the type SHALL exist
- **AND** nodes tagged `barrier=border_control` SHALL be importable as that type

#### Scenario: Planter type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `barrier_planter`
- **THEN** the type SHALL exist
- **AND** nodes tagged `barrier=planter` SHALL be importable as that type

#### Scenario: Debris type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `barrier_debris`
- **THEN** the type SHALL exist
- **AND** nodes tagged `barrier=debris` SHALL be importable as that type

#### Scenario: Full-height turnstile type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `barrier_full_height_turnstile`
- **THEN** the type SHALL exist
- **AND** nodes tagged `barrier=full-height_turnstile` SHALL be importable as that type

### Requirement: Discouraged and undocumented barrier values

The import-time stylesheet SHALL NOT define dedicated feature types for discouraged, deprecated, or undocumented `barrier=*` values, including but not limited to `barrier=yes`, `barrier=embankment`, `barrier=wire_fence`, and `barrier=door`. Such objects remain covered by the generic `barrier` catch-all type.

#### Scenario: Discouraged values have no dedicated type
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `barrier_yes`, `barrier_embankment`, `barrier_wire_fence`, or `barrier_door`
- **THEN** none of these types SHALL exist
- **AND** objects carrying the corresponding tags SHALL still be importable via the generic `barrier` type

### Requirement: Way rendering for new linear barrier types

The rendering stylesheet SHALL render ways of the new linear barrier types at close zoom following the existing barrier line patterns: wall-like types (`barrier_kerb`, `barrier_jersey_barrier`) with the wall color, fence-like types (`barrier_guard_rail`, `barrier_handrail`, `barrier_log`, `barrier_avalanche_protection`) with the fence color, and chain/rope types (`barrier_chain`, `barrier_rope`) with a dashed line.

#### Scenario: Kerb way gets wall-like line
- **GIVEN** a map rendered with the stylesheet at close zoom
- **WHEN** a way of type `barrier_kerb` is rendered
- **THEN** the way SHALL be rendered with a wall-colored line

#### Scenario: Guard rail way gets fence-like line
- **GIVEN** a map rendered with the stylesheet at close zoom
- **WHEN** a way of type `barrier_guard_rail` is rendered
- **THEN** the way SHALL be rendered with a fence-colored line

#### Scenario: Chain way gets dashed line
- **GIVEN** a map rendered with the stylesheet at close zoom
- **WHEN** a way of type `barrier_chain` is rendered
- **THEN** the way SHALL be rendered with a dashed line
