# Tasks

## 1. Java data class and listener callback

*All code in `libosmscout-client-java/java/com/framstag/libosmscout/client/`*

- [x] 1.1 Create `CurrentRoadInfo.java` with `ref`, `typeName`, `name` fields, `hasInfo()`, and `toDisplayString()` methods (spec: javascout-current-road-info, 1 SP)
- [x] 1.2 Add `CurrentRoadInfo.java` to JavaScout source tree (spec: javascout-current-road-info, 1 SP)

## 2. JNI bridge

*All code in `libosmscout-client-java/src/OSMScoutClient.cpp`*

- [x] 2.1 Add throttle fields `ROAD_INFO_THROTTLE_MS`, `lastRoadInfoTime`, `lastRoadInfoLat`, `lastRoadInfoLon` (spec: javascout-current-road-info, 1 SP)
- [x] 2.2 Implement `updateRoadInfoFromPosition()` using `client.getDescription()` on background thread, parsing General section entries for Ref/Type/Name (spec: javascout-current-road-info, 3 SP)
- [x] 2.3 Wire `updateRoadInfoFromPosition()` in `onPositionEstimate` handler (spec: javascout-current-road-info, 1 SP)

## 3. Build system

- [x] 3.1 No JNI or build system changes needed — road info derived from `getDescription()` API (spec: javascout-current-road-info, 1 SP)

## 4. JavaScout UI overlay

*All code in `JavaScout/src/main/java/com/framstag/libosmscout/MainController.java`*

- [x] 4.1 Add `currentRoadBox` and `currentRoadLabel` fields (spec: javascout-current-road-info, 1 SP)
- [x] 4.2 Create `createCurrentRoadOverlay()` method with muted gray styling, positioned top-left above next-turn overlay (spec: javascout-current-road-info, 2 SP)
- [x] 4.3 Wire `onCurrentRoadInfo` in `NavigationListener` to `updateCurrentRoadOverlay()` (spec: javascout-current-road-info, 1 SP)
- [x] 4.4 Implement `updateCurrentRoadOverlay()` to show/hide overlay based on `hasInfo()` (spec: javascout-current-road-info, 1 SP)
- [x] 4.5 Hide current road overlay in `stopNavigation()` (spec: javascout-current-road-info, 1 SP)

## 5. Testing

- [x] 5.1 Manual test: start navigation on a road with name+ref+type, verify all three shown in overlay (spec: javascout-current-road-info, 1 SP)
- [x] 5.2 Manual test: navigate to an unnamed road, verify overlay shows only available fields (spec: javascout-current-road-info, 1 SP)
- [x] 5.3 Manual test: go off-route, verify overlay hides (spec: javascout-current-road-info, 1 SP)
- [x] 5.4 Manual test: stop navigation, verify overlay is gone (spec: javascout-current-road-info, 1 SP)
