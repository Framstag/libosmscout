## Context

See proposal.md - Why for motivation.

Current state, relevant constraints:

- Stylesheets (`.oss`) define symbols as `SYMBOL` blocks of `POLYGON`/`RECTANGLE`/`CIRCLE` primitives; the OSS parser registers them in `StyleConfig` via `RegisterSymbol` into a private `std::unordered_map<std::string,SymbolRef> symbols` (`libosmscout-map/include/osmscoutmap/StyleConfig.h`). Only `GetSymbol(name)` and `RegisterSymbol` are public — there is no way to enumerate all symbols.
- Rendering a symbol standalone is already possible: `osmscout::SymbolRenderer::Render(projection, symbol, mapCenter, scaleFactor)` (`libosmscout-map/src/osmscoutmap/SymbolRenderer.cpp`) iterates primitives, scales them via the projection, and delegates to backend-specific `SetFill`/`SetBorder`/`DrawPolygon`/`DrawRect`/`DrawCircle`. Backends: `SymbolRendererCairo` (`libosmscout-map-cairo`, ctor takes `cairo_t*`) and `SymbolRendererSVG` (`libosmscout-map-svg`, ctor takes `std::ostream&`; emits SVG *fragments* only — no `<svg>` document wrapper).
- Precedent for a multi-backend "render everything for review" demo exists: `Demos/src/TextMetricsAll.cpp` + `DrawMapAll.cpp`, using `HAVE_OSMSCOUT_MAP_*` compile guards, `osmscout::CmdLineParser` options, `MercatorProjection::Set(coord, angle, mag, dpi, w, h)`, and `cairo_surface_write_to_png`.
- `StyleConfig` construction requires a `TypeConfig`; with no map database, `TypeConfig::LoadFromOSTFile` provides one. `StyleConfig::Load(styleFile)` resolves `MODULE "include/..."` includes from the stylesheet.

## Goals / Non-Goals

**Goals:**
- Zero new external dependencies; tool built only from existing public map APIs plus one small additive accessor on `StyleConfig`.
- Per-symbol standalone images from both Cairo (PNG bitmap) and SVG (vector file) backends, driven by one CLI invocation.
- Contact-sheet mode for fast scanning of many symbols.
- Deterministic, database-free output; non-zero exit on any failure (scriptable).
- Tool registered in both build systems (CMake + Meson), following the AGENTS.md two-build-system convention.

**Non-Goals:**
- No changes to any render backend or to `SymbolRenderer` itself.
- No automated pass/fail symbol validation (this is a manual review tool; a pixel-diff harness is future work).
- No rasterization of SVG output into bitmaps (would add a new dependency like librsvg).
- No Qt/Skia/AGG backends in v1 (can be added later behind the same loop).

## Decisions

### D1: Tool lives in `Demos/` as `SymbolsAll`, not in `Tests/` or a new top-level dir
`Demos/` already hosts exactly this kind of tool (`TextMetricsAll`, `DrawMapAll`, `DrawMapCairo`), with established CMake (`osmscout_demo_project`, `HAVE_OSMSCOUT_MAP_*` guards, `PNG::PNG` link) and Meson wiring (`Demos/CMakeLists.txt` ~line 382, `Demos/meson.build` ~line 205 patterns). It is a manually invoked app, not a Catch2 test.
- Alternatives: `Tests/` — wrong fit: that is the automated Catch2 suite, not manual tools; a GUI test binary would need X server, blocking headless use. Standalone top-level dir — new CMake/Meson scaffolding for one file, more churn than value.

### D2: Enumeration API is `StyleConfig::GetSymbolNames()` returning sorted `std::vector<std::string>`
New public method in `libosmscout-map/include/osmscoutmap/StyleConfig.h` (next to `GetSymbol`), implemented by copying keys of the `symbols` map and sorting them for deterministic output order.
- Alternatives: `ForEachSymbol(callback)` — avoids allocation but is more invasive and less useful for binding/wrappers; exposing the map directly — leaks the `unordered_map` type and is unordered anyway. Sorted vector keeps output order stable across runs (matters for contact-sheet layout and diffing).

### D3: Direct `SymbolRenderer` invocation, not full `MapPainter`
For each symbol the tool creates a plain canvas (cairo surface or `std::ofstream`), paints white background, instantiates the backend `SymbolRenderer` and calls `Render(projection, symbol, canvasCenter, scaleFactor)`. This is the same code path `MapPainterCairo::DrawSymbol` uses (`libosmscout-map-cairo/src/osmscoutmapcairo/MapPainterCairo.cpp:1180`), minus the map.
- Alternative: `MapPainterCairo::DrawMap(projection, parameter, {}, cr)` with empty data — requires font setup, label layouter, and other map machinery irrelevant to symbols, and has no SVG equivalent. Rejected.

