# Proposal

## Why

A style entry may carry both a raster icon name and a vector symbol for the same object. The painter
draws the raster icon whenever the renderer can serve it and treats the vector symbol only as a fallback.
A map that carries the symbol but not the icon image therefore loses the marker, and a consumer that wants
resolution-independent output has no way to ask for the symbol.

## What Changes

- A render SHALL offer a per-render choice of which of a style entry's two renderings is drawn when the
  entry carries both a raster icon name and a vector symbol.
- The choice SHALL default to the present precedence (raster icon first, symbol as fallback), so a render
  that does not set it is unchanged.
- With the vector symbol chosen, the entry SHALL be drawn from its symbol even when its raster icon image
  exists or would be served.
- The choice SHALL take effect on the next rendered frame without reloading the stylesheet.
- The Java client SHALL expose the choice through its public API.

## Capabilities

### New Capabilities

- `icon-symbol-preference`: which of a style entry's two renderings a render draws when the entry carries
  both, and how that choice is exposed to a caller.

### Modified Capabilities

<!-- None: no existing capability's requirements change. The default precedence is preserved, so every
     documented rendering behaviour that does not set the preference is unaffected. -->

## Impact

Affected files and modules:

- `libosmscout-map/include/osmscoutmap/MapParameter.h` — the new render parameter, its setter and its
  getter, documented as public API.
- `libosmscout-map/src/osmscoutmap/MapParameter.cpp` — the documented default.
- `libosmscout-map/src/osmscoutmap/MapPainter.cpp` — the point label stage of frame preparation honours
  the choice; the two existing branches are reordered, not removed.
- `Tests/src/MapPainterIconSymbolPreferenceTest.cpp` — new host test; drives the label stage with a no-op
  painter, so it needs no backend, image file or database.
- `Tests/CMakeLists.txt`, `Tests/meson.build` — register the new test in both build systems.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — the Java client entry point, stored with the client
  handle and read when the next frame's render parameters are built.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java` — the declaration and
  documentation of the new call.

No database, map or style file format changes, so no `FileFormatVersion.md` version bump applies. No new
dependency. No existing public signature changes; the new parameter is additive. The renderers
(`libosmscout-map-agg`, `-cairo`, `-opengl`, `-svg`, `-qt`, `-skia`, …) are unaffected: the decision is
taken in the shared frame preparation, before a backend draws.
