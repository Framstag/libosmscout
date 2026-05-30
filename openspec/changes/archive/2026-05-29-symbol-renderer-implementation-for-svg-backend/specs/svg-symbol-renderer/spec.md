## ADDED Requirements

### Requirement: SymbolRendererSVG implements SymbolRenderer interface

The system SHALL provide an `SymbolRendererSVG` class in the SVG map backend that implements the `osmscout::SymbolRenderer` interface and writes SVG elements to an output stream.

#### Scenario: Render accepts projection, symbol, and center position
- **WHEN** `SymbolRendererSVG::Render()` is called with a projection, symbol, and mapCenter coordinates
- **THEN** SVG elements are written to the output stream for each primitive in the symbol

#### Scenario: Render accepts optional scale factor
- **WHEN** `SymbolRendererSVG::Render()` is called with `scaleFactor > 1.0`
- **THEN** symbol primitives are scaled accordingly in the output

### Requirement: SetFill controls fill attribute

The system SHALL emit `fill` attribute on SVG elements based on the provided `FillStyle`.

#### Scenario: Fill with visible color
- **WHEN** `SetFill()` is called with a `FillStyle` whose `GetFillColor().IsVisible()` is true
- **THEN** subsequent draw calls produce elements with `fill="#RRGGBB"`

#### Scenario: Fill with transparent color
- **WHEN** `SetFill()` is called with a `FillStyle` whose fill color is not visible
- **THEN** subsequent draw calls produce elements with `fill="none"`

#### Scenario: Fill with null style
- **WHEN** `SetFill()` is called with a null `FillStyleRef`
- **THEN** subsequent draw calls produce elements with `fill="none"`

### Requirement: SetBorder controls stroke attributes

The system SHALL emit `stroke` and `stroke-width` attributes on SVG elements based on the provided `BorderStyle`.

#### Scenario: Border with visible color
- **WHEN** `SetBorder()` is called with a `BorderStyle`
- **THEN** subsequent draw calls produce elements with `stroke="#RRGGBB"` and `stroke-width="<width>"`

#### Scenario: Border with null style
- **WHEN** `SetBorder()` is called with a null `BorderStyleRef`
- **THEN** subsequent draw calls produce elements with `stroke="none"`

### Requirement: DrawPolygon outputs polyline element

The system SHALL output an SVG `<polyline>` element with fill and stroke attributes for polygon primitives.

#### Scenario: Polygon with coordinates
- **WHEN** `DrawPolygon()` is called with a vector of `Vertex2D` points
- **THEN** the output contains `<polyline points="x1,y1 x2,y2 ..."` with fill/stroke attributes and `/>`

### Requirement: DrawRect outputs rect element

The system SHALL output an SVG `<rect>` element with fill and stroke attributes for rectangle primitives.

#### Scenario: Rectangle with position and dimensions
- **WHEN** `DrawRect(x, y, w, h)` is called
- **THEN** the output contains `<rect x="..." y="..." width="..." height="..."` with fill/stroke attributes and `/>`

### Requirement: DrawCircle outputs circle element

The system SHALL output an SVG `<circle>` element with fill and stroke attributes for circle primitives.

#### Scenario: Circle with center and radius
- **WHEN** `DrawCircle(x, y, radius)` is called
- **THEN** the output contains `<circle cx="..." cy="..." r="..."` with fill/stroke attributes and `/>`

### Requirement: Output matches existing DrawSymbol behavior

The SVG output from `SymbolRendererSVG` SHALL produce identical structure to the previous inline `DrawSymbol` implementation for the same symbol inputs.

#### Scenario: Polygon symbol renders same output
- **WHEN** `SymbolRendererSVG::Render()` processes a symbol containing a `PolygonPrimitive`
- **THEN** the SVG output matches the element structure of the old inline renderer for the same projection, symbol, and position

#### Scenario: Rectangle symbol renders same output
- **WHEN** `SymbolRendererSVG::Render()` processes a symbol containing a `RectanglePrimitive`
- **THEN** the SVG output matches the element structure of the old inline renderer for the same projection, symbol, and position

#### Scenario: Circle symbol renders same output
- **WHEN** `SymbolRendererSVG::Render()` processes a symbol containing a `CirclePrimitive`
- **THEN** the SVG output matches the element structure of the old inline renderer for the same projection, symbol, and position

### Requirement: MapPainterSVG delegates to SymbolRendererSVG

The system SHALL refactor `MapPainterSVG::DrawSymbol` to delegate symbol rendering to `SymbolRendererSVG`.

#### Scenario: DrawSymbol uses SymbolRendererSVG
- **WHEN** `MapPainterSVG::DrawSymbol()` is called
- **THEN** a `SymbolRendererSVG` instance is created and its `Render()` method is called with the projection, symbol, screenPos, and scaleFactor

#### Scenario: DrawSymbol still outputs symbol name comment
- **WHEN** `MapPainterSVG::DrawSymbol()` is called
- **THEN** the output still contains `<!-- symbol: <name> -->` comment before rendered primitives
