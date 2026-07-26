## 1. Responsive shell and DPI-aware sizing

- [x] 1.1 Create `UIScale` helper class in `JavaScout/src/main/java/com/framstag/libosmscout/UIScale.java` (spec: `javascout-responsive-shell`) [2]
  - Compute scale from `Screen.getDpi()` with clamp to `[1.0, 2.0]`.
  - Provide `px(double designDp)`, `thumbSize()`, and small gap helper.
- [x] 1.2 Update `JavaScoutApp` to detect DPI and pass `uiScale` to `MainController` (spec: `javafx-ui-shell`) [1]
- [x] 1.3 Add canvas resize listener in `MainController.initMapView()` that re-renders with same center (spec: `javascout-responsive-shell`) [2]
- [x] 1.4 Remove fixed-size CSS rules from `style.css` for `.search-overlay-button` and `.route-overlay-button` (spec: `javascout-responsive-shell`) [1]
- [x] 1.5 Make `SearchOverlay` apply DPI-aware button size, panel max width, and bottom-right padding via `UIScale` (spec: `javascout-responsive-shell`) [3]
- [x] 1.6 Make `RoutePanel` apply matching DPI-aware button size and panel max width via `UIScale`; reduce button gap (spec: `javascout-responsive-shell`) [3]
- [x] 1.7 Ensure both panels share the same small-screen threshold and full-width behavior; implement shared `OverlayLayout` helper (spec: `javascout-responsive-shell`) [2]
- [x] 1.8 Add keyboard navigation to search result list (arrow keys, Enter, Escape) (spec: `javascout-responsive-shell`) [2]
- [x] 1.9 Add zoom in/out overlay buttons with keyboard/mouse access (spec: `javafx-ui-shell`) [2]

## 2. Route dialog pre-fill

- [x] 2.1 Extend `RoutePanel.LocationPicker` interface to accept an initial query string (spec: `route-input`) [1]
- [x] 2.2 Update `RoutePanel.pickStart()` and `pickDest()` to pass the current label text, skipping coordinate strings (spec: `javascout-route-dialog-data`) [2]
- [x] 2.3 Update `SearchOverlay.pickForRoute()` to accept and apply the initial query, triggering a debounced search when non-empty (spec: `javascout-route-dialog-data`) [2]
- [x] 2.4 Update `MainController` lambda that wires `RoutePanel` to `SearchOverlay` to match the new interface (spec: `route-input`) [1]

## 3. Route persistence across renders

- [x] 3.1 Add private `MapRenderer.requestRender()` overload that preserves existing `routeLats`/`routeLons` (spec: `route-visualization`) [2]
- [x] 3.2 Change resize/pan/zoom render paths to use the route-preserving overload (spec: `route-visualization`) [2]
- [x] 3.3 Verify `RoutePanel` still calls the route-setting overload after calculation and `clearRouteOverlay()` on clear (spec: `route-visualization`) [1]
- [x] 3.4 Render route instructions as web-like cards using structured `RouteInstruction` parser (spec: `route-visualization`) [3]
- [x] 3.5 Add keyboard navigation to route instruction cards and Escape to close route panel (spec: `route-visualization`) [2]

## 4. Verification and build

- [x] 4.1 Build `JavaScout` with `mvn package` and fix any Java compilation errors [2]
- [x] 4.2 Run JavaScout with a maps directory; verify resize keeps center and route remains visible (spec: `javascout-responsive-shell`, `route-visualization`) [3]
- [x] 4.3 Verify search and route panels have matching widths and buttons scale with DPI [2]
- [x] 4.4 Verify route location search pre-fills with existing named location and skips coordinates (spec: `javascout-route-dialog-data`) [2]
- [x] 4.5 Verify Ctrl+F, Ctrl+R, and zoom overlay buttons work [2]
