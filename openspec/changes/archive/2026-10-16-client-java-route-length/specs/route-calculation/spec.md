# Spec Delta

## Purpose

JNI bridge to libosmscout routing engine for async route calculation between two coordinates.

## ADDED Requirements

### Requirement: The published route length is a length of that route

A calculated route's published length SHALL be a length of that route, SHALL NOT be the start/target air-line
estimate used for the cost limit and the progress denominator, and SHALL be derived from the description's
own total (the cumulative distance at its last node) when the description produced one, else from the
great-circle length of the published route geometry. The estimated duration SHALL be derived from the
published length.

#### Scenario: The description's total is the published length

- **GIVEN** a route whose description produced an own total
- **WHEN** the route is delivered to Java
- **THEN** its published length SHALL equal the cumulative distance at the description's last node
- **AND** the description's steps SHALL cover that total (with `client-java-route-instruction-metrics`,
  the per-step leg distances sum to it exactly)

#### Scenario: The published geometry provides the length when the description does not

- **GIVEN** a route whose description produced no length but which publishes more than one geometry point
- **WHEN** the route is delivered to Java
- **THEN** its published length SHALL be the great-circle length of the published geometry
- **AND** it SHALL be greater than zero

#### Scenario: The air-line estimate is never a published length

- **GIVEN** a route whose start and target lie far apart with a long detour between them
- **WHEN** the route is delivered to Java
- **THEN** its published length SHALL be at least the air-line distance between its start and target
- **AND** it SHALL NOT equal the router's under-counted estimate

#### Scenario: The duration follows the published length

- **GIVEN** a route whose length is decided as above
- **WHEN** the route is delivered to Java
- **THEN** its estimated duration SHALL be derived from that published length
- **AND** SHALL NOT be derived from the air-line estimate

#### Scenario: A route without a usable length reports none

- **GIVEN** a route whose description produced no length and which publishes at most one geometry point
- **WHEN** the route is delivered to Java
- **THEN** its published length SHALL be zero
- **AND** the call SHALL NOT fault
