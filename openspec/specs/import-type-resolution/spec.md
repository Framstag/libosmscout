# import-type-resolution Specification

## Purpose
Fast and correct resolution of OSM object types from tags during import, with cost independent of the number of defined types.

## Requirements

### Requirement: Correct type resolution

The import pipeline SHALL resolve the type of every OSM node, way and relation from its tags according to the type definition, resolving the possible way type and area type for ways in a single pass.

#### Scenario: Node type resolved from tags

- **WHEN** an OSM node has tags matching a defined node type
- **THEN** the node is imported with that type

#### Scenario: Way and area types resolved in single pass

- **WHEN** an OSM way has tags matching a type defined for both way and area use
- **THEN** the import resolves both the way type and the area type in the same pass

### Requirement: Resolution order preserved

Type resolution SHALL return the first matching type in type-definition order, so that overlapping type conditions resolve exactly as before this change.

#### Scenario: Earlier-defined type wins on overlap

- **WHEN** an object's tags match the conditions of two defined types
- **THEN** the type defined earlier in the type definition file is selected

#### Scenario: First matching condition determines resolved kinds

- **WHEN** an object's tags match a type condition
- **THEN** the way type, the area type, or both are resolved from that single match according to the condition's geometry mask, and resolution stops

#### Scenario: Way-only match leaves area type unresolved

- **WHEN** an object's tags match a way-only type before any area-capable type
- **THEN** the way type is resolved and the area type remains unresolved

### Requirement: Resolution cost independent of type count

The cost of resolving an object's type SHALL depend on the tags present on that object, not on the total number of defined types. Objects whose tags match no type, or whose type is late in the type definition, SHALL NOT pay a full scan over all defined types.

#### Scenario: No-match object is cheap

- **WHEN** an object has tags that match no defined type
- **THEN** the resolution cost for that object is proportional to the number of tags it carries, not the number of defined types

#### Scenario: Early-match object unaffected by type growth

- **WHEN** an object's tags match a type early in the type definition
- **THEN** adding more types later in the type definition does not increase the resolution cost for that object

#### Scenario: Import throughput sustained

- **WHEN** the type definition grows from 671 to more than 1500 types
- **THEN** the import preprocessing time for a fixed input does not increase in proportion to the type count

### Requirement: No-match resolution

An object whose tags match no defined type SHALL be resolvable to the ignore state (not imported as a renderable, routable or searchable object) without scanning every defined type.

#### Scenario: Unknown tags resolve to ignore

- **WHEN** an object carries tags that are registered in the type definition but match no type condition
- **THEN** the object is marked as ignored and not imported for rendering, routing or location indexing

### Requirement: Backward compatibility

The type definition file format, the generated database format, and the public type resolution API SHALL remain unchanged.

#### Scenario: Existing type definitions load unchanged

- **WHEN** an existing type definition file is loaded
- **THEN** all defined types are registered with the same conditions and options as before

#### Scenario: Generated database format stable

- **WHEN** the same OSM input is imported with this change and without it
- **THEN** the generated database files are identical

### Requirement: Resolution performance benchmark

The test suite SHALL include a benchmark that measures per-call type resolution cost for the configured type definition, making regressions from type growth visible.

#### Scenario: Benchmark reports per-call cost

- **WHEN** the benchmark runs against a type definition file
- **THEN** it reports the number of resolved types and the per-call cost for representative tag sets

#### Scenario: Benchmark covers no-match and late-match cases

- **WHEN** the benchmark runs
- **THEN** it includes tag sets that scan to the end of the type list (no match) and tag sets that match types late in the type definition
