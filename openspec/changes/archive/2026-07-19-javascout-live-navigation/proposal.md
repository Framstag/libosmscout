# JavaScout live navigation

## Why

JavaScout already imports GPX tracks and calculates static routes, but it cannot replay a recorded track as a simulated vehicle position or process live GPS updates. To make the demo useful for navigation development and testing, JavaScout needs turn-by-turn navigation behavior: route snapping, off-route detection, rerouting, speed-limit display, and auto-center/follow.

## What Changes

- Extend `libosmscout-client-java` JNI layer to keep the `RouteDescriptionRef` produced during route calculation and expose a `NavigationController` API.
- Add Java-side `NavigationController` and callback interfaces so JavaScout can process `GPSUpdate`/`TimeTick` messages, receive `PositionChanged`, `RerouteRequest`, `TargetReached`, `ArrivalEstimate`, `CurrentSpeed`, `MaxAllowedSpeed`, `LaneUpdate`, and `VoiceInstruction` events.
- Add JavaScout playback controls for imported GPX tracks with speed multiplier (default 1x).
- Add JavaScout UI for live GPS mode (manual position input or future serial/location provider).
- Add map auto-center / follow-heading mode driven by navigation position estimates.
- Update `RoutePanel` to show current/next turn and lane hints during navigation.
- Add CMake/Meson build updates and JNI tests.

## Capabilities

### New Capabilities

- `javascout-navigation`: Turn-by-turn navigation in JavaScout via libosmscout NavigationEngine, including off-route detection, reroute requests, and speed/lane/voice callbacks.
- `javascout-track-playback`: Replay imported GPX tracks as simulated GPS fixes with configurable speed multiplier and auto-center/follow.
- `javascout-map-follow`: Map auto-center and rotation to follow estimated vehicle heading during navigation.

### Modified Capabilities

- `map-rendering`: Map renderer needs current-location marker and optional heading indicator during navigation.

## Impact

- `libosmscout-client-java/src/OSMScoutClient.cpp` and new files: route-description retention, navigation engine wiring, JNI callbacks.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/`: new `NavigationController`, `NavigationListener`, `NavigationPosition`, etc.
- `JavaScout/src/main/java/com/framstag/libosmscout/`: playback controls, follow mode, current-location overlay.
- Build files for `libosmscout-client-java` and `JavaScout`.
- No breaking change to existing routing/search/render APIs.
