## 1. JNI API — Add angle parameter to render methods

- [x] 1.1 Add `double angle` parameter to `render()` and `renderWithRouteAndPois()` native declarations in `OSMScoutClient.java` (spec: map-rotation, 1 SP)
- [x] 1.2 Update C++ JNI bridge `OSMScoutClient.cpp`: pass angle to `MercatorProjection::Set()` instead of hardcoded `0.0` (spec: map-rotation, 2 SP)
- [x] 1.3 Add `double angle` parameter to `projectToPixel()` native method in Java and C++ (spec: map-rotation, 1 SP)
- [x] 1.4 Add `renderWithRoute()` convenience overload that accepts angle (spec: map-rotation, 1 SP)

## 2. MapRenderer — Track and pass rotation angle

- [x] 2.1 Add `currentAngle` field (double, radians, default 0.0) to `MapRenderer.java` (spec: map-rotation, 1 SP)
- [x] 2.2 Add `setAngle(double)` / `getAngle()` methods to `MapRenderer` (spec: map-rotation, 1 SP)
- [x] 2.3 Update `requestRender()` and `requestRenderPreserveRoute()` to accept and forward angle (spec: map-rotation, 1 SP)
- [x] 2.4 Update `doRender()` to pass `currentAngle` to JNI render call (spec: map-rotation, 1 SP)
- [x] 2.5 Update `drawCurrentLocationMarker()` to use rotated projection via `projectToPixel(..., angle)` (spec: map-rotation, 2 SP)

## 3. MainController — Rotation mode and compass button

- [x] 3.1 Add `MapRotationMode` enum (`NORTH_UP`, `DRIVING_DIRECTION_UP`) and `rotationMode` field to `MainController.java` (spec: compass-toggle, 1 SP)
- [x] 3.2 Create compass button with SVGPath icon (circle + north-pointing triangle), positioned bottom-right above follow button (spec: compass-toggle, 2 SP)
- [x] 3.3 Implement toggle logic: tap compass switches rotation mode, updates button visual state (spec: compass-toggle, 1 SP)
- [x] 3.4 Wire compass icon rotation: when map angle changes, rotate compass icon counter-phase so north pointer stays accurate (spec: compass-toggle, 2 SP)
- [x] 3.5 Wire `NavigationListener.onPositionEstimate()` to set `renderer.setAngle(position.bearing)` when `followMode && rotationMode == DRIVING_DIRECTION_UP` (spec: map-rotation, 2 SP)
- [x] 3.6 Reset angle to 0 when switching to NORTH_UP mode (spec: compass-toggle, 1 SP)
- [x] 3.7 Ensure manual pan/zoom disables follow-mode (existing behavior) which stops rotation (spec: map-rotation, 1 SP)

## 4. Build & Test

- [x] 4.1 Build JavaScout with Maven and verify no compilation errors (spec: map-rotation, 1 SP)
- [x] 4.2 Run JavaScout with a map database, activate navigation, verify map rotates to driving direction (spec: map-rotation, 3 SP)
- [x] 4.3 Test compass toggle: switch between north-up and driving-direction-up, verify visual states (spec: compass-toggle, 2 SP)
- [x] 4.4 Test manual pan/zoom during rotation: verify follow-mode disables and rotation stops (spec: map-rotation, 2 SP)
- [x] 4.5 Test re-enabling follow-mode: verify rotation resumes (spec: map-rotation, 1 SP)
