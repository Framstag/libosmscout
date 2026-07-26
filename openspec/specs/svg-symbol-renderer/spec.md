# svg-symbol-renderer

## Purpose

The SVG map backend SHALL provide a `SymbolRendererSVG` class implementing `osmscout::SymbolRenderer` that writes SVG elements to an output stream.

## Requirements

### Requirement: SymbolRendererSVG writes SVG elements
The SVG map backend SHALL provide a `SymbolRendererSVG` class implementing `osmscout::SymbolRenderer` that writes SVG elements to an output stream.

#### Scenario: SVG symbol renderer exists
- **WHEN** the SVG backend is initialized
- **THEN** a `SymbolRendererSVG` class SHALL be available implementing `osmscout::SymbolRenderer`
- **AND** it SHALL write SVG elements to an output stream

### Requirement: SetFill emits fill attributes
The system SHALL emit `fill` attribute on SVG elements based on the provided `FillStyle`.

#### Scenario: Fill style converted to SVG fill
- **GIVEN** a `FillStyle` with color `#ff0000`
- **WHEN** `SetFill()` is applied before drawing a primitive
- **THEN** the resulting SVG element SHALL contain `fill="#ff0000"`

### Requirement: SetBorder emits stroke attributes
The system SHALL emit `stroke` and `stroke-width` attributes on SVG elements based on the provided `BorderStyle`.

#### Scenario: Border style converted to SVG stroke
- **GIVEN** a `BorderStyle` with color `#000000` and width `1.0`
- **WHEN** `SetBorder()` is applied before drawing a primitive
- **THEN** the resulting SVG element SHALL contain `stroke="#000000"` and `stroke-width="1.0"`

### Requirement: DrawPrimitives outputs SVG shapes
The system SHALL output SVG elements with fill and stroke attributes for each primitive type:
- `DrawPolygon()` → `<polyline>` with `points` attribute
- `DrawRect()` → `<rect>` with `x`/`y`/`width`/`height` attributes
- `DrawCircle()` → `<circle>` with `cx`/`cy`/`r` attributes

#### Scenario: Polygon primitive emits polyline
- **GIVEN** a polygon primitive with vertices `(0,0)`, `(10,0)`, `(10,10)`
- **WHEN** `DrawPolygon()` is called
- **THEN** the output SHALL contain a `<polyline>` element with a `points` attribute listing the vertices

#### Scenario: Rectangle primitive emits rect
- **GIVEN** a rectangle primitive at `(5,5)` with width `20` and height `10`
- **WHEN** `DrawRect()` is called
- **THEN** the output SHALL contain a `<rect>` element with `x="5"`, `y="5"`, `width="20"`, `height="10"`

#### Scenario: Circle primitive emits circle
- **GIVEN** a circle primitive at center `(15,15)` with radius `8`
- **WHEN** `DrawCircle()` is called
- **THEN** the output SHALL contain a `<circle>` element with `cx="15"`, `cy="15"`, `r="8"`

### Requirement: MapPainterSVG delegates to SymbolRendererSVG
`MapPainterSVG::DrawSymbol()` SHALL delegate to `SymbolRendererSVG` and preserve the symbol name comment output.

#### Scenario: Symbol rendering delegates correctly
- **GIVEN** a symbol named "hospital"
- **WHEN** `MapPainterSVG::DrawSymbol()` is called
- **THEN** it SHALL delegate drawing to `SymbolRendererSVG`
- **AND** the SVG output SHALL contain a comment identifying the symbol as "hospital"