### D4: Auto-fit each symbol to the canvas with margin
- Compute the symbol's pixel bounding box via `Symbol::GetBoundingBox(projection)` (handles both `MAP` and `GROUND` projection modes) plus border headroom via `GetMaxBorderWidth(projection)`; derive `scaleFactor = min((W-m)/bboxW, (H-m)/bboxH)` (clamped to a sane minimum) so symbols with unusual sizes — exactly the AI-generated ones under review — render fully inside the canvas. `SymbolRenderer::Render` already applies `scaleFactor` to both the box center and primitives, so centering falls out (`mapCenter - boxCenter * scaleFactor`).
- Alternative: fixed `scaleFactor = 1.0` — simplest, but oversized or badly-scaled symbols get clipped, defeating the tool's purpose. Rejected.

### D5: SVG backend needs a document wrapper
- `SymbolRendererSVG` emits fragments only. The tool wraps each render with an SVG document: `<?xml ...?><svg xmlns="http://www.w3.org/2000/svg" width height viewBox="0 0 W H">`, a white background `<rect>`, the rendered fragments, `</svg>`. Absolute pixel coordinates are already in the fragments, so the viewBox matches the canvas.
- Alternative: extend `SymbolRendererSVG` to own a document — pollutes the renderer interface contract, rejected.

### D6: Backend selection via compile guards, default `all`
- Mirror `TextMetricsAll`: `#if defined(HAVE_OSMSCOUT_MAP_CAIRO)` / `HAVE_OSMSCOUT_MAP_SVG` guards; `--backend cairo|svg|all` with `all` = whichever is compiled in; `--backend` naming an unavailable backend prints an error and exits non-zero (spec: "Cairo backend unavailable"). Default: Cairo when compiled, else SVG, else error.

### D7: Contact sheet layout
- Per backend, when `--sheet` is set: grid of `ceil(sqrt(n))` columns, one canvas-sized cell per symbol, symbol name text centered below each cell (cairo `cairo_show_text`; SVG `<text>`). Single output file `symbols.<backend-ext>`. Deterministic order from D2's sorted names.

### D8: Type configuration resolution
- `--ost <file>` overrides; otherwise `<stylesheet-dir>/map.ost` (the authoritative full type registry, matching how the main apps load types); otherwise sibling `<stylesheet-without-ext>.ost`; missing → error, non-zero exit (spec: "Type definition file resolution"). `StyleConfig::Load` then parses the stylesheet; symbol scanning only needs `Load` to succeed (styles are irrelevant to enumeration).

### D9: Exit codes and reporting
- 0 = success; 1 = usage / stylesheet or type-file load failure / backend unavailable; 2 = per-symbol render or write failure. Per-symbol line to stdout, errors to stderr (mirrors `TextMetricsAll` logging style). Output dir auto-created via `std::filesystem::create_directories`.

## Sequence (single symbol, Cairo backend)

```
CLI args -> parse (CmdLineParser)
  -> TypeConfig::LoadFromOSTFile(ost)
  -> StyleConfig(typeConfig).Load(stylesheet)
  -> names = styleConfig.GetSymbolNames()
  loop over names:
    symbol = styleConfig.GetSymbol(name)
    surface = cairo_image_surface_create(ARGB32, N, N); cairo paint white
    scale = fit(symbol.GetBoundingBox(projection), canvas, margin)
    SymbolRendererCairo renderer(cairo)
    renderer.Render(projection, *symbol, Vertex2D(N/2, N/2), scale)
    cairo_surface_write_to_png(surface, outDir/name.png)
```
SVG path identical except `std::ofstream` + document wrapper; contact sheet replaces the loop body with grid cells.

## Risks / Trade-offs

- [AI-generated symbol with extreme aspect ratio or degenerate geometry (zero-width bbox)] → `scaleFactor` clamped to a sane minimum; degenerate symbols still render (possibly tiny) and become visible on the sheet instead of crashing.
- [Symbol names with characters illegal in filenames] → sanitize to `[A-Za-z0-9_-]` when deriving file names; log the mapping.
- [Cairo bitmap drops alpha (RGB24 + white paint)] → intended: plain background makes partially-transparent fills visible, which is the review goal; ARGB32 not needed.
- [`HAVE_OSMSCOUT_MAP_SVG` build without Cairo: default backend must fall back] → default = Cairo if compiled else SVG; covered in D6.
- [Contact sheet text needs fonts; SVG `<text>` font availability varies] → cosmetic only; PNG labels use cairo's default font; label failures must not fail the whole run.

## Migration Plan

- Additive change only: new public accessor on `StyleConfig`, new `Demos/SymbolsAll` target in both `Demos/CMakeLists.txt` and `Demos/meson.build`. No removal, no behavior change to existing renderers — nothing to migrate.
- Rollback: revert the accessor, drop the demo target. No data migration.

## Open Questions

- None. (Alternative backends, automated diffing, SVG rasterization are explicitly out of scope per Non-Goals and would need their own change.)
