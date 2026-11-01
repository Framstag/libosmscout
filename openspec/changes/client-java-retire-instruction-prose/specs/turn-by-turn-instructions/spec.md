# Spec Delta

## MODIFIED Requirements

### Requirement: RouteInstructionAgent is registered in the Java navigation engine

The C++ `JavaNavigationController` SHALL include `RouteInstructionAgent<osmscout::client::RouteStep, osmscout::client::RouteStepBuilder>` in its `NavigationEngine` agent list. The engine SHALL be initialized with the vehicle type from the `RoutingProfile`.

#### Scenario: Engine includes RouteInstructionAgent
- **WHEN** a `JavaNavigationController` is constructed
- **THEN** its `NavigationEngine` SHALL have a `RouteInstructionAgent` registered alongside the existing agents

#### Scenario: Engine initialized with bicycle vehicle
- **WHEN** `startNavigation` is called with `Vehicle.BICYCLE`
- **THEN** the `NavigationEngine` SHALL use `osmscout::vehicleBicycle` for agent configuration

### Requirement: Next route instruction is bridged to Java

The C++ `DispatchMessage` SHALL handle `NextRouteInstructionsMessage<osmscout::client::RouteStep>` and invoke `NavigationListener.onNextRouteStep(RouteStep)` via JNI, marshalling the step through the same projection a calculated route's steps use.

#### Scenario: Next instruction emitted
- **WHEN** the `RouteInstructionAgent` emits a `NextRouteInstructionsMessage`
- **THEN** `DispatchMessage` SHALL construct a Java `RouteStep` object and call `NavigationListener.onNextRouteStep`

### Requirement: Full instruction list is bridged to Java

The C++ `DispatchMessage` SHALL handle `RouteInstructionsMessage<osmscout::client::RouteStep>` and invoke `NavigationListener.onRouteSteps(RouteStep[])` via JNI, marshalling each step through the same projection a calculated route's steps use.

#### Scenario: Route changes
- **WHEN** a new route is set or the route changes
- **THEN** `DispatchMessage` SHALL construct a Java `RouteStep` array and call `NavigationListener.onRouteSteps`

### Requirement: Next turn displayed in UI

The JavaScout `RoutePanel` SHALL show the next manoeuvre when `onNextRouteStep` is called, deriving the turn icon, distance and street name from the `RouteStep`. The turn icon SHALL adapt to the active vehicle type (e.g., walking icon for pedestrian).

#### Scenario: Next turn shown
- **WHEN** `NavigationListener.onNextRouteStep` is called with a valid step
- **THEN** the `RoutePanel` SHALL display the turn icon, distance in meters/kilometers, and street name

#### Scenario: No next instruction
- **WHEN** `onNextRouteStep` is called with the arrival step
- **THEN** the UI SHALL show "Destination reached" or clear the next-turn display

#### Scenario: Turn icon adapts to vehicle
- **WHEN** navigation is active with `Vehicle.BICYCLE`
- **THEN** the next-turn overlay SHALL show a bicycle icon instead of the default car icon
- **WHEN** navigation is active with `Vehicle.PEDESTRIAN`
- **THEN** the next-turn overlay SHALL show a walking icon

### Requirement: Callbacks are default methods

`NavigationListener.onRouteSteps` and `NavigationListener.onNextRouteStep` SHALL be Java default methods (no-op) so existing implementations continue to compile.

#### Scenario: Existing listener compiles
- **WHEN** a class implements `NavigationListener` without defining `onRouteSteps` or `onNextRouteStep`
- **THEN** compilation SHALL succeed and the default no-op body SHALL be used

### Requirement: The next-turn display shows the exit and the destination as data

The JavaScout next-turn display SHALL show the exit reference and the destination sign texts when the delivered step carries them, rendering them from the step's fields and not from any prose text. When they are absent the display SHALL keep its current appearance.

#### Scenario: A motorway exit is announced with its number

- **GIVEN** a next step that leaves a motorway at a junction with reference "12" and destination "Dortmund"
- **WHEN** the next-turn display updates
- **THEN** it SHALL show the exit reference 12 and the destination Dortmund
- **AND** it SHALL obtain both from the step's fields

#### Scenario: A turn without those facts renders as before

- **GIVEN** a next step that carries no exit reference and no destination
- **WHEN** the next-turn display updates
- **THEN** it SHALL render the turn icon, distance and street name exactly as before

## REMOVED Requirements

### Requirement: Java RouteInstruction data class

### Requirement: "Next next" hint for close-following manoeuvres

### Requirement: An instruction carries the per-step time of its segment

### Requirement: A roundabout instruction carries the exit taken

### Requirement: A route instruction carries the ring's circulation direction

### Requirement: The next-next hint carries its own roundabout exit ordinal

### Requirement: A route instruction can be built without the roundabout values

### Requirement: An instruction carries the length of its own leg

### Requirement: Route list instructions and the step model agree
