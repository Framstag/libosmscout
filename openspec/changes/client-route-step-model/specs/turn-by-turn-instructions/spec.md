# Spec Delta

## MODIFIED Requirements

### Requirement: Java RouteInstruction data class

The system SHALL provide a Java `RouteInstruction` class with fields: `distanceTo` (double, meters), `timeTo` (double, seconds), `legDistance` (double, meters, 0 if unknown), `turnType` (TurnType enum), `streetName` (String), `description` (String), `shortDescription` (String), plus the manoeuvre facts of its route node as published by the client library's route step model: the motorway junction's exit reference and name, the destination sign texts, the way type and the maximum speed. Each added field SHALL be readable as data and SHALL NOT have to be derived from `description` or `shortDescription`.

#### Scenario: RouteInstruction constructed from JNI
- **WHEN** the JNI bridge creates a `RouteInstruction` object
- **THEN** all of its fields SHALL be populated from the C++ `JavaRouteInstruction` struct, including the per-step time and the leg's length

#### Scenario: RouteInstruction constructed without a time
- **WHEN** a `RouteInstruction` is constructed through the constructor that takes no per-step values
- **THEN** `timeTo` and `legDistance` SHALL be 0.0
- **AND** the remaining fields SHALL be populated as given

#### Scenario: The manoeuvre facts arrive with the instruction

- **GIVEN** a route node that carries a motorway-junction description with a reference and a name and a destination description
- **WHEN** the instruction for that node is delivered to Java
- **THEN** it SHALL carry the exit reference, the junction name and the destinations
- **AND** `description` and `shortDescription` SHALL stay byte-identical to what they were before the fields were added

#### Scenario: An instruction without those facts reports none

- **GIVEN** a turn instruction whose node carries no junction, destination, way-type or max-speed description
- **WHEN** the instruction is delivered to Java
- **THEN** the added fields SHALL report "absent" rather than a zero or empty value indistinguishable from a known one
- **AND** no error SHALL be raised

#### Scenario: Every existing constructor keeps working

- **WHEN** a caller constructs a `RouteInstruction` through any constructor that predates the added fields
- **THEN** compilation SHALL succeed
- **AND** the added fields SHALL take their "absent" values

## ADDED Requirements

### Requirement: Route list instructions and the step model agree

When the bridge derives both the description lines and the structured steps for the same route, the values of a manoeuvre that both represent SHALL be equal: the instruction for a manoeuvre SHALL NOT report a turn type, an exit reference or a destination that differs from what its step carries.

#### Scenario: The dialog and the live guidance describe one manoeuvre alike

- **GIVEN** a route whose description lines and steps are both published
- **WHEN** the manoeuvre at a given node is read from the description-derived instruction and from its step
- **THEN** turn type, exit reference, junction name and destinations SHALL be equal
- **AND** no manoeuvre present in the steps SHALL be missing from the description lines

### Requirement: The next-turn display shows the exit and the destination as data

The JavaScout next-turn display SHALL show the exit reference and the destination sign texts when the delivered instruction carries them, rendering them from the instruction's fields and not from its description text. When they are absent the display SHALL keep its current appearance.

#### Scenario: A motorway exit is announced with its number

- **GIVEN** a next instruction that leaves a motorway at a junction with reference "12" and destination "Dortmund"
- **WHEN** the next-turn display updates
- **THEN** it SHALL show the exit reference 12 and the destination Dortmund
- **AND** it SHALL obtain both from the instruction's fields

#### Scenario: A turn without those facts renders as before

- **GIVEN** a next instruction that carries no exit reference and no destination
- **WHEN** the next-turn display updates
- **THEN** it SHALL render the turn icon, distance and street name exactly as before
