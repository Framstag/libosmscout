## 1. JNI Render Method (C++)

- [x] 1.1 Add `native int[] render(int width, int height, double lat, double lon, double magnification)` declaration to `OSMScoutClient.java`
- [x] 1.2 Implement JNI C++ function `Java_com_framstag_libosmscout_client_OSMScoutClient_render` — create `cairo_image_surface_t(CAIRO_FORMAT_RGB24)`, `MercatorProjection`, load tiles via `MapService`, render with `MapPainterCairo::DrawMap()`, read surface data, convert BGRx → `int[]` ARGB, cleanup
- [x] 1.3 Update `libosmscout-client-java/meson.build` to link `osmscout_map_cairo` and `osmscout_map` libraries
- [x] 1.4 Add `MapPainterCairo` header include and Cairo init in JNI render implementation

## 2. JavaFX MapRenderer

- [x] 2.1 Create `MapRenderer.java` — wraps JavaFX `Canvas`, exposes `requestRender(lat, lon, mag)` and `render()` (blocking JNI call)
- [x] 2.2 Implement background rendering via `javafx.concurrent.Task` — calls `OSMScoutClient.render()` off JavaFX thread, updates Canvas via `PixelWriter.setPixels()` on success
- [x] 2.3 Add 200ms debounce to `requestRender()` — coalesce rapid view changes into single render

## 3. Map Interaction

- [x] 3.1 Create `MapInteractionHandler.java` — attaches to Canvas, handles keyboard (arrows, +/-, PageUp/Down) and mouse (drag, scroll wheel) events
- [x] 3.2 Implement keyboard pan: arrow keys move 10% viewport, Page keys move 50% viewport
- [x] 3.3 Implement keyboard zoom: +/- change magnification by 1 level
- [x] 3.4 Implement mouse drag pan: track mouse press→move→release delta, update lat/lon
- [x] 3.5 Implement scroll wheel zoom: change magnification centered on cursor position
- [x] 3.6 Wire interaction events to `MapRenderer.requestRender()` with new lat/lon/mag

## 4. Position Persistence

- [x] 4.1 Add `map.latitude`, `map.longitude`, `map.magnification` key constants and getter/setter methods to `Config.java`
- [x] 4.2 Add `setMapPosition(lat, lon, mag)` and `getMapPosition()` returning `double[]` or null to `Config.java`
- [x] 4.3 Wire position save in `MainController` — on view change, debounced 500ms write to config
- [x] 4.4 Wire position restore in `MainController` — after database load, read config and set initial view (fallback: 51.5142273, 7.4652789, mag=5)

## 5. Integration & UI

- [x] 5.1 Update `MainController.java` — create `MapRenderer` and `MapInteractionHandler`, wire database load → initial render, wire shutdown → cleanup
- [x] 5.2 Update `main.fxml` — replace `Label` placeholder in `mapPanel` StackPane with `Canvas`
- [x] 5.3 Update `style.css` — add styles for map Canvas container if needed
- [x] 5.4 Update `JavaScoutApp.java` — pass physical DPI to controller for projection setup
