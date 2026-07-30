# Line Alpha Transparency

**Purpose:** The Skia backend uses `SkColorSetARGB` (or equivalent) throughout to preserve the alpha channel from the `Color` object in all drawing operations.

## Requirements

### Requirement: RGBA color support in DrawPath

The Skia backend SHALL use `SkColorSetARGB` (or equivalent) in `DrawPath()` to preserve the alpha channel from the `Color` object.

#### Scenario: DrawPath with transparent color
- **WHEN** `DrawPath()` is called with a `Color` having `GetA() < 1.0`
- **THEN** the rendered line SHALL have the corresponding alpha transparency

#### Scenario: DrawPath with opaque color
- **WHEN** `DrawPath()` is called with a `Color` having `GetA() == 1.0`
- **THEN** the rendered line SHALL be fully opaque

### Requirement: RGBA color support in DrawArea fill

The Skia backend SHALL use `SkColorSetARGB` in `DrawArea()` for fill colors.

#### Scenario: Area fill with transparent color
- **WHEN** `DrawArea()` renders a fill with `fillColor.GetA() < 1.0`
- **THEN** the fill SHALL have the corresponding alpha transparency

### Requirement: RGBA color support in DrawArea border

The Skia backend SHALL use `SkColorSetARGB` in `DrawArea()` for border colors.

#### Scenario: Area border with transparent color
- **WHEN** `DrawArea()` renders a border with `borderColor.GetA() < 1.0`
- **THEN** the border SHALL have the corresponding alpha transparency
