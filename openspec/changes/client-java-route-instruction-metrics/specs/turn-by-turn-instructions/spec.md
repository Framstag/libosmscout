# Spec Delta

## Purpose

Display visual turn-by-turn guidance during an active navigation session in JavaScout, showing the next manoeuvre, distance to it, turn type, and street name.

## MODIFIED Requirements

### Requirement: Java RouteInstruction data class

The system SHALL provide a Java `RouteInstruction` class with fields: `distanceTo` (double, meters), `timeTo` (double, seconds), `legDistance` (double, meters, 0 if unknown), `turnType` (TurnType enum), `streetName` (String), `description` (String), `shortDescription` (String).

#### Scenario: RouteInstruction constructed from JNI
- **WHEN** the JNI bridge creates a `RouteInstruction` object
- **THEN** all of its fields SHALL be populated from the C++ `JavaRouteInstruction` struct, including the per-step time and the leg's length

#### Scenario: RouteInstruction constructed without a time
- **WHEN** a `RouteInstruction` is constructed through the constructor that takes no per-step values
- **THEN** `timeTo` and `legDistance` SHALL be 0.0
- **AND** the remaining fields SHALL be populated as given

### Requirement: An instruction carries the per-step time of its segment

A Java `RouteInstruction` SHALL carry the estimated travel time of the leg that ends at that instruction's
manoeuvre, in seconds. The value SHALL be the time between the instruction preceding it and its own
manoeuvre, truncated to whole seconds, and SHALL NOT be derived from the instruction's distance or measured
between two route nodes that carry no instruction. When the route description provides no time for that leg,
the instruction SHALL carry 0.0 and no error SHALL be raised, so that a client can treat 0.0 as "unknown".

#### Scenario: The full instruction list carries the time of each step

- **GIVEN** a route whose description provides a time for every node
- **WHEN** the full instruction list is delivered to Java
- **THEN** each instruction SHALL carry the time between the preceding instruction and its own manoeuvre
- **AND** the entries SHALL add up to the route's duration
- **AND** its distance SHALL be the distance from the start of the route, as before

#### Scenario: The next instruction carries the time of its own step

- **GIVEN** a route whose description provides times
- **WHEN** the next instruction is delivered to Java while the position approaches the manoeuvre
- **THEN** it SHALL carry the remaining time of the leg ending at that manoeuvre, interpolated at the
  position with the same progress the remaining distance uses
- **AND** that time SHALL shrink as the manoeuvre is approached

#### Scenario: A segment without a time reports zero

- **GIVEN** a route description that provides no time for a leg
- **WHEN** an instruction for that leg is delivered to Java
- **THEN** its per-step time SHALL be 0.0
- **AND** no error SHALL be raised

#### Scenario: The time is not part of the next-next hint

- **GIVEN** an instruction whose following manoeuvre is close enough to be reported as the next-next hint
- **WHEN** the instruction is delivered to Java
- **THEN** its own per-step time SHALL be reported independently of the following manoeuvre
- **AND** the next-next hint fields SHALL keep their existing meaning

## ADDED Requirements

### Requirement: An instruction carries the length of its own leg

A Java `RouteInstruction` SHALL carry the length, in metres, of the leg that ends at its manoeuvre,
independently of whether its distance field carries an absolute or a remaining distance. The value SHALL be
0.0 when the leg is unknown, so that a client can tell "unknown" from "zero-length leg".

#### Scenario: The length relates a remaining distance to its leg

- **GIVEN** an instruction whose leg has a known length
- **WHEN** the instruction is delivered to Java
- **THEN** its leg length SHALL be that leg's length
- **AND** it SHALL NOT change as the position moves along the leg

#### Scenario: A leg whose beginning is behind the position reports unknown

- **GIVEN** an instruction list that was rebuilt from the current position
- **WHEN** the first instruction of that list is delivered to Java
- **THEN** its leg length SHALL be 0.0
- **AND** it SHALL NOT report a cumulative distance from the route start
