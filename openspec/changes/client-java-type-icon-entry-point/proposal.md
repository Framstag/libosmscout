# Proposal

## Why

A UI that lists search results or POIs wants the type's own map artwork next to each row, and the client
cannot deliver it. The only rendering entry point it has draws a whole map frame, so the caller would need a
position, a viewport, a magnification and a full-frame allocation per icon — and a search row has none of
those. Nothing in the client can ask the active stylesheet what artwork it has for `amenity_parking`, so a
consumer either draws no icon or invents one of its own, which is exactly what the stylesheet already
answers.

## What Changes

- The JNI bridge SHALL offer `renderTypeIcon(String objectType, int sizePx)` (Java `public native int[]`),
  returning one `0xAARRGGBB` word per pixel of a `sizePx x sizePx` square, or `null` when the type has no
  artwork or the artwork cannot be produced.
- The type SHALL be resolved through the loaded type configuration and its icon style through the style
  configuration's own selectors, with a feature buffer that carries the type and no tag values — so a rule
  that selects on a tag falls back to no artwork instead of to an invented icon.
- The entry point SHALL use a projection of its own at a detail magnification, so it needs no map frame,
  viewport or projection of the caller's, and SHALL scale the artwork to fit the requested square.
- The icon-versus-symbol preference the client already has SHALL apply: with `setPreferSymbolIcons(true)` and
  a symbol present the symbol is drawn, otherwise the map's own order (image first, symbol when the image is
  unusable).
- An unusable request (`sizePx` not positive or above 512, an unknown or empty type, no database loaded, a
  backend without Cairo) SHALL return `null` and SHALL NOT fault; a diagnosable absence SHALL be logged with
  its stage, a type the stylesheet gives no icon SHALL not be.
- The two pure decisions the entry point makes — the fit scale of the artwork and the un-premultiplication
  of the backend surface's channels into the straight ARGB Java receives — SHALL live in a
  dependency-free helper header so they can be unit-tested on the host.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `map-rendering`: the JNI rendering surface gains an entry point that returns the stylesheet artwork of a
  single object type, following the client's icon-versus-symbol preference, with a straight-ARGB result and
  a bounded, never-faulting request contract.

## Impact

Affected files and modules:

- `libosmscout-client-java/src/OSMScoutClient.cpp` — the new JNI entry point, its two fit/draw helpers for
  the symbol and the raster image, its cairo surface handling, and the includes it needs from the Cairo
  backend and the map styles.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java` — the `public native
  int[] renderTypeIcon(String objectType, int sizePx)` declaration and its Javadoc, so the entry point is
  callable from Java and is not dead bridge code.
- `libosmscout-client-java/src/type_icon_pixels.h` — the new dependency-free helper (fit scale,
  un-premultiply-and-pack), following the existing `frame_pixel_layout.h` and `route_length.h` pattern.
- `Tests/src/TypeIconPixelsTest.cpp` and its registration in `Tests/CMakeLists.txt` and `Tests/meson.build`.
- `openspec/changes/client-java-type-icon-entry-point/` — this change.

No database, map, style or file format change, so no `FileFormatVersion.md` version bump applies. No new
dependency: the Cairo backend headers and the map styles the entry point uses are already linked into the
bridge, and the entry point is compiled out (returning `null` with a warning) where the bridge is built
without them.

Out of scope: changing what a stylesheet declares, the icon directories the client is configured with, or
any existing render entry point. The Java `OSMScoutClient` gains a declaration only; no existing signature
changes.

Consumers: the Android app's search-result and POI rows ask for one type's artwork instead of rendering a
frame; this repository itself has no caller.
