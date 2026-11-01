# Spec Delta

## Purpose

Turn-by-turn route description display for JavaScout: the geometry, the description and the published total
of a route calculated for the Java client.

## ADDED Requirements

### Requirement: The published route starts at the requested start position

A calculated route SHALL publish its geometry and description from the requested start position. When the
requested start lies farther from the snapped routable node than the snap tolerance, the description's first
node SHALL take the requested position, the approach to the node SHALL become the polyline's first segment, and
every later node's cumulative distance and the published total SHALL include it. A requested start on the
network SHALL publish the routed geometry unchanged.

#### Scenario: A snapped start is published from the requested position

- **GIVEN** a requested start that lies off the network by more than the snap tolerance
- **WHEN** the route is delivered to Java
- **THEN** the published polyline's first vertex SHALL be the requested start
- **AND** the description's first node SHALL carry the requested position
- **AND** the description's first node SHALL carry a cumulative distance of zero

#### Scenario: The approach is counted once

- **GIVEN** a snapped start whose approach to the routed node is d metres
- **WHEN** the route is delivered to Java
- **THEN** every later description node's cumulative distance SHALL exceed the routed node's by d
- **AND** the first leg's delta, the per-step sums and the published total SHALL include d
- **AND** the published polyline SHALL lead with the same segment

#### Scenario: A start on the network is published unchanged

- **GIVEN** a requested start within the snap tolerance of the snapped node
- **WHEN** the route is delivered to Java
- **THEN** the published polyline SHALL equal the routed geometry
- **AND** it SHALL NOT carry a duplicate leading vertex

#### Scenario: A start that is not a position is not prepended

- **GIVEN** a requested start whose coordinate is not finite
- **WHEN** the approach is measured
- **THEN** the approach SHALL be zero
- **AND** the route SHALL NOT be prepended
