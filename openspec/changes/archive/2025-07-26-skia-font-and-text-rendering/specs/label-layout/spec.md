## ADDED Requirements

### Requirement: Text label layout

The Skia backend SHALL implement `Layout()` to measure text extents using `SkFont::measureText()` and `SkFontMetrics`, returning a `SkiaLabel` with correct width, height, and text content.

#### Scenario: Layout returns correct width and height
- **WHEN** `Layout()` is called with a text string, font size, and no wrapping
- **THEN** it SHALL return a label with `width` set to `measureText()` result and `height` set from font metrics

#### Scenario: Layout with wrapping splits text
- **WHEN** `Layout()` is called with `enableWrapping == true` and a positive `objectWidth`
- **THEN** the text SHALL be split at word boundaries to fit within the proposed width

#### Scenario: Layout without wrapping returns single line
- **WHEN** `Layout()` is called with `enableWrapping == false`
- **THEN** the text SHALL be measured as a single line
