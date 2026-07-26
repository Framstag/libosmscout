## 1. C++ JNI Library — JavaRouteInstruction types and builder

*All code in `libosmscout-client-java/src/OSMScoutClient.cpp`*

- [x] 1.1 Define `JavaRouteInstruction` struct with fields: distanceTo, turnType, streetName, description, shortDescription (1 SP)
- [x] 1.2 Implement `JavaRouteInstructionBuilder` class with `GenerateRouteInstructions()` and `GenerateNextRouteInstruction()` methods, walking `RouteDescription::Node` iterators (3 SP)
- [x] 1.3 Add `RouteInstructionAgent<JavaRouteInstruction, JavaRouteInstructionBuilder>` to the `JavaNavigationController` engine constructor (1 SP)
- [x] 1.4 Handle `RouteInstructionsMessage<JavaRouteInstruction>` in `DispatchMessage()` — construct Java `RouteInstruction[]` array and call `onRouteInstructions` on listener (2 SP)
- [x] 1.5 Handle `NextRouteInstructionsMessage<JavaRouteInstruction>` in `DispatchMessage()` — construct single Java `RouteInstruction` and call `onNextRouteInstruction` on listener (1 SP)

## 2. Java Client Library — API types and listener

*All code in `libosmscout-client-java/java/com/framstag/libosmscout/client/`*

- [x] 2.1 Create `RouteInstruction` data class with fields: distanceTo, turnType, streetName, description, shortDescription (1 SP)
- [x] 2.2 Create `TurnType` enum with values matching `RouteDescription::DirectionDescription::Move` string representations (1 SP)
- [x] 2.3 Add `onRouteInstructions(RouteInstruction[])` and `onNextRouteInstruction(RouteInstruction)` default methods to `NavigationListener` (1 SP)
- [x] 2.4 Cache JNI method IDs for the new callbacks in `NavigationListenerMethods` struct and look them up during `startNavigation` (1 SP)

## 3. JavaScout App — UI wiring

*All code in `JavaScout/src/main/java/com/framstag/libosmscout/`*

- [x] 3.1 Wire `onNextRouteInstruction` in `MainController`'s `NavigationListener` to update the `RoutePanel` next-turn display (1 SP)
- [x] 3.2 Add dedicated next-turn display area to `RoutePanel` showing turn icon, distance, and street name (2 SP)
- [x] 3.3 Wire `onRouteInstructions` in `MainController` to populate the route instruction list in `RoutePanel` (1 SP)
- [x] 3.4 Handle edge cases: destination reached (distanceTo ≈ 0), missing street name, unknown turn type (1 SP)

## 4. Testing

- [x] 4.1 Update `OSMScoutClientNavigationLiveTest` to verify `onNextRouteInstruction` is called after feeding GPS fixes (2 SP)
- [x] 4.2 Verify existing `NavigationListener` implementations compile with new default methods (1 SP)
