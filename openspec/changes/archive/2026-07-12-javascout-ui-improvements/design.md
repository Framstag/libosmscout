## Context

JavaScout is a JavaFX demo application built on top of `libosmscout-client-java`. It currently renders the map into a JavaFX `Canvas` via a JNI Cairo pipeline. The main window layout is loaded from `main.fxml`, and two floating overlays (search and route) are added programmatically on top of the map panel.

Current problems observed in the code:
- `JavaScoutApp` binds the `Canvas` size to the `StackPane` but never attaches a resize listener that triggers a re-render.
- Overlay buttons have fixed 40x40px size and hard-coded padding (8px). The route button uses `-fx-translate-y: -52px` to avoid overlapping the search button, producing a 52px gap regardless of DPI.
- `SearchOverlay` and `RoutePanel` each choose their own max widths (500px vs 360px) and layout breakpoints (600px threshold only in search). There is no shared sizing utility.
- Route search location dialog opened from `RoutePanel` calls `SearchOverlay.pickForRoute(prompt, callback)` but does not seed the text field with the current start/destination value.
- `RoutePanel` invokes `renderer.requestRender(lat, lon, mag, routeLats, routeLons)`, but `MapRenderer.requestRender` overwrites `currentLat/Lon/Mag` with the renderer's own state rather than the route center, and route data is only retained in `MapRenderer` fields. The canvas resize path may request a render with null route arrays, clearing the overlay.

This change is purely in the JavaScout UI layer; the JNI Cairo render path already supports route drawing.

## Goals / Non-Goals

**Goals:**
- Window resize redraws the map to the new canvas size while preserving the geographic center.
- Overlay button size, padding, and spacing become DPI-aware and touch-friendly.
- Search and route panels share a single responsive sizing strategy.
- Route location search dialog opens pre-filled from the active route endpoint value.
- Calculated route is rendered and stays rendered across pan, zoom, and resize.

**Non-Goals:**
- No changes to `libosmscout-client-java` public API or JNI implementation. (The JNI route description format was later adjusted internally for card rendering, but no public API changed.)
- No new map interaction gestures beyond existing drag/scroll/long-press.
- No support for multiple windows or split-screen layouts.

## Decisions

### 1. Track display DPI in `JavaScoutApp` and pass a scale factor to overlays
- **Rationale**: JavaFX `Screen.getDpi()` gives physical DPI. Using a single `uiScale` factor (e.g. `dpi / 96.0`, clamped to `[1.0, 3.0]`) keeps math simple and matches CSS device-pixel-ratio concepts.
- **Alternative considered**: Read DPI per-control inside each overlay. Rejected because the main stage already has a reliable screen reference and a single factor avoids inconsistent sizing.

### 2. Introduce `UIScale` helper class
- **Rationale**: Centralizes conversion from "design units" (based on a 96dpi desktop reference) to actual pixels: `px(designDp)`. Overlays and CSS-in-code styling use this for sizes, insets, and gaps.
- **Why not pure CSS?** CSS does not expose DPI-aware custom properties easily in JavaFX without extra work, and programmatic layout (panels, pref sizes, translate offsets) needs Java constants anyway.

### 3. Anchor overlays with a shared bottom-right padding budget instead of hard-coded translate
- **Rationale**: Current route button uses `-fx-translate-y: -52px` to sit above search button. Replace with programmatic bottom padding that scales with `UIScale.thumbSize()`, giving a consistent gap (e.g. 0.25x thumb height) between buttons.
- **Trade-off**: Removes the visual separation entirely controlled by CSS; layout becomes partly code-driven. Mitigation: keep CSS classes for colors/hover, use code only for metrics.

### 4. Use "thumb on display" as the touch target reference
- **Rationale**: A comfortable touch target is ~9-10mm. At 96dpi this is ~34-38px. `thumbSize()` returns `max(40px, 10mm * dpi / 25.4)`, ensuring buttons never shrink below desktop size and grow on high-DPI/touch screens.

### 5. Re-render on canvas size change by listening to `mapCanvas.widthProperty()` and `heightProperty()`
- **Rationale**: Canvas dimensions are already bound to the parent. Adding listeners in `MainController.initMapView()` lets the renderer request a new frame with the same center when the size changes.
- **Center preservation**: `MapRenderer` already stores `currentLat/Lon/Mag`; the resize listener calls `requestRender(currentLat, currentLon, currentMag)` and the existing route arrays are passed through the overload that preserves them.

