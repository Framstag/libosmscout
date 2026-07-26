# Tasks: JavaScout fix auto-zooming

## 1. Auto-zoom guard for unknown speed

*All code in `JavaScout/src/main/java/com/framstag/libosmscout/MainController.java`*

- [x] 1.1 Add `lastSpeedKmH >= 0` guard in `onPositionEstimate` auto-zoom block to skip zoom when speed unknown (spec: javascout-fix-auto-zooming, 1 SP)
- [x] 1.2 On first position estimate (`lastSpeedKmH < 0`), jump directly to default-speed target mag instead of smoothing from 5 (spec: javascout-fix-auto-zooming, 1 SP)

## 2. Smooth zoom transitions

*All code in `JavaScout/src/main/java/com/framstag/libosmscout/MainController.java`*

- [x] 2.1 Add `currentSmoothMag` field initialized to 15.0 (routing-sensible default) (spec: javascout-fix-auto-zooming, 1 SP)
- [x] 2.2 Implement 1-level-per-update smoothing: move `currentSmoothMag` toward target by at most 1.0 per position update (spec: javascout-fix-auto-zooming, 1 SP)
- [x] 2.3 Round `currentSmoothMag` to int when calling `renderer.requestRenderPreserveRoute()` (spec: javascout-fix-auto-zooming, 1 SP)

## 3. Speed spike rejection

*All code in `JavaScout/src/main/java/com/framstag/libosmscout/MainController.java`*

- [x] 3.1 Add `lastGoodSpeedKmH` field initialized to 20.0 (spec: javascout-fix-auto-zooming, 1 SP)
- [x] 3.2 In auto-zoom block: if `lastSpeedKmH <= 150`, accept and update `lastGoodSpeedKmH`; if > 150, use `lastGoodSpeedKmH` instead (spec: javascout-fix-auto-zooming, 2 SP)

## 4. Turn-aware zoom

*All code in `JavaScout/src/main/java/com/framstag/libosmscout/MainController.java`*

- [x] 4.1 Add `nextTurnDistanceM` field, updated from `onNextRouteInstruction` callback (spec: javascout-fix-auto-zooming, 1 SP)
- [x] 4.2 Add `turnZoomActive` flag with distance-based activation (< 600m) and deactivation (> 1200m) (spec: javascout-fix-auto-zooming, 1 SP)
- [x] 4.3 When turn zoom active: boost target mag to min 16.0 at < 300m, min 15.0 at < 600m (spec: javascout-fix-auto-zooming, 2 SP)

## 5. SpeedAgent fixes

*All code in `libosmscout/src/osmscout/navigation/SpeedAgent.cpp`*

- [x] 5.1 Reset FIFO when GPS gap > 10 seconds (tunnel dropout) (spec: javascout-fix-auto-zooming, 2 SP)
- [x] 5.2 Cap computed speed at 200 km/h: report `-1.0` if exceeded (spec: javascout-fix-auto-zooming, 1 SP)

## 6. SPEED_ZOOM_TABLE adjustment

*All code in `JavaScout/src/main/java/com/framstag/libosmscout/MainController.java`*

- [x] 6.1 Narrow table range to 17→13 (4 levels) with fractional `double` magnification values (spec: javascout-fix-auto-zooming, 1 SP)
- [x] 6.2 Change `SpeedZoomLevel` record to use `double magnification` instead of `int` (spec: javascout-fix-auto-zooming, 1 SP)
- [x] 6.3 Change `computeSpeedZoom` to return `double` (no rounding) (spec: javascout-fix-auto-zooming, 1 SP)

## 7. Debug logging

*All code in `JavaScout/src/main/java/com/framstag/libosmscout/MainController.java`*

- [x] 7.1 Add `[AutoZoom]` log for initial zoom level (spec: javascout-fix-auto-zooming, 1 SP)
- [x] 7.2 Add `[AutoZoom]` log for turn zoom activation/deactivation (spec: javascout-fix-auto-zooming, 1 SP)
- [x] 7.3 Add `[AutoZoom]` log when still converging (> 0.5 level from target) (spec: javascout-fix-auto-zooming, 1 SP)
- [x] 7.4 Add `[AutoZoom]` log for speed spike rejection (spec: javascout-fix-auto-zooming, 1 SP)

## 8. Icon path log removal

*All code in `libosmscout-client-java/src/OSMScoutClient.cpp`*

- [x] 8.1 Remove `std::cerr` line for icon path; keep only `osmscout::log.Debug()` (spec: javascout-fix-auto-zooming, 1 SP)

## Summary

- **Total tasks:** 20
- **Completed:** 20
- **Remaining:** 0
