# Spec Delta

## MODIFIED Requirements

### Requirement: The next-turn display shows the exit and the destination as data

The JavaScout next-turn display SHALL show the exit reference and the destination sign texts when the
delivered step carries them, rendering them from the step's fields and not from its description text. When
they are absent the display SHALL keep its current appearance.

#### Scenario: A motorway exit is announced with its number

- **GIVEN** a next step that leaves a motorway at a junction with reference "12" and destination "Dortmund"
- **WHEN** the next-turn display updates
- **THEN** it SHALL show the exit reference 12 and the destination Dortmund
- **AND** it SHALL obtain both from the step's fields

#### Scenario: A turn without those facts renders as before

- **GIVEN** a next step that carries no exit reference and no destination
- **WHEN** the next-turn display updates
- **THEN** it SHALL render the turn icon, distance and street name exactly as before

## ADDED Requirements

### Requirement: The next-turn display is worded from the published step

The JavaScout next-turn display SHALL word the manoeuvre and choose the turn arrow from the values of the
published step — its kind, the move it carries, the ordinal of a roundabout exit and the road it leads onto —
in the application's own language, and SHALL NOT render them from an instruction-shaped helper. A step that
names no road SHALL be worded as the manoeuvre alone.

#### Scenario: The manoeuvre is worded from the step's kind and move

- **GIVEN** a published step whose kind is a turn and whose move is a left turn
- **WHEN** the next-turn display updates
- **THEN** it SHALL word the manoeuvre as a left turn
- **AND** it SHALL choose the arrow from the step's move

#### Scenario: A roundabout exit is worded with the ordinal taken

- **GIVEN** a published step that leaves a roundabout by an exit
- **WHEN** the next-turn display updates
- **THEN** it SHALL word the manoeuvre with the ordinal of the exit taken

#### Scenario: The following manoeuvre is worded from its own step

- **GIVEN** a step ahead whose following manoeuvre is close enough to be announced
- **WHEN** the next-turn display updates
- **THEN** the second row SHALL word the following manoeuvre and its distance from its own step
- **AND** it SHALL NOT take them from an instruction-shaped helper
