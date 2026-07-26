# JavaScout automatic rerouting

## Why

JavaScout has live navigation: it tracks position, shows next turns, displays speed/ETA/lanes, and detects off-route conditions. But when the vehicle leaves the planned route, nothing happens. The `onRerouteRequest` callback fires with the current position, bearing, and destination — but the handler in `MainController` is a TODO comment.

Without rerouting, "live routing" is a display that goes stale the moment you deviate. The engine does the hard part (off-route detection); the app just needs to close the loop.

## What Changes

- Wire `onRerouteRequest` in `MainController` to automatically recalculate a new route from the current position to the original destination
- Add a reroute cooldown to prevent rapid re-calculation loops when still off-route after a reroute
- Show visual feedback during rerouting (spinner, "Rerouting..." toast)
- Handle edge cases: destination already reached, reroute failure, navigation not active
- Update `RoutePanel` to show reroute status

## Capabilities

### New Capabilities

- `javascout-auto-rerouting`: Automatic route recalculation when the navigation engine detects the vehicle has left the planned route

### Modified Capabilities

- `javascout-navigation`: Reroute request callback was defined but unhandled; now it triggers a full reroute flow

## Impact

- `JavaScout/src/main/java/com/framstag/libosmscout/MainController.java` — wire reroute handler, cooldown logic, status feedback
- `JavaScout/src/main/java/com/framstag/libosmscout/RoutePanel.java` — reroute status indicator
- No changes to `libosmscout-client-java` JNI layer or C++ code
- No changes to build files