### 6. Unify panel sizing in `OverlayLayout` helper
- **Rationale**: Both `SearchOverlay` and `RoutePanel` use bottom-right `StackPane` alignment with a max-width panel. `OverlayLayout` computes `panelMaxWidth` as a fraction of scene width (capped at a design dp converted via `UIScale`), and chooses between full-width (small screen) and floating (large screen) modes using the same threshold. Implemented in `JavaScout/src/main/java/com/framstag/libosmscout/OverlayLayout.java` and used by both overlays.

### 7. Route dialog pre-fill: `RoutePanel` passes the current label text as initial query
- **Rationale**: `SearchOverlay.pickForRoute` already accepts a prompt. Extend the `LocationPicker` interface signature to include an optional initial query. This is a breaking change within JavaScout only.
- **Sequence**: user taps start label → `pickLocation("Select start", startLabel.getText(), callback)` → search overlay opens with the previous value in the text field and performs a debounced search if non-empty.

### 8. Keep route arrays alive across renders
- **Rationale**: `MapRenderer.clearRouteOverlay()` is only called when `RoutePanel` explicitly clears the route. All other render paths (resize, pan, zoom) must preserve `routeLats/routeLons`. Introduce a private `requestRender()` overload that does not clear route data, and make the public overload that takes route arrays set them.
- **Fix for missing route**: The current code calls `renderer.requestRender(renderer.getLatitude(), renderer.getLongitude(), renderer.getMagnification(), routePanel.getCurrentRoute().latitudes, routePanel.getCurrentRoute().longitudes)`. This should work, but resize and `clearRouteOverlay` timing can drop the data. Keep route arrays as renderer fields and only mutate them on route set/clear.

### 9. Present route instructions as web-like cards with a structured parser
- **Rationale**: A plain monospace `ListView` looks dated and does not support optical navigation hints. Replacing it with a scrollable `VBox` of cards (icon + primary instruction + secondary distance/time) gives a modern, web-app feel and provides an extension point for future turn arrows, lane graphics, and exit numbers.
- **Implementation**: `RouteInstruction` parses the JNI description strings into structured fields; `RoutePanel.createInstructionCard()` builds an `HBox` card with a circular icon, bold primary label, and gray secondary label. The `RouteDescriptionPostprocessor` in JNI was updated to emit "Instruction [distance, time]" so the parser can split primary and secondary text cleanly.

### 10. Keyboard shortcuts for overlays and zoom buttons
- **Rationale**: Desktop users expect keyboard access. `Ctrl+F` already opened search; adding `Ctrl+R` for the route panel and on-screen zoom buttons improves discoverability and touch/tablet usability.
- **Implementation**: `MainController` makes `mapCanvas` focus traversable, focuses it on mouse press, and attaches the shortcut handler directly to `mapCanvas` instead of the `mapPanel` event filter. This prevents `Ctrl+F`/`Ctrl+R` from firing while focus is in search/route text fields or result lists.

- [Risk] CSS sizing mixed with code-driven sizing can conflict. → Mitigation: remove metric CSS rules (`-fx-translate-y`, fixed button sizes) and apply them via `UIScale` in code; keep CSS for color/rounded corners only.
- [Risk] High-DPI scaling may make panels too large on small high-res screens (e.g. 4K laptop). → Mitigation: cap `uiScale` at 2.0 for panels and use scene-width percentage for max widths.
- [Risk] Pre-filling route search with a coordinate string like "51.514227, 7.465279" may produce poor search results. → Mitigation: only pre-fill when the label text is a non-coordinate label; coordinates fall back to empty query.
- [Risk] Canvas resize events fire rapidly during window drag. → Mitigation: resize goes through the existing 200ms debounce loop in `MapRenderer`; no extra throttling needed.
- [Risk] Route overlay still not visible after fixes if stylesheet lacks `_route` style. → Mitigation: verify `stylesheets/include/route.oss` and `stylesheets/map.ost` already define the types; no changes needed unless spec validation fails.

## Migration Plan

No deployment or migration needed. This is a demo application. After implementation:
1. Build `JavaScout` with `mvn package`.
2. Run with a maps directory and verify:
   - resize window keeps center,
   - search/route buttons are usable,
   - route panel and search panel sizes match,
   - route is drawn after Calculate.

## Open Questions

- Should `uiScale` be persisted in `Config` so users can override DPI scaling? (Out of scope for now; use auto-DPI.)
- Should route search pre-fill attempt to parse coordinate labels back into lat/lon instead of text search? (Decision: no; coordinates are already set, text search is for named places.)
