# Spec Delta

## Purpose

Display visual turn-by-turn guidance during an active navigation session in JavaScout, showing the next manoeuvre, distance to it, turn type, and street name.

## ADDED Requirements

### Requirement: A roundabout instruction carries the exit taken

A Java `RouteInstruction` SHALL carry `roundaboutExitNumber` (int): the 1-based ordinal, counted in
circulation order, of the exit the route takes, and 0 when the instruction is not a roundabout exit. It
SHALL be the exit taken, which is what `RoundaboutLeaveDescription::GetExitCount()` carries
(`RoutePostprocessor::roundaboutCrossingCounter` counts the exits passed), and SHALL NOT be the number of
exits the ring has. The existing text fields SHALL stay byte-identical.

#### Scenario: A roundabout exit instruction reports its ordinal

- **WHEN** the bridge marshals an instruction built from a `RoundaboutLeaveDescription`
- **THEN** `roundaboutExitNumber` SHALL be the ordinal of the exit taken
- **AND** `description` and `shortDescription` SHALL stay byte-identical to what they were before

#### Scenario: A non-roundabout instruction reports no exit

- **WHEN** the bridge marshals a turn, a lane or an arrival instruction
- **THEN** `roundaboutExitNumber` SHALL be 0

### Requirement: A route instruction carries the ring's circulation direction

A Java `RouteInstruction` SHALL carry `roundaboutClockwise` (boolean): the circulation direction of the
roundabout the instruction is about. It SHALL default to `true` when the instruction is not a roundabout
manoeuvre, so that a consumer reads a direction only where an ordinal exists.

#### Scenario: Entering a roundabout reports the direction

- **WHEN** the bridge marshals an instruction built from a `RoundaboutEnterDescription`
- **THEN** `roundaboutClockwise` SHALL be that ring's circulation direction
- **AND** `roundaboutExitNumber` SHALL be 0, because entering a ring does not take an exit

#### Scenario: Leaving a roundabout reports the direction

- **WHEN** the bridge marshals an instruction built from a `RoundaboutLeaveDescription`
- **THEN** `roundaboutClockwise` SHALL be that ring's circulation direction

#### Scenario: A non-roundabout instruction reports the default

- **WHEN** the bridge marshals a turn, a lane or an arrival instruction
- **THEN** `roundaboutClockwise` SHALL be `true`

#### Scenario: A node without a roundabout description reports the defaults

- **WHEN** the route description carries no roundabout description for an instruction's node
- **THEN** the instruction SHALL report `roundaboutExitNumber` 0 and `roundaboutClockwise` `true`
- **AND** no error SHALL be raised

### Requirement: The next-next hint carries its own roundabout exit ordinal

A Java `RouteInstruction` SHALL carry `nextNextRoundaboutExitNumber` (int) for the "next next" hint's own
manoeuvre, on the same terms as `roundaboutExitNumber`, because that manoeuvre can be a different
roundabout from the one the current instruction leaves. It SHALL be 0 when the hint is absent or when the
hint's manoeuvre is not a roundabout exit.

#### Scenario: The hint's own roundabout ordinal is reported

- **GIVEN** an instruction whose following manoeuvre is close enough to be reported as the next-next hint
- **WHEN** that hint's manoeuvre takes a roundabout exit
- **THEN** `nextNextRoundaboutExitNumber` SHALL be that manoeuvre's own exit ordinal
- **AND** it MAY describe a different roundabout from the one `roundaboutExitNumber` describes

#### Scenario: A hint that is no roundabout exit reports zero

- **GIVEN** an instruction with a next-next hint
- **WHEN** the hint's manoeuvre is not a roundabout exit, or no hint is present
- **THEN** `nextNextRoundaboutExitNumber` SHALL be 0

### Requirement: A route instruction can be built without the roundabout values

The Java `RouteInstruction` SHALL keep a constructor arity that predates the roundabout values, so a
caller that constructs an instruction without them compiles unchanged. The roundabout values SHALL then
take their "no roundabout" defaults (0, `true`, 0), which is the state such a caller observed before they
existed.

#### Scenario: The constructor with the roundabout fields but without the next-next ordinal

- **WHEN** a `RouteInstruction` is constructed through the constructor that takes the roundabout fields but not the next-next ordinal
- **THEN** `nextNextRoundaboutExitNumber` SHALL be 0
- **AND** all other given values SHALL be carried

#### Scenario: The constructor without the roundabout fields

- **WHEN** a `RouteInstruction` is constructed through the full constructor that predates the roundabout fields
- **THEN** `roundaboutExitNumber` SHALL be 0, `roundaboutClockwise` SHALL be `true` and `nextNextRoundaboutExitNumber` SHALL be 0
- **AND** the remaining fields SHALL be populated as given
