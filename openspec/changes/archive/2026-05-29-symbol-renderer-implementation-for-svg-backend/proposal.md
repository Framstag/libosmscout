## Why

The SVG map backend currently implements symbol drawing inline in `MapPainterSVG::DrawSymbol`, duplicating the rendering logic that the `SymbolRenderer` interface was designed to abstract. This leads to code duplication across backends, makes the SVG backend incompatible with the unified symbol rendering API, and blocks future improvements to symbol rendering that would need to be replicated in each backend independently.

## What Changes

- New `SymbolRendererSVG` class implementing `osmscout::SymbolRenderer` for the SVG backend
- Refactor `MapPainterSVG::DrawSymbol` to delegate to `SymbolRendererSVG` instead of inline primitive handling
- Remove duplicated coordinate projection and primitive dispatch logic from `DrawSymbol`
- Add build system entries (CMake + Meson) for new source and header files

No breaking changes — the public `MapPainterSVG` API remains unchanged.

## Capabilities

### New Capabilities
- `svg-symbol-renderer`: SVG backend implementation of the `SymbolRenderer` interface, enabling unified symbol rendering for SVG output

### Modified Capabilities

(empty — no existing specs have requirement changes)

## Impact

- **Library**: `libosmscout-map-svg` — adds 2 files (header + source)
- **API**: None — `MapPainterSVG` public API unchanged; `DrawSymbol` signature unchanged
- **Build**: CMake `libosmscout-map-svg/CMakeLists.txt` and Meson build files updated
- **Dependencies**: None — only uses core library types (`Vertex2D`, `Color`, `FillStyle`, `BorderStyle`, `Symbol`)
- **Behavior**: No change in SVG output — rendering results are identical before and after
