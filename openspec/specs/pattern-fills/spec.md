# Pattern Fills

**Purpose:** The Skia backend implements `DrawFillStyle()` to support pattern fills for areas. Pattern images (PNG) are loaded, cached, and applied as repeating shaders.

## Requirements

### Requirement: Pattern fill support

The Skia backend SHALL implement `DrawFillStyle()` to support pattern fills for areas. Pattern images (PNG) SHALL be loaded, cached, and applied as repeating shaders.

#### Scenario: Area with pattern fill renders pattern
- **WHEN** `DrawFillStyle()` is called with a `FillStyle` having `HasPattern() == true` and the pattern image loads successfully
- **THEN** the area SHALL be filled with the repeating pattern via `SkImage::makeShader()` with `SkTileMode::kRepeat`

#### Scenario: Area with pattern fill falls back to solid color
- **WHEN** `DrawFillStyle()` is called with a `FillStyle` having `HasPattern() == true` but the pattern image fails to load
- **THEN** the area SHALL be filled with the solid `fillColor`

#### Scenario: Area with solid fill renders solid
- **WHEN** `DrawFillStyle()` is called with a `FillStyle` having `HasPattern() == false`
- **THEN** the area SHALL be filled with the solid `fillColor`

#### Scenario: Pattern images are cached
- **WHEN** the same pattern is used across multiple areas
- **THEN** the pattern image SHALL be loaded from disk only once and cached for subsequent uses

#### Scenario: Fill and border combined rendering
- **WHEN** both `fill` and `border` are present in `DrawFillStyle()`
- **THEN** the fill SHALL be drawn first (preserving the path), then the border stroke on top
