# Proposal

## Why

A label drawn through the Cairo or the SVG backend keeps the first font that backend resolved for a
font size: the resolved font is cached under the scaled font size alone, while the font itself is
resolved from the requested font name. A painter that is handed a different font name — a style
switch, a font setting, an application that changes the map font — therefore keeps drawing with the
earlier font until the painter is reopened, silently. Since the label stage already derives the reuse
key of a measurement from the font name, the painter reports measurements for the new font while
drawing the old one.

## What Changes

- The Cairo and the SVG backend select a cached resolved font by every input the font is resolved
  from, not by the scaled font size alone.
- After the requested font name changes, a live painter resolves and draws a font for the new name.
- The resolved font and the label measurements a painter reports stay consistent: what the painter
  measures for a font name is what it draws.
- The rendered output for an unchanged font name is unchanged.

## Capabilities

### New Capabilities

- `painter-font-cache`: the contract for the resolved-font cache a map painter keeps across frames —
  which inputs a cache entry depends on and when an existing entry must not be served.

### Modified Capabilities

- `map-painter-label-reuse`: the requirement that a changed drawing parameter is measured again gains
  a scenario tying the drawn font to the font the painter resolved for the new parameter; the earlier
  requirement only covered the measurement.

## Impact

Affected code:

- `libosmscout-map-cairo/include/osmscoutmapcairo/MapPainterCairo.h` — the resolved-font cache type
  (`FontMap`, `fonts`).
- `libosmscout-map-cairo/src/osmscoutmapcairo/MapPainterCairo.cpp` — both `GetFont` variants (Pango
  and the FreeType/Cairo toy-font path), plus the destructor and `Close()` that free the cache.
- `libosmscout-map-svg/include/osmscoutmapsvg/MapPainterSVG.h` — the resolved-font cache type
  (`FontMap`, `fonts`).
- `libosmscout-map-svg/src/osmscoutmapsvg/MapPainterSVG.cpp` — `GetFont` and the destructor that
  frees the cache.

Tests and build:

- `Tests/src/TextMetricsCairoTest.cpp`, `Tests/src/TextMetricsSVGTest.cpp` — a case that measures
  and draws through one painter under two font names.
- `Tests/CMakeLists.txt`, `Tests/meson.build` — only if a new test target is introduced; the existing
  text-metrics targets are expected to carry the case.

Not affected: the Qt and DirectX backends already key their font cache by name and size. The IOS
backend carries the same incomplete key, but its fix needs an Apple run and is recorded as a
follow-up rather than part of this change. The separate observation that the caches are keyed by an
unquantized size and are never bounded is out of scope here and stays a TODO entry.
