# Spec Delta

## Purpose

Name the reason a route calculation produced no route, and resolve a position to a routable way for the end
of the route it is being resolved for.

## ADDED Requirements

### Requirement: The bridge reports the kind of a failed calculation

A route calculation that produced no route SHALL report, to Java, the kind of failure the routing library
named, together with an English detail and how far the failing end of the route is from the nearest road the
router could use. The translation from the library's reason to the Java kind SHALL be one dependency-free
function of the reason, so a host test can exercise it and the two vocabularies cannot drift.

#### Scenario: An unattachable end reports the distance to the next road

- **GIVEN** a calculation whose start or target cannot be attached
- **WHEN** the failure reaches Java
- **THEN** the reported kind SHALL name that end
- **AND** the reported distance SHALL be how far the failing end is from the nearest usable road, or a
  non-finite value when the router could not measure one

#### Scenario: Every reason maps to a kind

- **GIVEN** each reason the routing library can name
- **WHEN** it is translated
- **THEN** it SHALL map to the kind of the same meaning, and an unnamed reason SHALL map to `KIND_UNKNOWN`

#### Scenario: A cancellation is not reported as a failure

- **GIVEN** a calculation aborted through its breaker
- **WHEN** the result reaches the bridge
- **THEN** it SHALL be reported as a cancellation, not as an error kind

#### Scenario: The distance is unknown where it is not measured

- **GIVEN** a failure that did not happen at an end of the route, or one where the router found no usable
  road to measure against
- **WHEN** the failure reaches Java
- **THEN** the reported distance SHALL be a non-finite value
- **AND** the kind SHALL NOT claim a cause the library did not name

### Requirement: The kind crosses the boundary as a number both sides declare

The failure kinds SHALL be integers whose values are equal on the Java side and in the bridge, because the
integer is what crosses the boundary. The bridge's cause vocabulary SHALL be its own — not the routing
library's enum — so the boundary keeps one translation point.

#### Scenario: The Java constants and the bridge agree

- **GIVEN** the kind constants the Java interface declares
- **WHEN** they are compared with the numbers the bridge sends for the same causes
- **THEN** each pair SHALL be equal
