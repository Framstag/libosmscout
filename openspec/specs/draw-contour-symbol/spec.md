# draw-contour-symbol

## Purpose

The Skia map backend SHALL implement `MapPainterSkia::DrawContourSymbol` to place `Symbol` instances along a contour path at regular intervals, rotated to match the path tangent.

## Requirements

### Requirement: MapPainterSkia::DrawContourSymbol places symbols along a path

The system SHALL implement `MapPainterSkia::DrawContourSymbol` to place copies of a `Symbol` along a contour path at regular intervals, with each symbol rotated to match the path tangent.

#### Scenario: DrawContourSymbol places symbols at intervals

- **WHEN** `DrawContourSymbol` is called with a contour path, symbol, and spacing data
- **THEN** symbols SHALL be placed along the path at intervals of `symbolSpace` starting from `symbolOffset`
- **THEN** each symbol SHALL be rotated to align with the path tangent at its placement position

#### Scenario: DrawContourSymbol uses FollowPath helpers

- **WHEN** `DrawContourSymbol` processes a path
- **THEN** it SHALL use `FollowPathInit` and `FollowPath` helper methods (private to `MapPainterSkia`) to walk the coordinate range, matching Qt's approach

#### Scenario: DrawContourSymbol handles closed paths

- **WHEN** the contour path is closed
- **THEN** symbols SHALL wrap around and continue past the end of the path

#### Scenario: DrawContourSymbol handles short paths

- **WHEN** the contour path is shorter than the symbol width
- **THEN** no symbols SHALL be placed

#### Scenario: DrawContourSymbol applies symbol scale

- **WHEN** `DrawContourSymbol` is called with `data.symbolScale`
- **THEN** each placed symbol SHALL be rendered at the specified scale factor
