## ADDED Requirements

### Requirement: Border dash pattern support

The Skia backend SHALL apply dash patterns from `BorderStyle::GetDash()` when drawing area borders in `DrawArea()`.

#### Scenario: Dashed border renders with pattern
- **WHEN** `DrawArea()` renders a border with non-empty `borderStyle->GetDash()`
- **THEN** the border stroke SHALL use `SkDashPathEffect` with the dash intervals

#### Scenario: Solid border renders without pattern
- **WHEN** `DrawArea()` renders a border with empty `borderStyle->GetDash()`
- **THEN** the border stroke SHALL be solid (no dash effect)
