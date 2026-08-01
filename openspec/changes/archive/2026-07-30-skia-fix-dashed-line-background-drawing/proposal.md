# Fix Skia Backend Dashed Line Background Rendering

## What Changes

Fix Skia map renderer (`libosmscout-map-skia`) to correctly render dashed lines with explicit gap colors (background). Currently the Skia backend produces visually different results compared to Cairo and AGG backends.

**Root cause:** Two issues found:

1. **Skia** (`MapPainterSkia::SetLineAttributes()`): Dash interval values passed directly to `SkDashPathEffect` without scaling by line width. Both Cairo (`dashArray[i] = dash[i] * width`) and AGG (`dasher.add_dash(dash[i]*width, dash[i+1]*width)`) multiply dash intervals by the stroke width. Without this scaling, Skia dashes are much smaller than intended — gaps between dashes shrink, making the gap color (background) barely visible or invisible.

2. **Qt** (`MapPainterQt::DrawPath()`): Spurious `[0, 0]` prefix prepended to dash pattern before `setDashPattern()`. Comment says "skip butt?" — likely a workaround for cap style issues. This shifts the dash phase and produces different dash lengths compared to Cairo/AGG/Skia.

**Example:** Dash pattern `[8, 4]` at width 5px:
- Cairo/AGG: 40px dash, 20px gap → gap color clearly visible
- Skia (current): 8px dash, 4px gap → gap color nearly invisible

## Capabilities

### New Capabilities
- `skia-dash-width-scaling`: Scale dash pattern intervals by line width in `MapPainterSkia::SetLineAttributes()`, matching Cairo and AGG behavior
- `qt-dash-pattern-fix`: Remove spurious `[0,0]` prefix from Qt dash pattern in `MapPainterQt::DrawPath()`
- `qt-dash-pattern-fix`: Remove spurious `[0,0]` prefix from Qt dash pattern in `MapPainterQt::DrawPath()`

### Modified Capabilities
*(none — no spec-level behavior changes, this is a rendering backend bug fix)*

## Impact

- **File:** `libosmscout-map-skia/src/osmscoutmapskia/MapPainterSkia.cpp`
  - **Function:** `MapPainterSkia::SetLineAttributes()` — multiply each dash interval by `width` before passing to `SkDashPathEffect::Make()`
- **File:** `libosmscout-map-qt/src/osmscoutmapqt/MapPainterQt.cpp`
  - **Function:** `MapPainterQt::DrawPath()` — remove `[0,0]` prefix from dash pattern
- **No API changes** — `DrawPath` signature stays the same
- **No style sheet changes** — dash values in `.oss` files remain unchanged
- **Visual change only** — dashed lines with gap colors now match across all backends
