## 1. Java Client — LaneTurn Enum & Listener API

- [x] 1.1 Add `LaneTurn.java` enum in `libosmscout-client-java/java/com/framstag/libosmscout/client/` with values matching C++ `osmscout::LaneTurn` (spec: javascout-lane-guidance)
- [x] 1.2 Extend `NavigationListener.onLaneUpdate` signature to add `LaneTurn[] turns` parameter; keep default no-op body for backward compatibility (spec: javascout-navigation)
- [x] 1.3 Verify `libosmscout-client-java` compiles with Maven/Meson after API change

## 2. Java Client — JNI Bridge

- [x] 2.1 Update `NavigationListenerMethods` struct in `OSMScoutClient.cpp` to reflect new `onLaneUpdate` signature with `LaneTurn[]` parameter (spec: javascout-navigation)
- [x] 2.2 Update `GetNavigationListenerMethods` JNI method lookup for new `onLaneUpdate` signature (spec: javascout-navigation)
- [x] 2.3 Update `DispatchMessage` lane handler in `OSMScoutClient.cpp` to build a `LaneTurn[]` Java array from `laneMessage->lane.turns` and pass it to the callback (spec: javascout-navigation)
- [x] 2.4 Verify JNI build compiles and existing tests pass

## 3. JavaScout — MainController Update

- [x] 3.1 Update `MainController.onLaneUpdate` to accept and forward `LaneTurn[] turns` to `RoutePanel.updateLaneInfo` (spec: javascout-navigation)
- [x] 3.2 Verify JavaScout compiles with Maven

## 4. JavaScout — Lane Guidance Overlay

- [x] 4.1 Add lane arrow `SVGPath` definitions in `RoutePanel` for each `LaneTurn` variant (left, slight-left, straight, slight-right, right, combined) (spec: javascout-lane-guidance)
- [x] 4.2 Add `HBox` lane container in `RoutePanel` next-turn display area, positioned below the turn instruction (spec: turn-by-turn-instructions)
- [x] 4.3 Implement `updateLaneInfo` to rebuild lane arrow display from `LaneTurn[]` array, highlighting suggested lane(s) with accent fill (spec: javascout-lane-guidance)
- [x] 4.4 Handle oneway road display (no centre divider) and empty/hide states (spec: javascout-lane-guidance)
- [x] 4.5 Verify JavaScout compiles and lane overlay renders in test navigation

## 5. Build & Test Verification

- [x] 5.1 Run `libosmscout-client-java` Meson build to confirm no regressions
- [x] 5.2 Run JavaScout Maven build to confirm compilation
- [x] 5.3 Run existing `libosmscout` core tests to confirm no regressions
