# draw-symbol

## Purpose

The Skia map backend SHALL implement `MapPainterSkia::DrawSymbol` to render `Symbol` objects at screen positions using `SymbolRendererSkia`.

## Requirements

### Requirement: MapPainterSkia::DrawSymbol uses SymbolRendererSkia

The system SHALL implement `MapPainterSkia::DrawSymbol` to render a `Symbol` at a given screen position using `SymbolRendererSkia`.

#### Scenario: DrawSymbol creates SymbolRendererSkia and renders

- **WHEN** `MapPainterSkia::DrawSymbol` is called with a valid projection, symbol, and screen position
- **THEN** a `SymbolRendererSkia` SHALL be constructed with the current SkCanvas
- **THEN** `SymbolRendererSkia::Render` SHALL be called with the projection, symbol, screen position, and scale factor

#### Scenario: DrawSymbol applies scale factor

- **WHEN** `DrawSymbol` is called with a scaleFactor other than 1.0
- **THEN** the symbol SHALL be rendered at the scaled size, with the base `SymbolRenderer::Render` method handling coordinate transformation

#### Scenario: DrawSymbol handles empty symbol

- **WHEN** `DrawSymbol` is called with a symbol that has no primitives
- **THEN** nothing SHALL be drawn and no error SHALL occur
