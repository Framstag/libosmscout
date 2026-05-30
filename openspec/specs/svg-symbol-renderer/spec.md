# svg-symbol-renderer

## SymbolRendererSVG

The SVG map backend SHALL provide a `SymbolRendererSVG` class implementing `osmscout::SymbolRenderer` that writes SVG elements to an output stream.

## SetFill

The system SHALL emit `fill` attribute on SVG elements based on the provided `FillStyle`.

## SetBorder

The system SHALL emit `stroke` and `stroke-width` attributes on SVG elements based on the provided `BorderStyle`.

## DrawPrimitives

The system SHALL output SVG elements with fill and stroke attributes for each primitive type:
- `DrawPolygon()` → `<polyline>` with `points` attribute
- `DrawRect()` → `<rect>` with `x`/`y`/`width`/`height` attributes
- `DrawCircle()` → `<circle>` with `cx`/`cy`/`r` attributes

## Delegation

`MapPainterSVG::DrawSymbol()` SHALL delegate to `SymbolRendererSVG` and preserve the symbol name comment output.
