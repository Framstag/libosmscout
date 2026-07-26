## ctx

JavaScout has a JavaFX shell with database loading via `OSMScoutClient` JNI. No rendering exists — `OSMScoutClient` only has `openDatabase()`, `close()`, `isInitialized()`. The FXML shows a `StackPane` placeholder with "No map loaded".

libosmscout has multiple render backends. Cairo (`MapPainterCairo`) renders to a `cairo_t` context. Cairo image surfaces provide direct pixel buffer access for Java bridge.

## Goals / Non-Goals

**Goals:**
- Render OSM map in JavaFX Canvas using AGG backend via JNI
- Keyboard pan (arrows, PageUp/Down) and zoom (+/-)
- Mouse pan (drag) and zoom (scroll wheel)
- Persist last map position (lat, lon, magnification) to config.properties
- Initial position: 51.5142273, 7.4652789, magnification level 5 (city zoom)
- Re-render on view change with debounce (200ms)

**Non-Goals:**
- No tile caching (re-render every frame — acceptable for desktop)
- No OpenGL rendering path
- No routing or location search rendering
- No overlay objects (pins, routes)
- No animation/smooth transitions between views

## Decisions

### 1. Cairo render-to-image-surface via JNI

**Decision**: Add `render()` native method to `OSMScoutClient` that renders current view to `int[]` ARGB pixels using `MapPainterCairo`.

**Rationale**:
- Cairo renders to `cairo_image_surface_t` — direct pixel buffer access, no window system dependency
- Single JNI call returns pixel data, JavaFX `PixelWriter` blits it to Canvas
- Reuses existing `MapPainterCairo` — no new C++ renderer needed
- Cairo has better platform support than AGG (libagg unmaintained)
- Alternative (JavaFX MapPainter backend in C++) would require duplicating all Cairo draw calls as JNI → JavaFX calls — much more complex

**Flow**:
```
Java: render(w,h,lat,lon,mag) → JNI
  C++: create cairo_image_surface_t(CAIRO_FORMAT_RGB24, w, h)
  C++: create cairo_t from surface
  C++: create MercatorProjection(lat, lon, mag, w, h, dpi)
  C++: create MapParameter
  C++: get DBThread → RunSynchronousJob → get databases + MapService
  C++: load tiles for projection area
  C++: create MapPainterCairo → DrawMap(projection, param, data, cairo_t)
  C++: read surface data → convert BGRx → int[] ARGB
  C++: destroy cairo_t + surface
  C++: return int[] to Java
Java: Canvas.getGraphicsContext2D().getPixelWriter().setPixels(...)
```

### 2. JNI render runs on background thread

**Decision**: `MapRenderer` Java class runs render on `javafx.concurrent.Task`, updates Canvas on JavaFX thread via `Platform.runLater()` or `Task.succeeded()`.

**Rationale**: AGG rendering is CPU-bound and may take 50-200ms. Blocking JavaFX thread would freeze UI.

### 3. DBThread access via existing C++ client infrastructure

**Decision**: The JNI `render()` implementation accesses the `DBThread` stored in the native `ClientData` object (created during `OSMScoutClientBuilder.build()`).

**Rationale**: The C++ client already has `DBThread` with `RunSynchronousJob()` that provides database list + read lock. No need to duplicate database management.

### 4. Position persistence in existing Config

**Decision**: Add `map.latitude`, `map.longitude`, `map.magnification` keys to `config.properties`. Save debounced (500ms after last view change). Load on startup, fall back to defaults.

**Rationale**: `Config.java` already handles OS-specific paths and read/write. Just add new keys.

### 5. Interaction handling in Java, not JNI

**Decision**: All keyboard/mouse event handling in JavaFX. On view change, call `MapRenderer.requestRender(lat, lon, mag)` which debounces and triggers JNI render.

**Rationale**: JavaFX event handling is straightforward. No need to pipe events through JNI.

### 6. Magnification levels

**Decision**: Use `Magnification` levels matching standard OSM zoom:
- Level 0-3: continent/country
- Level 4-6: city
- Level 7-9: street
- Level 10+: building
- Start at level 5 (city zoom for Dortmund area)

**Rationale**: Matches libosmscout's `Magnification` convention and gives usable default.

## Risks / Trade-offs

- **Performance**: Full re-render on every view change. For 800x600 at city zoom, expect ~50-150ms per frame. Acceptable for desktop but won't hit 60fps. Mitigation: debounce, consider dirty-region rendering later.
- **Cairo font rendering**: `MapPainterCairo` uses Cairo fonts (optionally Pango). Cairo is a dependency of `osmscout-map-cairo`. Pango optional but recommended for text layout.
- **BGRx → ARGB conversion**: Cairo `CAIRO_FORMAT_RGB24` stores pixels as 4-byte BGRx on little-endian. Conversion to Java ARGB int[] is a simple byte swap per pixel. ~2ms for 800x600.
- **JNI pixel array copy**: Returning large int[] (~1.9MB for 800x600) crosses JNI boundary. Could use direct ByteBuffer for zero-copy in future.
- **No tile cache**: Every pan/zoom re-fetches tile data from database. Acceptable for local database files. Could add tile cache later.
