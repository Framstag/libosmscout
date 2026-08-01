# skia-dash-width-scaling Specification

## Purpose

Scale dash pattern intervals by line width in the Skia map renderer so dashed lines with gap colors render identically to Cairo and AGG backends.

## Requirements

### Requirement: Dash intervals scaled by line width

The Skia renderer SHALL multiply each dash interval value by the line width before passing to `SkDashPathEffect::Make()`, matching the behavior of Cairo (`dash[i] * width`) and AGG (`dash[i] * width`).

#### Scenario: Dashed line with gap color matches Cairo output

- **GIVEN** a line style with dash pattern `[8, 4]`, line width 5px, and a visible gap color
- **WHEN** the Skia renderer draws the line
- **THEN** the dash length SHALL be 40px and the gap length SHALL be 20px
- **AND** the gap color SHALL be visible in the gaps between dashes
- **AND** the visual output SHALL match the Cairo renderer's output for the same style

#### Scenario: Solid line with no dashes is unaffected

- **GIVEN** a line style with no dash pattern (empty dash vector)
- **WHEN** the Skia renderer draws the line
- **THEN** the line SHALL be drawn as a solid stroke
- **AND** the output SHALL be identical to the current behavior
