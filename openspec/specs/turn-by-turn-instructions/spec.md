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

The system SHALL provide a Java `RouteInstruction` class with fields: `distanceTo` (double, meters), `timeTo` (double, seconds), `turnType` (TurnType enum), `streetName` (String), `description` (String), `shortDescription` (String).

#### Scenario: RouteInstruction constructed from JNI
- **WHEN** the JNI bridge creates a `RouteInstruction` object
- **THEN** all of its fields SHALL be populated from the C++ `JavaRouteInstruction` struct, including the per-step time

#### Scenario: RouteInstruction constructed without a time
- **WHEN** a `RouteInstruction` is constructed through the constructor that takes no per-step time
- **THEN** `timeTo` SHALL be 0.0
- **AND** the remaining fields SHALL be populated as given

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
