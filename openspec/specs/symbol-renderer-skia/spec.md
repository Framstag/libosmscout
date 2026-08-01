# symbol-renderer-skia

## Purpose

The Skia map backend SHALL provide a `SymbolRendererSkia` class implementing `osmscout::SymbolRenderer` that renders symbol primitives (polygon, rectangle, circle) with fill and border styles using Skia drawing APIs.

## Requirements

### Requirement: SymbolRendererSkia implements SymbolRenderer interface

The system SHALL provide a `SymbolRendererSkia` class in the Skia map backend that implements the `osmscout::SymbolRenderer` interface and renders symbol primitives using Skia drawing APIs.

The class SHALL be declared in `SymbolRendererSkia.h` and implemented in `SymbolRendererSkia.cpp` within `libosmscout-map-skia`.

#### Scenario: SymbolRendererSkia is constructable with SkCanvas pointer

- **WHEN** a `SymbolRendererSkia` is constructed with a valid `SkCanvas*` pointer
- **THEN** the object SHALL be in a valid state ready to render symbols

#### Scenario: SymbolRendererSkia renders filled polygon

- **WHEN** `SetFill` is called with a visible fill color, `BeginPrimitive` is called, and `DrawPolygon` is called with a closed set of vertices, and `EndPrimitive` is called
- **THEN** the polygon SHALL be filled on the canvas with the specified color

#### Scenario: SymbolRendererSkia renders bordered polygon

- **WHEN** `SetFill` and `SetBorder` are both called with visible styles, and a polygon is drawn
- **THEN** the polygon SHALL be filled first, then the border SHALL be stroked on top

#### Scenario: SymbolRendererSkia renders rectangle

- **WHEN** `DrawRect` is called with position and dimensions
- **THEN** a rectangle SHALL be drawn at the specified position with the specified width and height

#### Scenario: SymbolRendererSkia renders circle

- **WHEN** `DrawCircle` is called with center position and radius
- **THEN** a circle SHALL be drawn at the specified center with the specified radius

#### Scenario: SymbolRendererSkia handles border dashes

- **WHEN** `SetBorder` is called with a border style that has dashes
- **THEN** the border SHALL be rendered with the specified dash pattern

#### Scenario: SymbolRendererSkia warns on pattern fill

- **WHEN** `SetFill` is called with a fill style that has a pattern
- **THEN** a warning SHALL be logged and the pattern SHALL be ignored (matching Cairo/Qt behavior)

#### Scenario: SymbolRendererSkia handles invisible fill

- **WHEN** `SetFill` is called with a fill style whose color is not visible
- **THEN** no fill SHALL be applied to the primitive

#### Scenario: SymbolRendererSkia handles invisible border

- **WHEN** `SetBorder` is called with a border style whose color is not visible or width is zero
- **THEN** no border SHALL be applied to the primitive
