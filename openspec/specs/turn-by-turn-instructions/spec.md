# turn-by-turn-instructions

## Purpose

Display visual turn-by-turn guidance during an active navigation session in JavaScout, showing the next manoeuvre, distance to it, turn type, and street name.

## Requirements

### Requirement: RouteInstructionAgent is registered in the Java navigation engine

The C++ `JavaNavigationController` SHALL include `RouteInstructionAgent<JavaRouteInstruction, JavaRouteInstructionBuilder>` in its `NavigationEngine` agent list. The engine SHALL be initialized with the vehicle type from the `RoutingProfile`.

#### Scenario: Engine includes RouteInstructionAgent
- **WHEN** a `JavaNavigationController` is constructed
- **THEN** its `NavigationEngine` SHALL have a `RouteInstructionAgent` registered alongside the existing agents

#### Scenario: Engine initialized with bicycle vehicle
- **WHEN** `startNavigation` is called with `Vehicle.BICYCLE`
- **THEN** the `NavigationEngine` SHALL use `osmscout::vehicleBicycle` for agent configuration

### Requirement: Next route instruction is bridged to Java

The C++ `DispatchMessage` SHALL handle `NextRouteInstructionsMessage<JavaRouteInstruction>` and invoke `NavigationListener.onNextRouteInstruction(RouteInstruction)` via JNI.

#### Scenario: Next instruction emitted
- **WHEN** the `RouteInstructionAgent` emits a `NextRouteInstructionsMessage`
- **THEN** `DispatchMessage` SHALL construct a Java `RouteInstruction` object and call `NavigationListener.onNextRouteInstruction`

### Requirement: Full instruction list is bridged to Java

The C++ `DispatchMessage` SHALL handle `RouteInstructionsMessage<JavaRouteInstruction>` and invoke `NavigationListener.onRouteInstructions(RouteInstruction[])` via JNI.

#### Scenario: Route changes
- **WHEN** a new route is set or the route changes
- **THEN** `DispatchMessage` SHALL construct a Java `RouteInstruction` array and call `NavigationListener.onRouteInstructions`

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

### Requirement: Java TurnType enum

The system SHALL provide a Java `TurnType` enum with values matching `RouteDescription::DirectionDescription::Move` string representations.

#### Scenario: Turn type mapped
- **WHEN** a `NextRouteInstructionsMessage` carries a turn type string
- **THEN** the JNI bridge SHALL convert it to the corresponding `TurnType` enum value

### Requirement: Next turn displayed in UI

The JavaScout `RoutePanel` SHALL show the next manoeuvre when `onNextRouteInstruction` is called, displaying the turn icon, distance, and street name. The turn icon SHALL adapt to the active vehicle type (e.g., walking icon for pedestrian).

#### Scenario: Next turn shown
- **WHEN** `NavigationListener.onNextRouteInstruction` is called with a valid instruction
- **THEN** the `RoutePanel` SHALL display the turn icon, distance in meters/kilometers, and street name

#### Scenario: No next instruction
- **WHEN** `onNextRouteInstruction` is called with distanceTo = 0 and empty description
- **THEN** the UI SHALL show "Destination reached" or clear the next-turn display

#### Scenario: Turn icon adapts to vehicle
- **WHEN** navigation is active with `Vehicle.BICYCLE`
- **THEN** the next-turn overlay SHALL show a bicycle icon instead of the default car icon
- **WHEN** navigation is active with `Vehicle.PEDESTRIAN`
- **THEN** the next-turn overlay SHALL show a walking icon

### Requirement: Callbacks are default methods

`NavigationListener.onRouteInstructions` and `NavigationListener.onNextRouteInstruction` SHALL be Java default methods (no-op) so existing implementations continue to compile.

#### Scenario: Existing listener compiles
- **WHEN** a class implements `NavigationListener` without defining `onRouteInstructions` or `onNextRouteInstruction`
- **THEN** compilation SHALL succeed and the default no-op body SHALL be used

### Requirement: "Next next" hint for close-following manoeuvres

The `RouteInstruction` class SHALL carry optional fields for a following manoeuvre when the gap between consecutive instructions is ≤ 200m. The UI SHALL display this hint as a second row below the main instruction.

#### Scenario: Roundabout enter and exit
- **WHEN** a roundabout entrance is followed by an exit within 200m
- **THEN** the `RouteInstruction` SHALL have `hasNextNext()` return true and `nextNextDescription` describe the exit

#### Scenario: No close following manoeuvre
- **WHEN** the next instruction is more than 200m ahead of the current one
- **THEN** `hasNextNext()` SHALL return false and the second row SHALL be hidden

### Requirement: Lane guidance sub-component in next-turn display

The JavaScout `RoutePanel` next-turn display SHALL include a lane guidance sub-component that renders lane arrows when lane information is available. The sub-component SHALL be positioned below the turn icon and distance.

#### Scenario: Lane guidance shown in next-turn area
- **WHEN** `onLaneUpdate` is called with valid lane data during an active navigation session
- **THEN** the next-turn display SHALL show lane arrows below the turn icon and distance

#### Scenario: Lane guidance hidden when no lane data
- **WHEN** `onLaneUpdate` is called with `count=0`
- **THEN** the lane guidance sub-component SHALL be hidden
- **AND** the next-turn display SHALL retain its existing layout

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
- **AND** the entries SHALL NOT be required to add up to the route's published `duration`, which is the
  length-derived estimate of `client-java-route-length` at one average speed, while these are the
  description's own per-leg times
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
