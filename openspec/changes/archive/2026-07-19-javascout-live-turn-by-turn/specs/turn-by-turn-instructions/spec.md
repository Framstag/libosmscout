# turn-by-turn-instructions

## Purpose

Display visual turn-by-turn guidance during an active navigation session in JavaScout, showing the next manoeuvre, distance to it, turn type, and street name.

## ADDED Requirements

### Requirement: RouteInstructionAgent is registered in the Java navigation engine

The C++ `JavaNavigationController` SHALL include `RouteInstructionAgent<JavaRouteInstruction, JavaRouteInstructionBuilder>` in its `NavigationEngine` agent list.

#### Scenario: Engine includes RouteInstructionAgent
- **WHEN** a `JavaNavigationController` is constructed
- **THEN** its `NavigationEngine` SHALL have a `RouteInstructionAgent` registered alongside the existing agents

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

The system SHALL provide a Java `RouteInstruction` class with fields: `distanceTo` (double, meters), `turnType` (TurnType enum), `streetName` (String), `description` (String), `shortDescription` (String).

#### Scenario: RouteInstruction constructed from JNI
- **WHEN** the JNI bridge creates a `RouteInstruction` object
- **THEN** all five fields SHALL be populated from the C++ `JavaRouteInstruction` struct

### Requirement: Java TurnType enum

The system SHALL provide a Java `TurnType` enum with values matching `RouteDescription::DirectionDescription::Move` string representations.

#### Scenario: Turn type mapped
- **WHEN** a `NextRouteInstructionsMessage` carries a turn type string
- **THEN** the JNI bridge SHALL convert it to the corresponding `TurnType` enum value

### Requirement: Next turn displayed in UI

The JavaScout `RoutePanel` SHALL show the next manoeuvre when `onNextRouteInstruction` is called, displaying the turn icon, distance, and street name.

#### Scenario: Next turn shown
- **WHEN** `NavigationListener.onNextRouteInstruction` is called with a valid instruction
- **THEN** the `RoutePanel` SHALL display the turn icon, distance in meters/kilometers, and street name

#### Scenario: No next instruction
- **WHEN** `onNextRouteInstruction` is called with distanceTo = 0 and empty description
- **THEN** the UI SHALL show "Destination reached" or clear the next-turn display

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
