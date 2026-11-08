# Spec Delta

## ADDED Requirements

### Requirement: JavaScout words the route list from the published steps

The JavaScout route description list SHALL word each row from the values of the published route step — the
step's kind, the move it carries, the road it leads onto and the ordinal of a roundabout exit it takes — and
SHALL NOT word a row from an instruction-shaped helper. A step that names a road and carries no turn SHALL
show that road rather than an empty manoeuvre.

#### Scenario: A row's manoeuvre is worded from the step

- **GIVEN** a route whose steps publish a turn, the move it makes and the road it leads onto
- **WHEN** the route description list is shown
- **THEN** the row's manoeuvre SHALL be worded from those values
- **AND** it SHALL NOT come from an instruction-shaped helper

#### Scenario: A step that names a road and carries no turn

- **GIVEN** a published step that names a road and carries no turn
- **WHEN** its row is built
- **THEN** the row SHALL show the road the step leads onto
- **AND** the row SHALL NOT be empty

#### Scenario: A roundabout exit is worded with the ordinal taken

- **GIVEN** a published step that leaves a roundabout by an exit
- **WHEN** its row is built
- **THEN** the row SHALL name the ordinal of the exit taken
- **AND** it SHALL obtain the ordinal from the step
