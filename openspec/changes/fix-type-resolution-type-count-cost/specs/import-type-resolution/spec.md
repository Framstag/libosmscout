# Spec Delta

## MODIFIED Requirements

### Requirement: Resolution cost independent of type count

The cost of resolving an object's type SHALL depend on the tags present on that object, not on the
total number of defined types. Objects whose tags match no type, or whose type is late in the type
definition, SHALL NOT pay a full scan over all defined types. For each tag of the object, the cost SHALL
be bounded by the conditions that are not discriminated by that tag's value: a condition that declares a
distinct value for that tag SHALL NOT contribute to the cost of an object that carries another value.

#### Scenario: No-match object is cheap

- **WHEN** an object has tags that match no defined type
- **THEN** the resolution cost for that object is proportional to the number of tags it carries, not the number of defined types

#### Scenario: No-match value on a crowded key

- **GIVEN** a tag key for which many types declare distinct values
- **WHEN** an object carries that key with a value that no type declares
- **THEN** no condition declared for that key is evaluated for the object
- **THEN** the object resolves to the ignore state

#### Scenario: Early-match object unaffected by type growth

- **WHEN** an object's tags match a type early in the type definition
- **THEN** adding more types later in the type definition does not increase the resolution cost for that object

#### Scenario: Type growth on one key leaves a no-match object's cost unchanged

- **GIVEN** an object that carries a key with a value that no type declares
- **WHEN** more types that declare further values for that key are added to the type definition
- **THEN** the conditions evaluated for that object are unchanged
- **THEN** the object still resolves to the ignore state

#### Scenario: Import throughput sustained

- **WHEN** the type definition grows from 671 to more than 1500 types
- **THEN** the import preprocessing time for a fixed input does not increase in proportion to the type count

### Requirement: Resolution performance benchmark

The test suite SHALL include a benchmark that measures per-call type resolution cost for the configured type definition, making regressions from type growth visible.

#### Scenario: Benchmark reports per-call cost

- **WHEN** the benchmark runs against a type definition file
- **THEN** it reports the number of resolved types and the per-call cost for representative tag sets

#### Scenario: Benchmark covers no-match and late-match cases

- **WHEN** the benchmark runs
- **THEN** it includes tag sets that scan to the end of the type list (no match) and tag sets that match types late in the type definition

#### Scenario: Benchmark covers a crowded key with an undeclared value

- **WHEN** the benchmark runs
- **THEN** it includes a tag set that carries a key for which many types declare distinct values and a value that no type declares
- **THEN** it reports the per-call cost of that tag set for node and for way/area resolution
