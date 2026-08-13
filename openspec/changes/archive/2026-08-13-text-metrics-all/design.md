# TextMetricsAll — Design

## Context

See proposal.md — Why. Text metrics (label dimensions, per-glyph bounding boxes) are computed per backend inside protected methods (`Layout()`, `ToGlyphs()`, `GlyphBoundingBox()`) of each `MapPainter` subclass. The shared `LabelLayouter` template consumes these for label placement, contour label positioning, and culling. Backends currently return three different box semantics: logical extents (Cairo/SVG via Pango), ink bounds (AGG via FreeType, Skia via `getBounds`, Qt via `QRawFont::boundingRect`), and synthetic boxes (Cairo fallback, GDI, DirectX). All backends convert font size identically: `fontSize * projection.ConvertWidthToPixel(parameter.GetFontSize())` with `ConvertWidthToPixel = width * dpi / 25.4`.

## Goals / Non-Goals

**Goals**
- Public measurement API on `MapPainter` base, implemented by all Linux backends (Cairo, AGG, Qt, Skia, SVG).
- Standalone tool `TextMetricsAll` in `Demos/` that renders text + bounding boxes per backend, stores images, dumps values, and compares against a FreeType reference.
- Ink bounds as the target box semantics — boxes must enclose drawn pixels.

**Non-Goals**
- Fixing backend metric bugs (tool is diagnostic; fixes are follow-up changes).
- OpenGL backend (separate FreeType texture-atlas pipeline, no shared layouter).
- GDI, DirectX, iOSX backends (Windows/macOS only).
- Changing production rendering behavior — the API is additive.

## Decisions

### D1: Virtual `MeasureText()` on `MapPainter` base

```cpp
// libosmscout-map/include/osmscoutmap/MapPainter.h
struct TextMetrics {
  double width, height;              // label box
  struct Glyph {
    Vertex2D position;               // relative to label origin
    ScreenVectorRectangle box;       // relative to glyph base point
  };
  std::vector<Glyph> glyphs;
};
virtual TextMetrics MeasureText(const Projection&, const MapParameter&,
                                const std::string& text, double fontSize) = 0;
```

Each backend implements it as a thin wrapper over its existing `Layout()` + `ToGlyphs()` + `GlyphBoundingBox()` — no new measurement logic. The common loop (label dimensions + per-glyph iteration) lives once as a protected template helper `MeasureLabel()` on the base class; each backend only supplies its `Layout()` result and a `GlyphBoundingBox` lambda.

- **Alternative A: make `Layout()`/`ToGlyphs()`/`GlyphBoundingBox()` public.** Rejected — exposes text-layout internals as public API on 8 backends; no single contract.
- **Alternative B: per-backend non-virtual method.** Rejected — no common contract, tool needs per-backend dispatch anyway; virtual on base gives one contract and lets the tool use a single interface.
- **Alternative C: friend the tool class in each painter.** Rejected — friend declarations across backends for one demo app; breaks encapsulation without a contract.

### D2: FreeType as independent reference

The tool links `Freetype::Freetype` (already a project dependency — `cmake/features.cmake` registers it; AGG and OpenGL use it) and measures glyphs directly:

```cpp
FT_Set_Pixel_Sizes(face, px, px);   // px = fontSize * fontSizeParam * dpi / 25.4
FT_Load_Glyph(face, idx, FT_LOAD_RENDER);
// ink box relative to baseline origin (26.6 fixed point, /64):
x = metrics.horiBearingX / 64.0
y = -metrics.horiBearingY / 64.0    // FreeType y-up → screen y-down
w = metrics.width / 64.0
h = metrics.height / 64.0
```

- **Alternative: one backend as golden (e.g. AGG).** Rejected — circular; comparing backends to a backend cannot catch bugs shared with the reference.
- **Alternative: no reference, human eyeballs.** Rejected — the whole point is catching backend bugs; an independent ground truth makes differences quantitative.

### D3: Tool draws text natively, boxes from `MeasureText()`

The tool draws the text itself using each backend's native text API (cairo show text, `SkCanvas::drawString`, `QPainter::drawText`, ...) at a common baseline, then overlays the boxes returned by `MeasureText()`. If boxes are correct ink bounds, they wrap the drawn glyphs tightly.

- **Alternative: expose `DrawGlyphs()` and reuse production drawing.** Rejected — production drawing is protected for a reason; the tool's drawing is diagnostic and need not match production exactly. The *numbers* come from production code, which is what matters.

### D4: Ink bounds as target semantics

