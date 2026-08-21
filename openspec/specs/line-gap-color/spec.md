# Line Gap Color

## Purpose

When a `LineStyle` has dashes and a visible `gapColor`, the Skia backend renders a solid line in the gap color behind the dashed line. The base class `MapPainter::DrawWay()` calls `DrawPath()` twice — first with the gap color (solid dash), then with the line color (dashed dash). The backend renders each call correctly.

## Requirements

### Requirement: Line gap color rendering

When a `LineStyle` has dashes and a visible `gapColor`, the Skia backend SHALL render a solid line in the gap color behind the dashed line. The base class `MapPainter::DrawWay()` calls `DrawPath()` twice — first with the gap color (solid dash), then with the line color (dashed dash). The backend renders each call correctly.

#### Scenario: Dashed line with visible gap color renders two passes
- **WHEN** the base class calls `DrawPath()` with gap color and empty dash, then with line color and dash pattern
- **THEN** the backend draws a solid stroke in the gap color first, then draws the dashed stroke on top

#### Scenario: Dashed line with invisible gap color renders single pass
- **WHEN** `DrawPath()` is called for a line with non-empty `dash` and `gapColor.IsVisible() == false`
- **THEN** the backend draws only the dashed stroke (no gap color pass)

#### Scenario: Solid line ignores gap color
- **WHEN** `DrawPath()` is called for a line with empty `dash`
- **THEN** the backend draws only the solid stroke, regardless of `gapColor` visibility
