## Why

JavaScout has an app shell with database loading but no map display. Users see "No map loaded" placeholder. Need interactive map rendering with zoom/pan and persistent last position.

## What Changes

Add map rendering to JavaScout using libosmscout's Cairo renderer via JNI. Render map tiles to a pixel buffer in C++ using Cairo image surface, pass to JavaFX Canvas for display. Add keyboard/mouse zoom and pan controls. Persist last map position (lat, lon, zoom) to config file.

## Capabilities

### New Capabilities

- `map-rendering`: Render OSM map data to JavaFX Canvas via JNI bridge. Cairo backend renders to image surface, pixel data passed to Java as int[] for Canvas GraphicsContext.
- `map-interaction`: Keyboard (arrows, +/-, PageUp/Down) and mouse (drag, scroll wheel, click) for pan and zoom. Smooth interaction with debounced re-render.
- `position-persistence`: Save/restore last map center (lat, lon) and zoom level to config.properties. Initial position: 51.5142273, 7.4652789 (Dortmund area).

### Modified Capabilities

*(none)*

## Impact

- **JavaScout/**: New `MapRenderer.java` (JavaFX Canvas wrapper), `MapInteractionHandler.java` (key/mouse input), update `MainController.java`, `Config.java`, `main.fxml`, `style.css`
- **libosmscout-client-java/**: New JNI method `render(int width, int height, double lat, double lon, double zoom)` on `OSMScoutClient`, returns `int[]` ARGB pixel data. New C++ implementation using Cairo `MapPainterCairo` to render to image surface.
- **libosmscout-map-cairo/**: Already provides `MapPainterCairo` — no changes needed, just link it.
- **Build**: `libosmscout-client-java` Meson build needs to link `osmscout_map_cairo`. `JavaScout/pom.xml` may need additional JavaFX dependency `javafx-graphics` (already transitive via `javafx-controls`).
