## Why

JavaScout can compute routes, track the vehicle position, show speed/ETA/lanes, and even play voice guidance — but the driver sees no visual turn-by-turn instructions. The C++ `RouteInstructionAgent` exists and works in the Qt client, but it's not wired into the JavaScout navigation engine or its JNI bridge. Without it, the UI can't show "Turn left in 200m" or highlight the next manoeuvre.

## What Changes

- Add `RouteInstructionAgent` to the `JavaNavigationController` engine (matching the Qt `NavigationModule` setup)
- Bridge `RouteInstructionsMessage` and `NextRouteInstructionsMessage` through JNI to Java
- Add `onRouteInstructions` / `onNextRouteInstruction` callbacks to `NavigationListener`
- Create a Java `RouteInstruction` data class for the bridged instruction data
- Show the next turn instruction in the JavaScout UI (distance, turn type, street name)

## Capabilities

### New Capabilities
- `turn-by-turn-instructions`: Visual turn-by-turn guidance displayed during an active navigation session, showing the next manoeuvre, distance to it, turn type, and street name

### Modified Capabilities

(none)

## Impact

- **libosmscout-client-java/src/OSMScoutClient.cpp**: Add `RouteInstructionAgent` to engine; handle instruction messages in `DispatchMessage`; add JNI method IDs for new callbacks
- **libosmscout-client-java/java/.../NavigationListener.java**: New `onRouteInstructions` and `onNextRouteInstruction` default methods
- **libosmscout-client-java/java/.../NavigationController.java**: No change (instructions flow through listener, not controller)
- **JavaScout/src/.../MainController.java**: Wire new callbacks to update the UI
- **JavaScout/src/.../RoutePanel.java**: Dedicated next-turn display area (distance, icon, street name)
- **JavaScout/src/.../RouteInstruction.java**: New data class mirroring the C++ instruction fields
