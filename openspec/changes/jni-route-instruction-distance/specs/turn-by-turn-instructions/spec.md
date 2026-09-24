# Spec Delta

## ADDED Requirements

### Requirement: Instruction builder is offered the position's progress along its segment

The navigation instruction agent SHALL pass the progress of the snapped position along its current route segment to the instruction builder whenever the builder accepts it, so a builder does not have to re-derive progress from the raw coordinate. A builder that implements only the coordinate-based call SHALL keep working unchanged and SHALL still be called, and SHALL still produce the instruction that is published to the client.

#### Scenario: Progress-aware builder receives the reported progress

- **GIVEN** a position a quarter of the way along its current route segment, reported by the position agent
- **WHEN** the instruction agent asks its builder for the next instruction
- **THEN** the builder SHALL receive that same progress value for that segment

#### Scenario: Progress-aware builder receives the later progress

- **GIVEN** the same segment, with the position reported half way along it
- **WHEN** the instruction agent asks its builder for the next instruction
- **THEN** the builder SHALL receive the later, larger progress value

#### Scenario: Coordinate-only builder keeps working

- **GIVEN** an instruction builder that implements only the coordinate-based call
- **WHEN** the instruction agent asks it for the next instruction while a position is available
- **THEN** the builder SHALL be called
- **AND** the instruction it returns SHALL be published to the client

### Requirement: Next instruction distance follows the route progress

The remaining distance reported for the next route instruction SHALL be derived from how far the position has progressed along its current route segment, so that a segment that folds back and a fix that lies beside the route do not overstate the distance already travelled. When no progress is reported for that segment, the distance SHALL fall back to the estimate from the straight-line distance between the fix and the route node behind it. The reported remaining distance SHALL NOT be negative.

#### Scenario: Distance shrinks as the position advances

- **GIVEN** a route with a manoeuvre further ahead, and a first position on the segment before it
- **WHEN** the next instruction is delivered for that position, and again for a later position on the same segment
- **THEN** the remaining distance of the later instruction SHALL be smaller than that of the first
- **AND** neither distance SHALL be negative

#### Scenario: No progress reported falls back to the straight-line estimate

- **GIVEN** a position whose segment reports no progress
- **WHEN** the next instruction is delivered
- **THEN** its remaining distance SHALL be reported
- **AND** SHALL NOT be negative

### Requirement: Arrival instruction carries the distance of its own node

Every instruction SHALL carry the distance of its own route node from the start of the route, including the arrival instruction. The arrival instruction SHALL therefore report a distance greater than zero while the destination is still ahead, and SHALL NOT be skipped when the next instruction after the last manoeuvre is searched, so a client is told how far the destination is instead of receiving no instruction at all.

#### Scenario: Arrival instruction in the full list carries the destination distance

- **GIVEN** a route whose full instruction list is delivered to a Java client
- **WHEN** the list is inspected
- **THEN** the arrival instruction SHALL carry a distance greater than zero for a route of non-zero length
- **AND** that distance SHALL be at least as large as the distance carried by every other instruction of the list, the destination being the furthest node from the route start

#### Scenario: Arrival instruction is delivered after the last manoeuvre

- **GIVEN** a position on the route beyond the last manoeuvre but before the destination
- **WHEN** the next instruction is delivered
- **THEN** it SHALL describe the arrival, not an empty instruction
- **AND** the distance it carries SHALL be greater than zero
- **AND** it SHALL be at most the distance from the route start to the destination
