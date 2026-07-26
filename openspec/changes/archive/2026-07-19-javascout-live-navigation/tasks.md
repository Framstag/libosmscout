# Tasks

## 1. C++ route-description retention and navigation handle

- [x] 1.1 Extend `ClientData` to store the generated `RouteDescriptionRef` and a navigation handle after successful route calculation. (spec: javascout-navigation, 3 SP)
- [x] 1.2 Add `NavigationController` C++ class that wraps `NavigationEngine`, listener global ref, and background thread. (spec: javascout-navigation, 5 SP)
- [x] 1.3 Implement `DataAgent` adapter in `OSMScoutClient.cpp` that loads routable objects via `DBThread::RunSynchronousJob`. (spec: javascout-navigation, 5 SP)
- [x] 1.4 Wire `PositionAgent`, `RouteStateAgent`, `BearingAgent`, `ArrivalEstimateAgent`, `SpeedAgent`, `LaneAgent`, and `VoiceInstructionAgent` in the engine. (spec: javascout-navigation, 3 SP)

## 2. JNI bridge for navigation

- [x] 2.1 Create Java classes: `NavigationController`, `NavigationListener`, `NavigationPosition`, `NavigationState`. (spec: javascout-navigation, 3 SP)
- [x] 2.2 Add JNI methods: `startNavigation`, `stopNavigation`, `processLocation`, `cancelNavigation`. (spec: javascout-navigation, 5 SP)
- [x] 2.3 Add JNI callback marshalling for `PositionChanged`, `RerouteRequest`, `TargetReached`, `ArrivalEstimate`, `CurrentSpeed`, `MaxAllowedSpeed`, `LaneUpdate`, `VoiceInstruction`. (spec: javascout-navigation, 5 SP)
- [x] 2.4 Cache `jmethodID`s and attach C++ navigation thread to JVM safely. (spec: javascout-navigation, 2 SP)

## 3. JavaScout track playback UI and controller

- [x] 3.1 Create `TrackPlayer` class that replays `TrackPoint[]` using JavaFX `Timeline` with configurable speed multiplier. (spec: javascout-track-playback, 3 SP)
- [x] 3.2 Add playback toolbar with play, pause, stop, resume, and speed selector. (spec: javascout-track-playback, 2 SP)
- [x] 3.3 Connect `TrackPlayer` output to `NavigationController.processLocation`. (spec: javascout-track-playback, 2 SP)
- [x] 3.4 Add live-GPS mode toggle that feeds the same `NavigationController` from a manual input or future provider. (spec: javascout-navigation, 2 SP)

## 4. Map follow mode and current-location rendering

- [x] 4.1 Add follow-mode toggle and state management to `MainController`. (spec: javascout-map-follow, 3 SP)
- [x] 4.2 Suspend follow mode on manual pan/zoom, resume on explicit toggle. (spec: javascout-map-follow, 2 SP)
- [x] 4.3 Extend `MapRenderer` and `renderWithRouteAndPois` JNI to accept current-location parameters. (spec: javascout-map-follow, 3 SP)
- [x] 4.4 Add a JavaFX current-location overlay drawn on top of the Canvas after blitting the base map; use a native `projectToPixel()` helper for positioning. (spec: map-rendering, 2 SP)

## 5. Route panel navigation status

- [ ] 5.1 Add current/next turn display to `RoutePanel` from `NavigationListener` callbacks. Needs `RouteInstructionAgent` plus a Java-friendly instruction builder; out of scope for this pass. (spec: javascout-navigation, 3 SP)
- [x] 5.2 Display current speed limit and lane information in the route panel. (spec: javascout-navigation, 2 SP)
- [x] 5.3 Show arrival estimate and remaining distance. (spec: javascout-navigation, 2 SP)

## 6. Build and tests

- [x] 6.1 Update CMake and Meson build files for new C++ source files. (spec: javascout-navigation, 2 SP)
- [x] 6.2 Add JNI unit tests for `startNavigation`, `processLocation`, and `stopNavigation` using a sample route. (spec: javascout-navigation, 5 SP)
- [x] 6.3 Add Java unit tests for `TrackPlayer` timing and speed multiplier. (spec: javascout-track-playback, 3 SP)
- [x] 6.4 Build passes with no compiler warnings; native `.so` rebuilt and JavaScout `mvn test` green (36 tests). Manual smoke test with imported GPX track remains out of scope for headless CI. (spec: javascout-track-playback, 3 SP)
