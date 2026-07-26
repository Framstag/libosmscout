## 1. Java Model Classes

- [x] 1.1 Create `DescriptionEntry.java` — Java class with fields: `sectionKey`, `subsectionKey`, `hasIndex`, `index`, `labelKey`, `value` (Spec: `long-press-description`)
- [x] 1.2 Create `ObjectDescription.java` — Java class with `List<DescriptionEntry> getEntries()` (Spec: `long-press-description`)

## 2. JNI Bridge — C++ Side

- [x] 2.1 Add `DescriptionService` instance to `ClientData` struct in `OSMScoutClient.cpp`, initialized on database open (Design: D5)
- [x] 2.2 Implement JNI `getDescription(lat, lon)` in `OSMScoutClient.cpp`: query objects in bbox around coordinate via `Database` spatial indexes (Design: D1)
- [x] 2.3 Implement object ranking heuristic: filter by (has description data, visible at zoom, proximity), prefer nodes over ways over areas (Design: D1)
- [x] 2.4 Call `DescriptionService::GetDescription()` on best object and marshal `ObjectDescription` entries to Java `DescriptionEntry[]` (Design: D2)
- [x] 2.5 Verify `osmscout` library is linked in `libosmscout-client-java/CMakeLists.txt` and `meson.build`

## 3. JNI Bridge — Java Side

- [x] 3.1 Add `native ObjectDescription getDescription(double lat, double lon)` to `OSMScoutClient.java` (Spec: `long-press-description`, Design: D2)

## 4. Long-Press Detection

- [x] 4.1 Add `PauseTransition` timer to `MapInteractionHandler`: `MOUSE_PRESSED` starts, `MOUSE_RELEASED`/`MOUSE_DRAGGED` cancels (Design: D3)
- [x] 4.2 Add `Runnable onLongPress` callback to `MapInteractionHandler` with `(double lat, double lon)` parameters (Design: D3)
- [x] 4.3 Add `longPressTimeoutMs` property to `Config` class with default 500ms, read by `MapInteractionHandler` (Spec: `long-press-description`)

## 5. Description Overlay Dialog

- [x] 5.1 Create `DescriptionOverlay.java` extending `StackPane` — follows `SearchOverlay` pattern with fade animation (Design: D4)
- [x] 5.2 Implement dynamic content rendering: iterate `ObjectDescription` entries, render sections as bold headers, subsections as indented sub-headers, label/value as rows (Spec: `long-press-description`)
- [x] 5.3 Add `ScrollPane` for long content (Spec: `long-press-description`)
- [x] 5.4 Implement fullscreen mode when window width < 600px (Spec: `long-press-description`)
- [x] 5.5 Add close-on-click-outside and Escape-key-close behavior (Spec: `long-press-description`)
- [x] 5.6 Add CSS classes to `style.css` for description overlay styling

## 6. Wiring

- [x] 6.1 Wire `MapInteractionHandler.onLongPress` callback in `MainController.initMapView()` to call `client.getDescription(lat, lon)` on background thread (Design: sequence diagram)
- [x] 6.2 Create and show `DescriptionOverlay` in `MainController` when description data arrives (Spec: `long-press-description`)
- [x] 6.3 Handle empty description result (no objects or no data) gracefully — no dialog shown (Spec: `long-press-description`)
