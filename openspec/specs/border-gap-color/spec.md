# Border Gap Color

**Purpose:** When a `BorderStyle` has dashes and a visible `gapColor`, the Skia backend renders a solid border in the gap color behind the dashed border.

## Requirements

### Requirement: Border gap color rendering

When a `BorderStyle` has dashes and a visible `gapColor`, the Skia backend SHALL render a solid border in the gap color behind the dashed border.

#### Scenario: Dashed border with visible gap color renders two passes
- **WHEN** `DrawArea()` renders a border with non-empty `dash` and `borderStyle->GetGapColor().IsVisible() == true`
- **THEN** the backend draws a solid stroke in the gap color first, then draws the dashed border on top

#### Scenario: Dashed border with invisible gap color renders single pass
- **WHEN** `DrawArea()` renders a border with non-empty `dash` and `borderStyle->GetGapColor().IsVisible() == false`
- **THEN** the backend draws only the dashed border stroke

#### Scenario: Solid border ignores gap color
- **WHEN** `DrawArea()` renders a border with empty `dash`
- **THEN** the backend draws only the solid border stroke
