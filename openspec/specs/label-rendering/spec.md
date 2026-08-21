# Label Rendering

## Purpose

The Skia backend renders text labels in three styles: normal (solid text), emphasize (outline effect), and shield (background rectangle with border).

## Requirements

### Requirement: Normal text label rendering

The Skia backend SHALL render normal text labels in `DrawLabel()` using the style's text color with alpha.

#### Scenario: Normal label renders solid text
- **WHEN** `DrawLabel()` is called with a `TextStyle` having `GetStyle() == TextStyle::normal`
- **THEN** the text SHALL be rendered using `SkCanvas::drawString()` with the style's text color and alpha

### Requirement: Emphasize text label rendering

The Skia backend SHALL render emphasize text labels in `DrawLabel()` with an outline effect: draw the text offset in the emphasize color, then draw the text in the text color on top.

#### Scenario: Emphasize label renders with outline
- **WHEN** `DrawLabel()` is called with a `TextStyle` having `GetStyle() == TextStyle::emphasize`
- **THEN** the text SHALL be drawn multiple times offset by 1px in the emphasize color, then once in the text color on top

### Requirement: Shield label rendering

The Skia backend SHALL render shield labels in `DrawLabel()` with a colored background rectangle and border.

#### Scenario: Shield label renders with background and border
- **WHEN** `DrawLabel()` is called with a `ShieldStyle`
- **THEN** a filled rectangle SHALL be drawn in the background color, a border rectangle in the border color, and the text on top in the text color
