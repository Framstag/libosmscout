# Spec Delta

## MODIFIED Requirements

### Requirement: Java RouteInstruction data class

The system SHALL provide a Java `RouteInstruction` class with fields: `distanceTo` (double, meters), `timeTo` (double, seconds), `turnType` (TurnType enum), `streetName` (String), `description` (String), `shortDescription` (String).

#### Scenario: RouteInstruction constructed from JNI
- **WHEN** the JNI bridge creates a `RouteInstruction` object
- **THEN** all of its fields SHALL be populated from the C++ `JavaRouteInstruction` struct, including the per-step time

#### Scenario: RouteInstruction constructed without a time
- **WHEN** a `RouteInstruction` is constructed through the constructor that takes no per-step time
- **THEN** `timeTo` SHALL be 0.0
- **AND** the remaining fields SHALL be populated as given

## ADDED Requirements

### Requirement: An instruction carries the per-step time of its segment

A Java `RouteInstruction` SHALL carry the estimated time of the route segment that ends at that instruction,
in seconds. The value SHALL be the difference between the route description's time at that instruction's
node and the time at the preceding node, truncated to whole seconds, and SHALL NOT be derived from the
instruction's distance. When the route description provides no time for that segment, the instruction SHALL
carry 0.0 and no error SHALL be raised, so that a client can treat 0.0 as "unknown".

#### Scenario: The full instruction list carries the time of each step

- **GIVEN** a route whose description provides a time for every node
- **WHEN** the full instruction list is delivered to Java
- **THEN** each instruction SHALL carry the time between the node preceding it and its own node
- **AND** its distance SHALL be the distance from the start of the route, as before

#### Scenario: The next instruction carries the time of its own step

- **GIVEN** a route whose description provides times
- **WHEN** the next instruction is delivered to Java
- **THEN** it SHALL carry the time between the node preceding it and its own node
- **AND** that time SHALL be reported as the time of the whole segment, independent of how much of the
  segment has already been travelled

#### Scenario: A segment without a time reports zero

- **GIVEN** a route description that provides no time for a segment
- **WHEN** an instruction for that segment is delivered to Java
- **THEN** its per-step time SHALL be 0.0
- **AND** no error SHALL be raised

#### Scenario: The time is not part of the next-next hint

- **GIVEN** an instruction whose following manoeuvre is close enough to be reported as the next-next hint
- **WHEN** the instruction is delivered to Java
- **THEN** its own per-step time SHALL be reported independently of the following manoeuvre
- **AND** the next-next hint fields SHALL keep their existing meaning