The reference and the "box encloses drawn ink" spec requirement define ink bounds as the target. Backends returning logical extents (Cairo/SVG Pango) will show differences against the reference — the tool surfaces this; whether to change those backends is a follow-up decision informed by the tool's output.

- **Alternative: logical extents as target.** Rejected — user confirmed ink bounds are the relevant semantics; logical extents include side bearings and do not visually wrap drawn text.

### D5: Linux-only scope

Cairo, AGG, Qt, Skia, SVG in v1. GDI/DirectX/iOSX excluded (platform-specific, synthetic boxes, no Linux CI coverage). OpenGL excluded (separate pipeline).

- **Alternative: all backends.** Rejected — platform-specific backends cannot be built or verified on Linux CI; synthetic-box backends would need their own reference story.

## Tool Flow

```
TextMetricsAll main
   │
   ├─ parse args (text, fontName, fontSize, dpi, output dir)
   │
   ├─ FreeType reference: load face, set pixel size, per glyph:
   │     ink box + advance → reference table
   │
   ├─ for each compiled-in backend (Cairo, AGG, Qt, Skia, SVG):
   │     │
   │     ├─ create painter + canvas
   │     ├─ metrics = painter.MeasureText(projection, parameter, text, fontSize)
   │     ├─ draw text natively at common baseline
   │     ├─ overlay per-glyph boxes from metrics
   │     ├─ store PNG → <output>/<Backend>.png
   │     └─ print label w/h, per-glyph boxes, diff vs reference
   │
   └─ summary: per-backend max deviation vs reference
```

## Files Changed

| File | Change |
|---|---|
| `libosmscout-map/include/osmscoutmap/MapPainter.h` | `TextMetrics` struct + virtual `MeasureText()` |
| `libosmscout-map-cairo/src/osmscoutmapcairo/MapPainterCairo.cpp` + `.h` | implement `MeasureText()` |
| `libosmscout-map-agg/src/osmscoutmapagg/MapPainterAgg.cpp` + `.h` | implement `MeasureText()` |
| `libosmscout-map-qt/src/osmscoutmapqt/MapPainterQt.cpp` + `.h` | implement `MeasureText()` |
| `libosmscout-map-skia/src/osmscoutmapskia/MapPainterSkia.cpp` + `.h` | implement `MeasureText()` |
| `libosmscout-map-svg/src/osmscoutmapsvg/MapPainterSVG.cpp` + `.h` | implement `MeasureText()` |
| `Demos/CMakeLists.txt` | new `TextMetricsAll` target, conditional per backend, link `Freetype::Freetype` |
| `Demos/src/TextMetricsAll.cpp` | tool main |
| `Demos/include/TextMetricsAll.h` | shared helpers (reference measurement, output) |

## Risks / Trade-offs

- **Pango logical vs ink mismatch is expected, not a bug** → Tool prints both backend value and reference; the diff column makes the semantic gap explicit. Do not "fix" backends based on the tool alone — decide per backend whether logical extents are intentional (layout) or wrong (shields).
- **Font size rounding differs per backend** (Pango absolute size vs `FT_Set_Pixel_Sizes` integer pixels vs Qt pixel size) → Reference uses the same `px = fontSize·fontSizeParam·dpi/25.4` formula; tolerance of 0.5 px in specs absorbs sub-pixel rounding.
- **Font file loading differs per backend** (Pango family name vs file path vs Qt family) → Tool passes the same font file path; backends that resolve by family name may fall back to a different font — the tool should print the resolved font name per backend.
- **Skia dual-path boxes** (tight `getBounds` vs font-metrics fallback) → Reference comparison will expose the fallback path; acceptable, it is a real difference the tool is meant to surface.
- **New virtual method on base class** → All existing `MapPainter` subclasses (including non-Linux) must implement it or the class becomes abstract; provide a default implementation returning empty metrics to avoid breaking GDI/DirectX/iOSX builds.

## Migration Plan

1. Add `TextMetrics` + virtual `MeasureText()` with default empty implementation on `MapPainter` base.
2. Implement per Linux backend.
3. Add `TextMetricsAll` tool + CMake target.
4. Build, run tool on sample text, verify output images and terminal dump.
5. No rollback concern — additive API, no behavior change.

## Open Questions

- Whether to change Cairo/SVG to return ink extents (Pango exposes both logical and ink via `pango_font_get_glyph_extents`) — defer until tool output shows the actual gap.
- Whether the tool should also dump label-level (not just glyph-level) reference values — defer; glyph-level covers the shield case.
