# Skia Map Backend — Missing Features

This file documents features present in the Cairo and Qt backends that are not yet implemented in the Skia map backend (`libosmscout-map-skia`).

## Implemented Features

- [x] Area rendering (fill + border with patterns, dashes, gap colors)
- [x] Path/line rendering (color, width, dashes, cap styles)
- [x] Mixed cap style handling (restrictive cap + round cap overlay at ends)
- [x] Label layout and rendering (regular + contour labels)
- [x] Glyph rendering along paths
- [x] Symbol rendering (`SymbolRendererSkia`, `DrawSymbol`)
- [x] Contour symbol rendering (`DrawContourSymbol` with path-following)
- [x] Icon rendering (`HasIcon`, `DrawIcon` with PNG loading and caching)
- [x] SVG icon loading (SkSVGDOM primary + nanosvg fallback)
- [x] StyleSheetChanged cache cleanup (clears `iconCache` + `patternCache`)

## Missing Features

### SVG pattern fill

Qt backend supports SVG patterns (`.svg` extension) with `PatternMode::Scalable` via `QSvgRenderer`. Skia (and Cairo) only support PNG patterns.

**Qt reference:** `MapPainterQt::HasPattern()` loads `.svg` via `QSvgRenderer`, renders to QImage at configured pattern size, uses as fill brush.

### Pattern origin alignment

Qt's `DrawArea` aligns pattern origin to the area's first coordinate using `remainder()` so patterns tile consistently relative to the area. Skia's pattern shader always starts from (0,0), causing visual misalignment when adjacent areas use the same pattern.

**Qt reference:** `MapPainterQt::DrawArea()` lines 811-819.



### Performance

- No icon preloading at style load time (icons loaded on first use during drawing)
- No pattern preloading optimization
