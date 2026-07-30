## Purpose

Remove spurious `[0,0]` prefix from Qt dash pattern so dashed lines render identically to Cairo, AGG, and Skia backends.

## ADDED Requirements

### Requirement: No spurious prefix in Qt dash pattern

The Qt renderer SHALL NOT prepend `[0,0]` to the dash pattern before calling `QPen::setDashPattern()`. The dash values from the style SHALL be passed directly.

#### Scenario: Dashed line matches Cairo output

- **GIVEN** a line style with dash pattern `[8, 2]`
- **WHEN** the Qt renderer draws the line
- **THEN** the dash pattern passed to `setDashPattern()` SHALL be `[8, 2]`
- **AND** the visual output SHALL match the Cairo renderer's output for the same style

#### Scenario: Solid line with no dashes is unaffected

- **GIVEN** a line style with no dash pattern (empty dash vector)
- **WHEN** the Qt renderer draws the line
- **THEN** the line SHALL be drawn as a solid stroke
- **AND** the output SHALL be identical to the current behavior
