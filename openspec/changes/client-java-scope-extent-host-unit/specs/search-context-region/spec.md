# Spec Delta

## ADDED Requirements

### Requirement: The scope extent's derivation is reachable without a JNI build

The derivation of a resolved region's geographic extent SHALL be a unit the host test suite compiles and
links on its own, without building the JNI translation unit. A regression that derives the extent from the
wrong object, or that stops deriving one, SHALL therefore be observable in the host suite.

#### Scenario: The host suite derives the extent of a real region

- **GIVEN** an imported database that holds admin regions
- **WHEN** the host suite derives the extent of an outline region from the object that represents it
- **THEN** the derived box SHALL be the bounding box of that region's own object

#### Scenario: A derivation from the wrong object is observable

- **GIVEN** the same imported database and its regions
- **WHEN** the derivation reads the database's own box instead of the region's object
- **THEN** the host suite SHALL fail on a region whose own box differs from the database's box

#### Scenario: A region without a loadable object leaves the extent unset

- **GIVEN** a region whose object cannot be loaded, or no region at all
- **WHEN** the host suite derives its extent
- **THEN** the extent SHALL be unset
- **AND** it SHALL admit a position anywhere
